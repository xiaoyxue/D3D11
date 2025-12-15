#pragma once
#include <cstring>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace D3D11 {
template <typename T> class ConstantBuffer {
private:
  ComPtr<ID3D11Buffer> buffer_;

public:
  bool Create(ID3D11Device *device) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(T);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buffer_));
  }

  void Update(ID3D11DeviceContext *context, const T &data) {
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                               &mapped))) {
      std::memcpy(mapped.pData, &data, sizeof(T));
      context->Unmap(buffer_.Get(), 0);
    }
  }

  ID3D11Buffer *Get() const { return buffer_.Get(); }
  ID3D11Buffer *const *GetAddressOf() const { return buffer_.GetAddressOf(); }
};
} // namespace D3D11
