#pragma once
#include "renderer.h"
#include <memory>
#include <wrl/client.h>
#include <mutex>
#include "drawable_object.h"
#include "sampler_manager.h"
#include "blender_manager.h"

namespace D3D11 {
  class HighlightRenderer : public Renderer {
  public:
    HighlightRenderer();
    ~HighlightRenderer() override;

    bool Initialize(HWND hwnd, int width, int height) override;
    void Render(HWND hwnd) override;
    void Resize(int width, int height) override;
    void Reset() override;

    ID3D11Device* GetDevice() const override;
    ID3D11DeviceContext* GetContext() const override;

    void AddDrawCommand(std::unique_ptr<DrawCommand> command) override;
    void ClearDrawCommands() override;
    void DeleteLastDrawCommand() override;

  private:
    bool CreateDevice();
    bool CreateSwapChain(HWND hwnd);
    bool CreateSampler();
    bool CreateBlender();
    bool SetupDirectComposition(HWND hwnd);
    bool CreateBackBufferTargetView();
    bool CreateSamplers();
    bool CreateBlenders();
    void UpdateFps(HWND hwnd);
    

    void DrawSDF(DrawCommand* command, float time);
    void DrawQuad(DrawCommand* command, float time);
    void DrawCommands(float time);

  private:
    // Device & Context
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain_;

    // BackBuffer
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> back_buffer_rtv_;

    // DirectComposition
    Microsoft::WRL::ComPtr<IDCompositionDevice> dcomp_device_;
    Microsoft::WRL::ComPtr<IDCompositionTarget> dcomp_target_;
    Microsoft::WRL::ComPtr<IDCompositionVisual> dcomp_visual_;

    // Sampler and blend manager
    SamplerManager sampler_manager_;
    BlenderManager blender_manager_;

    // Blend
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state_;

    // Draw Commands
    std::vector<std::unique_ptr<DrawCommand>> draw_commands_;

    // Draw Commands Lock
    std::mutex draw_commands_mutex_;

    // Screen width and height
    int width_;
    int height_;

    // Fps Tracking
    int frame_count_;
    std::chrono::steady_clock::time_point last_fps_time_;

    // Time
    std::chrono::steady_clock::time_point start_time_;

    // drawable object
    std::unique_ptr<DrawableObject> sdf_;
    std::unique_ptr<DrawableObject> quad_;

  };
}
