#define NOMINMAX
#include <windows.h>
#include "Application.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
	Application app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	return app.Run();
}