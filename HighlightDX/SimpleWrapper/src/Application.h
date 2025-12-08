#pragma once
#include "Window/TransparentWindow.h"

class Application 
{
public:
	Application(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

	~Application();

	int Run();
private:
	std::unique_ptr<Window> mWindow;
};