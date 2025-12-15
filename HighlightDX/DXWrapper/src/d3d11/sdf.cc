#include "sdf.h"
#include "mathematics/Transform.h"

namespace D3D11 {
  static_assert(sizeof(ShaderConstants) % 16 == 0, "ShaderConstants must be aligned to 16 bytes");

  static const char* vertex_shader = R"(
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
  static const char* pixel_shader_original = R"(
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
  static const char* pixel_shader_blur_h = R"(
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
  static const char* pixel_shader_blur_v = R"(
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
  static const char* pixel_shader_composite = R"(
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

  SDF::SDF(int width, int height): width_(width), height_(height)
  {
    sampler_type_ = SamplerType::LinearClamp;
  }

  void SDF::Initialize(ID3D11Device* device)
  {
    if (!CreateRenderTargets(device)) {
      MessageBoxW(nullptr, L"Failed to CreateRenderTargets", L"Error", MB_OK);
    }
    if (!CreateShaders(device)) {
      MessageBoxW(nullptr, L"Failed to CreateShaders", L"Error", MB_OK);
    }
    if (!CreateConstantBuffer(device)) {
      MessageBoxW(nullptr, L"Failed to CreateConstantBuffer", L"Error", MB_OK);
    }
  }

  void SDF::CreateSamplerState(SamplerManager* sampler_manager)
  {
    sampler_state_ = sampler_manager->GetSampler(sampler_type_);
  }

  void SDF::CreateBendState(BlenderManager* blender_manager)
  {
    blend_state_ = blender_manager->GetBlender();
  }

  bool SDF::CreateShaders(ID3D11Device* device)
  {
    vertex_shader_ = std::make_unique<VertexShader>();
    if (!vertex_shader_->CompileFromSource(device, vertex_shader)) {
      return false;
    }

    ps_original_ = std::make_unique<PixelShader>();
    if (!ps_original_->CompileFromSource(device, pixel_shader_original)) {
      return false;
    }

    ps_blur_h_ = std::make_unique<PixelShader>();
    if (!ps_blur_h_->CompileFromSource(device, pixel_shader_blur_h)) {
      return false;
    }

    ps_blur_v_ = std::make_unique<PixelShader>();
    if (!ps_blur_v_->CompileFromSource(device, pixel_shader_blur_v)) {
      return false;
    }

    ps_composite_ = std::make_unique<PixelShader>();
    if (!ps_composite_->CompileFromSource(device, pixel_shader_composite)) {
      return false;
    }

    return true;
  }

  bool SDF::CreateRenderTargets(ID3D11Device* device)
  {
    rt_original_ = std::make_unique<RenderTarget>();
    if (!rt_original_->Create(device, width_, height_)) {
      return false;
    }

    rt_blur_h_ = std::make_unique<RenderTarget>();
    if (!rt_blur_h_->Create(device, width_, height_)) {
      return false;
    }

    rt_blur_v_ = std::make_unique<RenderTarget>();
    if (!rt_blur_v_->Create(device, width_, height_)) {
      return false;
    }

    return true;
  }

  bool SDF::CreateConstantBuffer(ID3D11Device* device)
  {
    constant_buffer_ = std::make_unique<ConstantBuffer<ShaderConstants>>();
    if (!constant_buffer_->Create(device)) {
      return false;
    }
    return true;
  }

  void SDF::RenderOriginal(ID3D11DeviceContext* context, DrawCommand* command, float iTime)
  {
    float clear[4] = { 0, 0, 0, 0 };
    context->ClearRenderTargetView(rt_original_->GetRTV(), clear);
    context->OMSetRenderTargets(1, rt_original_->GetRTVAddress(), nullptr);

    // Update constants
    ShaderConstants constants;
    if (command->GetType() == DrawCommandType::FULLSCREEN) {
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
      auto ndc = Transform::ScreenToNDC(static_cast<int>(circle_command->GetX()), static_cast<int>(circle_command->GetY()), width_, height_);
      constants.iCenter[0] = ndc.x;
      constants.iCenter[1] = ndc.y;
      constants.iRadius = circle_command->GetRadius();
      auto now = std::chrono::steady_clock::now();
      constants.animationTime = std::chrono::duration<float>(now - circle_command->GetAnimationStartTime()).count();
    }

    constants.iTime = iTime;
    constants.whFactor = wh_factor_;
    constant_buffer_->Update(context, constants);

    // Set shader and resources
    context->PSSetShader(ps_original_->Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());

    // Draw
    context->Draw(3, 0);
  }

  void SDF::RenderBlurH(ID3D11DeviceContext* context, float iTime)
  {
    // Set render target
    context->OMSetRenderTargets(1, rt_blur_h_->GetRTVAddress(), nullptr);

    // Update constants
    ShaderConstants constants;
    constants.iResolution[0] = static_cast<float>(width_);
    constants.iResolution[1] = static_cast<float>(height_);
    constants.iTime = iTime;
    // constants.whFactor = wh_factor_;
    constant_buffer_->Update(context, constants);

    // Set shader and resources
    context->PSSetShader(ps_blur_h_->Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
    context->PSSetShaderResources(0, 1, rt_original_->GetSRVAddress());
    context->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());

    // Draw
    context->Draw(3, 0);
  }

  void SDF::RenderBlurV(ID3D11DeviceContext* context, float iTime)
  {
    // Set render target
    context->OMSetRenderTargets(1, rt_blur_v_->GetRTVAddress(), nullptr);

    // Update constants
    ShaderConstants constants;
    constants.iResolution[0] = static_cast<float>(width_);
    constants.iResolution[1] = static_cast<float>(height_);
    constants.iTime = iTime;
    // constants.whFactor = wh_factor_;
    constant_buffer_->Update(context, constants);

    // Unbind previous SRV to avoid hazard
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    // Set shader and resources
    context->PSSetShader(ps_blur_v_->Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
    context->PSSetShaderResources(0, 1, rt_blur_h_->GetSRVAddress());
    context->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());

    // Draw
    context->Draw(3, 0);
  }

  void SDF::RenderCompositeToTarget(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float iTime)
  {
    context->OMSetRenderTargets(1, &rtv, nullptr);

    // Update constants
    ShaderConstants constants;
    if (command->GetType() == DrawCommandType::FULLSCREEN) {
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
      auto ndc = Transform::ScreenToNDC(static_cast<int>(circle_command->GetX()), static_cast<int>(circle_command->GetY()), width_, height_);
      constants.iCenter[0] = ndc.x;
      constants.iCenter[1] = ndc.y;
      constants.iRadius = circle_command->GetRadius();
      auto now = std::chrono::steady_clock::now();
      constants.animationTime = std::chrono::duration<float>(now - circle_command->GetAnimationStartTime()).count();
    }

    constants.iTime = iTime;
    constants.whFactor = wh_factor_;
    constant_buffer_->Update(context, constants);

    // Unbind previous SRV
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    // Set shader and resources
    context->PSSetShader(ps_composite_->Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
    context->PSSetShaderResources(0, 1, rt_blur_v_->GetSRVAddress());
    context->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());

    // Draw
    context->Draw(3, 0);
  }

  void SDF::Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float time)
  {
    context->VSSetShader(vertex_shader_->Get(), nullptr, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);

    if (command->GetType() == DrawCommandType::FULLSCREEN) {
      wh_factor_ = 2.005f;
    }

    RenderOriginal(context, command, time);
    RenderBlurH(context, time);
    RenderBlurV(context, time);
    RenderCompositeToTarget(context, rtv, command, time);

  }
}
