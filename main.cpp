// Boop.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Boop.h"
#include "Boop3D.h"

// Memory leak detection!!!!!
// Call _CrtDumpMemoryLeaks after main has returned and before program terminates.
// #define _CRTDBG_MAP_ALLOC
// #include <stdlib.h>
// #include <crtdbg.h>
//struct AtExit { ~AtExit() { _CrtDumpMemoryLeaks(); } } doAtExit;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[] = "BOOP 3D";					// The title bar text
TCHAR szWindowClass[] = "Window Class";			// the main window class name
HWND hWnd;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

///////// 3D ////////////////
Boop3D boop;

// Console window.
HANDLE wincon;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
//	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
//	LoadString(hInstance, IDC_BOOP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BOOP));

	// Pass main window to Boop3D.
	boop.Initialize(hWnd);
	boop.SetShading(SHADING_GOURAUD);
	boop.SetTextures(TEXTURES_ON);
	boop.CameraStrafeTo( vec3(0, 4, 1) );
	// boop.LoadMesh("box_v_uv_n.obj", "roadtile_1024x1024.bmp");
//	boop.LoadMesh("stylus.obj", "roadtile_1024x1024.bmp");
	// boop.LoadMesh("bman2.obj", "rainbow.bmp");
	// #define _ONE
	// #define _TWO
	// #define _THREE
	#define _FOUR
	#ifdef  _ONE
		boop.LoadMesh("bman2_3015_Tris.obj", "rainbow.bmp");
		boop.GetMesh(0)->matrix.columns[3].y += 1.0f;
		boop.GetMesh(0)->matrix.columns[3].z = 1.5f;
	#endif
	#ifdef _TWO
		boop.LoadMesh("smalltri.obj", "rainbow.bmp");
		boop.GetMesh(0)->matrix.columns[3].y += 1.8f;
		boop.GetMesh(0)->matrix.columns[3].z = 1.5f;
	#endif
	#ifdef _THREE
		boop.LoadMesh("box_v_uv_n.obj", "rainbow.bmp");
		boop.GetMesh(0)->matrix.columns[3].y += 1.8f;
		boop.GetMesh(0)->matrix.columns[3].z = 0.0f;
	#endif
	#ifdef _FOUR
		boop.LoadMesh("bman2.obj", "rainbow.bmp");
		boop.GetMesh(0)->matrix.columns[3].y += 1.0f;
		boop.GetMesh(0)->matrix.columns[3].z = 1.5f;
	#endif

	
	
	// boop.LoadMesh("bman.obj", "yellow.bmp");
	//boop.LoadMesh("bman.obj", "roadtile_1024x1024.bmp");
	//boop.LoadMesh("bman.obj", "red.bmp");

	//boop.GetMesh(0)->matrix.columns[3].x = 1.0f;
	//boop.GetMesh(2)->matrix.columns[3].x = -1.0f;
	// boop.GetMesh(0)->matrix.columns[3].y = 1.5f;
//	boop.GetMesh(1)->matrix.columns[3].y = 1.5f;

	// boop.GetMesh(0)->matrix *= scale( vec3(2, 2, 2) );

	// boop.LoadMesh("head_6364_poly_tx.obj", "red.bmp");
	// boop.LoadMesh("head_1026_poly.obj", "roadtile_1024x1024.bmp");
	// boop.LoadMesh("stylus.obj", "red.bmp");
	// boop.LoadMesh("roadtile.obj", "roadtile_1024x1024.bmp");
	// boop.LoadMesh("box_v_uv_n.obj", "texture.bmp");
	// boop.LoadMesh("box_v_uv_n.obj", "roadtile_1024x1024.bmp");

	
	int rottimer = GetTickCount();

	// Main message loop:
	while(true)
	{
		// Boop runs through its list of meshes and draws them all!
		if( GetTickCount() - rottimer > 10 ) {
			rottimer = GetTickCount();
			mat4 rotmtx = rotate(-1, vec3(0, 1, 0));
			boop.GetMesh(0)->matrix *= rotmtx;
		}
		boop.Render();

		// Message?!
		if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {

			// Wireframe, No Shading, Flat Shading, Gouraud Shading,
			// Texture, No Texture.
			// Shading and Texturing can be mixed.
			// Performance increases when using No Textures and
			// No Shading.
			if( GetAsyncKeyState(0x31) & 0x8000 )
				boop.SetShading(0);
			if( GetAsyncKeyState(0x32) & 0x8000 )
				boop.SetShading(1);
			if( GetAsyncKeyState(0x33) & 0x8000 )
				boop.SetShading(2);
			if( GetAsyncKeyState(0x34) & 0x8000 )
				boop.SetShading(3);
			if( GetAsyncKeyState(0x35) & 0x8000 )
				boop.SetTextures(0);
			if( GetAsyncKeyState(0x36) & 0x8000 )
				boop.SetTextures(1);
			if( GetAsyncKeyState(0x37) & 0x8000 )
				boop.SetTextures(2);

			///////////////////
			// Camera Controls

				// Move camera left, right, forward, back, up, down.
				float spd = 0.1f;
				// x.
				if( GetAsyncKeyState(VK_LEFT) & 0x8000 ) {
					boop.CameraStrafeToA( vec3(-spd, 0, 0) );
				}
				if( GetAsyncKeyState(VK_RIGHT) & 0x8000 ) {
					boop.CameraStrafeToA( vec3(spd, 0, 0) );
				}
				// y.
				if( GetAsyncKeyState(VK_NEXT) & 0x8000 ) {
					boop.CameraStrafeToA( vec3(0, -spd, 0) );
				}
				if( GetAsyncKeyState(VK_PRIOR) & 0x8000 ) {
					boop.CameraStrafeToA( vec3(0, spd, 0) );
				}
				// z.
				if( GetAsyncKeyState(VK_UP) & 0x8000 ) {
					boop.CameraStrafeToA( vec3(0, 0, -spd) );
				}
				if( GetAsyncKeyState(VK_DOWN) & 0x8000 ) {
					boop.CameraStrafeToA( vec3(0, 0, spd) );
				}

			// Camera Controls
			///////////////////

			// Leave message loop.
			if(msg.message == WM_QUIT || msg.message == WM_CLOSE)
				break;

			// Handle escape key and tell Windows we want to quit.
			if(GetAsyncKeyState(VK_ESCAPE) & 0x8000)
				PostQuitMessage(0);

			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	// Clean up Boop.
	// Explicitly calling this so memory isn't leaked.
	//
	// Originally thought I was leaking memory, but it turns out
	// everything was being deleted after going out of scope,
	// which was after running the memory leak check.
	//
	// Moved mem-leak check into nifty little structure decl at
	// top of this main. Must be at top... or at least before
	// you declare variables and such.
	// boop.Shutdown();

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOOP));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BOOP);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

//   hWnd = CreateWindow(szWindowClass, szTitle, /*WS_OVERLAPPEDWINDOW*/WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX/* | WS_MAXIMIZEBOX*/,
//      CW_USEDEFAULT, 0, 800, 600,/*CW_USEDEFAULT, 0,*/ NULL, NULL, hInstance, NULL);
hWnd = CreateWindow(szWindowClass,
                    szTitle,
                    /*WS_OVERLAPPEDWINDOW*/WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX/* | WS_MAXIMIZEBOX*/,
                    CW_USEDEFAULT,
                    0,
                    1280,
                    1024,
                    /*CW_USEDEFAULT, 0,*/ NULL,
                    NULL,
                    hInstance,
                    NULL);
   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		case WM_SIZE: {
			int param1 = LOWORD(lParam);
			int param2 = HIWORD(lParam);
			boop.ResizeView(param1, param2);
			break;
		}
		case WM_COMMAND:
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_CLOSE: // This was needed because the window wasn't closing properly.
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
