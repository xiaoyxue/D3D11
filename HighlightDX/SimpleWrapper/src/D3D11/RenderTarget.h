#pragma once
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace D3D11 
{
	class RenderTarget
	{
	private:
		ComPtr<ID3D11Texture2D> mTexture;
		ComPtr<ID3D11RenderTargetView> mRTV;
		ComPtr<ID3D11ShaderResourceView> mSRV;
		UINT mWidth;
		UINT mHeight;

	public:
		RenderTarget() : mWidth(0), mHeight(0) {}

		bool Create(ID3D11Device* device, UINT width, UINT height);

		ID3D11RenderTargetView* GetRTV() const { return mRTV.Get(); }
		ID3D11RenderTargetView* const* GetRTVAddress() const { return mRTV.GetAddressOf(); }
		ID3D11ShaderResourceView* GetSRV() const { return mSRV.Get(); }
		ID3D11ShaderResourceView* const* GetSRVAddress() const { return mSRV.GetAddressOf(); }
		ID3D11Texture2D* GetTexture() const { return mTexture.Get(); }

		UINT GetWidth() const { return mWidth; }
		UINT GetHeight() const { return mHeight; }
	};
}