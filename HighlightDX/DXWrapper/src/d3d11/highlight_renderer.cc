#include "highlight_renderer.h"
#include "sdf.h"
#include "quad.h"

using Microsoft::WRL::ComPtr;

namespace D3D11 {

  HighlightRenderer::HighlightRenderer():
    width_(0), height_(0)
  {
    start_time_ = std::chrono::steady_clock::now();
    last_fps_time_ = start_time_;
    frame_count_ = 0;
  }

  HighlightRenderer::~HighlightRenderer()
  {
    Reset();
  }

  bool HighlightRenderer::Initialize(HWND hwnd, int width, int height)
  {
    width_ = width;
    height_ = height;

    if (!CreateDevice()) {
      MessageBoxW(nullptr, L"CreateDevice failed", L"Error", MB_OK);
      return false;
    }
    if (!CreateSwapChain(hwnd)) {
      MessageBoxW(nullptr, L"CreateSwapChain failed", L"Error", MB_OK);
      return false;
    }
    if (!CreateBackBufferTargetView()) {
      MessageBoxW(nullptr, L"CreateBackBufferTargetView failed", L"Error", MB_OK);
      return false;
    }
    if (!SetupDirectComposition(hwnd)) {
      MessageBoxW(nullptr, L"SetupDirectComposition failed", L"Error", MB_OK);
      return false;
    }
    if (!CreateSamplers()) {
      MessageBoxW(nullptr, L"CreateSamplers failed", L"Error", MB_OK);
      return false;
    }
    if (!CreateBlenders()) {
      MessageBoxW(nullptr, L"CreateBlender failed", L"Error", MB_OK);
      return false;
    }

    // Init SDF
    sdf_ = std::make_unique<SDF>(width_, height_);
    sdf_->Initialize(device_.Get());
    sdf_->CreateSamplerState(&sampler_manager_);
    sdf_->CreateBendState(&blender_manager_);

    // Init Quad
    quad_ = std::make_unique<Quad>(width_, height_);
    quad_->Initialize(device_.Get());
    quad_->CreateSamplerState(&sampler_manager_);
    quad_->CreateBendState(&blender_manager_);

    // Load Quad texture: Auto-load OuterCursor.png
    auto quad = static_cast<Quad*>(quad_.get());
    if (!quad->LoadTexture(device_.Get(), context_.Get(), L"D:\\Code\\Work\\D3D11\\HighlightDX\\DXWrapper\\asset\\InnerCursor.png")) {
      MessageBoxW(nullptr, L"Failed to load OuterCursor.png texture", L"Warning", MB_OK | MB_ICONWARNING);
      // Continue execution even if texture loading fails, should not block initialization
    }

    return true;
  }

  void HighlightRenderer::Resize(int width, int height)
  {
    if (width == width_ && height == height_) {
      return;
    }

    width_ = width;
    height_ = height;
  }

  void HighlightRenderer::Reset()
  {
    dcomp_device_.Reset();
    dcomp_target_.Reset();
    dcomp_visual_.Reset();

    device_.Reset();
    context_.Reset();
    swap_chain_.Reset();

    // Drawable object reset
    sdf_.reset();
    quad_.reset();
  }

  ID3D11Device* HighlightRenderer::GetDevice() const
  {
    return device_.Get();
  }

  ID3D11DeviceContext* HighlightRenderer::GetContext() const
  {
    return context_.Get();
  }

  void HighlightRenderer::AddDrawCommand(std::unique_ptr<DrawCommand> command)
  {
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    draw_commands_.push_back(std::move(command));
  }

  void HighlightRenderer::ClearDrawCommands()
  {
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    if (draw_commands_.size() > 1) {
      draw_commands_.erase(draw_commands_.begin() + 1, draw_commands_.end());
    }
  }

  void HighlightRenderer::DeleteLastDrawCommand()
  {
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    if (!draw_commands_.empty()) {
      draw_commands_.pop_back();
    }
  }

  bool HighlightRenderer::CreateDevice()
  {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
      flags, levels, 1, D3D11_SDK_VERSION, &device_,
      nullptr, &context_);

    return SUCCEEDED(hr);
  }

  bool HighlightRenderer::CreateSwapChain(HWND hwnd)
  {
    ComPtr<IDXGIDevice> dxgiDevice;
    device_.As(&dxgiDevice);

    ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);

    ComPtr<IDXGIFactory2> factory;
    adapter->GetParent(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width_;
    desc.Height = height_;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    return SUCCEEDED(factory->CreateSwapChainForComposition(
      device_.Get(), &desc, nullptr, &swap_chain_));
  }

  bool HighlightRenderer::CreateSampler()
  {
    return sampler_manager_.CreateSamplerStates(device_.Get());
  }

  bool HighlightRenderer::CreateBlender()
  {
    return blender_manager_.CreateBlendStates(device_.Get());
  }

  bool HighlightRenderer::SetupDirectComposition(HWND hwnd)
  {
    ComPtr<IDXGIDevice> dxgiDevice;
    device_.As(&dxgiDevice);

    if (FAILED(DCompositionCreateDevice(dxgiDevice.Get(),
      IID_PPV_ARGS(&dcomp_device_)))) {
      return false;
    }

    if (FAILED(dcomp_device_->CreateTargetForHwnd(hwnd, TRUE, &dcomp_target_))) {
      return false;
    }

    if (FAILED(dcomp_device_->CreateVisual(&dcomp_visual_))) {
      return false;
    }

    dcomp_visual_->SetContent(swap_chain_.Get());
    dcomp_target_->SetRoot(dcomp_visual_.Get());
    dcomp_device_->Commit();

    return true;
  }

  bool HighlightRenderer::CreateBackBufferTargetView()
  {
    if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer_)))) {
      return false;
    }
    if (FAILED(device_->CreateRenderTargetView(back_buffer_.Get(), nullptr, &back_buffer_rtv_))) {
      return false;
    }

    return true;
  }

  bool HighlightRenderer::CreateSamplers()
  {
    return sampler_manager_.CreateSamplerStates(device_.Get());
  }

  bool HighlightRenderer::CreateBlenders()
  {
    if (!blender_manager_.CreateBlendStates(device_.Get())) {
      MessageBoxW(nullptr, L"CreateBlendStates failed", L"Error", MB_OK);
      return false;
    }
    blend_state_ = blender_manager_.GetBlender();
    float blendFactor[4] = { 0, 0, 0, 0 };
    context_->OMSetBlendState(blend_state_.Get(), blendFactor, 0xffffffff);
    return true;
  }

  void HighlightRenderer::UpdateFps(HWND hwnd)
  {
    frame_count_++;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - last_fps_time_).count();

    if (elapsed >= 1.0f) {
      float fps = frame_count_ / elapsed;
      wchar_t title[256];
      swprintf_s(title, L"Highlight renderer - FPS: %.1f", fps);
      SetWindowTextW(hwnd, title);
      frame_count_ = 0;
      last_fps_time_ = now;
    }
  }

  void HighlightRenderer::DrawSDF(DrawCommand* command, float time)
  {
    // Ensure blend state is set
    float blendFactor[4] = {0, 0, 0, 0};
    context_->OMSetBlendState(blend_state_.Get(), blendFactor, 0xffffffff);
    
    sdf_->Draw(context_.Get(), back_buffer_rtv_.Get(), command, time);
  }

  void HighlightRenderer::DrawQuad(DrawCommand* command, float time)
  {
    if (!quad_) {
      return;
    }

    // Only process CURSOR type commands
    if (!command || command->GetType() != DrawCommandType::CURSOR) {
      return;
    }

    auto quad = static_cast<Quad*>(quad_.get());
    auto cursor_command = static_cast<DrawCursorCommand*>(command);
    
    // Set initial position (cursor position)
    quad->SetPosition(cursor_command->GetX(), cursor_command->GetY());
    
    // Use default rotation (45 degrees counter-clockwise)
    quad->SetRotation(45.0f / 180.0f * PI);
    
    // Set size
    quad->SetSize(100.0f, 100.0f);
    
    // Keep scale constant at 1.0 (no scale animation)
    quad->SetScale(1.0f, 1.0f);
    
    // Animation: Calculate translation matrix (like SimpleQuad)
    auto now = std::chrono::steady_clock::now();
    float animation_time = std::chrono::duration<float>(
      now - cursor_command->GetAnimationStartTime()
    ).count();
    
    constexpr float max_animation_time = 1.0f;
    float animation_ratio = animation_time / max_animation_time;
    if (animation_ratio > 1.0f) animation_ratio = 1.0f;
    
    auto translate_matrix = DirectX::XMMatrixTranslation(
      100.0f * animation_ratio,
      -100.0f * animation_ratio,
      0.0f
    );
    
    quad->SetTranslateMatrix(translate_matrix);

    // Ensure blend state is set
    float blendFactor[4] = {0, 0, 0, 0};
    context_->OMSetBlendState(blend_state_.Get(), blendFactor, 0xffffffff);
    
    // Draw Quad
    quad_->Draw(context_.Get(), back_buffer_rtv_.Get(), command, time);
  }

  void HighlightRenderer::DrawCommands(float time)
  {
    if (draw_commands_.empty()) {
      return;
    }
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    for (auto& command : draw_commands_) {
      DrawSDF(command.get(), time);
      DrawQuad(command.get(), time);
    }
  }

  void HighlightRenderer::Render(HWND hwnd)
  {
    if (!device_ || width_ == 0 || height_ == 0) {
      return;
    }

    auto now = std::chrono::steady_clock::now();
    float iTime = std::chrono::duration<float>(now - start_time_).count();

    // Setup Viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);

    back_buffer_.Reset();
    swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer_));
    float clear[4] = { 0, 0, 0, 0 };
    context_->ClearRenderTargetView(back_buffer_rtv_.Get(), clear);

    DrawCommands(iTime);

    // SwapChain present
    swap_chain_->Present(0, 0);

    // Update fps
    UpdateFps(hwnd);
  }
}
