#pragma once
#include "drawable_object.h"
#include <wrl/client.h>
#include <memory>
#include "constant_buffer.h"
#include "common/core.h"
#include "shader.h"
#include "render_target.h"
#include "sampler_manager.h"
#include "blender_manager.h"
#include "draw_command.h"

namespace D3D11 {
  struct ShaderConstants {
    int iType;
    float iResolution[2];
    float iTime;
    // Total: 16 bytes

    float animationTime;
    float whFactor;
    float iCenter[2];
    // Total: 16 bytes

    float iRadius;
    float maxAnimationTime;
    float _padding1;
    float _padding2;
    // Total: 16 bytes

    ShaderConstants() : iType(0), iTime(0.0f), animationTime(0.0f),
      whFactor(2.005f), iRadius(0.0f), maxAnimationTime(2.0f),
      _padding1(0.0f), _padding2(0.0f) {
      iResolution[0] = iResolution[1] = 0.0f;
      iCenter[0] = iCenter[1] = 0.0f;
    }
  };

  class SDF : public DrawableObject {
  public:
    SDF(int width, int height);
    ~SDF() override = default;

    void Initialize(ID3D11Device* device) override;

    void Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float time) override;

    void CreateSamplerState(SamplerManager* sampler_manager) override;

    void CreateBendState(BlenderManager* blend_state) override;

   ID3D11SamplerState* GetSamplerState() const override { return sampler_state_.Get(); }

   ID3D11BlendState* GetBlendState() const override { return blend_state_.Get(); }


  private:
    bool CreateShaders(ID3D11Device* device);
    bool CreateRenderTargets(ID3D11Device* device);
    bool CreateConstantBuffer(ID3D11Device* device);

    void RenderOriginal(ID3D11DeviceContext* context, DrawCommand* command, float iTime);
    void RenderBlurH(ID3D11DeviceContext* context, float iTime);
    void RenderBlurV(ID3D11DeviceContext* context, float iTime);
    void RenderCompositeToTarget(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float iTime);

  private:
    // Render Targets
    std::unique_ptr<RenderTarget> rt_original_;
    std::unique_ptr<RenderTarget> rt_blur_h_;
    std::unique_ptr<RenderTarget> rt_blur_v_;

    // Resources
    std::unique_ptr<ConstantBuffer<ShaderConstants>> constant_buffer_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state_;
    SamplerType sampler_type_;

    // Shaders
    std::unique_ptr<VertexShader> vertex_shader_;
    std::unique_ptr<PixelShader> ps_original_;
    std::unique_ptr<PixelShader> ps_blur_h_;
    std::unique_ptr<PixelShader> ps_blur_v_;
    std::unique_ptr<PixelShader> ps_composite_;

    // properties
    int width_;
    int height_;
    float wh_factor_;
  };
}
