#pragma once
#include "common/core.h"

using Microsoft::WRL::ComPtr;

namespace D3D11 {
class RenderTarget {
private:
  ComPtr<ID3D11Texture2D> texture_;
  ComPtr<ID3D11RenderTargetView> rtv_;
  ComPtr<ID3D11ShaderResourceView> srv_;
  UINT width_;
  UINT height_;

public:
  RenderTarget() : width_(0), height_(0) {}

  bool Create(ID3D11Device *device, UINT width, UINT height);

  ID3D11RenderTargetView *GetRTV() const { return rtv_.Get(); }
  ID3D11RenderTargetView *const *GetRTVAddress() const {
    return rtv_.GetAddressOf();
  }
  ID3D11ShaderResourceView *GetSRV() const { return srv_.Get(); }
  ID3D11ShaderResourceView *const *GetSRVAddress() const {
    return srv_.GetAddressOf();
  }
  ID3D11Texture2D *GetTexture() const { return texture_.Get(); }

  UINT GetWidth() const { return width_; }
  UINT GetHeight() const { return height_; }
};
} // namespace D3D11
