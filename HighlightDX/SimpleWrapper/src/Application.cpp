#include "Application.h"

Application::Application(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	mWindow = std::make_unique<TransparentWindow>(hInstance, 800, 600);
}

Application::~Application()
{

}

int Application::Run()
{

	int result = mWindow->MainLoop();
	return result;
}
