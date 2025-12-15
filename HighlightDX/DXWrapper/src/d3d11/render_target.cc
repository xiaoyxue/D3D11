#include "render_target.h"

namespace D3D11 {
bool RenderTarget::Create(ID3D11Device *device, UINT width, UINT height) {
  width_ = width;
  height_ = height;

  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.Width = width;
  texDesc.Height = height;
  texDesc.MipLevels = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  texDesc.SampleDesc.Count = 1;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

  if (FAILED(device->CreateTexture2D(&texDesc, nullptr, &texture_))) {
    return false;
  }

  if (FAILED(device->CreateRenderTargetView(texture_.Get(), nullptr, &rtv_))) {
    return false;
  }

  if (FAILED(
          device->CreateShaderResourceView(texture_.Get(), nullptr, &srv_))) {
    return false;
  }

  return true;
}
} // namespace D3D11
