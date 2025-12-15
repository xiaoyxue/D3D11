#pragma once
#include <functional>
#include "window/transparent_window.h"

class Application {
public:
  Application(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine,
              int nCmdShow);

  ~Application();

  int Run();

  void AddDrawCommand(std::unique_ptr<D3D11::DrawCommand> command) {
    window_->AddDrawCommand(std::move(command));
  }

  void ClearDrawCommands() {
    window_->ClearDrawCommands();
  }

  void DeleteLastDrawCommand() {
    window_->DeleteLastDrawCommand();
  }

  int GetWidth() const {
    return window_->GetWidth();
  }

  int GetHeight() const {
    return window_->GetHeight();
  }

  void SetTimerCallback(std::function<void()> callback, UINT milliseconds) {
    auto* transparent_window = static_cast<TransparentWindow*>(window_.get());
    transparent_window->SetTimerCallback(std::move(callback), milliseconds);
  }

private:
  std::unique_ptr<Window> window_;
};
