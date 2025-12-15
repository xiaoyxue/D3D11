#pragma once
#include "common/core.h"

namespace D3D11 {

  class SamplerManager;
  class BlenderManager;
  class DrawCommand;

  class DrawableObject {
  public:
    DrawableObject() = default;
    virtual ~DrawableObject() = default;
		virtual void Initialize(ID3D11Device* device) {}
    virtual void Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv) {}
    virtual void Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command) {}
    virtual void Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float time) {}
    virtual void DrawSurface(ID3D11DeviceContext* context) {};
    virtual void DrawWireframe(ID3D11DeviceContext* context) {};
    virtual void CreateSamplerState(SamplerManager* sampler_manager) {}
    virtual void CreateBendState(BlenderManager* blender_manager) {}
    virtual ID3D11SamplerState* GetSamplerState() const { return nullptr; }
    virtual ID3D11BlendState* GetBlendState() const { return nullptr; }
  };
}
