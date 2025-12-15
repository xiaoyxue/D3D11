#pragma once
#include <memory>
#include <functional>
#include <map>
#include "window.h"
#include "../d3d11/bloom_border_renderer.h"
#include "../d3d11/highlight_renderer.h"

class TransparentWindow : public Window {
public:
  TransparentWindow() = default;

  TransparentWindow(HINSTANCE hInstance, HWND hwnd);

  TransparentWindow(HINSTANCE hInstance, int width, int height);

  ~TransparentWindow() override;

  void Initialize() override;

  int MainLoop() override;

  void DrawAll() override;

  void AddDrawCommand(std::unique_ptr<D3D11::DrawCommand> command) override;

  void ClearDrawCommands() override;

  void DeleteLastDrawCommand() override;

  HWND GetHwnd() const { return hwnd_; }

  void SetTimerCallback(std::function<void()> callback, UINT milliseconds);

  void OnTimer(UINT_PTR timer_id);

private:
  void Reset();

private:
  WNDPROC window_proc_;
  HWND hwnd_;
  HINSTANCE hinstance_;
  std::unique_ptr<D3D11::Renderer> renderer_;
  std::map<UINT_PTR, std::function<void()>> timer_callbacks_;
  UINT_PTR next_timer_id_ = 1;
};
