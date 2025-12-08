#pragma once
#include "Renderer.h"
#include <d3d11.h>
#include <dxgi1_3.h>
#include <dcomp.h>
#include <wrl/client.h>
#include <memory>
#include <chrono>
#include "Shader.h"
#include "RenderTarget.h"
#include "ConstantBuffer.h"

using Microsoft::WRL::ComPtr;

namespace D3D11
{
	struct ShaderConstants
	{
		float iResolution[2];
		float iTime;
		float whFactor;
	};

	class BloomBorderRenderer : public Renderer 
	{
	public:
		BloomBorderRenderer();
		~BloomBorderRenderer();

		bool Initialize(HWND hwnd, UINT width, UINT height) override;
		void Render(HWND hwnd) override;
		void Resize(UINT width, UINT height) override;
		void Reset() override;

		void SetWhFactor(float factor) { mWhFactor = factor; }
		float GetWhFactor() const { return mWhFactor; }

		ID3D11Device* GetDevice() const override { return mDevice.Get(); }
		ID3D11DeviceContext* GetContext() const override { return mContext.Get(); }

	private:
		// Device & Context
		ComPtr<ID3D11Device> mDevice;
		ComPtr<ID3D11DeviceContext> mContext;
		ComPtr<IDXGISwapChain1> mSwapChain;

		// Shaders
		std::unique_ptr<VertexShader> mVertexShader;
		std::unique_ptr<PixelShader> mPSOriginal;
		std::unique_ptr<PixelShader> mPSBlurH;
		std::unique_ptr<PixelShader> mPSBlurV;
		std::unique_ptr<PixelShader> mPSComposite;

		// Render Targets
		std::unique_ptr<RenderTarget> mRTOriginal;
		std::unique_ptr<RenderTarget> mRTBlurH;
		std::unique_ptr<RenderTarget> mRTBlurV;

		// Resources
		std::unique_ptr<ConstantBuffer<ShaderConstants>> mConstantBuffer;
		ComPtr<ID3D11SamplerState> mSamplerState;
		ComPtr<ID3D11BlendState> mBlendState;

		// DirectComposition
		ComPtr<IDCompositionDevice> mDCompDevice;
		ComPtr<IDCompositionTarget> mDCompTarget;
		ComPtr<IDCompositionVisual> mDCompVisual;

		// State
		UINT mWidth;
		UINT mHeight;
		float mWhFactor;
		std::chrono::steady_clock::time_point mStartTime;

		// FPS Tracking
		UINT mFrameCount;
		std::chrono::steady_clock::time_point mLastFpsTime;
	private:
		bool CreateDevice();
		bool CreateSwapChain(HWND hwnd);
		bool CreateShaders();
		bool CreateRenderTargets();
		bool CreateResources();
		bool SetupDirectComposition(HWND hwnd);

		void RenderOriginalPass1(float iTime);
		void RenderBlurHPass2(float iTime);
		void RenderBlurVPass3(float iTime);
		void RenderCompositePass4(float iTime);

		void UpdateFPS(HWND hwnd);
	};
}