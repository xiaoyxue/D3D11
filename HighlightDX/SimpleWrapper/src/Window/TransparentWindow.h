#pragma once
#define NOMINMAX
#include <windows.h>
#include "Window.h"
#include <memory>
#include "D3D11/BloomBorderRenderer.h"

class TransparentWindow : public Window
{
public:
	TransparentWindow() = default;

	TransparentWindow(HINSTANCE hInstance, HWND hwnd);

	TransparentWindow(HINSTANCE hInstance, int width, int height);

	~TransparentWindow() override;

	void Initialize() override;

	int MainLoop() override;

private:
	WNDPROC mWindowProc;
	HWND mHwnd;
	HINSTANCE mHInstance;
	std::unique_ptr<D3D11::Renderer> mRenderer;
	bool fullscreen = false;
};
