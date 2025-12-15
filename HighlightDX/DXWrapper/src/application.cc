#include "application.h"

Application::Application(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                         LPWSTR lpCmdLine, int nCmdShow) {
  window_ = std::make_unique<TransparentWindow>(hInstance, 800, 600);
}

Application::~Application() {}

int Application::Run() {

  int result = window_->MainLoop();
  return result;
}
