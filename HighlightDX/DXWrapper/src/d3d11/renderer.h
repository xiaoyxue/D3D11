#pragma once
#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <memory>
#include "draw_command.h"

namespace D3D11 {
class Renderer {
public:
  Renderer() = default;
  virtual ~Renderer() = default;

  virtual bool Initialize(HWND hwnd, int width, int height) = 0;
  virtual void Render(HWND hwnd) = 0;
  virtual void Resize(int width, int height) = 0;
  virtual void Reset() = 0;
  virtual ID3D11Device *GetDevice() const = 0;
  virtual ID3D11DeviceContext *GetContext() const = 0;
  virtual void AddDrawCommand(std::unique_ptr<DrawCommand> command) = 0;
  virtual void ClearDrawCommands() = 0;
  virtual void DeleteLastDrawCommand() = 0;
};
} // namespace D3D11
