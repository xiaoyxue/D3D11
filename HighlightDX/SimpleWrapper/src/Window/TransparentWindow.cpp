#define NOMINMAX
#include <windows.h>
#include "TransparentWindow.h"


LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) {
		auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	switch (msg) {
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


TransparentWindow::TransparentWindow(HINSTANCE hInstance, HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;
	mWidth = width;
	mHeight = height;
	mHInstance = hInstance;
	mWindowProc = WindowProc;
	Initialize();
}

TransparentWindow::TransparentWindow(HINSTANCE hInstance, int width, int height)
{
	mWindowProc = WindowProc;
	mWidth = width;
	mHeight = height;
	mHInstance = hInstance;
	Initialize();
}

TransparentWindow::~TransparentWindow()
{
	DestroyWindow(mHwnd);
}

void TransparentWindow::Initialize()
{
	const wchar_t* className = L"D3DExampleWindow";
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = mWindowProc;
	wc.hInstance = mHInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = className;

	if (!RegisterClassExW(&wc)) {
		CoUninitialize();
		exit(-1);
	}

	DWORD exStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (fullscreen) 
	{
		mWidth = screenWidth;
		mHeight = screenHeight;
	}


	mHwnd = CreateWindowExW(
		exStyle,
		className,
		L"Multi-Pass Blur",
		WS_POPUP,
		0, 0,
		screenWidth, screenHeight,
		nullptr,
		nullptr,
		mHInstance,
		nullptr
	);

	if (!mHwnd) {
		CoUninitialize();
		exit(-1);
	}

	ShowWindow(mHwnd, SW_SHOW);
	SetLayeredWindowAttributes(mHwnd, 0, 255, LWA_ALPHA);

	mRenderer = std::make_unique<D3D11::BloomBorderRenderer>();

	if (!mRenderer->Initialize(mHwnd, mWidth, mHeight))
	{
		MessageBoxW(nullptr, L"Failed to initialize renderer", L"Error", MB_OK);
		mRenderer.reset();
		DestroyWindow(mHwnd);
		CoUninitialize();
		exit(-1);
	}
}

int TransparentWindow::MainLoop()
{
	MSG msg = {};
	bool running = true;
	while (running)
	{
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (running)
		{
			mRenderer->Render(mHwnd);

			MsgWaitForMultipleObjects(0, nullptr, FALSE, 8, QS_ALLINPUT);
		}
	}

	mRenderer->Reset();
	DestroyWindow(mHwnd);
	CoUninitialize();

	return static_cast<int>(msg.wParam);
}

