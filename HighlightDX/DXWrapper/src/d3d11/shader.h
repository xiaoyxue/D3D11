#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace D3D11 {
class VertexShader {
private:
  Microsoft::WRL::ComPtr<ID3D11VertexShader> shader_;
  Microsoft::WRL::ComPtr<ID3DBlob> blob_;

public:
  bool CompileFromSource(ID3D11Device *device, const char *source);
  ID3D11VertexShader *Get() const { return shader_.Get(); }
  ID3DBlob *GetBlob() const { return blob_.Get(); }
};

class PixelShader {
private:
  Microsoft::WRL::ComPtr<ID3D11PixelShader> shader_;

public:
  bool CompileFromSource(ID3D11Device *device, const char *source);
  ID3D11PixelShader *Get() const { return shader_.Get(); }
};
} // namespace D3D11
