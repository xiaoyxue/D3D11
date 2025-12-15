#pragma once
#include "drawable_object.h"
#include "texture.h"
#include "shader.h"
#include "constant_buffer.h"
#include "sampler_manager.h"
#include "blender_manager.h"
#include <DirectXMath.h>
#include <memory>

namespace D3D11 {

  struct QuadVertex {
    float x, y, z;
    float u, v;
  };


  struct QuadConstants {
    DirectX::XMMATRIX mvp;
    float animiationTime;
    float _padding1;
    float _padding2;
    float _padding3;

    QuadConstants() 
      : mvp(DirectX::XMMatrixIdentity()), animiationTime(0.0f),
        _padding1(0.0f), _padding2(0.0f), _padding3(0.0f) {}
  };

  class Quad : public DrawableObject {
  public:
    Quad(int screen_width, int screen_height);
    ~Quad() override = default;

    void Initialize(ID3D11Device* device) override;
    void Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv) override;
    void Draw(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, DrawCommand* command, float time) override;

    void CreateSamplerState(SamplerManager* sampler_manager) override;
    void CreateBendState(BlenderManager* blender_manager) override;
    ID3D11SamplerState* GetSamplerState() const override { return sampler_state_.Get(); }
    ID3D11BlendState* GetBlendState() const override { return blend_state_.Get(); }

    void SetPosition(float x, float y) override;
    void SetRotation(float angle_radians) override;
    void SetScale(float scale_x, float scale_y) override;
    void Translate(float dx, float dy) override;
    void Rotate(float angle_radians) override;
    void Scale(float scale_x, float scale_y) override;
    
    // Set translation matrix for animation (like SimpleQuad's translateMatrix)
    void SetTranslateMatrix(const DirectX::XMMATRIX& translate_matrix);

    bool LoadTexture(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);
    void SetSize(float width, float height);
    void UpdateTransform();

  private:
    bool CreateShaders(ID3D11Device* device);
    bool CreateBuffers(ID3D11Device* device);
    bool CreateConstantBuffer(ID3D11Device* device);

  private:

    int screen_width_;
    int screen_height_;


    DirectX::XMMATRIX local_to_world_;
    DirectX::XMMATRIX projection_;
    DirectX::XMMATRIX initial_position_transform_;
    DirectX::XMMATRIX translate_matrix_;
    DirectX::XMMATRIX rotation_matrix_;
    DirectX::XMMATRIX scale_matrix_;


    float position_x_ = 0.0f;
    float position_y_ = 0.0f;
    float rotation_ = 0.0f;
    float scale_x_ = 1.0f;
    float scale_y_ = 1.0f;
    float width_ = 100.0f;
    float height_ = 100.0f;

    // D3D11 resources
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_;
    
    // Shaders
    std::unique_ptr<VertexShader> vertex_shader_;
    std::unique_ptr<PixelShader> pixel_shader_;

    // Constant buffer
    std::unique_ptr<ConstantBuffer<QuadConstants>> constant_buffer_;

    // Texture
    std::unique_ptr<Texture> texture_;

    // Sampler and blender
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state_;
    SamplerType sampler_type_;
  };
}

