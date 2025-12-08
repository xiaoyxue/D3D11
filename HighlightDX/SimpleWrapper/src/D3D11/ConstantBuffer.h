#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <cstring>

using Microsoft::WRL::ComPtr;

namespace D3D11
{
	template<typename T>
	class ConstantBuffer
	{
	private:
		ComPtr<ID3D11Buffer> mBuffer;

	public:
		bool Create(ID3D11Device* device)
		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(T);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			return SUCCEEDED(device->CreateBuffer(&desc, nullptr, &mBuffer));
		}

		void Update(ID3D11DeviceContext* context, const T& data)
		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(context->Map(mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				std::memcpy(mapped.pData, &data, sizeof(T));
				context->Unmap(mBuffer.Get(), 0);
			}
		}

		ID3D11Buffer* Get() const { return mBuffer.Get(); }
		ID3D11Buffer* const* GetAddressOf() const { return mBuffer.GetAddressOf(); }
	};
}