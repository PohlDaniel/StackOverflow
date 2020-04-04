#include <windows.h>			
#include <iostream>
#include <fstream>
#include <sstream>

#include "Bitmap.h"
#include "Model.h"
#include "GL.h"
#include  "Extension.h"
#include "Camera.h";
#include "Object.h"
#include "ModelMatrix.h"
#include "Skybox.h"
#include "Depthmap.h"
#include "Quad.h"
#include "SeparableSSS.h"

int height = 480;
int width = 640;

POINT g_OldCursorPos;
bool g_enableVerticalSync;

enum DIRECTION {DIR_FORWARD = 1, DIR_BACKWARD = 2, DIR_LEFT = 4, DIR_RIGHT = 8, DIR_UP = 16, DIR_DOWN = 32, DIR_FORCE_32BIT = 0x7FFFFFFF};

enum Buffer {COLOR, DEPTH, VELOCITY, SPECCOLOR};

Depthmap * depthmap[4];
Camera* camera;
Object *statue;
Shader *main;
SubsurfaceShader *sss;
SpecularShader *specular;
Quad *quad;
SeparableSSS *separableSSS;
Buffer buffer = Buffer::COLOR;
Matrix4f prevViewProj, currViewProj;
bool rotate = false;
bool sssEnabled = true;
float degree = 0.0;
float sssWidth = 200.0;
Matrix4f rot;

unsigned int beckmannMap;
unsigned int depthmapWidth = 1024;
unsigned int depthmapHeight = 1024;

unsigned int mainRT, gColor, gVelocity, gSpecularColor, gDepth;
unsigned int sssRTX, sssColorX, sssRTY, sssColorY;
//prototype funktions
LRESULT CALLBACK winProc(HWND hWnd, UINT message, WPARAM wParma, LPARAM lParam);
void setCursortoMiddle(HWND hwnd);
void processInput(HWND hWnd);
void render();

void initApp(HWND hWnd);
void enableVerticalSync(bool enableVerticalSync);

// the main windows entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	Vector3f camPos(-1.30592, 3.259203, -4.11601);
	Vector3f xAxis(1, 0, 0);
	Vector3f yAxis(0, 1, 0);
	Vector3f zAxis(0, 0, 1);
	Vector3f target(0.00597954, 1.00716528, -0.00461888);

	Vector3f up(0.0, 1.0, 0.0);

	camera = new Camera(camPos, xAxis, yAxis, zAxis, target, up);

	//camera = new Camera(Vector3f(4.0, 3.0, 3.0), xAxis, yAxis, zAxis, Vector3f(0.0, 0.0, 0.0), up);

	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	SetConsoleTitle("Debug console");
	MoveWindow(GetConsoleWindow(), 740, 0, 550, 200, true);

	std::cout << "w, a, s, d, mouse : move camera" << std::endl;
	std::cout << "r                 : rotate light" << std::endl;
	std::cout << "v                 : disable/enable vsync" << std::endl;
	std::cout << "space             : release capture" << std::endl;

	WNDCLASSEX		windowClass;		// window class
	HWND			hwnd;				// window handle
	MSG				msg;				// message
	HDC				hdc;				// device context handle

										// fill out the window class structure
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = winProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);		// default icon
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);			// default arrow
	windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	// white background
	windowClass.lpszMenuName = NULL;									// no menu
	windowClass.lpszClassName = "WINDOWCLASS";
	windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);			// windows logo small icon

																// register the windows class
	if (!RegisterClassEx(&windowClass))
		return 0;

	// class registered, so now create our window
	hwnd = CreateWindowEx(NULL,									// extended style
		"WINDOWCLASS",						// class name
		"OBJViewer",							// app name
		WS_OVERLAPPEDWINDOW,
		0, 0,									// x,y coordinate
		width,
		height,									// width, height
		NULL,									// handle to parent
		NULL,									// handle to menu
		hInstance,							// application instance
		NULL);								// no extra params

											// check if window creation failed (hwnd would equal NULL)
	if (!hwnd)
		return 0;

	ShowWindow(hwnd, SW_SHOW);			// display the window
	UpdateWindow(hwnd);					// update the window


	initApp(hwnd);



	// main message loop
	while (true) {

		// Did we recieve a message, or are we idling ?
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			// test if this is a quit
			if (msg.message == WM_QUIT) break;
			// translate and dispatch message
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {


			processInput(hwnd);


			render();
			//render2();

			hdc = GetDC(hwnd);
			SwapBuffers(hdc);
			ReleaseDC(hwnd, hdc);
		}
	} // end while

	delete statue;
	return msg.wParam;
}

// the Windows Procedure event handler
LRESULT CALLBACK winProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	static HGLRC hRC;					// rendering context
	static HDC hDC;						// device context
	int width, height;					// window width and height
	POINT pt;
	RECT rect;

	switch (message)
	{
	case WM_DESTROY: {

		PostQuitMessage(0);
		return 0;
	}

	case WM_CREATE: {

		GetClientRect(hWnd, &rect);
		g_OldCursorPos.x = rect.right / 2;
		g_OldCursorPos.y = rect.bottom / 2;
		pt.x = rect.right / 2;
		pt.y = rect.bottom / 2;
		SetCursorPos(pt.x, pt.y);
		// set the cursor to the middle of the window and capture the window via "SendMessage"
		SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
		return 0;
	}break;

	case WM_LBUTTONDOWN: { // Capture the mouse

		setCursortoMiddle(hWnd);
		SetCapture(hWnd);

		return 0;
	} break;

	case WM_KEYDOWN: {

		switch (wParam) {

		case VK_ESCAPE: {

			PostQuitMessage(0);
			return 0;

		}break;
		case VK_SPACE: {

			ReleaseCapture();
			return 0;

		}break;
		case 'v': case 'V': {

			enableVerticalSync(!g_enableVerticalSync);
			return 0;

		}break;
		case 'M': {
			sssEnabled = !sssEnabled;
			return 0;

		}break;
		case VK_ADD: {

			sssWidth = sssWidth + 50;
			std::cout << sssWidth << std::endl;
		}break;
		case VK_SUBTRACT: {
			sssWidth = sssWidth - 50;
			std::cout << sssWidth << std::endl;
		}break;
		case VK_RETURN: {
			degree = 0.0;

		}break;
		case 'r':case 'R': {

			if (rotate) rotate = false;
			else  rotate = true;
		}break;
		case 'h':case 'H': {

			switch (buffer) {
			case COLOR:
				buffer = Buffer::DEPTH;
				std::cout << "gDepth" << std::endl;
				break;
			case DEPTH:
				buffer = Buffer::VELOCITY;
				std::cout << "gVelocity" << std::endl;
				break;
			case VELOCITY:
				buffer = Buffer::SPECCOLOR;
				std::cout << "gSpecular" << std::endl;
				break;
			case SPECCOLOR:
				buffer = Buffer::COLOR;
				std::cout << "gColor" << std::endl;
				break;
			}
		}

				 return 0;
		}break;

		return 0;
	}break;

	case WM_SIZE: {

		int height2 = HIWORD(lParam);		// retrieve width and height
		int width2 = LOWORD(lParam);

		if (height2 == 0) {					// avoid divide by zero

			height2 = 1;
		}

		glViewport(0, 0, width2, height2);

		return 0;
	}break;

	default:
		break;
	}
	return (DefWindowProc(hWnd, message, wParam, lParam));
}


void initApp(HWND hWnd) {

	static HGLRC hRC;					// rendering context
	static HDC hDC;						// device context

	hDC = GetDC(hWnd);
	int nPixelFormat;					// our pixel format index

	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	// size of structure
		1,								// default version
		PFD_DRAW_TO_WINDOW |			// window drawing support
		PFD_SUPPORT_OPENGL |			// OpenGL support
		PFD_DOUBLEBUFFER,				// double buffering support
		PFD_TYPE_RGBA,					// RGBA color mode
		32,								// 32 bit color mode
		0, 0, 0, 0, 0, 0,				// ignore color bits, non-palettized mode
		0,								// no alpha buffer
		0,								// ignore shift bit
		0,								// no accumulation buffer
		0, 0, 0, 0,						// ignore accumulation bits
		24,								// Number of bits for the depthbuffer
		8,								// Number of bits for the stencilbuffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main drawing plane
		0,								// reserved
		0, 0, 0 };						// layer masks ignored

	nPixelFormat = ChoosePixelFormat(hDC, &pfd);	// choose best matching pixel format
	SetPixelFormat(hDC, nPixelFormat, &pfd);		// set pixel format to device context


													// create rendering context and make it current
	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);
	enableVerticalSync(true);

	
	Bitmap *bitmap = new Bitmap();
	if (bitmap->loadBitmap24("textures/BeckmannMap.bmp")) {
		//bitmap->flipHorizontal();
		//bitmap->flipVertical();

		glGenTextures(1, &beckmannMap);
		glBindTexture(GL_TEXTURE_2D, beckmannMap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap->width, bitmap->height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap->data);

		glGenerateMipmap(GL_TEXTURE_2D);

	}
	delete bitmap;
	camera->perspective(20.0f, (GLfloat)width / (GLfloat)height, 1.0f, 2000.0f);


	//glEnable(GL_CULL_FACE);				
	//glDepthRange(-1.0, 100.0);
	statue = new Object();
	statue->initModel("objs/statue/statue.obj");

	/*buddha->m_model->setRotXYZPos(Vector3f(0.0, 1.0, 0.0), 45.0,
	Vector3f(1.0, 0.0, 0.0), 270.0,
	Vector3f(0.0, 0.0, 1.0), 0.0,
	0.0, 0.0, 0.0);*/
	statue->m_model->scale(0.5, 0.5 , 0.5 );

	for (int j = 0; j < statue->m_model->getMesches().size(); j++) {
		statue->m_model->getMesches()[j]->generateTangents();		
	}


	depthmap[0] = new Depthmap(camera);
	depthmap[0]->setProjectionMatrix(45.0f, 1.0, 1.0f, 100.0f);

	Vector3f lightPos1 = Vector3f(0.0, 0.0, -8.0);
	rot.rotate(Vector3f(0.0, 1.0, 0.0), degree);
	depthmap[0]->setViewMatrix(rot * lightPos1, statue->m_model->getTransformationMatrix() * statue->m_model->getCenter(), Vector3f(0.0, 1.0, 0.0));

	depthmap[0]->setColor(Vector3f(1.2, 1.2, 1.2));

	depthmap[0]->renderToDepth(statue);
	depthmap[0]->renderToDepthPCF(statue);

	depthmap[1] = new Depthmap(camera);
	depthmap[1]->setProjectionMatrix(45.0f, 1.0, 1.0f, 100.0f);

	Vector3f lightPos2 = Vector3f(0.0, 0.0, 8.0);
	rot.rotate(Vector3f(0.0, 1.0, 0.0), degree);
	depthmap[1]->setViewMatrix(rot * lightPos2, statue->m_model->getTransformationMatrix() * statue->m_model->getCenter(), Vector3f(0.0, 1.0, 0.0));

	depthmap[1]->setColor(Vector3f(1.3, 1.3, 1.3));

	depthmap[1]->renderToDepth(statue);
	depthmap[1]->renderToDepthPCF(statue);

	depthmap[2] = new Depthmap(camera);
	depthmap[2]->setProjectionMatrix(45.0f, 1.0, 1.0f, 100.0f);

	Vector3f lightPos3 = Vector3f(8.0, 0.0, 0.0);
	rot.rotate(Vector3f(0.0, 1.0, 0.0), degree);
	depthmap[2]->setViewMatrix(rot * lightPos3, statue->m_model->getTransformationMatrix() * statue->m_model->getCenter(), Vector3f(0.0, 1.0, 0.0));

	depthmap[2]->setColor(Vector3f(1.3, 1.3, 1.3));

	depthmap[2]->renderToDepth(statue);
	depthmap[2]->renderToDepthPCF(statue);

	depthmap[3] = new Depthmap(camera);
	depthmap[3]->setProjectionMatrix(45.0f, 1.0, 1.0f, 100.0f);

	Vector3f lightPos4 = Vector3f(-8.0, 0.0, 0.0);
	rot.rotate(Vector3f(0.0, 1.0, 0.0), degree);
	depthmap[3]->setViewMatrix(rot * lightPos4, statue->m_model->getTransformationMatrix() * statue->m_model->getCenter(), Vector3f(0.0, 1.0, 0.0));

	depthmap[3]->setColor(Vector3f(1.3, 1.3, 1.3));

	depthmap[3]->renderToDepth(statue);
	depthmap[3]->renderToDepthPCF(statue);


	main = new Shader("sss/Main.vert", "sss/Main.frag");
	quad = new Quad();
	glUniform1i(glGetUniformLocation(main->m_program, "u_beckmannTex"), 7);

	sss = new SubsurfaceShader("sss/SparableSSS.vert", "sss/SparableSSS.frag");
	separableSSS = new SeparableSSS();

	specular = new SpecularShader("sss/Specular.vert", "sss/Specular.frag");

	//////////////////////////////////////////////////////////gbuffer/////////////////////////////////////////
	glGenTextures(1, &gDepth);
	glBindTexture(GL_TEXTURE_2D, gDepth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &gVelocity);
	glBindTexture(GL_TEXTURE_2D, gVelocity);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &gSpecularColor);
	glBindTexture(GL_TEXTURE_2D, gSpecularColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &gColor);
	glBindTexture(GL_TEXTURE_2D, gColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &mainRT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mainRT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gDepth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gVelocity, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gSpecularColor, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gColor, 0);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// buffer for depth and stencil testing
	unsigned int rboDepthStencil;
	glGenRenderbuffers(1, &rboDepthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	/////////////////////////////////////////sssTarget/////////////////////////////////////////////
	glGenTextures(1, &sssColorX);
	glBindTexture(GL_TEXTURE_2D, sssColorX);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenFramebuffers(1, &sssRTX);
	glBindFramebuffer(GL_FRAMEBUFFER, sssRTX);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sssColorX, 0);

	// buffer for depth and stencil testing
	unsigned int rboDepthStencilX;
	glGenRenderbuffers(1, &rboDepthStencilX);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencilX);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencilX);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glGenTextures(1, &sssColorY);
	glBindTexture(GL_TEXTURE_2D, sssColorY);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenFramebuffers(1, &sssRTY);
	glBindFramebuffer(GL_FRAMEBUFFER, sssRTY);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sssColorY, 0);

	// buffer for depth and stencil testing
	unsigned int rboDepthStencilY;
	glGenRenderbuffers(1, &rboDepthStencilY);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencilY);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencilY);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void setCursortoMiddle(HWND hwnd) {

	RECT rect;

	GetClientRect(hwnd, &rect);
	SetCursorPos(rect.right / 2, rect.bottom / 2);

}
void enableVerticalSync(bool enableVerticalSync) {

	// WGL_EXT_swap_control.

	typedef BOOL(WINAPI * PFNWGLSWAPINTERVALEXTPROC)(GLint);

	static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
		reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(
			wglGetProcAddress("wglSwapIntervalEXT"));

	if (wglSwapIntervalEXT){
		wglSwapIntervalEXT(enableVerticalSync ? 1 : 0);
		g_enableVerticalSync = enableVerticalSync;
	}
}


void processInput(HWND hWnd) {

	static UCHAR pKeyBuffer[256];
	ULONG        Direction = 0;
	POINT        CursorPos;
	float        X = 0.0f, Y = 0.0f;

	// Retrieve keyboard state
	if (!GetKeyboardState(pKeyBuffer)) return;

	// Check the relevant keys
	if (pKeyBuffer['W'] & 0xF0) Direction |= DIR_FORWARD;
	if (pKeyBuffer['S'] & 0xF0) Direction |= DIR_BACKWARD;
	if (pKeyBuffer['A'] & 0xF0) Direction |= DIR_LEFT;
	if (pKeyBuffer['D'] & 0xF0) Direction |= DIR_RIGHT;
	if (pKeyBuffer['E'] & 0xF0) Direction |= DIR_UP;
	if (pKeyBuffer['Q'] & 0xF0) Direction |= DIR_DOWN;

	// Now process the mouse (if the button is pressed)
	if (GetCapture() == hWnd) {

		// Hide the mouse pointer
		SetCursor(NULL);

		// Retrieve the cursor position
		GetCursorPos(&CursorPos);

		// Calculate mouse rotational values
		X = (float)(g_OldCursorPos.x - CursorPos.x) * 0.1;
		Y = (float)(g_OldCursorPos.y - CursorPos.y) * 0.1;

		// Reset our cursor position so we can keep going forever :)
		SetCursorPos(g_OldCursorPos.x, g_OldCursorPos.y);

		if (Direction > 0 || X != 0.0f || Y != 0.0f) {

			// Rotate camera
			if (X || Y) {

				camera->rotate(X, Y, 0.0f);


			} // End if any rotation


			if (Direction) {

				float dx = 0, dy = 0, dz = 0, speed = 0.1;

				if (Direction & DIR_FORWARD) dz = speed;
				if (Direction & DIR_BACKWARD) dz = -speed;
				if (Direction & DIR_LEFT) dx = -speed;
				if (Direction & DIR_RIGHT) dx = speed;
				if (Direction & DIR_UP) dy = speed;
				if (Direction & DIR_DOWN) dy = -speed;

				camera->move(dx, dy, dz);

			}

		}// End if any movement
	} // End if Captured
}

void render() {

	if (rotate) {
		degree = degree + 0.05;
		rot.rotate(Vector3f(0.0, 1.0, 0.0), degree);
		depthmap[0]->setViewMatrix(rot * depthmap[0]->getPosition(), statue->m_model->getTransformationMatrix() * statue->m_model->getCenter(), Vector3f(0.0, 1.0, 0.0));
		depthmap[0]->renderToDepth(statue);
		depthmap[0]->renderToDepthPCF(statue);
	}

	glUseProgram(main->m_program);	
		//vertex shader
		main->loadMatrix("u_projection", camera->getProjectionMatrix());
		main->loadMatrix("u_model", statue->m_model->getTransformationMatrix());
		main->loadMatrix("u_view", camera->getViewMatrix());
		main->loadMatrix("u_modelView", statue->m_model->getTransformationMatrix() * camera->getViewMatrix());
		main->loadMatrix("u_normalMatrix", Matrix4f::getNormalMatrix(statue->m_model->getTransformationMatrix()));

		for (int i = 0; i < 4; i++) {
			main->loadMatrix("u_viewShadow", depthmap[i]->getViewMatrix(), i);
			main->loadMatrix("u_projectionShadow", depthmap[i]->getDepthPassMatrix(), i);
		}

		currViewProj = camera->getViewMatrix() * camera->getProjectionMatrix();
		Matrix4f world = statue->m_model->getTransformationMatrix();
		Matrix4f currWorldViewProj = world * currViewProj;
		Matrix4f prevWorldViewProj = world * prevViewProj;

		main->loadMatrix("u_currWorldViewProj", currWorldViewProj);
		main->loadMatrix("u_prevWorldViewProj", prevWorldViewProj);
		main->loadVector("u_cameraPosition", camera->getPosition());
		main->loadVector("u_jitter", Vector2f(0.0, 0.0) * 2);

		//fragment shader
		for (int i = 0; i < 4; i++) {
			main->loadVector("lightPosition", depthmap[i]->getPosition(), i);
			main->loadVector("lightDirection", depthmap[i]->getViewDirection(), i);
			main->loadVector("lightColor", depthmap[i]->getColor(), i);	
		}

		main->loadFloat("lightFalloffStart", cos(0.5f * (45.0f * PI / 180.f)));
		main->loadFloat("lightFalloffWidth", 0.05f);
		main->loadFloat("lightAttenuation", 1.0f / 128.0f);
		main->loadFloat("lightFarPlane", 100.0f);
		main->loadFloat("lightBias", -0.01f);
		main->loadBool("sssEnabled", sssEnabled);
		main->loadBool("translucencyEnabled", true);
		main->loadBool("separateSpeculars", false);
		main->loadFloat("sssWidth", sssWidth);
		main->loadFloat("translucency", 0.0f);
		main->loadFloat("ambient", 0.12f);
		main->loadFloat("bumpiness", 0.89);
		main->loadFloat("specularIntensity", 0.0);
		main->loadFloat("specularRoughness", 0.3);
		main->loadFloat("specularFresnel", 0.81);	

		glBindFramebuffer(GL_FRAMEBUFFER, mainRT);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		for (int i = 0; i < statue->m_model->numberOfMeshes(); i++) {

			glBindBuffer(GL_ARRAY_BUFFER, statue->m_model->getMesches()[i]->getVertexName());

			main->bindAttributes(statue->m_model->getMesches()[i], depthmap[0]->depthmapTexture, depthmap[0]->depthmapTexturePCF);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, beckmannMap);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, statue->m_model->getMesches()[i]->getIndexName());
			glDrawElements(GL_TRIANGLES, statue->m_model->getMesches()[i]->getNumberOfTriangles() * 3, GL_UNSIGNED_INT, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			main->unbindAttributes(statue->m_model->getMesches()[i]);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Update previous view-projection matrix:
		prevViewProj = currViewProj;
	glUseProgram(0);

	glUseProgram(sss->m_program);
		sss->loadFloat("sssWidth", sssWidth);
		sss->loadFloat("id", 1.0f);
		sss->loadVector("dir", Vector2f((GLfloat)width / (GLfloat)height, 0.0));

		glBindFramebuffer(GL_FRAMEBUFFER, sssRTX);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glBindBuffer(GL_ARRAY_BUFFER, separableSSS->m_quadVBO);
		sss->bindAttributes(gColor, gDepth, gSpecularColor);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, separableSSS->m_indexQuad);
		glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_SHORT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		sss->loadVector("dir", Vector2f(0.0f, 1.0));

		glBindFramebuffer(GL_FRAMEBUFFER, sssRTY);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glBindBuffer(GL_ARRAY_BUFFER, separableSSS->m_quadVBO);
		sss->bindAttributes(sssColorX, gDepth, gSpecularColor);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, separableSSS->m_indexQuad);
		glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_SHORT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		sss->unbindAttributes();

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (buffer) {
	case COLOR:

		if (sssEnabled) {
			quad->render(sssColorY);
		}else {
			quad->render(gColor);
		}

		break;
	case DEPTH:
		quad->render(sssColorY);
		break;
	case VELOCITY:
		quad->render(sssColorX);
		break;
	case SPECCOLOR:
		quad->render(sssColorY);
		break;
	}
}

