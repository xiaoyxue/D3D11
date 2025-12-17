#include "sdf.h"
#include "mathematics/transform.h"

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


static const char* pixel_shader = R"(

cbuffer Constants : register(b0)
{
    int iType;
    float2 iResolution;
    float iTime;

    float animationTime;
    float whFactor;
    float2 iCenter;

    float iRadius;
    float maxAnimationTime;
    float borderWidth;
    int AA;
    
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
    float3 baseColor = float3(1.0, 0.5, 0.0);
    float radiusRatio = 1.0;
    float d = 0.0;
    if (iType == 0 || iType == 1)
    {
        float2 b = iResolution / whFactor;
        d = sdBox(normalizedCoord, b);
    }
    else if (iType == 2)
    {
        radiusRatio = min(animationTime / maxAnimationTime, 1.0);
        d = sdCircle(normalizedCoord - iCenter, iRadius * radiusRatio);
    }
    if (iType == 0)
    {
        d = -d;
    }

    //float alpha = abs(cos(iTime * 1.5));
    float aaWidth = 3.0;

    if (iType == 2)
    {
        bool needAA = (d > -aaWidth && d < aaWidth);
        
        if (needAA)
        {
            // SSAA
            float alphaSum = 0.0;
            int gridSize = AA;  // N x N grids
            int samples = gridSize * gridSize;
            float3 colorSum = float3(0.0, 0.0, 0.0);
            
            for (int y = 0; y < gridSize; y++)
            {
                for (int x = 0; x < gridSize; x++)
                {
                    // Sample offset
                    float2 offset = float2(
                        (float(x) + 0.5) / float(gridSize) - 0.5,
                        (float(y) + 0.5) / float(gridSize) - 0.5
                    );
                    
                    float2 sampleCoord = normalizedCoord + offset;
                    float sampleD = sdCircle(sampleCoord - iCenter, iRadius * radiusRatio);
                    
                    if (sampleD > 0.0 && sampleD < borderWidth)
                    {
                        // Calculate color based on distance from edge
                        float deltaEdge = 1.0;
                        float3 sampleColor;
                        
                        if (sampleD < deltaEdge)
                        {
                            // Near edge: use black
                            sampleColor = float3(0.0, 0.0, 0.0);
                        }
                        else
                        {
                            // Far from edge: use base color
                            sampleColor = baseColor;
                        }
                        
                        colorSum += sampleColor;
                        float sampleAlpha = 1.0 - smoothstep(0.0, borderWidth, sampleD);
                        alphaSum += sampleAlpha;
                    }
                }
            }
            
            float alpha = (alphaSum / float(samples)) * abs(cos(iTime * 1.5));
            float3 finalColor = colorSum / float(samples);

            if (alpha > 0.0)
            {
                return float4(finalColor * alpha, alpha); 
            }
            
            return float4(0, 0, 0, 0);
        }
    }

    if (d > 0.0 && d < borderWidth)
    {
        float alpha = (1.0 - smoothstep(0.0, borderWidth, d)) * abs(cos(iTime * 1.5));
        return float4(baseColor * alpha, alpha);  
    }
    
    return float4(0, 0, 0, 0);

}
)";

  SDF::SDF(int width, int height): width_(width), height_(height)
  {
    sampler_type_ = SamplerType::LinearClamp;
  }

  void SDF::Initialize(ID3D11Device* device)
  {
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
    pixel_shader_ = std::make_unique<PixelShader>();
    if (!pixel_shader_->CompileFromSource(device, pixel_shader)) {
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

  
  void SDF::RenderCommand(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float iTime)
  {
    context->OMSetRenderTargets(1, &rtv, nullptr);
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(blend_state_.Get(), blendFactor, 0xffffffff);

    // Update constants
    ShaderConstants constants;
    if (command->GetType() == DrawCommandType::FULLSCREEN) {
      auto fulls_screen_command = static_cast<DrawFullScreenCommand*>(command);
      constants.iType = static_cast<int>(DrawCommandType::FULLSCREEN);
    }

    if (command->GetType() == DrawCommandType::CIRCLE) {
      auto circle_command = static_cast<DrawCircleCommand*>(command);
      constants.iType = static_cast<int>(DrawCommandType::CIRCLE);
      auto ndc = Transform::ScreenToNDC(static_cast<int>(circle_command->GetX()), static_cast<int>(circle_command->GetY()), width_, height_);
      constants.iCenter[0] = ndc.x;
      constants.iCenter[1] = ndc.y;
      constants.iRadius = circle_command->GetRadius();
      auto now = std::chrono::steady_clock::now();
      constants.animationTime = std::chrono::duration<float>(now - circle_command->GetAnimationStartTime()).count();
    }

    constants.maxAnimationTime = 1.0f;
    constants.iTime = iTime;
    constants.whFactor = wh_factor_;
    constants.iResolution[0] = static_cast<float>(width_);
    constants.iResolution[1] = static_cast<float>(height_);
    constants.borderWidth = 15.0f;
    constants.AA = 4;
    constant_buffer_->Update(context, constants);

    // Unbind previous SRV
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    // Set shader and resources
    context->PSSetShader(pixel_shader_->Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());


    // Draw
    context->Draw(3, 0);
  }

  void SDF::Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float time)
  {
    context->VSSetShader(vertex_shader_->Get(), nullptr, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);

    if (command->GetType() == DrawCommandType::FULLSCREEN) {
      wh_factor_ = 2.00f;
    }

    RenderCommand(context, rtv, command, time);

  }
}
