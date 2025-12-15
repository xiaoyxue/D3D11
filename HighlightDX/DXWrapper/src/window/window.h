#pragma once
#include "../d3d11/draw_command.h"
#include <memory>

class Window {
public:
  Window() : width_(0), height_(0) {}

  Window(int width, int height) : width_(width), height_(height) {}

  virtual ~Window() = default;

  virtual int MainLoop() = 0;

  virtual void Initialize() {}

  virtual void DrawAll() {}

  virtual void AddDrawCommand(std::unique_ptr<D3D11::DrawCommand> command) = 0;

  virtual void ClearDrawCommands() = 0;

  virtual void DeleteLastDrawCommand() = 0;

  virtual int GetWidth() const { return width_; }

  virtual int GetHeight() const { return height_; }

protected:
  int width_;
  int height_;
};
