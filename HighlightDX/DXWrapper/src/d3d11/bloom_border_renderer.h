#pragma once
#include "constant_buffer.h"
#include "render_target.h"
#include "renderer.h"
#include "shader.h"
#include <chrono>
#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_3.h>
#include <memory>
#include <wrl/client.h>
#include "draw_command.h"
#include <utility>
#include <vector>
#include "../mathematics/lingal.h"
#include <mutex>
#include "../system/threading.h"

using Microsoft::WRL::ComPtr;

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
    float _padding1;
    float _padding2;
    float _padding3;
    // Total: 16 bytes

    ShaderConstants() : iType(0), iTime(0.0f), animationTime(0.0f),
                        whFactor(2.005f), iRadius(0.0f),
                        _padding1(0.0f), _padding2(0.0f), _padding3(0.0f) {
        iResolution[0] = iResolution[1] = 0.0f;
        iCenter[0] = iCenter[1] = 0.0f;
    }
};

class BloomBorderRenderer : public Renderer {
public:
  BloomBorderRenderer();
  ~BloomBorderRenderer() override;

  bool Initialize(HWND hwnd, int width, int height) override;
  void Render(HWND hwnd) override;
  void Resize(int width, int height) override;
  void Reset() override;

  void SetWhFactor(float factor) { wh_factor_ = factor; }
  float GetWhFactor() const { return wh_factor_; }

  void AddDrawCommand(std::unique_ptr<DrawCommand> command) override {
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    draw_commands_.push_back(std::move(command));
  }

  void ClearDrawCommands() override {
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    if (draw_commands_.size() > 1){
      draw_commands_.erase(draw_commands_.begin() + 1, draw_commands_.end());
    }
  }

  void DeleteLastDrawCommand() override {
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    if (!draw_commands_.empty()) {
      draw_commands_.pop_back();
    }
  }

  ID3D11Device *GetDevice() const override { return device_.Get(); }
  ID3D11DeviceContext *GetContext() const override { return context_.Get(); }

private:
  // Device & Context
  ComPtr<ID3D11Device> device_;
  ComPtr<ID3D11DeviceContext> context_;
  ComPtr<IDXGISwapChain1> swap_chain_;

  // Shaders
  std::unique_ptr<VertexShader> vertex_shader_;
  std::unique_ptr<PixelShader> ps_original_;
  std::unique_ptr<PixelShader> ps_blur_h_;
  std::unique_ptr<PixelShader> ps_blur_v_;
  std::unique_ptr<PixelShader> ps_composite_;

  // Render Targets
  std::unique_ptr<RenderTarget> rt_original_;
  std::unique_ptr<RenderTarget> rt_blur_h_;
  std::unique_ptr<RenderTarget> rt_blur_v_;

  // Resources
  std::unique_ptr<ConstantBuffer<ShaderConstants>> constant_buffer_;
  ComPtr<ID3D11SamplerState> sampler_state_;
  ComPtr<ID3D11BlendState> blend_state_;

  // BackBuffer
  ComPtr<ID3D11Texture2D> back_buffer_;
  ComPtr<ID3D11RenderTargetView> back_buffer_rtv_;

  // DirectComposition
  ComPtr<IDCompositionDevice> dcomp_device_;
  ComPtr<IDCompositionTarget> dcomp_target_;
  ComPtr<IDCompositionVisual> dcomp_visual_;

  // State
  int width_;
  int height_;
  float wh_factor_;
  std::chrono::steady_clock::time_point start_time_;
  // FPS Tracking
  int frame_count_;
  std::chrono::steady_clock::time_point last_fps_time_;

  // Draw Commands
  std::vector<std::unique_ptr<DrawCommand>> draw_commands_;
  // std::vector<std::unique_ptr<DrawCommand>> draw_commands_buffer_;

  std::mutex draw_commands_mutex_;
  Spinlock draw_commands_spinlock_;

private:
  bool CreateDevice();
  bool CreateSwapChain(HWND hwnd);
  bool CreateShaders();
  bool CreateRenderTargets();
  bool CreateResources();
  bool SetupDirectComposition(HWND hwnd);
  bool CreateBackBufferTargetView();

  void RenderOriginal(DrawCommand* command, float iTime);
  void RenderBlurH(float iTime);
  void RenderBlurV(float iTime);
  void RenderComposite(DrawCommand* command,float iTime);
  void RenderCompositeToTarget(DrawCommand* command, float iTime);
  void RenderCommands(float iTime);

  void UpdateFPS(HWND hwnd);

  Vec2 ScreenToNDC(int x, int y);
};
} // namespace D3D11
