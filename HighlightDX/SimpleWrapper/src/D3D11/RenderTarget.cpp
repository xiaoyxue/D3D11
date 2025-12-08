#include "RenderTarget.h"

namespace D3D11
{
	bool RenderTarget::Create(ID3D11Device* device, UINT width, UINT height)
	{
		mWidth = width;
		mHeight = height;

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		if (FAILED(device->CreateTexture2D(&texDesc, nullptr, &mTexture)))
		{
			return false;
		}

		if (FAILED(device->CreateRenderTargetView(mTexture.Get(), nullptr, &mRTV)))
		{
			return false;
		}

		if (FAILED(device->CreateShaderResourceView(mTexture.Get(), nullptr, &mSRV)))
		{
			return false;
		}

		return true;
	}
}