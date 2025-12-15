#pragma once
#include "common/core.h"
#include <wincodec.h>
#include <vector>
#include <string>

namespace D3D11 {

class Texture {
public:
  Texture() = default;
  ~Texture() = default;

  bool LoadFromFile(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);
  
  ID3D11ShaderResourceView* GetSRV() const { return srv_.Get(); }
  ID3D11ShaderResourceView* const* GetSRVAddress() const { return srv_.GetAddressOf(); }

  uint32 GetWidth() const { return width_; }
  uint32 GetHeight() const { return height_; }
  uint32 GetMipLevels() const { return mip_levels_; }

  // 释放资源
  void Release();

private:
  bool LoadTextureFromWIC(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);

private:
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv_;
  uint32 width_ = 0;
  uint32 height_ = 0;
  uint32 mip_levels_ = 0;
};

} // namespace D3D11
