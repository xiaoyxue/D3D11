#include "Shader.h"
#include <cstring>

namespace D3D11
{
	static bool CompileShaderInternal(const char* source, const char* target, ID3DBlob** outBlob)
	{
		ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3DCompile(
			source, std::strlen(source),
			nullptr, nullptr, nullptr,
			"main", target,
			D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
			0, outBlob, &errorBlob
		);

		if (FAILED(hr))
		{
			if (errorBlob) 
			{
				OutputDebugStringA("Shader Error: ");
				OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
			}
			return false;
		}
		return true;
	}

	bool VertexShader::CompileFromSource(ID3D11Device* device, const char* source)
	{
		ComPtr<ID3DBlob> blob;
		if (!CompileShaderInternal(source, "vs_5_0", &blob))
		{
			return false;
		}

		return SUCCEEDED(device->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			nullptr,
			&mShader
		));
	}

	bool PixelShader::CompileFromSource(ID3D11Device* device, const char* source)
	{
		ComPtr<ID3DBlob> blob;
		if (!CompileShaderInternal(source, "ps_5_0", &blob))
		{
			return false;
		}

		return SUCCEEDED(device->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			nullptr,
			&mShader
		));
	}
}


