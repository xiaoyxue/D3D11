#include "bloom_border_renderer.h"
#include "draw_command.h"

namespace D3D11 {

static_assert(sizeof(ShaderConstants) % 16 == 0, "ShaderConstants must be aligned to 16 bytes");

static const char *g_vertexShader = R"(
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}
)";

// Pass 1:
static const char *g_pixelShaderOriginal = R"(
cbuffer Constants : register(b0)
{
    int iType;
    float2 iResolution;
    float iTime;

    float animationTime;
    float whFactor;
    float2 iCenter;

    float iRadius;
    float3 _padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float sdBox(float2 p, float2 b)
{
    float2 d = abs(p) - b;
    return length(max(d, float2(0, 0))) + min(max(d.x, d.y), 0.0);
}

float sdCircle(float2 p, float r)
{
    return length(p) - r;
}

float4 main(PSInput input) : SV_TARGET
{
    float2 fragCoord = input.uv * iResolution;
    float2 normalizedCoord = fragCoord - iResolution / 2.0;
    float2 b = iResolution / whFactor;
    float maxAnimationTime = 2.0; // seconds
    float radiusRatio = min(animationTime / maxAnimationTime, 1.0);
    float d = 0.0;
    if (iType == 0 || iType == 1)
    {
        d = sdBox(normalizedCoord, b);
    }
    else if (iType == 2)
    {
        d = sdCircle(normalizedCoord - iCenter, iRadius * radiusRatio);
    }

    float2 uv = fragCoord / iResolution;
    //float3 col = 0.5 + 0.5 * cos(iTime + float3(uv.x, uv.y, uv.x) + float3(0, 2, 4));
    float3 col = 0.6 * float3(1, 0.5, 0) + 0.4 * abs(cos(iTime * 1.5)) * float3(1, 0.5, 0);
    float scale = 1.0;

    if (iType == 0)
    {
      d = -d;
    }

    if (d < 0.0)
    {
        return float4(col * scale, 1.0);
    }
    else
    {
        return float4(0, 0, 0, 0);
    }
}
)";

// Pass 2:
static const char *g_pixelShaderBlurH = R"(
#define SIGMA 10.0

cbuffer Constants : register(b0)
{
    int iType;
    float2 iResolution;
    float iTime;

    float animationTime;
    float whFactor;
    float2 iCenter;

    float iRadius;
    float3 _padding;
};

Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

float4 main(PSInput input) : SV_TARGET
{
    float2 texelSize = 1.0 / iResolution;
    float sigma = SIGMA;
    int radius = int(ceil(3.0 * sigma));

    float3 result = float3(0, 0, 0);
    float weightSum = 0.0;

    for (int x = -radius; x <= radius; x++)
    {
        float weight = gaussian(float(x), sigma);
        float2 offset = float2(float(x) * texelSize.x, 0.0);

        result += inputTexture.Sample(samplerState, input.uv + offset).rgb * weight;
        weightSum += weight;
    }

    return float4(result / weightSum, 1.0);
}
)";

// Pass 3:
static const char *g_pixelShaderBlurV = R"(
#define SIGMA 10.0

cbuffer Constants : register(b0)
{
    int iType;
    float2 iResolution;
    float iTime;

    float animationTime;
    float whFactor;
    float2 iCenter;

    float iRadius;
    float3 _padding;
};

Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

float4 main(PSInput input) : SV_TARGET
{
    float2 texelSize = 1.0 / iResolution;
    float sigma = SIGMA;
    int radius = int(ceil(3.0 * sigma));

    float3 result = float3(0, 0, 0);
    float weightSum = 0.0;

    for (int y = -radius; y <= radius; y++)
    {
        float weight = gaussian(float(y), sigma);
        float2 offset = float2(0.0, float(y) * texelSize.y);

        result += inputTexture.Sample(samplerState, input.uv + offset).rgb * weight;
        weightSum += weight;
    }

    return float4(result / weightSum, 1.0);
}
)";

// Pass 4:
static const char *g_pixelShaderComposite = R"(
#define BLOOM_STRENGTH 1.8
#define SIGMA 10.0

cbuffer Constants : register(b0)
{
    int iType;
    float2 iResolution;
    float iTime;

    float animationTime;
    float whFactor;
    float2 iCenter;

    float iRadius;
    float3 _padding;
};

Texture2D bloomTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float sdBox(float2 p, float2 b)
{
    float2 d = abs(p) - b;
    return length(max(d, float2(0, 0))) + min(max(d.x, d.y), 0.0);
}

float sdCircle(float2 p, float r)
{
    return length(p) - r;
}

float4 main(PSInput input) : SV_TARGET
{
    float2 fragCoord = input.uv * iResolution;
    float2 normalizedCoord = fragCoord - iResolution / 2.0;
    float2 b = iResolution / whFactor;
    float maxAnimationTime = 2.0; // seconds
    float radiusRatio = min(animationTime / maxAnimationTime, 1.0);

    float d = 0.0;
    if (iType == 0 || iType == 1)
    {
      d = sdBox(normalizedCoord, b);
    }
    else if (iType == 2)
    {
      d = sdCircle(normalizedCoord - iCenter, iRadius * radiusRatio);
    }

    float3 bloom = bloomTexture.Sample(samplerState, input.uv).rgb;
    float3 color = bloom * BLOOM_STRENGTH;

    if (iType == 0)
    {
      d = -d;
    }

    if (d > 0.0)
    {
		    return float4(color, 0.0);
    }
    else
    {
        return float4(0, 0, 0, 0);
    }
}
)";

BloomBorderRenderer::BloomBorderRenderer()
    : width_(0), height_(0), wh_factor_(2.005f), frame_count_(0) {
  start_time_ = std::chrono::steady_clock::now();
  last_fps_time_ = start_time_;
}

BloomBorderRenderer::~BloomBorderRenderer() = default;

bool BloomBorderRenderer::Initialize(HWND hwnd, int width, int height) {
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
  if (!CreateShaders()) {
    MessageBoxW(nullptr, L"CreateShaders failed", L"Error", MB_OK);
    return false;
  }
  if (!CreateRenderTargets()) {
    MessageBoxW(nullptr, L"CreateRenderTargets failed", L"Error", MB_OK);
    return false;
  }
  if (!CreateResources()) {
    MessageBoxW(nullptr, L"CreateResources failed", L"Error", MB_OK);
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
  return true;
}

bool BloomBorderRenderer::CreateDevice() {
  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                 flags, levels, 1, D3D11_SDK_VERSION, &device_,
                                 nullptr, &context_);

  return SUCCEEDED(hr);
}

bool BloomBorderRenderer::CreateSwapChain(HWND hwnd) {
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

bool BloomBorderRenderer::CreateShaders() {
  vertex_shader_ = std::make_unique<VertexShader>();
  if (!vertex_shader_->CompileFromSource(device_.Get(), g_vertexShader)) {
    return false;
  }

  ps_original_ = std::make_unique<PixelShader>();
  if (!ps_original_->CompileFromSource(device_.Get(), g_pixelShaderOriginal)) {
    return false;
  }

  ps_blur_h_ = std::make_unique<PixelShader>();
  if (!ps_blur_h_->CompileFromSource(device_.Get(), g_pixelShaderBlurH)) {
    return false;
  }

  ps_blur_v_ = std::make_unique<PixelShader>();
  if (!ps_blur_v_->CompileFromSource(device_.Get(), g_pixelShaderBlurV)) {
    return false;
  }

  ps_composite_ = std::make_unique<PixelShader>();
  if (!ps_composite_->CompileFromSource(device_.Get(),
                                        g_pixelShaderComposite)) {
    return false;
  }

  return true;
}

bool BloomBorderRenderer::CreateRenderTargets() {
  rt_original_ = std::make_unique<RenderTarget>();
  if (!rt_original_->Create(device_.Get(), width_, height_)) {
    return false;
  }

  rt_blur_h_ = std::make_unique<RenderTarget>();
  if (!rt_blur_h_->Create(device_.Get(), width_, height_)) {
    return false;
  }

  rt_blur_v_ = std::make_unique<RenderTarget>();
  if (!rt_blur_v_->Create(device_.Get(), width_, height_)) {
    return false;
  }

  return true;
}

bool BloomBorderRenderer::CreateResources() {
  // Constant Buffer
  constant_buffer_ = std::make_unique<ConstantBuffer<ShaderConstants>>();
  if (!constant_buffer_->Create(device_.Get())) {
    MessageBoxW(nullptr, L"Failed to create constant buffer", L"Error", MB_OK);
    return false;
  }

  // Sampler State
  D3D11_SAMPLER_DESC samplerDesc = {};
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

  if (FAILED(device_->CreateSamplerState(&samplerDesc, &sampler_state_))) {
    return false;
  }

  // Blend State
  D3D11_BLEND_DESC blendDesc = {};
  blendDesc.RenderTarget[0].BlendEnable = TRUE;
  blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
  blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D11_COLOR_WRITE_ENABLE_ALL;

  if (FAILED(device_->CreateBlendState(&blendDesc, &blend_state_))) {
    return false;
  }

  float blendFactor[4] = {0, 0, 0, 0};
  context_->OMSetBlendState(blend_state_.Get(), blendFactor, 0xffffffff);

  return true;
}

bool BloomBorderRenderer::CreateBackBufferTargetView() {

  if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer_)))) {
    return false;
  }
  if (FAILED(device_->CreateRenderTargetView(back_buffer_.Get(), nullptr, &back_buffer_rtv_))) {
    return false;
  }

  return true;
}

bool BloomBorderRenderer::SetupDirectComposition(HWND hwnd) {
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

void BloomBorderRenderer::Render(HWND hwnd) {
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

  // Setup Common State
  context_->VSSetShader(vertex_shader_->Get(), nullptr, 0);
  context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context_->IASetInputLayout(nullptr);

  //ComPtr<ID3D11Texture2D> backBuffer;
  //swap_chain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

  //ComPtr<ID3D11RenderTargetView> backBufferRTV;
  //device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &backBufferRTV);

  //float clear[4] = { 0, 0, 0, 0 };
  //context_->ClearRenderTargetView(backBufferRTV.Get(), clear);
  //// Render Passes
  //RenderCommands(iTime, backBufferRTV.Get());

  back_buffer_.Reset();
  swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer_));
  float clear[4] = { 0, 0, 0, 0 };
  context_->ClearRenderTargetView(back_buffer_rtv_.Get(), clear);

  // Render Passes
  RenderCommands(iTime);

  // SwapChain present
  swap_chain_->Present(1, 0);

  // Update FPS
  UpdateFPS(hwnd);
}

void BloomBorderRenderer::RenderCommands(float iTime) {

  if (draw_commands_.empty()) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(draw_commands_mutex_);
    for (auto& command : draw_commands_) {
      RenderOriginal(command.get(), iTime);
      RenderBlurH(iTime);
      RenderBlurV(iTime);
      // RenderComposite(command.get(),iTime);
      RenderCompositeToTarget(command.get(), iTime);
    }
  }
}

void BloomBorderRenderer::RenderOriginal(DrawCommand* command, float iTime) {
  // Clear and set render target
  float clear[4] = {0, 0, 0, 0};
  context_->ClearRenderTargetView(rt_original_->GetRTV(), clear);
  context_->OMSetRenderTargets(1, rt_original_->GetRTVAddress(), nullptr);

  // Update constants
  ShaderConstants constants;
  if(command->GetType() == DrawCommandType::FULLSCREEN) {
    auto fulls_screen_command = static_cast<DrawFullScreenCommand*>(command);
    constants.iType = static_cast<int>(DrawCommandType::FULLSCREEN);
    constants.iResolution[0] = static_cast<float>(fulls_screen_command->GetWidth() / 4.0);
    constants.iResolution[1] = static_cast<float>(fulls_screen_command->GetHeight() / 4.0);
  }

  if (command->GetType() == DrawCommandType::CIRCLE) {
    auto circle_command = static_cast<DrawCircleCommand*>(command);
    constants.iType = static_cast<int>(DrawCommandType::CIRCLE);
    constants.iResolution[0] = static_cast<float>(width_);
    constants.iResolution[1] = static_cast<float>(height_);
    auto ndc = ScreenToNDC(static_cast<int>(circle_command->GetX()), static_cast<int>(circle_command->GetY()));
    constants.iCenter[0] = ndc.x;
    constants.iCenter[1] = ndc.y;
    constants.iRadius = circle_command->GetRadius();
    auto now = std::chrono::steady_clock::now();
    constants.animationTime = std::chrono::duration<float>(now - circle_command->GetAnimationStartTime()).count();
  }

  constants.iTime = iTime;
  constants.whFactor = wh_factor_;
  constant_buffer_->Update(context_.Get(), constants);

  // Set shader and resources
  context_->PSSetShader(ps_original_->Get(), nullptr, 0);
  context_->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());

  // Draw
  context_->Draw(3, 0);
}

void BloomBorderRenderer::RenderBlurH(float iTime) {
  // Set render target
  context_->OMSetRenderTargets(1, rt_blur_h_->GetRTVAddress(), nullptr);

  // Update constants
  ShaderConstants constants;
  constants.iResolution[0] = static_cast<float>(width_);
  constants.iResolution[1] = static_cast<float>(height_);
  constants.iTime = iTime;
  // constants.whFactor = wh_factor_;
  constant_buffer_->Update(context_.Get(), constants);

  // Set shader and resources
  context_->PSSetShader(ps_blur_h_->Get(), nullptr, 0);
  context_->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
  context_->PSSetShaderResources(0, 1, rt_original_->GetSRVAddress());
  context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());

  // Draw
  context_->Draw(3, 0);
}

void BloomBorderRenderer::RenderBlurV(float iTime) {
  // Set render target
  context_->OMSetRenderTargets(1, rt_blur_v_->GetRTVAddress(), nullptr);

  // Update constants
  ShaderConstants constants;
  constants.iResolution[0] = static_cast<float>(width_);
  constants.iResolution[1] = static_cast<float>(height_);
  constants.iTime = iTime;
  // constants.whFactor = wh_factor_;
  constant_buffer_->Update(context_.Get(), constants);

  // Unbind previous SRV to avoid hazard
  ID3D11ShaderResourceView *nullSRV = nullptr;
  context_->PSSetShaderResources(0, 1, &nullSRV);

  // Set shader and resources
  context_->PSSetShader(ps_blur_v_->Get(), nullptr, 0);
  context_->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
  context_->PSSetShaderResources(0, 1, rt_blur_h_->GetSRVAddress());
  context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());

  // Draw
  context_->Draw(3, 0);
}

void BloomBorderRenderer::RenderComposite(DrawCommand* command,float iTime) {
  // Get back buffer
  //ComPtr<ID3D11Texture2D> backBuffer;
  //swap_chain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

  //ComPtr<ID3D11RenderTargetView> rtv;
  //device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv);

  // Clear and set render target
  //float clear[4] = {0, 0, 0, 0};
  //context_->ClearRenderTargetView(rtv.Get(), clear);
  //context_->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

  // Clear and set render target
  float clear[4] = { 0, 0, 0, 0 };
  context_->ClearRenderTargetView(back_buffer_rtv_.Get(), clear);
  context_->OMSetRenderTargets(1, back_buffer_rtv_.GetAddressOf(), nullptr);

  // Update constants
  ShaderConstants constants;
  if(command->GetType() == DrawCommandType::FULLSCREEN) {
    auto fulls_screen_command = static_cast<DrawFullScreenCommand*>(command);
    constants.iType = static_cast<int>(DrawCommandType::FULLSCREEN);
    constants.iResolution[0] = static_cast<float>(fulls_screen_command->GetWidth() / 4.0);
    constants.iResolution[1] = static_cast<float>(fulls_screen_command->GetHeight() / 4.0);
  }

  if (command->GetType() == DrawCommandType::CIRCLE) {
    auto circle_command = static_cast<DrawCircleCommand*>(command);
    constants.iType = static_cast<int>(DrawCommandType::CIRCLE);
    constants.iResolution[0] = static_cast<float>(width_);
    constants.iResolution[1] = static_cast<float>(height_);
    auto ndc = ScreenToNDC(static_cast<int>(circle_command->GetX()), static_cast<int>(circle_command->GetY()));
    constants.iCenter[0] = ndc.x;
    constants.iCenter[1] = ndc.y;
    constants.iRadius = circle_command->GetRadius();
    auto now = std::chrono::steady_clock::now();
    constants.animationTime = std::chrono::duration<float>(now - circle_command->GetAnimationStartTime()).count();
  }

  constants.iTime = iTime;
  constants.whFactor = wh_factor_;
  constant_buffer_->Update(context_.Get(), constants);

  // Unbind previous SRV
  ID3D11ShaderResourceView *nullSRV = nullptr;
  context_->PSSetShaderResources(0, 1, &nullSRV);

  // Set shader and resources
  context_->PSSetShader(ps_composite_->Get(), nullptr, 0);
  context_->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
  context_->PSSetShaderResources(0, 1, rt_blur_v_->GetSRVAddress());
  context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());

  // Draw
  context_->Draw(3, 0);
}

void BloomBorderRenderer::RenderCompositeToTarget(DrawCommand* command, float iTime) {
context_->OMSetRenderTargets(1, &back_buffer_rtv_, nullptr);

// Update constants
ShaderConstants constants = {};  // Initialize to zero

  if(command->GetType() == DrawCommandType::FULLSCREEN) {
    auto fullscreen_command = static_cast<DrawFullScreenCommand*>(command);
    constants.iType = static_cast<int>(DrawCommandType::FULLSCREEN);
    constants.iResolution[0] = static_cast<float>(fullscreen_command->GetWidth() / 4.0);
    constants.iResolution[1] = static_cast<float>(fullscreen_command->GetHeight() / 4.0);
  }

  if (command->GetType() == DrawCommandType::CIRCLE) {
    auto circle_command = static_cast<DrawCircleCommand*>(command);
    constants.iType = static_cast<int>(DrawCommandType::CIRCLE);
    constants.iResolution[0] = static_cast<float>(width_);
    constants.iResolution[1] = static_cast<float>(height_);

    auto ndc = ScreenToNDC(
      static_cast<int>(circle_command->GetX()),
      static_cast<int>(circle_command->GetY())
    );
    constants.iCenter[0] = ndc.x;
    constants.iCenter[1] = ndc. y;
    constants.iRadius = circle_command->GetRadius();

    auto now = std::chrono::steady_clock::now();
    constants.animationTime = std::chrono::duration<float>(
      now - circle_command->GetAnimationStartTime()
    ).count();
  }

  constants.iTime = iTime;
  constants.whFactor = wh_factor_;
  constant_buffer_->Update(context_.Get(), constants);

  // Unbind previous SRV
  ID3D11ShaderResourceView *nullSRV = nullptr;
  context_->PSSetShaderResources(0, 1, &nullSRV);

  // Set shader and resources
  context_->PSSetShader(ps_composite_->Get(), nullptr, 0);
  context_->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
  context_->PSSetShaderResources(0, 1, rt_blur_v_->GetSRVAddress());
  context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());

  // Draw
  context_->Draw(3, 0);
}

void BloomBorderRenderer::UpdateFPS(HWND hwnd) {
  frame_count_++;
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration<float>(now - last_fps_time_).count();

  if (elapsed >= 1.0f) {
    float fps = frame_count_ / elapsed;
    wchar_t title[256];
    swprintf_s(title, L"Multi-Pass Blur - FPS: %.1f | whFactor: %.1f", fps,
               wh_factor_);
    SetWindowTextW(hwnd, title);

    frame_count_ = 0;
    last_fps_time_ = now;
  }
}

void BloomBorderRenderer::Resize(int width, int height) {
  if (width == width_ && height == height_) {
    return;
  }

  width_ = width;
  height_ = height;
  CreateRenderTargets();
}

void BloomBorderRenderer::Reset() {
  dcomp_device_.Reset();
  dcomp_target_.Reset();
  dcomp_visual_.Reset();

  device_.Reset();
  context_.Reset();
  swap_chain_.Reset();

  rt_original_.reset();
  rt_blur_h_.reset();
  rt_blur_v_.reset();

  vertex_shader_.reset();
  ps_original_.reset();
  ps_blur_h_.reset();
  ps_blur_v_.reset();
  ps_composite_.reset();
}

Vec2 BloomBorderRenderer::ScreenToNDC(int x, int y) {
  return Vec2(float(x) - float(width_) / 2.0f, float(y) - float(height_) / 2.0f);
}

} // namespace D3D11
