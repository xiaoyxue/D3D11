#include "transparent_window.h"
#include "render_state.h"
#include <windows.h>
#include <WinUser.h>

std::unordered_map<HWND, Window*> id_to_instance;

static int g_circle_num = 0;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

  if (msg == WM_NCCREATE) {
    auto *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }

  auto* window = reinterpret_cast<TransparentWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  switch (msg) {
  case WM_TIMER:
	if (window) {
		window->OnTimer(wParam);
		KillTimer(hwnd, wParam);
	}
	return 0;
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE) {
      DestroyWindow(hwnd);
    }
    else if (wParam == static_cast<WPARAM>('C')) {
      if (g_circle_num >= 6) {
        g_circle_num = 0;
      }
      ++g_circle_num;
      std::unique_ptr<D3D11::DrawCommand> command = std::make_unique<D3D11::DrawCircleCommand>(g_circle_num * 100.f, g_circle_num * 100.f, 100.f);
      window->ClearDrawCommands();
      window->AddDrawCommand(std::move(command));
    }
    else if (wParam == static_cast<WPARAM>('F')) {
      int screenWidth = GetSystemMetrics(SM_CXSCREEN);
      int screenHeight = GetSystemMetrics(SM_CYSCREEN);
      std::unique_ptr<D3D11::DrawCommand> command = std::make_unique<D3D11::DrawFullScreenCommand>(screenWidth, screenHeight);
      window->AddDrawCommand(std::move(command));
    }
    else if (wParam == static_cast<WPARAM>('W') || wParam == static_cast<WPARAM>('B')) {
      // TODO
      // ...
    }
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

TransparentWindow::TransparentWindow(HINSTANCE hInstance, HWND hwnd) {
  RECT rc;
  GetClientRect(hwnd, &rc);

  int width = rc.right - rc.left;
  int height = rc.bottom - rc.top;
  width_ = width;
  height_ = height;
  hinstance_ = hInstance;
  window_proc_ = WindowProc;
  Initialize();
}

TransparentWindow::TransparentWindow(HINSTANCE hInstance, int width,
                                     int height) {
  window_proc_ = WindowProc;
  width_ = width;
  height_ = height;
  hinstance_ = hInstance;
  Initialize();
}

TransparentWindow::~TransparentWindow() { DestroyWindow(hwnd_); }

void TransparentWindow::Initialize() {
  const wchar_t *className = L"D3DExampleWindow";
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = window_proc_;
  wc.hInstance = hinstance_;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = className;

  if (!RegisterClassExW(&wc)) {
    CoUninitialize();
    exit(-1);
  }

  DWORD exStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT |
                  WS_EX_NOREDIRECTIONBITMAP;
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  width_ = screenWidth;
  height_ = screenHeight;

  hwnd_ = CreateWindowExW(exStyle, className, L"Multi-Pass Blur", WS_POPUP, 0,
                          0, screenWidth, screenHeight, nullptr, nullptr,
                          hinstance_, nullptr);

  if (!hwnd_) {
    CoUninitialize();
    exit(-1);
  }

  id_to_instance[hwnd_] = this;

  SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  ShowWindow(hwnd_, SW_SHOW);
  SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);

  //renderer_ = std::make_unique<D3D11::BloomBorderRenderer>();
  renderer_ = std::make_unique<D3D11::HighlightRenderer>();

  if (!renderer_->Initialize(hwnd_, width_, height_)) {
    MessageBoxW(nullptr, L"Failed to initialize renderer", L"Error", MB_OK);
    renderer_.reset();
    DestroyWindow(hwnd_);
    CoUninitialize();
    exit(-1);
  }
}

int TransparentWindow::MainLoop() {
  MSG msg = {};
  bool running = true;
  while (running) {
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running = false;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

    if (running) {
      // renderer_->Render(hwnd_);
      DrawAll();
      MsgWaitForMultipleObjects(0, nullptr, FALSE, 8, QS_ALLINPUT);
    }
  }

  Reset();
  DestroyWindow(hwnd_);
  CoUninitialize();

  return static_cast<int>(msg.wParam);
}

void TransparentWindow::DrawAll() {
  if (RenderState::g_draw_box) {
    renderer_->Render(hwnd_);
  }
}

void TransparentWindow::Reset() {
  renderer_->Reset();
}

void TransparentWindow::AddDrawCommand(std::unique_ptr<D3D11::DrawCommand> command) {
  renderer_->AddDrawCommand(std::move(command));
}

void TransparentWindow::ClearDrawCommands() {
  renderer_->ClearDrawCommands();
}

void TransparentWindow::DeleteLastDrawCommand() {
  renderer_->DeleteLastDrawCommand();
}

void TransparentWindow::SetTimerCallback(std::function<void()> callback, UINT milliseconds) {
  UINT_PTR timer_id = next_timer_id_++;
  timer_callbacks_[timer_id] = std::move(callback);
  SetTimer(hwnd_, timer_id, milliseconds, nullptr);
}

void TransparentWindow::OnTimer(UINT_PTR timer_id) {
  auto it = timer_callbacks_.find(timer_id);
  if (it != timer_callbacks_.end()) {
    it->second();
    timer_callbacks_.erase(it);
  }
}
