//#define NOMINMAX
//#include <windows.h>
//#include <algorithm>
//#include "D3D11/BloomBorderRenderer.h"
//#include <memory>
//#include "Application.h"
//
//std::unique_ptr<D3D11::BloomBorderRenderer> g_renderer;
//
//LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	if (msg == WM_NCCREATE) {
//		auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
//		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
//		return DefWindowProcW(hwnd, msg, wParam, lParam);
//	}
//
//	switch (msg) {
//	case WM_KEYDOWN:
//		if (wParam == VK_ESCAPE) {
//			DestroyWindow(hwnd);
//		}
//		return 0;
//
//	case WM_DESTROY:
//		PostQuitMessage(0);
//		return 0;
//	}
//
//	return DefWindowProcW(hwnd, msg, wParam, lParam);
//}
//
//int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
//{
//	(void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
//
//	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
//	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
//
//	const wchar_t* className = L"D3DExampleWindow";
//	WNDCLASSEXW wc = {};
//	wc.cbSize = sizeof(wc);
//	wc.lpfnWndProc = WindowProc;
//	wc.hInstance = hInstance;
//	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
//	wc.lpszClassName = className;
//
//	if (!RegisterClassExW(&wc)) {
//		CoUninitialize();
//		return -1;
//	}
//
//	DWORD exStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP;
//	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
//	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
//
//
//	HWND hwnd = CreateWindowExW(
//		exStyle,
//		className,
//		L"Multi-Pass Blur",
//		WS_POPUP,
//		0, 0,
//		screenWidth, screenHeight,
//		nullptr,
//		nullptr,
//		hInstance,
//		nullptr
//	);
//
//	if (!hwnd) {
//		CoUninitialize();
//		return -1;
//	}
//
//	ShowWindow(hwnd, SW_SHOW);
//	SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
//
//	g_renderer = std::make_unique<D3D11::BloomBorderRenderer>();
//
//	if (!g_renderer->Initialize(hwnd, screenWidth, screenHeight))
//	{
//		MessageBoxW(nullptr, L"Failed to initialize renderer", L"Error", MB_OK);
//		g_renderer.reset();
//		DestroyWindow(hwnd);
//		CoUninitialize();
//		return -1;
//	}
//
//
//	MSG msg = {};
//	bool running = true;
//
//	while (running)
//	{
//		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
//		{
//			if (msg.message == WM_QUIT)
//			{
//				running = false;
//				break;
//			}
//			TranslateMessage(&msg);
//			DispatchMessageW(&msg);
//		}
//
//		if (running)
//		{
//			g_renderer->Render(hwnd);
//
//			MsgWaitForMultipleObjects(0, nullptr, FALSE, 8, QS_ALLINPUT);
//		}
//	}
//
//	g_renderer.reset();
//	DestroyWindow(hwnd);
//	CoUninitialize();
//
//	return static_cast<int>(msg.wParam);
//}
