#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dcomp.h>
#include <dxgi1_3.h>
#include <wrl/client.h>

#include <chrono>
#include <thread>
#include <cstring>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

// ========== HLSL Shaders ==========

// Vertex Shader
const char* g_vertexShader = R"(
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
const char* g_pixelShaderOriginal = R"(
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
const char* g_pixelShaderBlurH = R"(
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
const char* g_pixelShaderBlurV = R"(
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
const char* g_pixelShaderComposite = R"(
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

// ========== Render Target Structure ==========
struct RenderTarget
{
	ComPtr<ID3D11Texture2D> texture;
	ComPtr<ID3D11RenderTargetView> rtv;
	ComPtr<ID3D11ShaderResourceView> srv;
};

// ========== Constant Buffer ==========
struct ShaderConstants
{
	float iResolution[2];
	float iTime;
	float whFactor;
};

// ========== Context Structure ==========
struct D3D11Context
{
	// D3D11
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ComPtr<IDXGISwapChain1> swapChain;

	// Shaders
	ComPtr<ID3D11VertexShader> vertexShader;
	ComPtr<ID3D11PixelShader> psOriginal;
	ComPtr<ID3D11PixelShader> psBlurH;
	ComPtr<ID3D11PixelShader> psBlurV;
	ComPtr<ID3D11PixelShader> psComposite;

	// Render Targets
	RenderTarget rtOriginal;   // Pass 1
	RenderTarget rtBlurH;      // Pass 2
	RenderTarget rtBlurV;      // Pass 3

	// Resources
	ComPtr<ID3D11Buffer> constantBuffer;
	ComPtr<ID3D11SamplerState> samplerState;
	ComPtr<ID3D11BlendState> blendState;

	// DirectComposition
	ComPtr<IDCompositionDevice> dcompDevice;
	ComPtr<IDCompositionTarget> dcompTarget;
	ComPtr<IDCompositionVisual> dcompVisual;

	// State
	UINT width;
	UINT height;
	float whFactor;
	std::chrono::steady_clock::time_point startTime;

	// FPS
	UINT frameCount;
	std::chrono::steady_clock::time_point lastFpsTime;

	D3D11Context() : width(0), height(0), whFactor(2.005f), frameCount(0) {}
};

// ========== Forward Declarations ==========
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
bool Initialize(HWND hwnd, D3D11Context& ctx);
bool CompileShader(const char* source, const char* target, ID3DBlob** outBlob);
bool CompileShaders(D3D11Context& ctx);
bool CreateRenderTarget(ID3D11Device* device, UINT width, UINT height, RenderTarget& rt);
void Render(D3D11Context& ctx, HWND hwnd);
void Cleanup(D3D11Context& ctx);


// ========== Compile Shader Helper ==========
bool CompileShader(const char* source, const char* target, ID3DBlob** outBlob)
{
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompile(
		source, std::strlen(source),
		nullptr, nullptr, nullptr,
		"main", target,
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
		0, outBlob, &errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA("Shader Error: ");
			OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
		}
		return false;
	}

	return true;
}

// ========== Compile Shaders ==========
bool CompileShaders(D3D11Context& ctx)
{
	ComPtr<ID3DBlob> blob;

	// Vertex Shader
	if (!CompileShader(g_vertexShader, "vs_5_0", &blob)) return false;
	if (FAILED(ctx.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ctx.vertexShader))) return false;

	// Pixel Shaders
	if (!CompileShader(g_pixelShaderOriginal, "ps_5_0", &blob)) return false;
	if (FAILED(ctx.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ctx.psOriginal))) return false;

	if (!CompileShader(g_pixelShaderBlurH, "ps_5_0", &blob)) return false;
	if (FAILED(ctx.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ctx.psBlurH))) return false;

	if (!CompileShader(g_pixelShaderBlurV, "ps_5_0", &blob)) return false;
	if (FAILED(ctx.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ctx.psBlurV))) return false;

	if (!CompileShader(g_pixelShaderComposite, "ps_5_0", &blob)) return false;
	if (FAILED(ctx.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ctx.psComposite))) return false;

	return true;
}

// ========== Create Render Target ==========
bool CreateRenderTarget(ID3D11Device* device, UINT width, UINT height, RenderTarget& rt)
{
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(device->CreateTexture2D(&texDesc, nullptr, &rt.texture))) return false;
	if (FAILED(device->CreateRenderTargetView(rt.texture.Get(), nullptr, &rt.rtv))) return false;
	if (FAILED(device->CreateShaderResourceView(rt.texture.Get(), nullptr, &rt.srv))) return false;

	return true;
}

// ========== Initialize ==========
bool Initialize(HWND hwnd, D3D11Context& ctx)
{
	HRESULT hr;

	// D3D11 Device
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
		levels, 1, D3D11_SDK_VERSION,
		&ctx.device, nullptr, &ctx.context);
	if (FAILED(hr)) return false;

	// DXGI
	ComPtr<IDXGIDevice> dxgiDevice;
	ctx.device.As(&dxgiDevice);
	ComPtr<IDXGIAdapter> adapter;
	dxgiDevice->GetAdapter(&adapter);
	ComPtr<IDXGIFactory2> factory;
	adapter->GetParent(IID_PPV_ARGS(&factory));

	RECT rect;
	GetClientRect(hwnd, &rect);
	ctx.width = rect.right - rect.left;
	ctx.height = rect.bottom - rect.top;

	// Swap Chain
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = ctx.width;
	desc.Height = ctx.height;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 2;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

	hr = factory->CreateSwapChainForComposition(ctx.device.Get(), &desc, nullptr, &ctx.swapChain);
	if (FAILED(hr)) return false;

	// Shaders
	if (!CompileShaders(ctx)) return false;

	// Render Targets
	if (!CreateRenderTarget(ctx.device.Get(), ctx.width, ctx.height, ctx.rtOriginal)) return false;
	if (!CreateRenderTarget(ctx.device.Get(), ctx.width, ctx.height, ctx.rtBlurH)) return false;
	if (!CreateRenderTarget(ctx.device.Get(), ctx.width, ctx.height, ctx.rtBlurV)) return false;

	// Constant Buffer
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.ByteWidth = sizeof(ShaderConstants);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ctx.device->CreateBuffer(&cbDesc, nullptr, &ctx.constantBuffer);

	// Sampler State
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ctx.device->CreateSamplerState(&samplerDesc, &ctx.samplerState);

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
	ctx.device->CreateBlendState(&blendDesc, &ctx.blendState);

	float blendFactor[4] = { 0, 0, 0, 0 };
	ctx.context->OMSetBlendState(ctx.blendState.Get(), blendFactor, 0xffffffff);

	// DirectComposition
	DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(&ctx.dcompDevice));
	ctx.dcompDevice->CreateTargetForHwnd(hwnd, TRUE, &ctx.dcompTarget);
	ctx.dcompDevice->CreateVisual(&ctx.dcompVisual);
	ctx.dcompVisual->SetContent(ctx.swapChain.Get());
	ctx.dcompTarget->SetRoot(ctx.dcompVisual.Get());
	ctx.dcompDevice->Commit();

	return true;
}

// ========== Render ==========
void Render(D3D11Context& ctx, HWND hwnd)
{
	if (!ctx.device || ctx.width == 0 || ctx.height == 0) return;

	auto now = std::chrono::steady_clock::now();
	float iTime = std::chrono::duration<float>(now - ctx.startTime).count();

	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(ctx.width);
	viewport.Height = static_cast<float>(ctx.height);
	viewport.MaxDepth = 1.0f;
	ctx.context->RSSetViewports(1, &viewport);

	ctx.context->VSSetShader(ctx.vertexShader.Get(), nullptr, 0);
	ctx.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ctx.context->IASetInputLayout(nullptr);

	// Update Constants
	D3D11_MAPPED_SUBRESOURCE mapped;
	if (SUCCEEDED(ctx.context->Map(ctx.constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
		ShaderConstants* constants = static_cast<ShaderConstants*>(mapped.pData);
		constants->iResolution[0] = static_cast<float>(ctx.width);
		constants->iResolution[1] = static_cast<float>(ctx.height);
		constants->iTime = iTime;
		constants->whFactor = ctx.whFactor;
		ctx.context->Unmap(ctx.constantBuffer.Get(), 0);
	}

	// Pass 1
	{
		ctx.context->OMSetRenderTargets(1, ctx.rtOriginal.rtv.GetAddressOf(), nullptr);
		float clear[4] = { 0, 0, 0, 0 };
		ctx.context->ClearRenderTargetView(ctx.rtOriginal.rtv.Get(), clear);

		ctx.context->PSSetShader(ctx.psOriginal.Get(), nullptr, 0);
		ctx.context->PSSetConstantBuffers(0, 1, ctx.constantBuffer.GetAddressOf());
		ctx.context->Draw(3, 0);
	}

	// Pass 2
	{
		ctx.context->OMSetRenderTargets(1, ctx.rtBlurH.rtv.GetAddressOf(), nullptr);

		ctx.context->PSSetShader(ctx.psBlurH.Get(), nullptr, 0);
		ctx.context->PSSetConstantBuffers(0, 1, ctx.constantBuffer.GetAddressOf());
		ctx.context->PSSetShaderResources(0, 1, ctx.rtOriginal.srv.GetAddressOf());
		ctx.context->PSSetSamplers(0, 1, ctx.samplerState.GetAddressOf());
		ctx.context->Draw(3, 0);
	}

	// Pass 3
	{
		ctx.context->OMSetRenderTargets(1, ctx.rtBlurV.rtv.GetAddressOf(), nullptr);

		ctx.context->PSSetShader(ctx.psBlurV.Get(), nullptr, 0);
		ctx.context->PSSetConstantBuffers(0, 1, ctx.constantBuffer.GetAddressOf());

		ID3D11ShaderResourceView* nullSRV = nullptr;
		ctx.context->PSSetShaderResources(0, 1, &nullSRV);

		ctx.context->PSSetShaderResources(0, 1, ctx.rtBlurH.srv.GetAddressOf());
		ctx.context->PSSetSamplers(0, 1, ctx.samplerState.GetAddressOf());
		ctx.context->Draw(3, 0);
	}

	// Pass 4: Merge into SwapChain
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		ctx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		ComPtr<ID3D11RenderTargetView> rtv;
		ctx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv);

		ctx.context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
		float clear[4] = { 0, 0, 0, 0 };
		ctx.context->ClearRenderTargetView(rtv.Get(), clear);

		ctx.context->PSSetShader(ctx.psComposite.Get(), nullptr, 0);
		ctx.context->PSSetConstantBuffers(0, 1, ctx.constantBuffer.GetAddressOf());

		ID3D11ShaderResourceView* nullSRV = nullptr;
		ctx.context->PSSetShaderResources(0, 1, &nullSRV);

		ctx.context->PSSetShaderResources(0, 1, ctx.rtBlurV.srv.GetAddressOf());
		ctx.context->PSSetSamplers(0, 1, ctx.samplerState.GetAddressOf());
		ctx.context->Draw(3, 0);
	}

	ctx.swapChain->Present(1, 0);

	// FPS
	ctx.frameCount++;
	auto elapsed = std::chrono::duration<float>(now - ctx.lastFpsTime).count();
	if (elapsed >= 1.0f) {
		float fps = ctx.frameCount / elapsed;
		wchar_t title[256];
		swprintf_s(title, L"Multi-Pass Blur - FPS: %.1f | whFactor: %.1f", fps, ctx.whFactor);
		SetWindowTextW(hwnd, title);
		ctx.frameCount = 0;
		ctx.lastFpsTime = now;
	}
}

// ========== Cleanup ==========
void Cleanup(D3D11Context& ctx)
{
	ctx.rtOriginal.srv.Reset(); ctx.rtOriginal.rtv.Reset(); ctx.rtOriginal.texture.Reset();
	ctx.rtBlurH.srv.Reset(); ctx.rtBlurH.rtv.Reset(); ctx.rtBlurH.texture.Reset();
	ctx.rtBlurV.srv.Reset(); ctx.rtBlurV.rtv.Reset(); ctx.rtBlurV.texture.Reset();

	ctx.samplerState.Reset();
	ctx.blendState.Reset();
	ctx.constantBuffer.Reset();
	ctx.psComposite.Reset();
	ctx.psBlurV.Reset();
	ctx.psBlurH.Reset();
	ctx.psOriginal.Reset();
	ctx.vertexShader.Reset();

	ctx.dcompVisual.Reset();
	ctx.dcompTarget.Reset();
	ctx.dcompDevice.Reset();
	ctx.swapChain.Reset();
	ctx.context.Reset();
	ctx.device.Reset();
}

// ========== Window Procedure ==========
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) {
		auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	auto* ctx = reinterpret_cast<D3D11Context*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

	switch (msg) {
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			DestroyWindow(hwnd);
		}
		else if (ctx) {
			if (wParam >= '1' && wParam <= '9') {
				ctx->whFactor = static_cast<float>(wParam - '0');
			}
			else if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
				ctx->whFactor += 0.5f;
			}
			else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
				ctx->whFactor = max(1.0f, ctx->whFactor - 0.5f);
			}
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ========== Main ==========
int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	(void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	const wchar_t* className = L"D3D11MultiPassBlur";
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = className;

	if (!RegisterClassExW(&wc)) {
		CoUninitialize();
		return -1;
	}

	D3D11Context ctx;
	ctx.startTime = std::chrono::steady_clock::now();
	ctx.lastFpsTime = ctx.startTime;

	DWORD exStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP;

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	HWND hwnd = CreateWindowExW(
		exStyle, className, L"Multi-Pass Blur",
		WS_POPUP, 0, 0, screenWidth, screenHeight,
		nullptr, nullptr, hInstance, &ctx
	);

	if (!hwnd) {
		CoUninitialize();
		return -1;
	}

	ShowWindow(hwnd, SW_SHOW);
	SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

	if (!Initialize(hwnd, ctx)) {
		MessageBoxW(nullptr, L"Failed to initialize", L"Error", MB_OK);
		Cleanup(ctx);
		DestroyWindow(hwnd);
		CoUninitialize();
		return -1;
	}

	MSG msg = {};
	while (true) {
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) goto cleanup;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		Render(ctx, hwnd);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

cleanup:
	Cleanup(ctx);
	DestroyWindow(hwnd);
	CoUninitialize();
	return static_cast<int>(msg.wParam);
}
