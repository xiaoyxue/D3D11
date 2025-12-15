#include "texture.h"

namespace D3D11 {

bool Texture::LoadFromFile(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename) {
  return LoadTextureFromWIC(device, context, filename);
}

bool Texture::LoadTextureFromWIC(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename) {
  // ===== 1. WIC =====
  Microsoft::WRL::ComPtr<IWICImagingFactory> wic_factory;
  HRESULT hr = CoCreateInstance(
    CLSID_WICImagingFactory,
    nullptr,
    CLSCTX_INPROC_SERVER,
    IID_PPV_ARGS(&wic_factory)
  );
  if (FAILED(hr)) {
    return false;
  }

  // ===== 2. Decoder =====
  Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
  hr = wic_factory->CreateDecoderFromFilename(
    filename,
    nullptr,
    GENERIC_READ,
    WICDecodeMetadataCacheOnDemand,
    &decoder
  );
  if (FAILED(hr)) {
    return false;
  }

  // ===== 3. First Frame =====
  Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr)) {
    return false;
  }

  // ===== 4. Convert to RGBA Format =====
  Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
  hr = wic_factory->CreateFormatConverter(&converter);
  if (FAILED(hr)) {
    return false;
  }

  hr = converter->Initialize(
    frame.Get(),
    GUID_WICPixelFormat32bppRGBA,
    WICBitmapDitherTypeNone,
    nullptr,
    0.0,
    WICBitmapPaletteTypeCustom
  );
  if (FAILED(hr)) {
    return false;
  }

  // ===== 5. Get Image Size =====
  UINT tex_width, tex_height;
  converter->GetSize(&tex_width, &tex_height);
  width_ = tex_width;
  height_ = tex_height;

  // ===== 6. Get Pixel Data =====
  UINT stride = tex_width * 4;  // RGBA = 4 bytes per pixel
  UINT image_size = stride * tex_height;
  std::vector<BYTE> pixels(image_size);

  hr = converter->CopyPixels(nullptr, stride, image_size, pixels.data());
  if (FAILED(hr)) {
    return false;
  }

  // ===== 7. Calculate Mipmap Levels =====
  mip_levels_ = 0;
  UINT width = tex_width;
  UINT height = tex_height;
  while (width > 1 || height > 1) {
    width = max(1, width / 2);
    height = max(1, height / 2);
    mip_levels_++;
  }
  mip_levels_++;  // Include level 0

  // ===== 8. Create Texture (Supports Automatic Mipmap Generation) =====
  D3D11_TEXTURE2D_DESC tex_desc = {};
  tex_desc.Width = tex_width;
  tex_desc.Height = tex_height;
  tex_desc.MipLevels = 0;  // 0 = Automatically generate full mipmap chain
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_DEFAULT;
  tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
  tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
  tex_desc.CPUAccessFlags = 0;

  // First create an empty texture (no initial data)
  hr = device->CreateTexture2D(&tex_desc, nullptr, &texture_);
  if (FAILED(hr)) {
    return false;
  }

  // ===== 9. Upload Level 0 Mipmap Data =====
  context->UpdateSubresource(
    texture_.Get(),
    0,              // Subresource index (mip level 0)
    nullptr,        // Entire region
    pixels.data(),  // Source data
    stride,         // Row pitch
    0               // Depth pitch (not needed for 2D textures)
  );

  // ===== 10. Create Shader Resource View =====
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = tex_desc.Format;
  srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MostDetailedMip = 0;      // From level 0
  srv_desc.Texture2D.MipLevels = -1;           // -1 = Use all available mipmap levels

  hr = device->CreateShaderResourceView(texture_.Get(), &srv_desc, &srv_);
  if (FAILED(hr)) {
    return false;
  }

  // ===== 11. Generate Mips =====
  context->GenerateMips(srv_.Get());

  // ===== 12. Verify Created Mipmap Levels =====
  D3D11_TEXTURE2D_DESC actual_desc;
  texture_->GetDesc(&actual_desc);
  mip_levels_ = actual_desc.MipLevels;

  return true;
}

void Texture::Release() {
  srv_.Reset();
  texture_.Reset();
  width_ = 0;
  height_ = 0;
  mip_levels_ = 0;
}

} // namespace D3D11
