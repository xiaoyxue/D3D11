#pragma once

class Window 
{
public:
	Window(): mWidth(0), mHeight(0) {}

	Window(int width, int height) : mWidth(width), mHeight(height) {}

	virtual ~Window() = default;

	virtual int MainLoop() = 0;

	virtual void Initialize() {};

protected:
	int mWidth;
	int mHeight;
};