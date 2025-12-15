#include <windows.h>
#include <winuser.h>
#include "Application.h"
#include "d3d11/draw_command.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPWSTR lpCmdLine, [[maybe_unused]] int nCmdShow) {

  Application app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

  // First draw fullscreen immediately
  app.AddDrawCommand(std::make_unique<D3D11::DrawFullScreenCommand>(app.GetWidth(), app.GetHeight()));

  return app.Run();
}
