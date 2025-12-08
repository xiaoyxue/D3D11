#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace D3D11
{
	class VertexShader
	{
	private:
		ComPtr<ID3D11VertexShader> mShader;

	public:
		bool CompileFromSource(ID3D11Device* device, const char* source);
		ID3D11VertexShader* Get() const { return mShader.Get(); }
	};

	class PixelShader
	{
	private:
		ComPtr<ID3D11PixelShader> mShader;

	public:
		bool CompileFromSource(ID3D11Device* device, const char* source);
		ID3D11PixelShader* Get() const { return mShader.Get(); }
	};
}