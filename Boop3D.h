/*
	TODO:
		PERFORMANCE:
			* DONE - Add references to functions so we're not copying so much.
			* Switch to TemplateLinkedList3.5.h instead of vector. Test speed.
			* Possibly switch to contiguous arrays of simple types instead of
			  B3DVertex.
			* DONE - Draw to buffer instead of Window's calls to DC.
			* DONE - Back-face Culling.
			* Implement pixel and line drawing algorithms.
			* Inline Assembly?
			* Store potientially reusable variables.
			* << and >> when you can.
			* Triple-check Render() method and any methods it calls.
			* DONE - Threads!
			* Check [] with matrices and vectors. Switch to explicit
			vec3.x instead of vec3[0]. Same with mat4.
			* Cut down on calculations.
			- Make sure we're not doing calculations more than once.
			* DONE - NO DIFFERENCE - Use own FillRect algorithm. Check performance.
			* Common matrix calculations could be stored. Rotations, for example.
			* DONE - When indexing buffers, don't calculate the index every time.
			buff[y * w + x] = 2; Pull it out!
			int row = y * w;
			buff[row + x] = 2;
			* Switch statements?
			* Speed up texturing.
			* Speed up flat shading.
			* Speed up gouraud shading.
			* DONE - If a high poly(6000+) mesh is very close to the camera,
			the scane line algorithm takes forever to draw. Fix.
			* Implement a quad-tree object hierarchy. Can use with culling
			code. Would reduce a million checks against the frustum to just
			a few.
		STYLE:
			* Lower-case-ify local variables.
			* Underscore approriate function parameters.
			* Make variables more obvious.
			* Use unsigned where appropriate.
		IMPLEMENTATION:
			* C++11/14?
			* Smart Pointers.
			* Switch away from COLORREF to vec3.
			* Draw to char buffer instead of a memory context.
		DOCUMENTATION:
			* Header comment - Name, Author, Date, Usage, Notes, etc.
			* Comments... everywhere.
			* Function Header-comments.
		FEATURE TODO:
			* Perspective-correct texture mapping.
			* Multitexturing.
			* Bump/Normal Mapping.
			* Done - Add function to retrieve mesh in list.
			* Camera functions(translate, rotate, etc.)
			* Mesh functions(translate, scale, rotate, etc.)
			* Change textures on the fly.
			* Switch on/off back-face culling.
			* Allow resizing of color/depth buffers.
			- ResizeView()
		OTHER:
			* Remove commented code unless it's pertinent.
	============================================================================
	EXPORTING MESHES:
		* Boop uses meshes in .obj format. You can export these from programs like Blender.
		* Currently, Boop can only handle single object meshes. No grouping/parent,child
		hierarchies.
		* Before exporting:
		- Ctrl + J - Join(Multiple Objects become one)
		- Tab into edit mode. Select all verts.
		- You may have to Ctrl + N to recalc normals.
		- Hit U, Select Unwrap. You now have UV's.
		- Create a new view, switch it to UV Editor.
		-- Make sure there are UVS.
		- May have to scale things(-1 on x or z)
		- Only things selected when exporting to .obj should be: Write Normals,
		Include UVs, Triangulate Faces, and Keep Vertex Order.
	============================================================================
	CODE USAGE:
		// Call these once to initialize.
		Boop3D boop; // ...or Boop3D boop(HWND); Won't need to call initialize().
		boop.Initialize(HWND);
		boop.LoadMesh("mesh.obj", "red.bmp");
		boop.GetMesh(0)->matrix.columns[3].x = 1.0f; // Put's mesh zero on positive x.
		// Put this in your main loop.
		boop.Render();
		// Don't need to call, but you can if you want.
		boop.Shutdown();

*/
// Only #include this file once.
#ifndef BOOP_3D
	#define BOOP_3D

////////////
// #INCLUDES

	// Windows specific vars and functions.
	#include <windows.h>
	// Lightweight version of GLM.
	#include "GLM_Lite.h"
	// Std.
	#include <vector>
	#include <string>
	using namespace std;
	// fabs() and such.
	#include <math.h>

// #INCLUDES
////////////

///////////
// #DEFINES

	// Shading/drawing modes: Wireframe, No Shading, Flat Shading,
	// Smooth/Gouraud Shading.
	// Set SHADING to one of these defines.
	#define SHADING_WIRE	0
	#define SHADING_NONE	1
	#define SHADING_FLAT	2
	#define SHADING_GOURAUD 3
	static int SHADING = SHADING_GOURAUD;
	// Set TEXTURING to one of these. Not used in
	// SHADING_WIRE mode.
	#define TEXTURES_OFF 0
	#define TEXTURES_ON  1
	#define TEXTURES_AF  2
	static int TEXTURING = TEXTURES_ON;

// #DEFINES
///////////

// Forward declaration of our vertex.
struct B3DVertex;

// Information passed to scanline drawing function.
struct B3DScanLineInfo {
	int y;
	float dota;
	float dotb;
	float dotc;
	float dotd;
	float ua, ub, uc, ud;
	float va, vb, vc, vd;
	B3DVertex *v[4];
	mat4 *m;
	B3DVertex *v2[4];
	COLORREF sclr;
};

// Vertices have position, normals, texture coordinates, and color.
struct B3DVertex {
	vec3 xyz;
	vec3 nrm;
	vec3 uv;
	vec3 clr;
};
// Three vertices per triangle.
struct B3DTriangle {
	B3DVertex verts[3];
};
// 3D objects that are displayed. Can have a variable number of triangles.
// Features are transform(position/orientation), dimensions of mesh,
// texture(no multitexturing yet), and texture dimensions.
struct B3DMesh {
	vector<B3DTriangle> tris;
	B3DTriangle *ptris;
	mat4 matrix;
	float width, height, depth;
	unsigned char *texturebuffer;
	int txwidth, txheight, txdepth;
};

// For-dec so ThreadStruct knows what's going on.
class Boop3D;

// Multithreading Struct. Pass a unique one to the scan line
// drawing thread func.
#define NUM_THREADS (1)
struct ThreadStruct {
	B3DScanLineInfo *sli;
	Boop3D *bp;
};

// 3D Win32 Software Renderer
class Boop3D
{
	public:
		// Allows each thread unique data to work with.
		volatile long threadidx;
		volatile long dethread;
		ThreadStruct ts[NUM_THREADS];
		// The window we draw to.
		HWND windowHandle;
		// Device and memory contexts for drawing.
		HDC deviceContext;
		HDC hdcMem;
		HBITMAP hbmMem;
		HANDLE hOld;
		// DIB - Direct access to bitmap/buffer. We draw to this then
		// blit to screen.
		unsigned char *bmbuffer;
		// Colors for mesh and wireframe.
		COLORREF colorRef;
		COLORREF greenColor;
		HPEN hBluePen;
		HPEN hGreenPen;
		HPEN hRedPen;
		HPEN hPen;
		// List of meshes to draw.
		vector<B3DMesh> meshlist;
		// Projection and view matrices.
		mat4 projmat, viewmat;
		vec3 vieweye, viewcenter, viewup;
		// Z-Buffer - client area width * height. For depth testing.
		float *zbuffer;
		// Debug info displayed over 3D.
		int fps;		    // Frames per second.
		int trisrendered;   // Triangles rendered.
		int fpstimer;	    // For timing fps updates.
		char fpsstr[10];    // Frames per second string.
		char trirenstr[10]; // Triangles Rendered string.
		// Dimensions of our client area that we draw to.
		RECT clirect;
		// Current scan line.
		B3DScanLineInfo sli[NUM_THREADS];
		// Light Info: Color, transform, ambient light.
		// Transform can change the direction of the light.
		// Ambient light can raise or lower the base light in the scene.
		vec3 liteclr;
		mat4 litemtx;
		vec3 ambilite;
		// Current mesh being rendered.
		B3DMesh *curMesh;

	public:
		////////////////////////////////////////////////////////////
		// Constructor.
		Boop3D();
		////////////////////////////////////////////////////////////
		// Parameterized Constructor.
		Boop3D(HWND winHandle);
		////////////////////////////////////////////////////////////
		// Destructor.
		~Boop3D(void);

		////////////////////////////////////////////////////////////
		// The BIG initializer function. Needs to be called at least
		// once before calling anything else. Called for you if
		// creating a Boop3D object with Boop3D(HWND);
		//
		// Creates contexts, sets up memory and lists,
		// inits matrices, buffers, everything.
		void Initialize(HWND _winHandle);

		////////////////////////////////////////////////////////////
		// Cleans up memory, deletes handles, buffers, etc.
		// Shouldn't need to call explicitly, as it's invoked from
		// our destructor. Just let Boop go out of scope.
		void Shutdown(void);

		////////////////////////////////////////////////////////////
		// Called automatically in Render(). Clears our backbuffer
		// and the zbuffer.
		void Clear();

		////////////////////////////////////////////////////////////
		// This is put into your window procedure for the message:
		// WM_SIZE.
		//
		// TODO: Currently, it's not really needed because Boop cannot
		// handle resizing. Will be added later. Upon calling this,
		// Boop3D should resize color and depth/z buffers.
		void ResizeView(int w, int h);

		////////////////////////////////////////////////////////////
		// OBJ Loader. Pass a file path and texture path. Refer
		// to EXPORTING MESHES for object file info.
		// Textures are also 32bit .bmp's.
		void LoadMesh(string filepath, string texturepath);

		////////////////////////////////////////////////////////////
		// Retrieves mesh at index idx.
		B3DMesh *GetMesh(int idx);

		////////////////////////////////////////////////////////////
		// Takes a point and a matrix and "projects" it into the
		// screen.
		vec3 Project( vec3 point, mat4 *mvpmatrix );

		////////////////////////////////////////////////////////////
		// Called in a loop. Traverses mesh list and draws them all.
		void Render(void);

		////////////////////////////////////////////////////////////
		// Accepts a single mesh and renders its triangles to screen.
		// [Optional] Will also accept a pointer to another transform to use
		// instead of the one the mesh is using.
		void DrawMesh( B3DMesh &m, mat4 *trans = 0 );

		////////////////////////////////////////////////////////////
		// If using the one-shot DrawMesh() or you've been drawing
		// directly to our backbuffer, call this to render every-
		// thing.
		void Blit( void );

		////////////////////////////////////////////////////////////
		// Faster than vanilla mat4 *operator.
		void FastMat4Mult( mat4 *dest, mat4 *m1, mat4 *m2 );

		////////////////////////////////////////////////////////////
		// Faster than vanilla glml transpose().
		void FastMat4Transpose( mat4 *dest, mat4 *m );

		////////////////////////////////////////////////////////////
		// Draws a single triangle. Performs back-face culling and
		// flat shading.
		void DrawFilledTri(B3DTriangle &tri, mat4 &filledtrimat, mat4 &_pmtx, COLORREF _tricolor);

		////////////////////////////////////////////////////////////
		// Draws pixels on a scanline using info passed.
		// B3DScanLineInfo contains y screen coordinate, light, texture info.
		// B3DVertex * 4 are the TWO DEE coordinates for the vertices.
		// _lineclr is the color of the parent triangle.
		void DrawTriScanLine( B3DScanLineInfo &b3dsli,
							  const B3DVertex &vstart1,
							  const B3DVertex &vend1,
							  const B3DVertex &vstart2,
							  const B3DVertex &vend2,
							  COLORREF _lineclr
							);

		////////////////////////////////////////////////////////////
		// Ensures value doesn't go above or below max/min.
		float Clamp(float value, float min = 0, float max = 1);

		////////////////////////////////////////////////////////////
		// Returns value that is the gradient/percentage between
		// max and min. For example: 25 - 15, x 0.5, + 15 = 20.
		// The value 20 is the halfway value between 25 and 15.
		float Interpolate(float min, float max, float gradient);

		////////////////////////////////////////////////////////////
		// Takes a triangle and matrix. Draws 3 lines between vertices
		// instead of a filled triangle.
		void DrawWireFrameTri(B3DTriangle &tri, mat4 &wiretrimat);

		////////////////////////////////////////////////////////////
		// Checks point against screen boundaries.
		bool IsPointInView(vec3 _point);

		////////////////////////////////////////////////////////////
		// Pass in one of the shading #defines at the top of this
		// file. SHADING_WIRE, SHADING_NONE, SHADING_FLAT, SHADING_GOURAUD.
		void SetShading(int shadetype);

		////////////////////////////////////////////////////////////
		// Pass in one of the texturing #defines at the top of this
		// file. TEXTURES_OFF, TEXTURES_ON
		void SetTextures(int setting);

		////////////////////////////////////////////////////////////
		// Load 32bit .bmp.
		// szPath - Path to .bmp file.
		// ppBuffer - Pointer to pointer. Function creates buffer and points pointer to it.
		// pWidth/pHeight/pDepth - If not zero, will set variable to dimension wanted.
		void LoadBmp(const char *szPath,
					 unsigned char **ppBuffer,
					 int *pWidth/* = 0*/,
					 int *pHeight/* = 0*/,
					 int *pDepth/* = 0*/);

		//////////////////
		// Camera Controls

			////////////////////////////////////////////////////////////////////
			// Set camera matrix to a look-at matrix.
			// eye - position, center - where it's looking, up - up vector(0,1,0)
			void CameraLookat(vec3 eye, vec3 center, vec3 up);
			////////////////////////////////////////////////////////////////////
			// Moves camera position to given, and moves camera's lookat point.
			// Not additive.
			void CameraStrafeTo(vec3 pos);
			////////////////////////////////////////////////////////////////////
			// Moves camera position in direction, and moves camera's lookat point.
			// Additive, which means it uses the vector to move in a direction.
			// Doesn't explicitly set camera position.
			void CameraStrafeToA(vec3 dir);

		// Camera Controls
		//////////////////

		/////////////////////////////////////////////////////////////////////////////
		// Returns the Device Context Boop3D is using to draw.
		HDC GetDCtxt( void );

		/////////////////////////////////////////////////////////////////////////////
		// Returns double buffer mem context.
		HDC GetBackbuffer( void );

};

#endif // BOOP_3D
