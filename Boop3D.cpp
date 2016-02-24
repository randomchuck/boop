#include "Boop3D.h"
#include <stdio.h>

CRITICAL_SECTION cs;

void CALLBACK DrawLineCallback(PTP_CALLBACK_INSTANCE instance, void *context) {
	long value = InterlockedIncrement( &(((Boop3D *)context)->threadidx) );
	Boop3D *bp = (Boop3D *)context;
	// long value = -1;
//	EnterCriticalSection(&cs);
//		value = ++(bp->threadidx);
//	LeaveCriticalSection(&cs);
	// printf("%s\n", ((Boop3D *)context)->fpsstr );
	// printf("%ld, %ld\n", hival, value );
	//printf("A - %ld\n", value );
	bp->DrawTriScanLine(   bp->sli[value - 1],
						 *(bp->sli[value - 1].v2[0]),
						 *(bp->sli[value - 1].v2[1]),
						 *(bp->sli[value - 1].v2[2]),
						 *(bp->sli[value - 1].v2[3]),
						   bp->sli[value - 1].sclr );
	long devalue = InterlockedIncrement( &bp->dethread );
	/*long devalue = -1;
		EnterCriticalSection(&cs);
		devalue = ++(bp->dethread);
		LeaveCriticalSection(&cs);*/
	//printf("B - %ld\n", devalue);
}

////////////////////////////////////////////////////////////
// Constructor.
Boop3D::Boop3D() : windowHandle(0), zbuffer(0), threadidx(0), dethread(0) {}
////////////////////////////////////////////////////////////
// Parameterized Constructor.
Boop3D::Boop3D(HWND winHandle) { Initialize(winHandle); }
////////////////////////////////////////////////////////////
// Destructor.
Boop3D::~Boop3D(void) { Shutdown(); }

////////////////////////////////////////////////////////////
// The BIG initializer function. Needs to be called at least
// once before calling anything else. Called for you if
// creating a Boop3D object with Boop3D(HWND);
//
// Creates contexts, sets up memory and lists,
// inits matrices, buffers, everything.
void Boop3D::Initialize(HWND _winHandle) {
	InitializeCriticalSection(&cs);
	// Hold onto our client window handle.
	windowHandle = _winHandle;
	// DC to client area.
	deviceContext = GetDC(_winHandle);
	// Pens for wire frame lines.
	hBluePen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
	hGreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
	hRedPen = CreatePen(PS_SOLID, 1, RGB(225, 0, 0));
	hPen = (HPEN)SelectObject(deviceContext, hBluePen);

	// Debug info.
	fpstimer = GetTickCount();
	fps = 0;
	trisrendered = 0;

	// Backbuffer.
	GetClientRect(windowHandle, &clirect);
	hdcMem = CreateCompatibleDC(deviceContext);

	// Just using CreateDIBSection improved performance by double.
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = clirect.right;
	bmi.bmiHeader.biHeight = -clirect.bottom; // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // So we don't have to worry about padding on each line.
	bmi.bmiHeader.biCompression = BI_RGB;
	hbmMem = CreateDIBSection( deviceContext,
							   &bmi,
							   DIB_RGB_COLORS,
							   (void **)&bmbuffer,
							   0,
							   0 );
	// This commented bit just for info. Not used since we're specifying
	// 32bit color depth. This part figures out how many bytes are
	// left after the last pixel of the row's byte.
	/*RowMult = 4 - (((bitmapInfo.bmiHeader.biBitCount * bitmapInfo.bmiHeader.biWidth) / 8) % 4);
	if ( RowMult == 4 )
		RowMult = 0;
	RowMult += (bitmapInfo.bmiHeader.biBitCount * bitmapInfo.bmiHeader.biWidth / 8);*/

	// Use this instead of default.
	hOld = SelectObject(hdcMem, hbmMem);

	// Calculate projection and view matrices.
	projmat = perspective(45.0f, (float)clirect.right / (float)clirect.bottom, 0.01f, 100.0f);
	// 3 Batmans
	// viewmat = lookat( vec3(0, 2, 1.0f), vec3(0, 1.25f, -2), vec3(0, 1, 0) );
	CameraLookat( vec3(0, 2, 1.0f), vec3(0, 1.25f, -2), vec3(0, 1, 0) );
	// CameraLookat( vec3(0, 2, 0.5f), vec3(0, 1.25f, -2), vec3(0, 1, 0) );
	// Closeup on the one batman.
	// viewmat = lookat( vec3(0, 4, 0.2f), vec3(0, 4, 0), vec3(0, 1, 0) );
	// CameraLookat( vec3(0, 4, 0.2f), vec3(0, 4, 0), vec3(0, 1, 0) );
	// Boxes, planes, road tile, etc.
	// viewmat = lookat( vec3(0, 2.75, 1.75f), vec3(0, 1.25f, 0), vec3(0, 1, 0) );

	// Set up z buffeclirect.
	zbuffer = new float[clirect.right * clirect.bottom];

	// Calc Light.
	vec3 v1 = vec3(1, 1, 1);
	vec3 v2 = vec3(0, 0, 0);
	vec3 v3 = vec3(0, 1, 0);
	litemtx = lookat( v1, v2, v3 );
	liteclr = vec3(1, 1, 1, 1);
	// Ambient Light.
	ambilite = vec3(1, 1, 1, 1);

	// Clear screen and zbuffer.
	Clear();

	// Start the thread index at the beginning.
	threadidx = 0;
	dethread = 0;

} // Initialize()

////////////////////////////////////////////////////////////
// Cleans up memory, deletes handles, buffers, etc.
// Shouldn't need to call explicitly, as it's invoked from
// our destructor. Just let Boop go out of scope.
void Boop3D::Shutdown(void) {
	DeleteCriticalSection(&cs);
	// Not Init()'d so nothing to do.
	if(windowHandle == 0)
		return;
	SelectObject(hdcMem, hOld);
	DeleteObject(hbmMem);
	DeleteDC    (hdcMem);
	SelectObject(hdcMem, hPen);
	ReleaseDC(windowHandle, deviceContext);
	DeleteObject(hBluePen);
	DeleteObject(hGreenPen);
	DeleteObject(hRedPen);
	for(int midx = 0; midx < (int)meshlist.size(); midx++) {
		meshlist[midx].tris.clear();
		delete [] meshlist[midx].ptris;
		// Sometimes textures won't be loaded.
		if(meshlist[midx].texturebuffer)
			delete [] (meshlist[midx].texturebuffer);
	}

	// Originally thought I was leaking memory, but it turns out
	// everything was being deleted after going out of scope,
	// which was after running the memory leak check.
	//
	// Wow, had to explicity call this to prevent triangles in
	// meshes from leaking.
	// meshlist.clear();

	if(zbuffer)
		delete [] zbuffer;
	windowHandle = 0;

} // Shutdown()

////////////////////////////////////////////////////////////
// Called automatically in Render(). Clears our backbuffer
// and the zbuffer.
void Boop3D::Clear() {
	// Clear screen.
	FillRect( hdcMem, &clirect, HBRUSH(GetStockObject(LTGRAY_BRUSH)) );

	// Clear z buffer.
	// memset doesn't properly set floating point value.
	// memset(zbuffer, 1000, sizeof(float) * clirect.right * clirect.bottom);
	for(int i = 0; i < clirect.right * clirect.bottom; i++)
		zbuffer[i] = 1000.0f;

} // Clear()

////////////////////////////////////////////////////////////
// This is put into your window procedure for the message:
// WM_SIZE.
//
// TODO: Currently, it's not really needed because Boop cannot
// handle resizing. Will be added later. Upon calling this,
// Boop3D should resize color and depth/z buffers.
void Boop3D::ResizeView(int w, int h) {
	GetClientRect(windowHandle, &clirect);
	projmat = perspective(45.0f, (float)w / (float)h, 0.01f, 100.0f);
} // ResizeView()

////////////////////////////////////////////////////////////
// OBJ Loader. Pass a file path and texture path. Refer
// to EXPORTING MESHES for object file info.
// Textures are also 32bit .bmp's.
void Boop3D::LoadMesh(string filepath, string texturepath) {
	// A lot of vectors/lists. The data needs to be sorted after it's loaded.
	vector<float> vertices;
	vector<float> vertices_sorted;
	vector<float> uvs;
	vector<float> uvs_sorted;
	vector<float> normals;
	vector<float> normals_sorted;
	vector<unsigned int> vert_indices;
	vector<unsigned int> normal_indices;
	vector<unsigned int> uv_indices;
	vector<unsigned int> indices;

	// All sorted mesh data will end up in here.
	B3DMesh mesh;
	mesh.texturebuffer = 0;
	//
	FILE *fp;
	float x, y, z;
	char ch = 0;
	// We track extreme vertex positions so
	// we can calculate width/height/depth.
	glml::vec3 leftmostVertex, rightmostVertex;
	glml::vec3 topmostVertex, bottommostVertex;
	glml::vec3 frontmostVertex, backmostVertex;

	// Open the file.
	fp = fopen(filepath.c_str(), "r");

	// Keep going until we hit end of file.
	while( !(feof(fp))  ) {

		// Grab a line.
		char buff[1000] = {0};
		if( !fgets(buff, 1000, fp) )
			break;

		// Skip comments.
		if( buff[0] == '#' )
			continue;

		// Init these temp vars to something crazy.
		x = y = z = -333.0f;

		// Vertex?
		if( buff[0] == 'v' && buff[1] == ' ' ) {
			sscanf(buff, "%c %f %f %f", &ch, &x, &y, &z);
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

			vec3 storeMe = vec3(x, y, z);
			if( x < leftmostVertex.x )
				leftmostVertex = storeMe;
			if( x > rightmostVertex.x )
				rightmostVertex = storeMe;
			if( y < bottommostVertex.y )
				bottommostVertex = storeMe;
			if( y > topmostVertex.y )
				topmostVertex = storeMe;
			if( z < backmostVertex.z )
				backmostVertex = storeMe;
			if( z > frontmostVertex.z )
				frontmostVertex = storeMe;
		}
		else if( buff[0] == 'v' && buff[1] == 't' ) { // UV's?
			sscanf(buff, "%c %c %f %f", &ch, &ch, &x, &y);
			uvs.push_back(x);
			uvs.push_back(y);
		}
		else if( buff[0] == 'v' && buff[1] == 'n' ) { // Normal?
			sscanf(buff, "%c %c %f %f %f", &ch, &ch, &x, &y, &z);
			normals.push_back(x);
			normals.push_back(y);
			normals.push_back(z);
		}
		else if( buff[0] == 'f' && buff[1] == ' ' ) { // Indices?
			int vertIndices[3] = {0};
			int uvIndices[3] = {0};
			int normalIndices[3] = {0};

			sscanf(buff,
			"%c\
			 %d %*c %d %*c %d\
			 %d %*c %d %*c %d\
			 %d %*c %d %*c %d",
			 &ch,
			 &vertIndices[0], &uvIndices[0], &normalIndices[0],
			 &vertIndices[1], &uvIndices[1], &normalIndices[1],
			 &vertIndices[2], &uvIndices[2], &normalIndices[2]
			 );

			// Each face has 3 verts, a uv for each vert, and
			// a normal for each vert.
			// 3 vts, 3 uvs, 3 nrms.
			vert_indices.push_back(vertIndices[0] - 1);
			vert_indices.push_back(vertIndices[1] - 1);
			vert_indices.push_back(vertIndices[2] - 1);
			uv_indices.push_back(uvIndices[0] - 1);
			uv_indices.push_back(uvIndices[1] - 1);
			uv_indices.push_back(uvIndices[2] - 1);
			normal_indices.push_back(normalIndices[0] - 1);
			normal_indices.push_back(normalIndices[1] - 1);
			normal_indices.push_back(normalIndices[2] - 1);

		} // else if indices

	} // while( not end of file)

	// Close the file.
	fclose(fp);

	// Determine width/height/depth of mesh.
	mesh.width = fabs(rightmostVertex.x - leftmostVertex.x);
	mesh.height = fabs(topmostVertex.y - bottommostVertex.y);
	mesh.depth = fabs(frontmostVertex.z - backmostVertex.z);

	// Move data into mesh object.
	int numTris = vert_indices.size() / 3;
	for(int curTri = 0; curTri < numTris; curTri++) {

		B3DTriangle b3dtri;
		// Vertices.
		b3dtri.verts[0].xyz.x = vertices[ vert_indices	[curTri * 3 + 0] * 3 + 0 ];
		b3dtri.verts[0].xyz.y = vertices[ vert_indices	[curTri * 3 + 0] * 3 + 1 ];
		b3dtri.verts[0].xyz.z = vertices[ vert_indices	[curTri * 3 + 0] * 3 + 2 ];
		b3dtri.verts[1].xyz.x = vertices[ vert_indices	[curTri * 3 + 1] * 3 + 0 ];
		b3dtri.verts[1].xyz.y = vertices[ vert_indices	[curTri * 3 + 1] * 3 + 1 ];
		b3dtri.verts[1].xyz.z = vertices[ vert_indices	[curTri * 3 + 1] * 3 + 2 ];
		b3dtri.verts[2].xyz.x = vertices[ vert_indices	[curTri * 3 + 2] * 3 + 0 ];
		b3dtri.verts[2].xyz.y = vertices[ vert_indices	[curTri * 3 + 2] * 3 + 1 ];
		b3dtri.verts[2].xyz.z = vertices[ vert_indices	[curTri * 3 + 2] * 3 + 2 ];
		// Normals.
		b3dtri.verts[0].nrm.x = normals [ normal_indices[curTri * 3 + 0] * 3 + 0 ];
		b3dtri.verts[0].nrm.y = normals [ normal_indices[curTri * 3 + 0] * 3 + 1 ];
		b3dtri.verts[0].nrm.z = normals [ normal_indices[curTri * 3 + 0] * 3 + 2 ];
		b3dtri.verts[1].nrm.x = normals [ normal_indices[curTri * 3 + 1] * 3 + 0 ];
		b3dtri.verts[1].nrm.y = normals [ normal_indices[curTri * 3 + 1] * 3 + 1 ];
		b3dtri.verts[1].nrm.z = normals [ normal_indices[curTri * 3 + 1] * 3 + 2 ];
		b3dtri.verts[2].nrm.x = normals [ normal_indices[curTri * 3 + 2] * 3 + 0 ];
		b3dtri.verts[2].nrm.y = normals [ normal_indices[curTri * 3 + 2] * 3 + 1 ];
		b3dtri.verts[2].nrm.z = normals [ normal_indices[curTri * 3 + 2] * 3 + 2 ];
		// UV's.
		b3dtri.verts[0].uv.x  = uvs		[ uv_indices	[curTri * 3 + 0] * 2 + 0 ];
		b3dtri.verts[0].uv.y  = uvs		[ uv_indices	[curTri * 3 + 0] * 2 + 1 ];
		b3dtri.verts[1].uv.x  = uvs		[ uv_indices	[curTri * 3 + 1] * 2 + 0 ];
		b3dtri.verts[1].uv.y  = uvs		[ uv_indices	[curTri * 3 + 1] * 2 + 1 ];
		b3dtri.verts[2].uv.x  = uvs		[ uv_indices	[curTri * 3 + 2] * 2 + 0 ];
		b3dtri.verts[2].uv.y  = uvs		[ uv_indices	[curTri * 3 + 2] * 2 + 1 ];

		mesh.tris.push_back( b3dtri );

	}

	int sz = mesh.tris.size();
	mesh.ptris = new B3DTriangle[ sz ];
	for( int t = 0; t < sz; t++ ) {
		mesh.ptris[t] = mesh.tris[t];
	}

	// Set transform to an identity matrix.
	mesh.matrix = mat4();

	// Load the texture.
	if( texturepath.size() > 0 ) {
		LoadBmp( texturepath.c_str(),
				 &mesh.texturebuffer,
				 &mesh.txwidth,
				 &mesh.txheight,
				 &mesh.txdepth );
	}

	// Finally add mesh to list.
	meshlist.push_back(mesh);

	// Point to first mesh.
	curMesh = &meshlist[0];

} // LoadMesh()

////////////////////////////////////////////////////////////
// Retrieves mesh at index idx.
B3DMesh *Boop3D::GetMesh(int idx) {
	return &meshlist[idx];
}

////////////////////////////////////////////////////////////
// Takes a point and a matrix and "projects" it into the
// screen.
vec3 Boop3D::Project( vec3 point, mat4 *mvpmatrix ) {
	// Transform the point in 3D first.
	mat4 pntmtx(point);
	mat4 finalmtx = *mvpmatrix * pntmtx;
	// Compensate for view.
	point.x = finalmtx[3].x - viewmat[3].x;
	point.y = finalmtx[3].y - viewmat[3].y;
	point.z = finalmtx[3].z - viewmat[3].z;

	// Project this point from 3D to 2D. Typicall accomplished
	// by dividing by z.
	vec3 result;
	int half_screen_width = clirect.right / 2;
	int half_screen_height = clirect.bottom / 2;
	result.x = ((point.x * half_screen_width / point.z) + half_screen_width);
	result.y = (-(point.y * half_screen_height  / point.z) + half_screen_height);
	result.z = point.z;
	// Return 2D point.
	return result;

} // Project()

////////////////////////////////////////////////////////////
// Called in a loop. Traverses mesh list and draws them all.
void Boop3D::Render(void) {
	// Clear screen and depth buffer.
	Clear();

	// TODO: Remove.
	// Rotate the meshes.
	static float rot = 1;
	// rot += 0.1f;

	// Draw every mesh.
	for(unsigned int midx = 0; midx < meshlist.size(); midx++) {

		// Pick a wireframe color for this mesh.
		if( midx % 2 == 0 )
			SelectObject(hdcMem, hBluePen);
		else if( midx % 3 == 0 )
			SelectObject(hdcMem, hGreenPen);
		else if( midx % 4 == 0 )
			SelectObject(hdcMem, hRedPen);
		else
			SelectObject(hdcMem, hRedPen);

		// Rotate and scale this mesh's matrix.
		/*mat4 rotamtx = rotate( rot, vec3(0, 1, 0) );
		mat4 scalemtx = scale( vec3(1.0f, 1.0f, 1.0f) );*/
//		meshlist[midx].matrix = mat4( meshlist[midx].matrix[3] ) *
//								rotamtx *
//								scalemtx;
		// meshlist[midx].matrix *= rotamtx;
		// Point to current mesh.
		curMesh = &meshlist[midx];
		// Finally draw it.
		DrawMesh( meshlist[midx] );
	}

	///////
	// FPS

		//// Frames Per Second.
			fps++;
			RECT fpsrect = {100, 100, 500, 100};
			if(GetTickCount() - fpstimer > 1000) {
				memset(fpsstr, 0, 10);
				itoa(fps, fpsstr, 10);
				fps = 0;
				fpstimer = GetTickCount();
			}
			DrawText(hdcMem, fpsstr, strlen(fpsstr), &fpsrect, DT_NOCLIP);
		//// Triangle Count.
			fpsrect.top += 100;
			fpsrect.bottom += 100;
			memset(trirenstr, 0, 10);
			itoa(trisrendered, trirenstr, 10);
			trisrendered = 0;
			DrawText(hdcMem, trirenstr, strlen(trirenstr), &fpsrect, DT_NOCLIP);
		////

	// FPS
	///////

	// Transfer the off-screen DC to the screen.
	BitBlt(deviceContext, 0, 0, clirect.right, clirect.bottom, hdcMem, 0, 0, SRCCOPY);

} // Render()

////////////////////////////////////////////////////////////
// Accepts a single mesh and renders its triangles to screen.
void Boop3D::DrawMesh( B3DMesh &m, mat4 *trans/* = 0*/ ) {

	// Calc mvp matrix.
	mat4 mtx;
	mtx = (trans) ? (projmat * viewmat * *trans) : (projmat * viewmat * m.matrix);


	// Draw every triangle.
	int sz = m.tris.size();
	for(unsigned int tidx = 0; tidx < sz; tidx++) {
		// Wireframe.
		if( SHADING == SHADING_WIRE )
			DrawWireFrameTri( m.tris[tidx], mtx );
		else
			DrawFilledTri( m.tris[tidx], mtx, m.matrix, RGB(255, 255, 255) ); // Shaded... or not.
	} // for each(B3DTriangle...

} // DrawMesh()

////////////////////////////////////////////////////////////
// If using the one-shot DrawMesh() or you've been drawing
// directly to our backbuffer, call this to render every-
// thing.
void Boop3D::Blit( void ) {
	// Transfer the off-screen DC to the screen.
	BitBlt(deviceContext, 0, 0, clirect.right, clirect.bottom, hdcMem, 0, 0, SRCCOPY);
}

////////////////////////////////////////////////////////////
// Faster than vanilla mat4 *operator.
void Boop3D::FastMat4Mult( mat4 *dest, mat4 *m1, mat4 *m2 ) {

	/*float wun = m1->columns[0].x;
	float too = m2->columns[0].x;
	float res = 0.0f;*/
	/*__asm {
	__asm mov eax, wun
	__asm mov ecx, too
	__asm mul ecx
	__asm mov res, 1234
	}*/

	/*float asdf = m1->columns[0].x * m2->columns[0].x;
	float asdf2 = 0.0f;
	__asm {
		mov  eax, dword ptr [m1]
		fld		  dword ptr [eax]
		mov  ecx, dword ptr [m2]
		fmul	  dword ptr [ecx]
		fstp	  dword ptr [asdf2]
	}*/

	
	dest->columns[0].x = m1->columns[0].x * m2->columns[0].x + 
						 m1->columns[1].x * m2->columns[0].y + 
						 m1->columns[2].x * m2->columns[0].z + 
						 m1->columns[3].x * m2->columns[0].w;
	dest->columns[1].x = m1->columns[0].x * m2->columns[1].x + 
						 m1->columns[1].x * m2->columns[1].y + 
						 m1->columns[2].x * m2->columns[1].z + 
						 m1->columns[3].x * m2->columns[1].w;
	dest->columns[2].x = m1->columns[0].x * m2->columns[2].x + 
						 m1->columns[1].x * m2->columns[2].y + 
						 m1->columns[2].x * m2->columns[2].z + 
						 m1->columns[3].x * m2->columns[2].w;
	dest->columns[3].x = m1->columns[0].x * m2->columns[3].x + 
						 m1->columns[1].x * m2->columns[3].y + 
						 m1->columns[2].x * m2->columns[3].z + 
						 m1->columns[3].x * m2->columns[3].w;
	dest->columns[0].y = m1->columns[0].y * m2->columns[0].x + 
						 m1->columns[1].y * m2->columns[0].y + 
						 m1->columns[2].y * m2->columns[0].z + 
						 m1->columns[3].y * m2->columns[0].w;
	dest->columns[1].y = m1->columns[0].y * m2->columns[1].x + 
						 m1->columns[1].y * m2->columns[1].y + 
						 m1->columns[2].y * m2->columns[1].z + 
						 m1->columns[3].y * m2->columns[1].w;
	dest->columns[2].y = m1->columns[0].y * m2->columns[2].x + 
						 m1->columns[1].y * m2->columns[2].y + 
						 m1->columns[2].y * m2->columns[2].z + 
						 m1->columns[3].y * m2->columns[2].w;
	dest->columns[3].y = m1->columns[0].y * m2->columns[3].x + 
						 m1->columns[1].y * m2->columns[3].y + 
						 m1->columns[2].y * m2->columns[3].z + 
						 m1->columns[3].y * m2->columns[3].w;
	dest->columns[0].z = m1->columns[0].z * m2->columns[0].x + 
						 m1->columns[1].z * m2->columns[0].y + 
						 m1->columns[2].z * m2->columns[0].z + 
						 m1->columns[3].z * m2->columns[0].w;
	dest->columns[1].z = m1->columns[0].z * m2->columns[1].x + 
						 m1->columns[1].z * m2->columns[1].y + 
						 m1->columns[2].z * m2->columns[1].z + 
						 m1->columns[3].z * m2->columns[1].w;
	dest->columns[2].z = m1->columns[0].z * m2->columns[2].x + 
						 m1->columns[1].z * m2->columns[2].y + 
						 m1->columns[2].z * m2->columns[2].z + 
						 m1->columns[3].z * m2->columns[2].w;
	dest->columns[3].z = m1->columns[0].z * m2->columns[3].x + 
						 m1->columns[1].z * m2->columns[3].y + 
						 m1->columns[2].z * m2->columns[3].z + 
						 m1->columns[3].z * m2->columns[3].w;
	dest->columns[0].w = m1->columns[0].w * m2->columns[0].x + 
						 m1->columns[1].w * m2->columns[0].y + 
						 m1->columns[2].w * m2->columns[0].z + 
						 m1->columns[3].w * m2->columns[0].w;
	dest->columns[1].w = m1->columns[0].w * m2->columns[1].x + 
						 m1->columns[1].w * m2->columns[1].y + 
						 m1->columns[2].w * m2->columns[1].z + 
						 m1->columns[3].w * m2->columns[1].w;
	dest->columns[2].w = m1->columns[0].w * m2->columns[2].x + 
						 m1->columns[1].w * m2->columns[2].y + 
						 m1->columns[2].w * m2->columns[2].z + 
						 m1->columns[3].w * m2->columns[2].w;
	dest->columns[3].w = m1->columns[0].w * m2->columns[3].x + 
						 m1->columns[1].w * m2->columns[3].y + 
						 m1->columns[2].w * m2->columns[3].z + 
						 m1->columns[3].w * m2->columns[3].w;

	//float f1 = m1->columns[0].x;
	//float f2 = m2->columns[0].x;
	//float res1, res2, res3, res4;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].x;
	//f2 = m2->columns[0].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].x;
	//f2 = m2->columns[0].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].x;
	//f2 = m2->columns[0].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[0].x = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].x;
	//f2 = m2->columns[1].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].x;
	//f2 = m2->columns[1].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].x;
	//f2 = m2->columns[1].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].x;
	//f2 = m2->columns[1].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[1].x = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].x;
	//f2 = m2->columns[2].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].x;
	//f2 = m2->columns[2].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].x;
	//f2 = m2->columns[2].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].x;
	//f2 = m2->columns[2].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[2].x = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].x;
	//f2 = m2->columns[3].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].x;
	//f2 = m2->columns[3].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].x;
	//f2 = m2->columns[3].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].x;
	//f2 = m2->columns[3].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[3].x = res1 + res2 + res3 + res4;
	//// 
	//f1 = m1->columns[0].y;
	//f2 = m2->columns[0].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].y;
	//f2 = m2->columns[0].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].y;
	//f2 = m2->columns[0].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].y;
	//f2 = m2->columns[0].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[0].y = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].y;
	//f2 = m2->columns[1].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].y;
	//f2 = m2->columns[1].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].y;
	//f2 = m2->columns[1].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].y;
	//f2 = m2->columns[1].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[1].y = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].y;
	//f2 = m2->columns[2].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].y;
	//f2 = m2->columns[2].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].y;
	//f2 = m2->columns[2].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].y;
	//f2 = m2->columns[2].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[2].y = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].y;
	//f2 = m2->columns[3].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].y;
	//f2 = m2->columns[3].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].y;
	//f2 = m2->columns[3].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].y;
	//f2 = m2->columns[3].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[3].y = res1 + res2 + res3 + res4;
	//// 
	//f1 = m1->columns[0].z;
	//f2 = m2->columns[0].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].z;
	//f2 = m2->columns[0].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].z;
	//f2 = m2->columns[0].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].z;
	//f2 = m2->columns[0].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[0].z = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].z;
	//f2 = m2->columns[1].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].z;
	//f2 = m2->columns[1].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].z;
	//f2 = m2->columns[1].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].z;
	//f2 = m2->columns[1].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[1].z = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].z;
	//f2 = m2->columns[2].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].z;
	//f2 = m2->columns[2].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].z;
	//f2 = m2->columns[2].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].z;
	//f2 = m2->columns[2].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[2].z = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].z;
	//f2 = m2->columns[3].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].z;
	//f2 = m2->columns[3].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].z;
	//f2 = m2->columns[3].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].z;
	//f2 = m2->columns[3].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[3].z = res1 + res2 + res3 + res4;
	//// 
	//f1 = m1->columns[0].w;
	//f2 = m2->columns[0].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].w;
	//f2 = m2->columns[0].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].w;
	//f2 = m2->columns[0].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].w;
	//f2 = m2->columns[0].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[0].w = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].w;
	//f2 = m2->columns[1].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].w;
	//f2 = m2->columns[1].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].w;
	//f2 = m2->columns[1].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].w;
	//f2 = m2->columns[1].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[1].w = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].w;
	//f2 = m2->columns[2].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].w;
	//f2 = m2->columns[2].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].w;
	//f2 = m2->columns[2].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].w;
	//f2 = m2->columns[2].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[2].w = res1 + res2 + res3 + res4;
	//f1 = m1->columns[0].w;
	//f2 = m2->columns[3].x;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res1] }
	//f1 = m1->columns[1].w;
	//f2 = m2->columns[3].y;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res2] }
	//f1 = m1->columns[2].w;
	//f2 = m2->columns[3].z;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res3] }
	//f1 = m1->columns[3].w;
	//f2 = m2->columns[3].w;
	//__asm { fld dword ptr [f1] } __asm {fmul dword ptr [f2] } __asm { fst dword ptr [res4] }
	//dest->columns[3].w = res1 + res2 + res3 + res4;
}

////////////////////////////////////////////////////////////
// Faster than vanilla glml transpose().
void Boop3D::FastMat4Transpose( mat4 *dest, mat4 *m ) {
	dest->columns[0].x = m->columns[0].x;
	dest->columns[0].y = m->columns[1].x;
	dest->columns[0].z = m->columns[2].x;
	dest->columns[0].w = m->columns[3].x;
	// 
	dest->columns[1].x = m->columns[0].y;
	dest->columns[1].y = m->columns[1].y;
	dest->columns[1].z = m->columns[2].y;
	dest->columns[1].w = m->columns[3].y;
	// 
	dest->columns[2].x = m->columns[0].z;
	dest->columns[2].y = m->columns[1].z;
	dest->columns[2].z = m->columns[2].z;
	dest->columns[2].w = m->columns[3].z;
	// 
	dest->columns[3].x = m->columns[0].w;
	dest->columns[3].y = m->columns[1].w;
	dest->columns[3].z = m->columns[2].w;
	dest->columns[3].w = m->columns[3].w;
}

////////////////////////////////////////////////////////////
// Draws a single triangle. Performs back-face culling and
// flat shading.
void Boop3D::DrawFilledTri(B3DTriangle &tri, mat4 &filledtrimat, mat4 &_pmtx, COLORREF _tricolor) {

	// Get transformed vert position.
	mat4 v1mtx; mat4 trans1( tri.verts[0].xyz );
	FastMat4Mult( &v1mtx, &filledtrimat, &trans1 );
	mat4 v2mtx; mat4 trans2( tri.verts[1].xyz );
	FastMat4Mult( &v2mtx, &filledtrimat, &trans2 );
	mat4 v3mtx; mat4 trans3( tri.verts[2].xyz );
	FastMat4Mult( &v3mtx, &filledtrimat, &trans3 );
	/*mat4 v1mtx = filledtrimat * mat4( tri.verts[0].xyz );
	mat4 v2mtx = filledtrimat * mat4( tri.verts[1].xyz );
	mat4 v3mtx = filledtrimat * mat4( tri.verts[2].xyz );*/

	///////////////////
	// Camera Cull Test

		// TODO: Fix this. Not able to handle camera on -z.
		// UPDATE - 11.07.2015: Figured out distance to cam on
		// pos z. Neg z distance might be wrong. Still need to
		// add code to check for tris behind camera.
		if(true) {

		// Distance test. If any of the components of the triangle are too close,
		// discard it.
		float dist1 = gdistance( viewmat[3], v1mtx[3] );
		float dist2 = gdistance( viewmat[3], v2mtx[3] );
		float dist3 = gdistance( viewmat[3], v3mtx[3] );

		// Discard triangle if too close.
		if( dist1 < 0.1f || dist2 < 0.1f || dist3 < 0.1f )
			return;

		// Here, we determine if the triangle is in view.
		// If any of the vertices are behind the camera, don't
		// draw the triangle.

		// Create vectors from triangle vertex to camera position.
		vec3 v1vec = vec3( v1mtx[3].x - viewmat[3].x, v1mtx[3].y - viewmat[3].y, v1mtx[3].z - viewmat[3].z );
		vec3 v2vec = vec3( v2mtx[3].x - viewmat[3].x, v2mtx[3].y - viewmat[3].y, v2mtx[3].z - viewmat[3].z );
		vec3 v3vec = vec3( v3mtx[3].x - viewmat[3].x, v3mtx[3].y - viewmat[3].y, v3mtx[3].z - viewmat[3].z );

		// Dot camera's z(at) with these vectors.
		// Positive value - vertex is in front of plane.
		// Negative value - vertex is behind plane.
		// Zero value - vertex is on plane.
		float dotv1z = dot(v1vec, viewmat[2]);
		float dotv2z = dot(v2vec, viewmat[2]);
		float dotv3z = dot(v3vec, viewmat[2]);

		// Discard triangle if behind camera.
		if( dotv1z < 0 || dotv2z < 0 || dotv3z < 0 )
			return;

		// TODO: Add other frustum sides.
		// Far frustum plane.
		{
			// Create vectors from far camera plane to vertices.
			const float fardist = 100.0f;
			vec3 farCamPos = vec3( viewmat[3].x + viewmat[2].x * fardist, viewmat[3].y + viewmat[2].y * fardist, viewmat[3].z  + viewmat[2].z * fardist );
			vec3 v1vec = vec3( v1mtx[3].x - farCamPos.x, v1mtx[3].y - farCamPos.y, v1mtx[3].z - farCamPos.z );
			vec3 v2vec = vec3( v2mtx[3].x - farCamPos.x, v2mtx[3].y - farCamPos.y, v2mtx[3].z - farCamPos.z );
			vec3 v3vec = vec3( v3mtx[3].x - farCamPos.x, v3mtx[3].y - farCamPos.y, v3mtx[3].z - farCamPos.z );

			// Dot with negated camera at vector. This is our far plane normal.
			float dotv1z = dot( v1vec, viewmat[2] * -1.0f );
			float dotv2z = dot( v2vec, viewmat[2] * -1.0f );
			float dotv3z = dot( v3vec, viewmat[2] * -1.0f );

			// Discard triangle if past far camera plane.
			if( dotv1z < 0 || dotv2z < 0 || dotv3z < 0 )
				return;

		}
		/*float dist1 = sqrt( dot(viewmat[3], v1mtx[3]) );
		float dist2 = sqrt( dot(viewmat[3], v2mtx[3]) );
		float dist3 = sqrt( dot(viewmat[3], v3mtx[3]) );*/

	} // if false
	// Camera Cull Test
	///////////////////

	/////////////////////
	// Back-face Culling

		// With back-face culling, we could potentially
		// remove half of the triangles to be rasterized
		// on closed objects.

		// Create vector from vert1 to vert2, and one from vert2 to
		// vert3.
		vec3 vt1tovt2( v2mtx[3].x - v1mtx[3].x, v2mtx[3].y - v1mtx[3].y, v2mtx[3].z - v1mtx[3].z );
		vec3 vt2tovt3( v3mtx[3].x - v2mtx[3].x, v3mtx[3].y - v2mtx[3].y, v3mtx[3].z - v2mtx[3].z );
		// Cross those two vectors.
		vec3 crx_v12_v23 = cross( vt1tovt2, vt2tovt3 );
		// Normalize the vector.
		crx_v12_v23 = normalize( crx_v12_v23 );

		// Create normal from camera position to vertex.
		vec3 camnorm = vec3( v1mtx[3].x - viewmat[3].x, v1mtx[3].y - viewmat[3].y, v1mtx[3].z - viewmat[3].z );
		// vec3 camnorm = viewmat[2];
		camnorm = normalize( camnorm );

		// Dot triangle normal with camera at.
		float normdist = dot(crx_v12_v23, camnorm);

		// Zero or higher value means the triangle is facing the camera.
		// Negative means it is facing away from the camera. Discard.
		if( normdist < 0 )
			return;

	// Back-face Culling
	/////////////////////

	////////////////
	// Flat Shading.
		if( SHADING == SHADING_FLAT ) {
			vec3 tricolor(GetRValue(_tricolor), GetGValue(_tricolor), GetBValue(_tricolor), 1.0f);
			tricolor.x = tricolor.x / 255;
			tricolor.y = tricolor.y / 255;
			tricolor.z = tricolor.z / 255;

			vec3 trinorm = cross(vt1tovt2, vt2tovt3);
			trinorm = normalize( trinorm );
			float colordot = Clamp( dot(trinorm, litemtx[2]) );
			vec3 difflite = liteclr * colordot;
			vec3 finalclr = ambilite * difflite * tricolor;
			_tricolor = RGB(255 * finalclr.x, 255 * finalclr.y, 255 * finalclr.z);
		}
	// Flat Shading.
	////////////////

	// Store triangle in a copy so we aren't modifying
	// the original.
	// B3DTriangle copytri = tri;
	// This is a few frames faster.
	B3DTriangle copytri;
	for( int v = 0; v < 3; v++ ) {
		copytri.verts[v].xyz.x = tri.verts[v].xyz.x;
		copytri.verts[v].xyz.y = tri.verts[v].xyz.y;
		copytri.verts[v].xyz.z = tri.verts[v].xyz.z;
		copytri.verts[v].xyz.w = tri.verts[v].xyz.w;
		// 
		copytri.verts[v].nrm.x = tri.verts[v].nrm.x;
		copytri.verts[v].nrm.y = tri.verts[v].nrm.y;
		copytri.verts[v].nrm.z = tri.verts[v].nrm.z;
		copytri.verts[v].nrm.w = tri.verts[v].nrm.w;
		// 
		copytri.verts[v].clr.x = tri.verts[v].clr.x;
		copytri.verts[v].clr.y = tri.verts[v].clr.y;
		copytri.verts[v].clr.z = tri.verts[v].clr.z;
		copytri.verts[v].clr.w = tri.verts[v].clr.w;
		// 
		copytri.verts[v].uv.x = tri.verts[v].uv.x;
		copytri.verts[v].uv.y = tri.verts[v].uv.y;
	}

	// Sort vertices by y.
	B3DVertex tempv;
	copytri.verts[0].xyz = Project( copytri.verts[0].xyz, &filledtrimat);
	copytri.verts[1].xyz = Project( copytri.verts[1].xyz, &filledtrimat);
	copytri.verts[2].xyz = Project( copytri.verts[2].xyz, &filledtrimat);
	for(int vidx = 0; vidx < 3; vidx++) {
		for(int vidx2 = 0; vidx2 < 3; vidx2++) {
			if( copytri.verts[vidx2].xyz.y > copytri.verts[vidx].xyz.y ) {
				tempv = copytri.verts[vidx];
				copytri.verts[vidx] = copytri.verts[vidx2];
				copytri.verts[vidx2] = tempv;
			}
		}
	}

	// If triangle is outside of bounds of screen,
	// don't bother drawing it.
	//bool drawTri = true;
	//for(int ct = 0; ct < 3; ct++) {
	//	if(copytri.verts[ct].xyz.x < 0 || copytri.verts[ct].xyz.x >= clirect.right ||
	//	   copytri.verts[ct].xyz.y < 0 || copytri.verts[ct].xyz.y >= clirect.bottom) {
	//		drawTri = false;
	//		break;
	//	}
	//}

	//// If just ONE of the vertices was outside of the screen, don't
	//// draw the entire triangle.
	//if(drawTri == false)
	//	return;

	// Get dot between vertex normals and light direction.
	vec3 invnrm1 = copytri.verts[0].nrm;
	invnrm1.x *= -1;
	invnrm1.y *= -1;
	vec3 invnrm2 = copytri.verts[1].nrm;
	invnrm2.x *= -1;
	invnrm2.y *= -1;
	vec3 invnrm3 = copytri.verts[2].nrm;
	invnrm3.x *= -1;
	invnrm3.y *= -1;
	// Move normal into world.
	mat4 nmtx1 = transpose(_pmtx) * mat4(invnrm1);
	mat4 nmtx2 = transpose(_pmtx) * mat4(invnrm2);
	mat4 nmtx3 = transpose(_pmtx) * mat4(invnrm3);
	// This is a little slower than above.
	//mat4 mtr1;
	//FastMat4Transpose( &mtr1, &_pmtx );
	//mat4 mn1( invnrm1 );
	//mat4 nmtx1;
	//FastMat4Mult( &nmtx1, &mtr1, &mn1 );
	////
	//mat4 mtr2;
	//FastMat4Transpose( &mtr2, &_pmtx );
	//mat4 mn2( invnrm2 );
	//mat4 nmtx2;
	//FastMat4Mult( &nmtx2, &mtr2, &mn2 );
	////
	//mat4 mtr3;
	//FastMat4Transpose( &mtr3, &_pmtx );
	//mat4 mn3( invnrm3 );
	//mat4 nmtx3;
	//FastMat4Mult( &nmtx3, &mtr3, &mn3 );
	//
	// Dot with light and clamp.
	float dotn1 = Clamp( dot(nmtx1[3], litemtx[2]) );
	float dotn2 = Clamp( dot(nmtx2[3], litemtx[2]) );
	float dotn3 = Clamp( dot(nmtx3[3], litemtx[2]) );

	// Make typing easier, thank you.
	// At this point, these are no longer vectors.
	// X and y are screen coordinates.
	B3DVertex v1 = copytri.verts[0];
	B3DVertex v2 = copytri.verts[1];
	B3DVertex v3 = copytri.verts[2];

	// Compute inverse slopes.
	float v1v2slope, v1v3slope;
	// Line one slope.
	if( v2.xyz.y - v1.xyz.y )
		v1v2slope = (v2.xyz.x - v1.xyz.x) / (v2.xyz.y - v1.xyz.y);
	else
		v1v2slope = 0;
	// Line two slope.
	if( v3.xyz.y - v1.xyz.y )
		v1v3slope = (v3.xyz.x - v1.xyz.x) / (v3.xyz.y - v1.xyz.y);
	else
		v1v3slope = 0;

	int cursli = 0;
	// Start the thread index at the beginning.
	threadidx = 0;
	dethread = 0;

	// Draw top then bottom of triangle.
	for( int yidx = (int)v1.xyz.y; yidx <= (int)v3.xyz.y; yidx++ ) {
		// HACK: Fix for performance issues when object too close to camera.
		// -100 because some tris are tall and you can see some polys
		// disappearing.
		if(yidx < -100 || yidx >= clirect.bottom)
			break;

		// We only have so many threads.
		if( cursli >= NUM_THREADS ) {
			cursli = 0;
			// CreateThread(0, 0, DrawLineThread, this, 0, NULL);
			// static DWORD WINAPI DrawLineThread(void *param) { return 0; }

			for( int i = 0; i < NUM_THREADS; i++ )
				TrySubmitThreadpoolCallback(DrawLineCallback, this, 0);
			while( dethread < NUM_THREADS ) {}

			// Start the thread index at the beginning.
			threadidx = 0;
			dethread = 0;
		}
		//cursli = 0;
		sli[cursli].y = yidx;
		if( v1v2slope > v1v3slope ) {
			if( yidx < v2.xyz.y ) {
				// Shading values.
				sli[cursli].dota = dotn1;
				sli[cursli].dotb = dotn3;
				sli[cursli].dotc = dotn1;
				sli[cursli].dotd = dotn2;
				// Texture coordinate values.
				sli[cursli].ua = v1.uv.x;
				sli[cursli].ub = v3.uv.x;
				sli[cursli].uc = v1.uv.x;
				sli[cursli].ud = v2.uv.x;
				sli[cursli].va = v1.uv.y;
				sli[cursli].vb = v3.uv.y;
				sli[cursli].vc = v1.uv.y;
				sli[cursli].vd = v2.uv.y;
				// Vertices.
				sli[cursli].v[0] = &tri.verts[0];
				sli[cursli].v[1] = &tri.verts[2];
				sli[cursli].v[2] = &tri.verts[0];
				sli[cursli].v[3] = &tri.verts[1];
				//
				sli[cursli].v2[0] = &v1;
				sli[cursli].v2[1] = &v3;
				sli[cursli].v2[2] = &v1;
				sli[cursli].v2[3] = &v2;
				//
				// Point to transform for mesh.
				sli[cursli].m = &filledtrimat;
				//
				sli[cursli].sclr = _tricolor;
				//
				//DrawTriScanLine(sli[cursli], v1, v3, v1, v2, _tricolor); // Top.
				cursli++;
			}
			else {
				// Shading values.
				sli[cursli].dota = dotn1;
				sli[cursli].dotb = dotn3;
				sli[cursli].dotc = dotn2;
				sli[cursli].dotd = dotn3;
				// Texture coordinate values.
				sli[cursli].ua = v1.uv.x;
				sli[cursli].ub = v3.uv.x;
				sli[cursli].uc = v2.uv.x;
				sli[cursli].ud = v3.uv.x;
				sli[cursli].va = v1.uv.y;
				sli[cursli].vb = v3.uv.y;
				sli[cursli].vc = v2.uv.y;
				sli[cursli].vd = v3.uv.y;
				// Vertices.
				sli[cursli].v[0] = &tri.verts[0];
				sli[cursli].v[1] = &tri.verts[2];
				sli[cursli].v[2] = &tri.verts[1];
				sli[cursli].v[3] = &tri.verts[2];
				//
				sli[cursli].v2[0] = &v1;
				sli[cursli].v2[1] = &v3;
				sli[cursli].v2[2] = &v2;
				sli[cursli].v2[3] = &v3;
				// Point to transform for mesh.
				sli[cursli].m = &filledtrimat;
				//
				sli[cursli].sclr = _tricolor;
				//
				//DrawTriScanLine(sli[cursli], v1, v3, v2, v3, _tricolor); // Bottom.
				cursli++;
			}
		} // if( v1v2slope...
		else {
			if( yidx < v2.xyz.y ) {
				// Shading values.
				sli[cursli].dota = dotn1;
				sli[cursli].dotb = dotn2;
				sli[cursli].dotc = dotn1;
				sli[cursli].dotd = dotn3;
				// Texture coordinate values.
				sli[cursli].ua = v1.uv.x;
				sli[cursli].ub = v2.uv.x;
				sli[cursli].uc = v1.uv.x;
				sli[cursli].ud = v3.uv.x;
				sli[cursli].va = v1.uv.y;
				sli[cursli].vb = v2.uv.y;
				sli[cursli].vc = v1.uv.y;
				sli[cursli].vd = v3.uv.y;
				// Vertices.
				sli[cursli].v[0] = &tri.verts[0];
				sli[cursli].v[1] = &tri.verts[1];
				sli[cursli].v[2] = &tri.verts[0];
				sli[cursli].v[3] = &tri.verts[2];
				//
				sli[cursli].v2[0] = &v1;
				sli[cursli].v2[1] = &v2;
				sli[cursli].v2[2] = &v1;
				sli[cursli].v2[3] = &v3;
				// Point to transform for mesh.
				sli[cursli].m = &filledtrimat;
				//
				sli[cursli].sclr = _tricolor;
				//
				//DrawTriScanLine(sli[cursli], v1, v2, v1, v3, _tricolor); // Top.
				cursli++;
			}
			else {
				// Shading values.
				sli[cursli].dota = dotn2;
				sli[cursli].dotb = dotn3;
				sli[cursli].dotc = dotn1;
				sli[cursli].dotd = dotn3;
				// Texture coordinate values.
				sli[cursli].ua = v2.uv.x;
				sli[cursli].ub = v3.uv.x;
				sli[cursli].uc = v1.uv.x;
				sli[cursli].ud = v3.uv.x;
				sli[cursli].va = v2.uv.y;
				sli[cursli].vb = v3.uv.y;
				sli[cursli].vc = v1.uv.y;
				sli[cursli].vd = v3.uv.y;
				// Vertices.
				sli[cursli].v[0] = &tri.verts[1];
				sli[cursli].v[1] = &tri.verts[2];
				sli[cursli].v[2] = &tri.verts[0];
				sli[cursli].v[3] = &tri.verts[2];
				//
				sli[cursli].v2[0] = &v2;
				sli[cursli].v2[1] = &v3;
				sli[cursli].v2[2] = &v1;
				sli[cursli].v2[3] = &v3;
				// Point to transform for mesh.
				sli[cursli].m = &filledtrimat;
				//
				sli[cursli].sclr = _tricolor;
				//
				//DrawTriScanLine(sli[cursli], v2, v3, v1, v3, _tricolor); // Bottom.
				cursli++;
			}
		}
	} // for(int yidx...

	// Handle leftover lines.
	if (cursli > 0) {

		// Start the thread index at the beginning.
		threadidx = 0;
		dethread = 0;
		// Use a few threads to draw the rest.
		for (int i = 0; i < cursli; i++)
			TrySubmitThreadpoolCallback(DrawLineCallback, this, 0);
		while (dethread < cursli) {}
	}

	// We just rendered triangle.
	// Update amount we've been showing the user.
	trisrendered++;

} // DrawFilledTri()

////////////////////////////////////////////////////////////
// Draws pixels on a scanline using info passed.
// B3DScanLineInfo contains y screen coordinate, light, texture info.
// B3DVertex * 4 are the TWO DEE coordinates for the vertices.
// _lineclr is the color of the parent triangle.
void Boop3D::DrawTriScanLine( B3DScanLineInfo &b3dsli,
							  const B3DVertex &vstart1,
							  const B3DVertex &vend1,
							  const B3DVertex &vstart2,
							  const B3DVertex &vend2,
							  COLORREF _lineclr
							)
{
	// Store color values for a bit.
	int r = GetRValue(_lineclr);
	int g = GetGValue(_lineclr);
	int b = GetBValue(_lineclr);

	// Calc gradient, or how far on the line we are.
	float gradient1 = vstart1.xyz.y != vend1.xyz.y ? (b3dsli.y - vstart1.xyz.y) / (vend1.xyz.y - vstart1.xyz.y): 1;
	float gradient2 = vstart2.xyz.y != vend2.xyz.y ? (b3dsli.y - vstart2.xyz.y) / (vend2.xyz.y - vstart2.xyz.y): 1;

	// Determine start and end points to draw a line between.
	int sx = (int)Interpolate(vstart1.xyz.x, vend1.xyz.x, gradient1);
	int ex = (int)Interpolate(vstart2.xyz.x, vend2.xyz.x, gradient2);

	////////////////////
	// Scan line bounds.

		// Make sure the scan line is even within the bounds of the screen.
		if(sx >= clirect.right || ex < 0)
			return;
		// Clamp scan line.
		if(sx < 0)
			sx = 0;
		if(ex > clirect.right)
			ex = clirect.right;

	// Scan line bounds.
	////////////////////

	// Starting and ending z.
	float z1 = Interpolate(vstart1.xyz.z, vend1.xyz.z, gradient1);
	float z2 = Interpolate(vstart2.xyz.z, vend2.xyz.z, gradient2);

	// Interp for shading values. Start and end.
	float shst = Interpolate( b3dsli.dota, b3dsli.dotb, gradient1 );
	float shen = Interpolate( b3dsli.dotc, b3dsli.dotd, gradient2 );

	// Interpolated texture values.
	float su = 0.0f;
	float eu = 0.0f;
	float sv = 0.0f;
	float ev = 0.0f;
	float sz = 0.0f;
	float ez = 0.0f;

	// Interpolate texture coordinates on Y. (Perspective Correct)
	// This is not the right way...
	/*su = Interpolate(b3dsli.ua / z1, b3dsli.ub / z1, gradient1);
	eu = Interpolate(b3dsli.uc / z2, b3dsli.ud / z2, gradient2);
	sv = Interpolate(b3dsli.va / z1, b3dsli.vb / z1, gradient1);
	ev = Interpolate(b3dsli.vc / z2, b3dsli.vd / z2, gradient2);*/
	// This is better. Perspective Correct.
	if( TEXTURING == TEXTURES_ON ) {
		su = Interpolate(b3dsli.ua / vstart1.xyz.z, b3dsli.ub / vend1.xyz.z, gradient1);
		eu = Interpolate(b3dsli.uc / vstart2.xyz.z, b3dsli.ud / vend2.xyz.z, gradient2);
		sv = Interpolate(b3dsli.va / vstart1.xyz.z, b3dsli.vb / vend1.xyz.z, gradient1);
		ev = Interpolate(b3dsli.vc / vstart2.xyz.z, b3dsli.vd / vend2.xyz.z, gradient2);
		// Originally had sz = 1.0f / Interp(v1s.z, v1e.z, grad1);
		sz = Interpolate(1.0f / vstart1.xyz.z, 1.0f / vend1.xyz.z, gradient1);
		ez = Interpolate(1.0f / vstart2.xyz.z, 1.0f / vend2.xyz.z, gradient2);
	}
	// Affine Texturing.
	if( TEXTURING == TEXTURES_AF ) {
		su = Interpolate(b3dsli.ua, b3dsli.ub, gradient1);
		eu = Interpolate(b3dsli.uc, b3dsli.ud, gradient2);
		sv = Interpolate(b3dsli.va, b3dsli.vb, gradient1);
		ev = Interpolate(b3dsli.vc, b3dsli.vd, gradient2);
	}

	// Fix issue with sx being bigger than ex on certain triangles.
	// Causes triangle to not be drawn at all!
	// Probably because I'm not handling flat-top triangles. Oh well.
	if( sx > ex) {
		float tx = sx;
		sx = ex;
		ex = tx;
		tx = su;
		su = eu;
		eu = tx;
		tx = sv;
		sv = ev;
		ev = tx;
	}

	// Draw the line.
	vec3 texturecolor;
	float zgradient = 0.0f;
	float z = 0.0f;
	int yof = clirect.right * b3dsli.y;
	for(int pixl = sx; pixl < ex; pixl++) {

		// Make sure the pixel is within the
		// viewport limits.
		if( pixl >= clirect.right || pixl < 0 ) {
			continue;
		}
		if( b3dsli.y >= clirect.bottom || b3dsli.y < 0 )
			continue;

		// Z-Buffer.
		zgradient = (float)(pixl - sx) / (float)(ex - sx);
		z = Interpolate(z1, z2, zgradient);
		if (zbuffer[yof + pixl] < z)
			continue;
		zbuffer[yof + pixl] = z;

		// Texture color - White and full alpha.
		texturecolor.x = 1.0f;
		texturecolor.y = 1.0f;
		texturecolor.z = 1.0f;
		texturecolor.w = 1.0f;

		/////////////////////////////////
		// Perspective Correct Texturing.

			if( curMesh->texturebuffer && TEXTURING == TEXTURES_ON ) {
				float u = Interpolate(su, eu, zgradient) * z;
				float v = Interpolate(sv, ev, zgradient) * z;
				int _u = abs( (int)(u * curMesh->txwidth) % curMesh->txwidth );
				int _v = abs( (int)(v * curMesh->txheight) % curMesh->txheight );
				int idx = (_v * curMesh->txwidth + _u) * curMesh->txdepth;
				float r = curMesh->texturebuffer[ idx + 0 ];
				float g = curMesh->texturebuffer[ idx + 1 ];
				float b = curMesh->texturebuffer[ idx + 2 ];
				float a = curMesh->texturebuffer[ idx + 3 ];
				texturecolor = vec3(r / 255, g / 255, b / 255, a / 255);
			}

		// Perspective Correct Texturing.
		/////////////////////////////////

		////////////////////
		// Affine Texturing.

			if( curMesh->texturebuffer && TEXTURING == TEXTURES_AF ) {
				float u = Interpolate(su, eu, zgradient);
				float v = Interpolate(sv, ev, zgradient);
				int _u = abs( (int)(u * curMesh->txwidth) % curMesh->txwidth );
				int _v = abs( (int)(v * curMesh->txheight) % curMesh->txheight );
				int idx = (_v * curMesh->txwidth + _u) * curMesh->txdepth;
				float r = curMesh->texturebuffer[ idx + 0 ];
				float g = curMesh->texturebuffer[ idx + 1 ];
				float b = curMesh->texturebuffer[ idx + 2 ];
				float a = curMesh->texturebuffer[ idx + 3 ];
				texturecolor = vec3(r / 255, g / 255, b / 255, a / 255);
			}

		// Affine Texturing.
		////////////////////

		/////////////////////////////////
		// Perspective-correct Texturing.

			//if( curMesh->texturebuffer && TEXTURING == TEXTURES_ON && false ) {
			//
			//	float U1 = b3dsli.v[0]->uv.x;
			//	float V1 = b3dsli.v[0]->uv.y;
			//	float Z1 = b3dsli.v[0]->xyz.z;
			//	float U2 = b3dsli.v[2]->uv.x;
			//	float V2 = b3dsli.v[2]->uv.y;
			//	float Z2 = b3dsli.v[2]->xyz.z;
			//
			//	float scru1 = U1 / Z1;
			//	float scrv1 = V1 / Z1;
			//	float scrz1 = 1 / Z1;
			//	float scru2 = U2 / Z2;
			//	float scrv2 = V2 / Z2;
			//	float scrz2 = 1 / Z2;
			//
			//	// float tgrad = (scru2 - scru1) / (vend1.xyz.y - vstart1.xyz.y)
			//
			//	float uu = Interpolate(scru1, scru2, zgradient);
			//	float vv = Interpolate(scrv1, scrv2, zgradient);
			//	float zz = Interpolate(scrz1, scrz2, zgradient);
			//
			//
			//
			//	float deltascru = 0;
			//	float deltascrv = 0;
			//	float deltascrz = 0;
			//
			//	float recip = 1.0f / zz;
			//	int _z = 1;
			//	int _u = (uu / zz) * recip;
			//	int _v = (vv / zz) * recip;
			//
			//	scru1 += deltascru;
			//	scrv1 += deltascrv;
			//	scrz1 += deltascrz;
			//
			//	int idx = ( ((int)_v * curMesh->txwidth + (int)_u) * curMesh->txdepth );
			//	if(idx < 0)
			//		idx = 0;
			//	if(idx > curMesh->txwidth * curMesh->txheight * curMesh->txdepth)
			//		idx = curMesh->txwidth * curMesh->txheight * curMesh->txdepth - 1;
			//	// int idx = ((_v * curMesh->txwidth + _u) * curMesh->txdepth);
			//	float r = curMesh->texturebuffer[ idx + 0 ];
			//	float g = curMesh->texturebuffer[ idx + 1 ];
			//	float b = curMesh->texturebuffer[ idx + 2 ];
			//	float a = curMesh->texturebuffer[ idx + 3 ];
			//	texturecolor = vec3(r / 255, g / 255, b / 255, a / 255);
			//}

		// Perspective-correct Texturing.
		/////////////////////////////////

		/////////////////////////////////
		// Perspective-correct Texturing.

			//if( curMesh->texturebuffer && TEXTURING == TEXTURES_ON ) {
			//
			//	// filledtrimat * mat4( tri.verts[0].xyz );
			//
			//	mat4 v1mtx = *b3dsli.m * mat4( b3dsli.v[0]->xyz );
			//	mat4 v2mtx = *b3dsli.m * mat4( b3dsli.v[1]->xyz );
			//	mat4 v3mtx = *b3dsli.m * mat4( b3dsli.v[2]->xyz );
			//
			//	/*mat4 v1mtx = mat4( b3dsli.v[0]->xyz ) * *b3dsli.m;
			//	mat4 v2mtx = mat4( b3dsli.v[1]->xyz ) * *b3dsli.m;
			//	mat4 v3mtx = mat4( b3dsli.v[2]->xyz ) * *b3dsli.m;*/;
			//
			//	vec3 Bp = v1mtx[3];
			//	vec3 Up = v2mtx[3] - v1mtx[3];
			//	vec3 Vp = v3mtx[3] - v1mtx[3];
			//	Up = normalize(Up);
			//	Vp = normalize(Vp);
			//	/*vec3 Up = vec3(1, 0);
			//	vec3 Vp = vec3(0, -1);*/
			//
			//	vec3 Us = cross(Bp, Vp);
			//	vec3 Vs = cross(Up, Bp);
			//	vec3 Zs = cross(Vp, Up);
			//	Us = normalize(Us);
			//	Vs = normalize(Vs);
			//	Zs = normalize(Zs);
			//
			//
			//	float _z = fabs( (Zs.z + Zs.y * b3dsli.y + Zs.x * pixl) );
			//	float _u = fabs( (Us.z + Us.y * b3dsli.y + Us.x * pixl) ) - clirect.right / 2;
			//	float _v = fabs( (Vs.z + Vs.y * b3dsli.y + Vs.x * pixl) ) - clirect.bottom / 2;
			//	// float _z = 25;
			//
			//	int idx = ( ((int)_v * curMesh->txwidth + (int)_u) * curMesh->txdepth );
			//	if(idx < 0)
			//		idx = 0;
			//	if(idx > curMesh->txwidth * curMesh->txheight * curMesh->txdepth)
			//		idx = curMesh->txwidth * curMesh->txheight * curMesh->txdepth - 1;
			//	// int idx = ((_v * curMesh->txwidth + _u) * curMesh->txdepth);
			//	float r = curMesh->texturebuffer[ idx + 0 ];
			//	float g = curMesh->texturebuffer[ idx + 1 ];
			//	float b = curMesh->texturebuffer[ idx + 2 ];
			//	float a = curMesh->texturebuffer[ idx + 3 ];
			//	texturecolor = vec3(r / 255, g / 255, b / 255, a / 255);
			//}

		// Perspective-correct Texturing.
		/////////////////////////////////

		/////////////////////////////////
		// Perspective-correct Texturing.

			//if( curMesh->texturebuffer && TEXTURING == TEXTURES_ON ) {
			//
			//	// Texture values. Point/Vector/Vector
			//	// P = (0,1), M = (1,1)-(0,1), N = (0,0)-(0,1)
			//	// P=P1, M=P2-P1, N=P3-P1 etc
			//
			//	// Points for texture.
			//	vec3 Pp = vec3(0, 1);
			//	vec3 Mp = vec3(1, 1);
			//	vec3 Np = vec3(0, 0);
			//	// Create matrices for these points, and transform.
			//	mat4 Pm = *b3dsli.m * mat4(Pp);
			//	mat4 Mm = *b3dsli.m * mat4(Mp);
			//	mat4 Nm = *b3dsli.m * mat4(Np);
			//
			//	/*mat4 Pm = mat4(Pp) * *b3dsli.m;
			//	mat4 Mm = mat4(Mp) * *b3dsli.m;
			//	mat4 Nm = mat4(Np) * *b3dsli.m;*/
			//	// Store texture origin. Calc M and N vectors.
			//	vec3 Pv = Pm[3];
			//	vec3 Mv = Mm[3] - Pm[3];
			//	vec3 Nv = Nm[3] - Pm[3];
			//	Mv = normalize(Mv);
			//	Nv = normalize(Nv);
			//	// Calc "3 Magic Vectors/9 Magic Constants".
			//	vec3 A = cross(Pv, Nv);
			//	vec3 B = cross(Mv, Pv);
			//	vec3 C = cross(Nv, Mv);
			//	A = normalize(A);
			//	B = normalize(B);
			//	C = normalize(C);
			//	// Screen Vector? Uses current pixel position and
			//	// near plane depth.
			//	vec3 S = vec3( (float)pixl, (float)b3dsli.y, 375/*(float)clirect.right*/ );
			//	// More Magic. :/
			//	float ad = -dot(S, A) * curMesh->txwidth;
			//	float bd = dot(S, B) * curMesh->txheight;
			//	float cd = dot(S, C);
			//	// Finally calc uv.
			//	int _u = int(ad / cd);
			//	int _v = int(bd / cd);
			//	int idx = ((_v * curMesh->txwidth + _u) * curMesh->txdepth);
			//	if(idx < 0) idx = 0;
			//	if(idx > curMesh->txwidth * curMesh->txheight * curMesh->txdepth)
			//		idx = curMesh->txwidth * curMesh->txheight * curMesh->txdepth - 1;
			//	// int idx = ((_v * curMesh->txwidth + _u) * curMesh->txdepth);
			//	float r = curMesh->texturebuffer[ idx + 0 ];
			//	float g = curMesh->texturebuffer[ idx + 1 ];
			//	float b = curMesh->texturebuffer[ idx + 2 ];
			//	float a = curMesh->texturebuffer[ idx + 3 ];
			//	texturecolor = vec3(r / 255, g / 255, b / 255, a / 255);
			//}

		// Perspective-correct Texturing.
		/////////////////////////////////

		/////////////////////////////////
		// Perspective-correct Texturing.

			//if( curMesh->texturebuffer && TEXTURING == TEXTURES_ON && false  ) {
			//
			//	B3DVertex v1 = *b3dsli.v[0];
			//	B3DVertex v2 = *b3dsli.v[1];
			//	B3DVertex v3 = *b3dsli.v[2];
			//	B3DVertex v4 = *b3dsli.v[3];
			//
			//
			//	//vec3 Up = v2 - v1;
			//	//vec3 Vp = v3 - v1;
			//	//Up = normalize(Up);
			//	//Vp = normalize(Vp);
			//	//vec3 sZ = cross(Up, Vp); // Face normal.
			//	//vec3 sU = cross(Vp, v1); // Right.
			//	//vec3 sV = cross(v1, Up); // Up.
			//	//mat4 uvmtx;
			//	//uvmtx[0] = sU;
			//	//uvmtx[1] = sV;
			//	//uvmtx[2] = sZ;
			//
			//
			//
			//	/*int u = a * curMesh->txwidth / c;
			//	int v = b * curMesh->txwidth / c;*/
			//
			//	float u = Interpolate(su, eu, zgradient);
			//	float v = Interpolate(sv, ev, zgradient);
			//
			//
			//	float ssu = u / (1 / z);
			//	float ssv = v / (1 / z);
			//
			//	int _u = abs( (int)(ssu) / (1 / z) );
			//	int _v = abs( (int)(ssv) / (1 / z) );
			//	/*int _u = int(u / sz);
			//	int _v = int(v / sz);*/
			//	int idx = ((_v * curMesh->txwidth + _u) * curMesh->txdepth);
			//	/*int idx = 0;*/
			//	float r = curMesh->texturebuffer[ idx + 0 ];
			//	float g = curMesh->texturebuffer[ idx + 1 ];
			//	float b = curMesh->texturebuffer[ idx + 2 ];
			//	float a = curMesh->texturebuffer[ idx + 3 ];
			//	texturecolor = vec3(r / 255, g / 255, b / 255, a / 255);
			//}

		// Perspective-correct Texturing.
		/////////////////////////////////

		// Pixel Color.
		// 255 = 1.0f. 128 = 0.5f. 0 = 0.0f.
		vec3 pixclr( (float)r / 255,
					 (float)g / 255,
					 (float)b / 255 );
		// This line very slow. Gained about 30fps after rolling it out.
		// vec3 * operator creates a bunch of temporary objects.
		// pixclr = pixclr * texturecolor;
		pixclr.x = pixclr.x * texturecolor.x;
		pixclr.y = pixclr.y * texturecolor.y;
		pixclr.z = pixclr.z * texturecolor.z;
		pixclr.w = pixclr.w * texturecolor.w;

		///////////////////
		// Gouraud Shading.
			if( SHADING == SHADING_GOURAUD ) {
				// Add lighting/shading value to pixel color.
				float ndotl = Interpolate(shst, shen, zgradient);
				vec3 diffuse = liteclr * ndotl;
				pixclr = ambilite * diffuse * pixclr;
			}
		// Gouraud Shading.
		///////////////////

		// Put pixel.
		int startoff = yof * 4 + pixl * 4;
		bmbuffer[startoff + 0] = (unsigned char)(255 * pixclr.z);
		bmbuffer[startoff + 1] = (unsigned char)(255 * pixclr.y);
		bmbuffer[startoff + 2] = (unsigned char)(255 * pixclr.x);
		bmbuffer[startoff + 3] = 0xFF;

	} // for(int pixl...

} // DrawTriScanLine()

////////////////////////////////////////////////////////////
// Ensures value doesn't go above or below max/min.
float Boop3D::Clamp(float value, float min/* = 0*/, float max/* = 1*/) {
	if(value < 0) return 0;
	if(value > 1) return 1;
	return value;
} // Clamp()

////////////////////////////////////////////////////////////
// Returns value that is the gradient/percentage between
// max and min. For example: 25 - 15, x 0.5, + 15 = 20.
// The value 20 is the halfway value between 25 and 15.
float Boop3D::Interpolate(float min, float max, float gradient) {
	return min + (max - min) * Clamp(gradient);
} // Interpolate()

////////////////////////////////////////////////////////////
// Takes a triangle and matrix. Draws 3 lines between vertices
// instead of a filled triangle.
void Boop3D::DrawWireFrameTri(B3DTriangle &tri, mat4 &wiretrimat) {
	// If just ONE point is outside of the drawing area, skip whole
	// triangle. May add logic to project point to collision
	// position in view area with line.
	vec3 pnt = Project( tri.verts[0].xyz, &wiretrimat );
	if( !IsPointInView(pnt) ) return;
	MoveToEx(hdcMem, (int)pnt.x, (int)pnt.y, NULL);

	pnt = Project( tri.verts[1].xyz, &wiretrimat );
	if( !IsPointInView(pnt) ) return;
	LineTo(hdcMem, (int)pnt.x, (int)pnt.y);

	pnt = Project( tri.verts[2].xyz, &wiretrimat );
	if( !IsPointInView(pnt) ) return;
	LineTo(hdcMem, (int)pnt.x, (int)pnt.y);

	pnt = Project( tri.verts[0].xyz, &wiretrimat );
	if( !IsPointInView(pnt) ) return;
	LineTo(hdcMem, (int)pnt.x, (int)pnt.y);

} // DrawWireFrameTri()

////////////////////////////////////////////////////////////
// Checks point against screen boundaries.
bool Boop3D::IsPointInView(vec3 _point) {
	if( _point.x < clirect.left || _point.x > clirect.right )
		return false;
	if( _point.y < clirect.top || _point.y > clirect.bottom )
		return false;
	return true;
} // IsPointInView()

////////////////////////////////////////////////////////////
// Pass in one of the shading #defines at the top of this
// file. SHADING_WIRE, SHADING_NONE, SHADING_FLAT, SHADING_GOURAUD.
void Boop3D::SetShading(int shadetype) {
	SHADING = shadetype;
} // SetShading()

////////////////////////////////////////////////////////////
// Pass in one of the texturing #defines at the top of this
// file. TEXTURES_OFF, TEXTURES_ON
void Boop3D::SetTextures(int setting) {
	TEXTURING = setting;
} // SetTextures()

////////////////////////////////////////////////////////////
// Load 32bit .bmp.
// szPath - Path to .bmp file.
// ppBuffer - Pointer to pointer. Function creates buffer and points pointer to it.
// pWidth/pHeight/pDepth - If not zero, will set variable to dimension wanted.
void Boop3D::LoadBmp(const char *szPath,
					 unsigned char **ppBuffer,
					 int *pWidth = 0,
					 int *pHeight = 0,
					 int *pDepth = 0)
{
	// Handles for bitmap loadin'.
	HBITMAP hBitMap = 0;
	BITMAP bmpBitMap;
	wchar_t convPath[100] = {0};
	int stringLen = strlen(szPath);
	for(int c = 0; c < stringLen; c++)
		convPath[c] = szPath[c];

	// Load it.
	hBitMap = (HBITMAP)LoadImage(NULL, szPath, IMAGE_BITMAP,
								 0, 0,
								 LR_LOADFROMFILE | LR_CREATEDIBSECTION);

	/*hBitMap = SHLoadDIBitmap(convPath);*/
	// If it didn't load, return error.
	if(!hBitMap)
	{
		// Make NULL.
		*ppBuffer = NULL;
		return;

	} // if(!hBitmap)

	// Use this to get info and pointer to bits.
	GetObject(hBitMap, sizeof(bmpBitMap), &bmpBitMap);

	// Calculate the number of bytes in the image.
	int iNumBytes = bmpBitMap.bmWidth *
					bmpBitMap.bmHeight *
					(bmpBitMap.bmBitsPixel / 8);

	// Reverse the channels.
	// For every pixel, switch the bytes around.
	unsigned char bRed = 0; unsigned char bBlue = 0;
	for( int iByte = 0; iByte < iNumBytes; iByte += (bmpBitMap.bmBitsPixel / 8) )
	{
		// Switch it!
		bRed = ((unsigned char *)bmpBitMap.bmBits)[iByte];
		bBlue = ((unsigned char *)bmpBitMap.bmBits)[iByte + 2];
		((unsigned char *)bmpBitMap.bmBits)[iByte] = bBlue;
		((unsigned char *)bmpBitMap.bmBits)[iByte + 2] = bRed;

	} // for(int iByte...

	// Fill variables.
	if(pWidth)
		*pWidth = bmpBitMap.bmWidth;
	if(pHeight)
		*pHeight = bmpBitMap.bmHeight;
	if(pDepth)
		*pDepth = bmpBitMap.bmBitsPixel / 8;
	if(ppBuffer)
	{
		// Create buffer.
		*ppBuffer = new unsigned char[iNumBytes];

		// Fill.
		for(int iCurByte = 0; iCurByte < iNumBytes; iCurByte++)
			(*ppBuffer)[iCurByte] = ((unsigned char *)bmpBitMap.bmBits)[iCurByte];
	}

	// Delete temp image data.
	DeleteObject(hBitMap);

} // LoadBmp()

//////////////////
// Camera Controls

	////////////////////////////////////////////////////////////////////
	// Set camera matrix to a look-at matrix.
	// eye - position, center - where it's looking, up - up vector(0,1,0)
	void Boop3D::CameraLookat(vec3 eye, vec3 center, vec3 up) {
		vieweye = eye; viewcenter = center; viewup = up;
		viewmat = lookat( vieweye, viewcenter, viewup );
	}
	////////////////////////////////////////////////////////////////////
	// Moves camera position to given, and moves camera's lookat point.
	// Not additive.
	void Boop3D::CameraStrafeTo(vec3 pos) {
		// Get vector from old position to new position.
		vec3 viewvec = pos - vieweye;
		// Add this vector to the center(lookat) position.
		viewcenter = viewcenter + viewvec;
		// Update camera position.
		vieweye = pos;
		// Build new camera matrix.
		viewmat = lookat(vieweye, viewcenter, viewup);
	}
	////////////////////////////////////////////////////////////////////
	// Moves camera position in direction, and moves camera's lookat point.
	// Additive, which means it uses the vector to move in a direction.
	// Doesn't explicitly set camera position.
	void Boop3D::CameraStrafeToA(vec3 dir) {
		// Update camera position and center.
		vieweye = vieweye + dir;
		viewcenter = viewcenter + dir;
		// Build new camera matrix.
		viewmat = lookat(vieweye, viewcenter, viewup);
	}

// Camera Controls
//////////////////

///////////////////////////////////////////////////////////////////////////////
// Returns the Device Context Boop3D is using to draw.
HDC Boop3D::GetDCtxt( void ) {
	return deviceContext;
}

///////////////////////////////////////////////////////////////////////////////
// Get double buffer mem context.
HDC Boop3D::GetBackbuffer( void ) {
	return hdcMem;
}
