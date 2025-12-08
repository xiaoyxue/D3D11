#pragma once
#include <d3d11.h>
#include <dxgi1_3.h>
#include <dcomp.h>
#include <wrl/client.h>

namespace D3D11
{
	class Renderer 
	{
	public:
		Renderer() = default;
		virtual ~Renderer() = default;

		virtual bool Initialize(HWND hwnd, UINT width, UINT height) = 0;
		virtual void Render(HWND hwnd) = 0;
		virtual void Resize(UINT width, UINT height) = 0;
		virtual void Reset() = 0;
		virtual ID3D11Device* GetDevice() const = 0;
		virtual ID3D11DeviceContext* GetContext() const = 0;
	};
}