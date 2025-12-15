#include "quad.h"
#include <d3dcompiler.h>

namespace D3D11 {

// Vertex Shader
static const char* vertex_shader_code = R"(
cbuffer TransformBuffer : register(b0)
{
    matrix mvp;
    float iTime;
    float _padding1;
    float _padding2;
    float _padding3;
};

struct VS_INPUT {
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, mvp);
    output.pos = pos;
    output.uv = input.uv;
    return output;
}
)";

// Pixel Shader
static const char* pixel_shader_code = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float4 color = tex.Sample(samp, input.uv);
    // 预乘 alpha
    color.rgb *= color.a;
    return color;
}
)";

Quad::Quad(int screen_width, int screen_height)
  : screen_width_(screen_width), screen_height_(screen_height) {
  sampler_type_ = SamplerType::Anisotropic;
  
  // Orthographic
  projection_ = DirectX::XMMatrixOrthographicOffCenterLH(
    -(float)screen_width_ / 2.0f, (float)screen_width_ / 2.0f,
    -(float)screen_height_ / 2.0f, (float)screen_height_ / 2.0f,
    0.0f, 1.0f
  );

  // Init
  scale_matrix_ = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
  rotation_matrix_ = DirectX::XMMatrixRotationZ(rotation_);
  initial_position_transform_ = DirectX::XMMatrixIdentity();
  translate_matrix_ = DirectX::XMMatrixIdentity();
  
  UpdateTransform();
}

void Quad::Initialize(ID3D11Device* device) {
  if (!CreateShaders(device)) {
    MessageBoxW(nullptr, L"Failed to create shaders for Quad", L"Error", MB_OK);
    return;
  }

  if (!CreateBuffers(device)) {
    MessageBoxW(nullptr, L"Failed to create buffers for Quad", L"Error", MB_OK);
    return;
  }

  if (!CreateConstantBuffer(device)) {
    MessageBoxW(nullptr, L"Failed to create constant buffer for Quad", L"Error", MB_OK);
    return;
  }

  texture_ = std::make_unique<Texture>();
}

bool Quad::CreateShaders(ID3D11Device* device) {

  vertex_shader_ = std::make_unique<VertexShader>();
  if (!vertex_shader_->CompileFromSource(device, vertex_shader_code)) {
    return false;
  }

  pixel_shader_ = std::make_unique<PixelShader>();
  if (!pixel_shader_->CompileFromSource(device, pixel_shader_code)) {
    return false;
  }

  D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };

  HRESULT hr = device->CreateInputLayout(
    layout, 
    2, 
    vertex_shader_->GetBlob()->GetBufferPointer(),
    vertex_shader_->GetBlob()->GetBufferSize(),
    &input_layout_
  );

  return SUCCEEDED(hr);
}

bool Quad::CreateBuffers(ID3D11Device* device) {
// Vertex buffer - 4 vertices with texture coordinates
QuadVertex vertices[] = {
  { -0.337f,  0.355f, 0.0f,  0.0f, 0.0f },  // TL
  {  0.337f,  0.355f, 0.0f,  1.0f, 0.0f },  // TR
  {  0.337f, -0.355f, 0.0f,  1.0f, 1.0f },  // BR
  { -0.337f, -0.355f, 0.0f,  0.0f, 1.0f }   // BL
};

  D3D11_BUFFER_DESC vb_desc = {};
  vb_desc.ByteWidth = sizeof(vertices);
  vb_desc.Usage = D3D11_USAGE_DEFAULT;
  vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA vb_data = {};
  vb_data.pSysMem = vertices;

  HRESULT hr = device->CreateBuffer(&vb_desc, &vb_data, &vertex_buffer_);
  if (FAILED(hr)) {
    return false;
  }

  UINT indices[] = {
    0, 1, 2,
    0, 2, 3
  };

  D3D11_BUFFER_DESC ib_desc = {};
  ib_desc.ByteWidth = sizeof(indices);
  ib_desc.Usage = D3D11_USAGE_DEFAULT;
  ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

  D3D11_SUBRESOURCE_DATA ib_data = {};
  ib_data.pSysMem = indices;

  hr = device->CreateBuffer(&ib_desc, &ib_data, &index_buffer_);
  return SUCCEEDED(hr);
}

bool Quad::CreateConstantBuffer(ID3D11Device* device) {
  constant_buffer_ = std::make_unique<ConstantBuffer<QuadConstants>>();
  return constant_buffer_->Create(device);
}

bool Quad::LoadTexture(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename) {
  if (!texture_) {
    texture_ = std::make_unique<Texture>();
  }
  return texture_->LoadFromFile(device, context, filename);
}

void Quad::SetSize(float width, float height) {
  width_ = width;
  height_ = height;
  scale_matrix_ = DirectX::XMMatrixScaling(width_ * scale_x_, height_ * scale_y_, 1.0f);
  UpdateTransform();
}

void Quad::SetPosition(float x, float y) {
  position_x_ = x;
  position_y_ = y;
  
  initial_position_transform_ = 
    DirectX::XMMatrixTranslation(-(float)screen_width_ / 2.0f, (float)screen_height_ / 2.0f, 0.0f) *
    DirectX::XMMatrixTranslation(position_x_, -position_y_, 0.0f);
  
  UpdateTransform();
}

void Quad::SetRotation(float angle_radians) {
  rotation_ = angle_radians;
  rotation_matrix_ = DirectX::XMMatrixRotationZ(rotation_);
  UpdateTransform();
}

void Quad::SetScale(float scale_x, float scale_y) {
  scale_x_ = scale_x;
  scale_y_ = scale_y;
  scale_matrix_ = DirectX::XMMatrixScaling(width_ * scale_x_, height_ * scale_y_, 1.0f);
  UpdateTransform();
}

void Quad::Translate(float dx, float dy) {
  position_x_ += dx;
  position_y_ += dy;
  SetPosition(position_x_, position_y_);
}

void Quad::Rotate(float angle_radians) {
  rotation_ += angle_radians;
  SetRotation(rotation_);
}

void Quad::Scale(float scale_x, float scale_y) {
  scale_x_ *= scale_x;
  scale_y_ *= scale_y;
  SetScale(scale_x_, scale_y_);
}

void Quad::UpdateTransform() {
  auto local_transform = scale_matrix_ * rotation_matrix_;
  local_to_world_ = local_transform * DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
}

void Quad::SetTranslateMatrix(const DirectX::XMMATRIX& translate_matrix) {
  translate_matrix_ = translate_matrix;
}

void Quad::CreateSamplerState(SamplerManager* sampler_manager) {
  sampler_state_ = sampler_manager->GetSampler(sampler_type_);
}

void Quad::CreateBendState(BlenderManager* blender_manager) {
  blend_state_ = blender_manager->GetBlender();
}

void Quad::Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv) {
  Draw(context, rtv, nullptr, 0.0f);
}

void Quad::Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float time) {
  // render target view
  context->OMSetRenderTargets(1, &rtv, nullptr);

  // Vertex buffer layout
  context->IASetInputLayout(input_layout_.Get());

  // Vertex buffer
  UINT stride = sizeof(QuadVertex);
  UINT offset = 0;
  context->IASetVertexBuffers(0, 1, vertex_buffer_.GetAddressOf(), &stride, &offset);

  // Index buffer
  context->IASetIndexBuffer(index_buffer_.Get(), DXGI_FORMAT_R32_UINT, 0);

  // Set primitive category and topology
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


  context->VSSetShader(vertex_shader_->Get(), nullptr, 0);
  context->PSSetShader(pixel_shader_->Get(), nullptr, 0);

  QuadConstants constants;
  DirectX::XMMATRIX mvp = local_to_world_ * initial_position_transform_ * translate_matrix_ * projection_;
  constants.mvp = DirectX::XMMatrixTranspose(mvp);  // hit: Transpose is needed in DirectX
  constants.animiationTime = time;
  context->VSSetConstantBuffers(0, 1, constant_buffer_->GetAddressOf());
  constant_buffer_->Update(context, constants);

  if (texture_ && texture_->GetSRV()) {
    context->PSSetShaderResources(0, 1, texture_->GetSRVAddress());
    context->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());
  }

  context->DrawIndexed(6, 0, 0);
}

} // namespace D3D11
