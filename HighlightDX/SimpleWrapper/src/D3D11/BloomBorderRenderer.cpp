#include "BloomBorderRenderer.h"

namespace D3D11
{
	static const char* g_vertexShader = R"(
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}
)";
	// Pass 1:
	static const char* g_pixelShaderOriginal = R"(
cbuffer Constants : register(b0)
{
    float2 iResolution;
    float iTime;
    float whFactor;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float sdBox(float2 p, float2 b)
{
    float2 d = abs(p) - b;
    return length(max(d, float2(0, 0))) + min(max(d.x, d.y), 0.0);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 fragCoord = input.uv * iResolution;
    float2 normalizedCoord = fragCoord - iResolution / 2.0;
    float2 b = iResolution / whFactor;
    
    float d = sdBox(normalizedCoord, b);
    
    float2 uv = fragCoord / iResolution;
    // float3 col = 0.5 + 0.5 * cos(iTime + float3(uv.x, uv.y, uv.x) + float3(0, 2, 4));
	float3 col = 0.7 * float3(1, 0.5, 0) + 0.3 * abs(cos(iTime * 3)) * float3(1, 0.5, 0);
    float scale = 1.0;

    if (d > 0.0)
    {
        return float4(col * scale, 1.0);
    }
    else
    {
        return float4(0, 0, 0, 0);
    }
}
)";

	// Pass 2:
	static const char* g_pixelShaderBlurH = R"(
#define SIGMA 15.0

cbuffer Constants : register(b0)
{
    float2 iResolution;
    float iTime;
    float padding;
};

Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

float4 main(PSInput input) : SV_TARGET
{
    float2 texelSize = 1.0 / iResolution;
    float sigma = SIGMA;
    int radius = int(ceil(3.0 * sigma));
    
    float3 result = float3(0, 0, 0);
    float weightSum = 0.0;
    
    for (int x = -radius; x <= radius; x++)
    {
        float weight = gaussian(float(x), sigma);
        float2 offset = float2(float(x) * texelSize.x, 0.0);
        
        result += inputTexture.Sample(samplerState, input.uv + offset).rgb * weight;
        weightSum += weight;
    }
    
    return float4(result / weightSum, 1.0);
}
)";

	// Pass 3: 
	static const char* g_pixelShaderBlurV = R"(
#define SIGMA 15.0

cbuffer Constants : register(b0)
{
    float2 iResolution;
    float iTime;
    float padding;
};

Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

float4 main(PSInput input) : SV_TARGET
{
    float2 texelSize = 1.0 / iResolution;
    float sigma = SIGMA;
    int radius = int(ceil(3.0 * sigma));
    
    float3 result = float3(0, 0, 0);
    float weightSum = 0.0;
    
    for (int y = -radius; y <= radius; y++)
    {
        float weight = gaussian(float(y), sigma);
        float2 offset = float2(0.0, float(y) * texelSize.y);
        
        result += inputTexture.Sample(samplerState, input.uv + offset).rgb * weight;
        weightSum += weight;
    }
    
    return float4(result / weightSum, 1.0);
}
)";

	// Pass 4:
	static const char* g_pixelShaderComposite = R"(
#define BLOOM_STRENGTH 2.8
#define SIGMA 15.0

cbuffer Constants : register(b0)
{
    float2 iResolution;
    float iTime;
    float whFactor;
};

Texture2D bloomTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float sdBox(float2 p, float2 b)
{
    float2 d = abs(p) - b;
    return length(max(d, float2(0, 0))) + min(max(d.x, d.y), 0.0);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 fragCoord = input.uv * iResolution;
    float2 normalizedCoord = fragCoord - iResolution / 2.0;
    float2 b = iResolution / whFactor;
    
    float d = sdBox(normalizedCoord, b);

    float3 bloom = bloomTexture.Sample(samplerState, input.uv).rgb;
    float3 color = bloom * BLOOM_STRENGTH;
    
    if (d < 0.0)
    {
		return float4(color, 0.0);
    }
    else
    {
        return float4(0, 0, 0, 0);
    }
}
)";

	BloomBorderRenderer::BloomBorderRenderer()
		: mWidth(0)
		, mHeight(0)
		, mWhFactor(2.005f)
		, mFrameCount(0)
	{
		mStartTime = std::chrono::steady_clock::now();
		mLastFpsTime = mStartTime;
	}

	BloomBorderRenderer::~BloomBorderRenderer() = default;

	bool BloomBorderRenderer::Initialize(HWND hwnd, UINT width, UINT height)
	{
		mWidth = width;
		mHeight = height;

		if (!CreateDevice()) return false;
		if (!CreateSwapChain(hwnd)) return false;
		if (!CreateShaders()) return false;
		if (!CreateRenderTargets()) return false;
		if (!CreateResources()) return false;
		if (!SetupDirectComposition(hwnd)) return false;

		return true;
	}

	bool BloomBorderRenderer::CreateDevice()
	{
		UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
		HRESULT hr = D3D11CreateDevice(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
			levels, 1, D3D11_SDK_VERSION,
			&mDevice, nullptr, &mContext
		);

		return SUCCEEDED(hr);
	}

	bool BloomBorderRenderer::CreateSwapChain(HWND hwnd)
	{
		ComPtr<IDXGIDevice> dxgiDevice;
		mDevice.As(&dxgiDevice);

		ComPtr<IDXGIAdapter> adapter;
		dxgiDevice->GetAdapter(&adapter);

		ComPtr<IDXGIFactory2> factory;
		adapter->GetParent(IID_PPV_ARGS(&factory));

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = mWidth;
		desc.Height = mHeight;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

		return SUCCEEDED(factory->CreateSwapChainForComposition(mDevice.Get(), &desc, nullptr, &mSwapChain));
	}

	bool BloomBorderRenderer::CreateShaders()
	{
		mVertexShader = std::make_unique<VertexShader>();
		if (!mVertexShader->CompileFromSource(mDevice.Get(), g_vertexShader)) 
		{
			return false;
		}

		mPSOriginal = std::make_unique<PixelShader>();
		if (!mPSOriginal->CompileFromSource(mDevice.Get(), g_pixelShaderOriginal))
		{
			return false;
		}

		mPSBlurH = std::make_unique<PixelShader>();
		if (!mPSBlurH->CompileFromSource(mDevice.Get(), g_pixelShaderBlurH))
		{
			return false;
		}

		mPSBlurV = std::make_unique<PixelShader>();
		if (!mPSBlurV->CompileFromSource(mDevice.Get(), g_pixelShaderBlurV))
		{
			return false;
		}

		mPSComposite = std::make_unique<PixelShader>();
		if (!mPSComposite->CompileFromSource(mDevice.Get(), g_pixelShaderComposite))
		{
			return false;
		}

		return true;
	}

	bool BloomBorderRenderer::CreateRenderTargets()
	{
		mRTOriginal = std::make_unique<RenderTarget>();
		if (!mRTOriginal->Create(mDevice.Get(), mWidth, mHeight))
		{
			return false;
		}

		mRTBlurH = std::make_unique<RenderTarget>();
		if (!mRTBlurH->Create(mDevice.Get(), mWidth, mHeight))
		{
			return false;
		}

		mRTBlurV = std::make_unique<RenderTarget>();
		if (!mRTBlurV->Create(mDevice.Get(), mWidth, mHeight))
		{	
			return false;
		}

		return true;
	}

	bool BloomBorderRenderer::CreateResources()
	{
		// Constant Buffer
		mConstantBuffer = std::make_unique<ConstantBuffer<ShaderConstants>>();
		if (!mConstantBuffer->Create(mDevice.Get()))
		{
			return false;
		}

		// Sampler State
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

		if (FAILED(mDevice->CreateSamplerState(&samplerDesc, &mSamplerState)))
		{
			return false;
		}

		// Blend State
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		if (FAILED(mDevice->CreateBlendState(&blendDesc, &mBlendState)))
		{
			return false;
		}

		float blendFactor[4] = { 0, 0, 0, 0 };
		mContext->OMSetBlendState(mBlendState.Get(), blendFactor, 0xffffffff);

		return true;
	}

	bool BloomBorderRenderer::SetupDirectComposition(HWND hwnd)
	{
		ComPtr<IDXGIDevice> dxgiDevice;
		mDevice.As(&dxgiDevice);

		if (FAILED(DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(&mDCompDevice))))
		{
			return false;
		}

		if (FAILED(mDCompDevice->CreateTargetForHwnd(hwnd, TRUE, &mDCompTarget)))
		{
			return false;
		}

		if (FAILED(mDCompDevice->CreateVisual(&mDCompVisual)))
		{
			return false;
		}

		mDCompVisual->SetContent(mSwapChain.Get());
		mDCompTarget->SetRoot(mDCompVisual.Get());
		mDCompDevice->Commit();

		return true;
	}

	void BloomBorderRenderer::Render(HWND hwnd)
	{
		if (!mDevice || mWidth == 0 || mHeight == 0)
		{
			return;
		}

		auto now = std::chrono::steady_clock::now();
		float iTime = std::chrono::duration<float>(now - mStartTime).count();

		// Setup Viewport
		D3D11_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(mWidth);
		viewport.Height = static_cast<float>(mHeight);
		viewport.MaxDepth = 1.0f;
		mContext->RSSetViewports(1, &viewport);

		// Setup Common State
		mContext->VSSetShader(mVertexShader->Get(), nullptr, 0);
		mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mContext->IASetInputLayout(nullptr);

		// Render Passes
		RenderOriginalPass1(iTime);
		RenderBlurHPass2(iTime);
		RenderBlurVPass3(iTime);
		RenderCompositePass4(iTime);

		// Present
		mSwapChain->Present(1, 0);

		// Update FPS
		UpdateFPS(hwnd);
	}

	void BloomBorderRenderer::RenderOriginalPass1(float iTime)
	{
		// Clear and set render target
		float clear[4] = { 0, 0, 0, 0 };
		mContext->ClearRenderTargetView(mRTOriginal->GetRTV(), clear);
		mContext->OMSetRenderTargets(1, mRTOriginal->GetRTVAddress(), nullptr);

		// Update constants
		ShaderConstants constants;
		constants.iResolution[0] = static_cast<float>(mWidth);
		constants.iResolution[1] = static_cast<float>(mHeight);
		constants.iTime = iTime;
		constants.whFactor = mWhFactor;
		mConstantBuffer->Update(mContext.Get(), constants);

		// Set shader and resources
		mContext->PSSetShader(mPSOriginal->Get(), nullptr, 0);
		mContext->PSSetConstantBuffers(0, 1, mConstantBuffer->GetAddressOf());

		// Draw
		mContext->Draw(3, 0);
	}

	void BloomBorderRenderer::RenderBlurHPass2(float iTime)
	{
		// Set render target
		mContext->OMSetRenderTargets(1, mRTBlurH->GetRTVAddress(), nullptr);

		// Update constants
		ShaderConstants constants;
		constants.iResolution[0] = static_cast<float>(mWidth);
		constants.iResolution[1] = static_cast<float>(mHeight);
		constants.iTime = iTime;
		constants.whFactor = mWhFactor;
		mConstantBuffer->Update(mContext.Get(), constants);

		// Set shader and resources
		mContext->PSSetShader(mPSBlurH->Get(), nullptr, 0);
		mContext->PSSetConstantBuffers(0, 1, mConstantBuffer->GetAddressOf());
		mContext->PSSetShaderResources(0, 1, mRTOriginal->GetSRVAddress());
		mContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

		// Draw
		mContext->Draw(3, 0);
	}

	void BloomBorderRenderer::RenderBlurVPass3(float iTime)
	{
		// Set render target
		mContext->OMSetRenderTargets(1, mRTBlurV->GetRTVAddress(), nullptr);

		// Update constants
		ShaderConstants constants;
		constants.iResolution[0] = static_cast<float>(mWidth);
		constants.iResolution[1] = static_cast<float>(mHeight);
		constants.iTime = iTime;
		constants.whFactor = mWhFactor;
		mConstantBuffer->Update(mContext.Get(), constants);

		// Unbind previous SRV to avoid hazard
		ID3D11ShaderResourceView* nullSRV = nullptr;
		mContext->PSSetShaderResources(0, 1, &nullSRV);

		// Set shader and resources
		mContext->PSSetShader(mPSBlurV->Get(), nullptr, 0);
		mContext->PSSetConstantBuffers(0, 1, mConstantBuffer->GetAddressOf());
		mContext->PSSetShaderResources(0, 1, mRTBlurH->GetSRVAddress());
		mContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

		// Draw
		mContext->Draw(3, 0);
	}

	void BloomBorderRenderer::RenderCompositePass4(float iTime)
	{
		// Get back buffer
		ComPtr<ID3D11Texture2D> backBuffer;
		mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

		ComPtr<ID3D11RenderTargetView> rtv;
		mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv);

		// Clear and set render target
		float clear[4] = { 0, 0, 0, 0 };
		mContext->ClearRenderTargetView(rtv.Get(), clear);
		mContext->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

		// Update constants
		ShaderConstants constants;
		constants.iResolution[0] = static_cast<float>(mWidth);
		constants.iResolution[1] = static_cast<float>(mHeight);
		constants.iTime = iTime;
		constants.whFactor = mWhFactor;
		mConstantBuffer->Update(mContext.Get(), constants);

		// Unbind previous SRV
		ID3D11ShaderResourceView* nullSRV = nullptr;
		mContext->PSSetShaderResources(0, 1, &nullSRV);

		// Set shader and resources
		mContext->PSSetShader(mPSComposite->Get(), nullptr, 0);
		mContext->PSSetConstantBuffers(0, 1, mConstantBuffer->GetAddressOf());
		mContext->PSSetShaderResources(0, 1, mRTBlurV->GetSRVAddress());
		mContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

		// Draw
		mContext->Draw(3, 0);
	}

	void BloomBorderRenderer::UpdateFPS(HWND hwnd)
	{
		mFrameCount++;
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration<float>(now - mLastFpsTime).count();

		if (elapsed >= 1.0f)
		{
			float fps = mFrameCount / elapsed;
			wchar_t title[256];
			swprintf_s(title, L"Multi-Pass Blur - FPS: %.1f | whFactor: %.1f", fps, mWhFactor);
			SetWindowTextW(hwnd, title);

			mFrameCount = 0;
			mLastFpsTime = now;
		}
	}

	void BloomBorderRenderer::Resize(UINT width, UINT height)
	{
		if (width == mWidth && height == mHeight)
		{
			return;
		}

		mWidth = width;
		mHeight = height;
		CreateRenderTargets();
	}

	void BloomBorderRenderer::Reset()
	{
		mDCompDevice.Reset();
		mDCompTarget.Reset();
		mDCompVisual.Reset();

		mDevice.Reset();
		mContext.Reset();
		mSwapChain.Reset();

		mRTOriginal.reset();
		mRTBlurH.reset();
		mRTBlurV.reset();

		mVertexShader.reset();
		mPSOriginal.reset();
		mPSBlurH.reset();
		mPSBlurV.reset();
		mPSComposite.reset();
	}
}