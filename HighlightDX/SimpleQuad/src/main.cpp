//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0A00
//#endif
//
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#include <d3d11.h>
//#include <d3dcompiler.h>
//#include <dcomp.h>
//#include <dxgi1_3.h>
//#include <wrl/client.h>
//
//#pragma comment(lib, "d3d11.lib")
//#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "dcomp.lib")
//#pragma comment(lib, "dxgi.lib")
//
//using Microsoft::WRL::ComPtr;
//
//// 全局变量
//ComPtr<ID3D11Device> g_device;
//ComPtr<ID3D11DeviceContext> g_context;
//ComPtr<IDXGISwapChain1> g_swapChain;
//ComPtr<IDCompositionDevice> g_dcompDevice;
//ComPtr<IDCompositionTarget> g_dcompTarget;
//ComPtr<IDCompositionVisual> g_dcompVisual;
//
//ComPtr<ID3D11Buffer> g_vertexBuffer;
//ComPtr<ID3D11Buffer> g_indexBuffer;  // 索引缓冲区
//ComPtr<ID3D11VertexShader> g_vertexShader;
//ComPtr<ID3D11PixelShader> g_pixelShader;
//ComPtr<ID3D11InputLayout> g_inputLayout;
//ComPtr<ID3D11BlendState> g_blendState;
//
//UINT g_width = 0;
//UINT g_height = 0;
//
//// 顶点结构
//struct Vertex {
//	float x, y, z;
//	float r, g, b, a;
//};
//
//// Vertex Shader
//const char* vsCode = R"(
//struct VS_INPUT {
//    float3 pos : POSITION;
//    float4 color : COLOR;
//};
//
//struct VS_OUTPUT {
//    float4 pos : SV_POSITION;
//    float4 color : COLOR;
//};
//
//VS_OUTPUT main(VS_INPUT input) {
//    VS_OUTPUT output;
//    output.pos = float4(input.pos, 1.0f);
//    output.color = input.color;
//    return output;
//}
//)";
//
//// Pixel Shader
//const char* psCode = R"(
//struct PS_INPUT {
//    float4 pos : SV_POSITION;
//    float4 color : COLOR;
//};
//
//float4 main(PS_INPUT input) : SV_TARGET {
//    // 预乘 alpha
//    float4 color = input.color;
//    color.rgb *= color.a;
//    return color;
//}
//)";
//
//bool Initialize(HWND hwnd) {
//	// 创建 D3D11 设备
//	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
//	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
//
//	HRESULT hr = D3D11CreateDevice(
//		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
//		levels, 1, D3D11_SDK_VERSION,
//		&g_device, nullptr, &g_context
//	);
//	if (FAILED(hr)) return false;
//
//	// 获取 DXGI
//	ComPtr<IDXGIDevice> dxgiDevice;
//	g_device.As(&dxgiDevice);
//	ComPtr<IDXGIAdapter> adapter;
//	dxgiDevice->GetAdapter(&adapter);
//	ComPtr<IDXGIFactory2> factory;
//	adapter->GetParent(IID_PPV_ARGS(&factory));
//
//	// 获取窗口大小
//	RECT rect;
//	GetClientRect(hwnd, &rect);
//	g_width = rect.right;
//	g_height = rect.bottom;
//
//	// 创建交换链
//	DXGI_SWAP_CHAIN_DESC1 desc = {};
//	desc.Width = g_width;
//	desc.Height = g_height;
//	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
//	desc.SampleDesc.Count = 1;
//	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	desc.BufferCount = 2;
//	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
//	desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
//
//	hr = factory->CreateSwapChainForComposition(g_device.Get(), &desc, nullptr, &g_swapChain);
//	if (FAILED(hr)) return false;
//
//	// 编译 Vertex Shader
//	ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
//
//	hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr,
//		"main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
//	if (FAILED(hr)) {
//		if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
//		return false;
//	}
//	g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vertexShader);
//
//	// 编译 Pixel Shader
//	hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
//		"main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
//	if (FAILED(hr)) {
//		if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
//		return false;
//	}
//	g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pixelShader);
//
//	// 创建输入布局
//	D3D11_INPUT_ELEMENT_DESC layout[] = {
//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
//	};
//	g_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_inputLayout);
//
//	// 创建顶点缓冲区 - 4 个顶点组成一个 Quad
//	Vertex vertices[] = {
//		{ -0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 0.8f }, 
//		{  0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 0.8f },  
//		{  0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 0.8f },  
//		{ -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 0.0f, 0.8f },  
//		{ -0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 0.8f },
//		{  0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 0.8f },
//		{ -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 0.0f, 0.8f }
//	};
//
//	D3D11_BUFFER_DESC vbDesc = {};
//	vbDesc.ByteWidth = sizeof(vertices);
//	vbDesc.Usage = D3D11_USAGE_DEFAULT;
//	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//
//	D3D11_SUBRESOURCE_DATA vbData = {};
//	vbData.pSysMem = vertices;
//
//	g_device->CreateBuffer(&vbDesc, &vbData, &g_vertexBuffer);
//
//	// 创建索引缓冲区 - 两个三角形
//	UINT indices[] = {
//		0, 1, 2,  
//		3, 4, 5   
//	};
//
//	D3D11_BUFFER_DESC ibDesc = {};
//	ibDesc.ByteWidth = sizeof(indices);
//	ibDesc.Usage = D3D11_USAGE_DEFAULT;
//	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
//
//	D3D11_SUBRESOURCE_DATA ibData = {};
//	ibData.pSysMem = indices;
//
//	g_device->CreateBuffer(&ibDesc, &ibData, &g_indexBuffer);
//
//	// 创建混合状态
//	D3D11_BLEND_DESC blendDesc = {};
//	blendDesc.RenderTarget[0].BlendEnable = TRUE;
//	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
//	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
//	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//	g_device->CreateBlendState(&blendDesc, &g_blendState);
//
//	// DirectComposition
//	DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(&g_dcompDevice));
//	g_dcompDevice->CreateTargetForHwnd(hwnd, TRUE, &g_dcompTarget);
//	g_dcompDevice->CreateVisual(&g_dcompVisual);
//	g_dcompVisual->SetContent(g_swapChain.Get());
//	g_dcompTarget->SetRoot(g_dcompVisual.Get());
//	g_dcompDevice->Commit();
//
//	return true;
//}
//
//void Render() {
//	// 获取后台缓冲区
//	ComPtr<ID3D11Texture2D> backBuffer;
//	g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
//	ComPtr<ID3D11RenderTargetView> rtv;
//	g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv);
//
//	// 清除为透明
//	float clearColor[4] = { 0, 0, 0, 0 };
//	g_context->ClearRenderTargetView(rtv.Get(), clearColor);
//
//	// 设置渲染目标
//	g_context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
//
//	// 设置视口
//	D3D11_VIEWPORT viewport = { 0, 0, (float)g_width, (float)g_height, 0.0f, 1.0f };
//	g_context->RSSetViewports(1, &viewport);
//
//	// 设置混合状态
//	float blendFactor[4] = { 0, 0, 0, 0 };
//	g_context->OMSetBlendState(g_blendState.Get(), blendFactor, 0xFFFFFFFF);
//
//	// 设置输入布局
//	g_context->IASetInputLayout(g_inputLayout.Get());
//
//	// 设置顶点缓冲区
//	UINT stride = sizeof(Vertex);
//	UINT offset = 0;
//	g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);
//
//	// 设置索引缓冲区
//	g_context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
//
//	// 设置图元类型
//	g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//	// 设置着色器
//	g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
//	g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);
//
//	// 使用 DrawIndexed 绘制 Quad（6 个索引 = 2 个三角形）
//	g_context->DrawIndexed(6, 0, 0);
//
//	// 呈现
//	g_swapChain->Present(1, 0);
//}
//
//void Cleanup() {
//	g_indexBuffer.Reset();
//	g_blendState.Reset();
//	g_inputLayout.Reset();
//	g_pixelShader.Reset();
//	g_vertexShader.Reset();
//	g_vertexBuffer.Reset();
//	g_dcompVisual.Reset();
//	g_dcompTarget.Reset();
//	g_dcompDevice.Reset();
//	g_swapChain.Reset();
//	g_context.Reset();
//	g_device.Reset();
//}
//
//LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//	if (msg == WM_DESTROY || (msg == WM_KEYDOWN && wParam == VK_ESCAPE)) {
//		PostQuitMessage(0);
//		return 0;
//	}
//	return DefWindowProc(hwnd, msg, wParam, lParam);
//}
//
//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
//	// 注册窗口类
//	WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
//	wc.lpfnWndProc = WndProc;
//	wc.hInstance = hInstance;
//	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
//	wc.lpszClassName = L"QuadWindow";
//	RegisterClassExW(&wc);
//
//	// 创建全屏透明窗口
//	int w = GetSystemMetrics(SM_CXSCREEN);
//	int h = GetSystemMetrics(SM_CYSCREEN);
//
//	HWND hwnd = CreateWindowExW(
//		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP,
//		L"QuadWindow", L"Hello Quad",
//		WS_POPUP,
//		0, 0, w, h,
//		nullptr, nullptr, hInstance, nullptr
//	);
//
//	if (!hwnd) return -1;
//
//	ShowWindow(hwnd, SW_SHOW);
//
//	if (!Initialize(hwnd)) {
//		MessageBoxW(nullptr, L"Failed to initialize", L"Error", MB_OK);
//		return -1;
//	}
//
//	// 消息循环
//	MSG msg = {};
//	while (true) {
//		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
//			if (msg.message == WM_QUIT) break;
//			TranslateMessage(&msg);
//			DispatchMessage(&msg);
//		}
//		Render();
//	}
//
//	Cleanup();
//	return 0;
//}



#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dcomp.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <wincodec.h>  // WIC 加载图片
#include <DirectXMath.h>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;
using Matrix = DirectX::XMMATRIX;

// 全局变量
ComPtr<ID3D11Device> g_device;
ComPtr<ID3D11DeviceContext> g_context;
ComPtr<IDXGISwapChain1> g_swapChain;
ComPtr<IDCompositionDevice> g_dcompDevice;
ComPtr<IDCompositionTarget> g_dcompTarget;
ComPtr<IDCompositionVisual> g_dcompVisual;

ComPtr<ID3D11Buffer> g_vertexBuffer;
ComPtr<ID3D11Buffer> g_indexBuffer;
ComPtr<ID3D11VertexShader> g_vertexShader;
ComPtr<ID3D11PixelShader> g_pixelShader;
ComPtr<ID3D11InputLayout> g_inputLayout;
ComPtr<ID3D11BlendState> g_blendState;

ComPtr<ID3D11Texture2D> g_texture;
ComPtr<ID3D11ShaderResourceView> g_textureSRV;
ComPtr<ID3D11SamplerState> g_samplerState;

ComPtr<ID3D11Buffer> g_constantBuffer;

Matrix g_LocalToWorld;
Matrix g_rotate;
Matrix g_scale;
Matrix g_translate;
Matrix g_projection;
Matrix g_initialPositionTransform;

int g_initialPosition[2];

int g_width = 0;
int g_height = 0;

float g_iTime = 0.0f;

std::chrono::steady_clock::time_point g_StartTime;

constexpr float MaxAnimationTime = 2.f; // seconds

// 顶点结构 - 加上纹理坐标
struct Vertex {
	float x, y, z;
	float u, v;  // 纹理坐标
};

struct ConstantBuffer
{
	DirectX::XMMATRIX mvp;
	float iTime;
	int radius;

	ConstantBuffer()
		: mvp(DirectX::XMMatrixIdentity()), iTime(0.0f), radius(50){ }
};

// Vertex Shader
const char* vsCode = R"(
cbuffer TransformBuffer :  register(b0)
{
    matrix mvp;
	int2 position;
	float iTime;
	int radius;
};

struct VS_INPUT {
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, mvp);
	output.pos = pos;
    output.uv = input.uv;
    return output;
}
)";

// Pixel Shader - 采样纹理
const char* psCode = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float4 color = tex.Sample(samp, input.uv);
    // 预乘 alpha
    color.rgb *= color.a;
    return color;
}
)";

// 加载 PNG 纹理
bool LoadTexture(const wchar_t* filename) {

	OutputDebugStringA("=== Loading texture with Mipmaps + Anisotropic filtering ===\n");

	// ===== 1. 初始化 WIC =====
	ComPtr<IWICImagingFactory> wicFactory;
	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wicFactory)
	);
	if (FAILED(hr)) {
		OutputDebugStringA("❌ Failed to create WIC factory\n");
		return false;
	}

	// ===== 2. 创建解码器 =====
	ComPtr<IWICBitmapDecoder> decoder;
	hr = wicFactory->CreateDecoderFromFilename(
		filename,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnDemand,
		&decoder
	);
	if (FAILED(hr)) {
		OutputDebugStringA("❌ Failed to load image file\n");
		return false;
	}

	// ===== 3. 获取第一帧 =====
	ComPtr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) {
		OutputDebugStringA("❌ Failed to get frame\n");
		return false;
	}

	// ===== 4. 转换为 RGBA 格式 =====
	ComPtr<IWICFormatConverter> converter;
	hr = wicFactory->CreateFormatConverter(&converter);
	if (FAILED(hr)) {
		OutputDebugStringA("❌ Failed to create format converter\n");
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
		OutputDebugStringA("❌ Failed to initialize converter\n");
		return false;
	}

	// ===== 5. 获取图像尺寸 =====
	UINT texWidth, texHeight;
	converter->GetSize(&texWidth, &texHeight);

	char buf[256];
	sprintf_s(buf, "✅ Texture size: %dx%d\n", texWidth, texHeight);
	OutputDebugStringA(buf);

	// ===== 6. 读取像素数据 =====
	UINT stride = texWidth * 4;  // RGBA = 4 bytes per pixel
	UINT imageSize = stride * texHeight;
	std::vector<BYTE> pixels(imageSize);

	hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.data());
	if (FAILED(hr)) {
		OutputDebugStringA("❌ Failed to copy pixels\n");
		return false;
	}

	// ===== 7. 计算 Mipmap 级数（可选，用于调试）=====
	UINT mipLevels = 0;
	UINT width = texWidth;
	UINT height = texHeight;
	while (width > 1 || height > 1) {
		width = max(1, width / 2);
		height = max(1, height / 2);
		mipLevels++;
	}
	mipLevels++;  // 包括 level 0

	sprintf_s(buf, "📊 Calculated mip levels: %d\n", mipLevels);
	OutputDebugStringA(buf);

	// ===== 8. 创建纹理（支持 Mipmap 自动生成）=====
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = texWidth;
	texDesc.Height = texHeight;
	texDesc.MipLevels = 0;  // ✅ 0 = 自动计算完整 mipmap 链
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;

	// ✅ 必须包含 RENDER_TARGET 以支持 GenerateMips
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	// ✅ 允许自动生成 mipmap
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	texDesc.CPUAccessFlags = 0;

	// 先创建空纹理（不传初始数据）
	hr = g_device->CreateTexture2D(&texDesc, nullptr, &g_texture);
	if (FAILED(hr)) {
		sprintf_s(buf, "❌ Failed to create texture (HRESULT: 0x%08X)\n", hr);
		OutputDebugStringA(buf);
		return false;
	}
	OutputDebugStringA("✅ Texture created\n");

	// ===== 9. 上传第 0 级 Mipmap 数据 =====
	g_context->UpdateSubresource(
		g_texture.Get(),
		0,              // Subresource index (mip level 0)
		nullptr,        // 整个区域
		pixels.data(),  // 源数据
		stride,         // 行间距
		0               // 深度间距（2D 纹理不需要）
	);
	OutputDebugStringA("✅ Level 0 data uploaded\n");

	// ===== 10. 创建 Shader Resource View =====
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;      // 从 level 0 开始
	srvDesc.Texture2D.MipLevels = -1;           // ✅ -1 = 使用所有可用的 mipmap 级别

	hr = g_device->CreateShaderResourceView(g_texture.Get(), &srvDesc, &g_textureSRV);
	if (FAILED(hr)) {
		sprintf_s(buf, "❌ Failed to create SRV (HRESULT: 0x%08X)\n", hr);
		OutputDebugStringA(buf);
		return false;
	}
	OutputDebugStringA("✅ SRV created\n");

	// ===== 11. 自动生成 Mipmap =====
	g_context->GenerateMips(g_textureSRV.Get());
	OutputDebugStringA("✅ Mipmaps generated\n");

	// ===== 12. 验证创建的 Mipmap 级数 =====
	D3D11_TEXTURE2D_DESC actualDesc;
	g_texture->GetDesc(&actualDesc);

	sprintf_s(buf, "📊 Actual texture:  %dx%d, Mip levels: %d\n",
		actualDesc.Width, actualDesc.Height, actualDesc.MipLevels);
	OutputDebugStringA(buf);

	if (actualDesc.MipLevels <= 1) {
		OutputDebugStringA("⚠️ Warning: Mipmaps not generated properly!\n");
	}
	else {
		sprintf_s(buf, "✅ Successfully created %d mipmap levels\n", actualDesc.MipLevels);
		OutputDebugStringA(buf);
	}

	OutputDebugStringA("=== Texture loading complete ===\n");
	return true;
}

// ===== 实时更新函数 =====
void UpdateConstantBuffer(const DirectX::XMMATRIX& mvp, float time) {
	// 1. 映射缓冲区（获取 CPU 可写指针）
	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = g_context->Map(
		g_constantBuffer.Get(),      // 要映射的资源
		0,                            // 子资源索引（常量缓冲区总是0）
		D3D11_MAP_WRITE_DISCARD,     // ✅ 丢弃旧数据（最快）
		0,                            // 标志（通常是0）
		&mapped                       // 输出映射的内存指针
	);

	if (FAILED(hr)) {
		OutputDebugStringA("Failed to map constant buffer\n");
		return;
	}

	// 2. 写入新数据
	ConstantBuffer* cb = (ConstantBuffer*)mapped.pData;
	cb->mvp = DirectX::XMMatrixTranspose(mvp);  // DirectX 需要转置
	cb->iTime = time;

	// 3. 解除映射（提交数据到 GPU）
	g_context->Unmap(g_constantBuffer.Get(), 0);
}

bool Initialize(HWND hwnd) {
	// 创建 D3D11 设备
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

	HRESULT hr = D3D11CreateDevice(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
		levels, 1, D3D11_SDK_VERSION,
		&g_device, nullptr, &g_context
	);
	if (FAILED(hr)) return false;

	// 获取 DXGI
	ComPtr<IDXGIDevice> dxgiDevice;
	g_device.As(&dxgiDevice);
	ComPtr<IDXGIAdapter> adapter;
	dxgiDevice->GetAdapter(&adapter);
	ComPtr<IDXGIFactory2> factory;
	adapter->GetParent(IID_PPV_ARGS(&factory));

	// 获取窗口大小
	RECT rect;
	GetClientRect(hwnd, &rect);
	g_width = rect.right;
	g_height = rect.bottom;

	// 创建交换链
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = g_width;
	desc.Height = g_height;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 2;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

	hr = factory->CreateSwapChainForComposition(g_device.Get(), &desc, nullptr, &g_swapChain);
	if (FAILED(hr)) return false;

	// 编译 Vertex Shader
	ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

	hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr,
		"main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		return false;
	}
	g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vertexShader);

	// 编译 Pixel Shader
	hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
		"main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		return false;
	}
	g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pixelShader);

	// 创建输入布局
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	g_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_inputLayout);

	// 创建顶点缓冲区 - 4 个顶点，带纹理坐标
	Vertex vertices[] = {
		{ -0.337f,  0.355f, 0.0f,  0.0f, 0.0f },  // 左上
		{  0.337f,  0.355f, 0.0f,  1.0f, 0.0f },  // 右上
		{  0.337f, -0.355f, 0.0f,  1.0f, 1.0f },  // 右下
		{ -0.337f, -0.355f, 0.0f,  0.0f, 1.0f }   // 左下
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(vertices);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices;

	g_device->CreateBuffer(&vbDesc, &vbData, &g_vertexBuffer);

	// 创建索引缓冲区
	UINT indices[] = {
		0, 1, 2,
		0, 2, 3
	};

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = sizeof(indices);
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	g_device->CreateBuffer(&ibDesc, &ibData, &g_indexBuffer);

	// 创建常量缓冲区
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.ByteWidth = sizeof(ConstantBuffer);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_device->CreateBuffer(&cbDesc, nullptr, &g_constantBuffer);

	g_scale = DirectX::XMMatrixScaling(200.0f, 200.0f, 1.0f);
	g_rotate = DirectX::XMMatrixRotationZ(45.0f / 180.f * DirectX::XM_PI);
	auto LocalTransform = g_scale * g_rotate;
	auto LocalToWorld = LocalTransform * DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	g_LocalToWorld = LocalToWorld;
	auto WorldToLocal = DirectX::XMMatrixInverse(nullptr, LocalToWorld);

	g_projection = DirectX::XMMatrixOrthographicOffCenterLH(
		-(float)g_width / 2.f, (float)g_width / 2.f, -(float)g_height / 2.f, (float)g_height / 2.f, 0.0f, 1.0f
	);

	//float initialPosition[2];
	//initialPosition[0] = 100 - g_width / 2.0f;
	//initialPosition[1] = -100 + g_height / 2.0f;
	g_initialPosition[0] = 100.f;
	g_initialPosition[1] = 100.f;
	g_initialPositionTransform =
		DirectX::XMMatrixTranslation(-1 * g_width / 2.f,  g_height / 2.f, 0.0f) *
		DirectX::XMMatrixTranslation(g_initialPosition[0], -1 * g_initialPosition[1], 0.0f);


	Matrix mvp = g_LocalToWorld * g_projection;

	// 更新常量缓冲区
	D3D11_MAPPED_SUBRESOURCE mapped;
	g_context->Map(g_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	ConstantBuffer* buffer = (ConstantBuffer*)mapped.pData;
	buffer->mvp = XMMatrixTranspose(mvp);
	buffer->iTime = g_iTime;
	g_context->Unmap(g_constantBuffer.Get(), 0);

	// 加载纹理
	if (!LoadTexture(L"D:\\Code\\Work\\D3D11\\HighlightDX\\SimpleQuad\\Asset\\OuterCursor.png")) {
		OutputDebugStringA("Failed to load texture\n");
		return false;
	}

	// ===== 创建各向异性采样器 =====
	D3D11_SAMPLER_DESC samplerDesc = {};

	// ✅ 各向异性过滤（最高质量）
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;

	// 纹理寻址模式
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

	// ✅ 各向异性级别（1-16，越高越好但越慢）
	samplerDesc.MaxAnisotropy = 16;  // 推荐：4, 8, 或 16

	// Mipmap LOD 设置
	samplerDesc.MinLOD = 0.0f;                    // 最详细的级别
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;       // 允许使用所有级别
	samplerDesc.MipLODBias = 0.0f;                // LOD 偏移（0 = 自动选择）

	// 比较函数（深度纹理用，这里不需要）
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	// 边界颜色（BORDER 模式才用）
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;

	hr = g_device->CreateSamplerState(&samplerDesc, &g_samplerState);
	if (FAILED(hr)) {
		OutputDebugStringA("❌ Failed to create sampler state\n");
		return false;
	}
	OutputDebugStringA("✅ Anisotropic sampler created (16x)\n");

	// 创建混合状态
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	g_device->CreateBlendState(&blendDesc, &g_blendState);

	// DirectComposition
	DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(&g_dcompDevice));
	g_dcompDevice->CreateTargetForHwnd(hwnd, TRUE, &g_dcompTarget);
	g_dcompDevice->CreateVisual(&g_dcompVisual);
	g_dcompVisual->SetContent(g_swapChain.Get());
	g_dcompTarget->SetRoot(g_dcompVisual.Get());
	g_dcompDevice->Commit();

	g_StartTime = std::chrono::steady_clock::now();
	return true;
}

void Render() {
	// 获取后台缓冲区
	ComPtr<ID3D11Texture2D> backBuffer;
	g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	ComPtr<ID3D11RenderTargetView> rtv;
	g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv);

	// 清除为透明
	float clearColor[4] = { 0, 0, 0, 0 };
	g_context->ClearRenderTargetView(rtv.Get(), clearColor);

	// 设置渲染目标
	g_context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

	// 设置视口
	D3D11_VIEWPORT viewport = { 0, 0, (float)g_width, (float)g_height, 0.0f, 1.0f };
	g_context->RSSetViewports(1, &viewport);

	// 设置混合状态
	float blendFactor[4] = { 0, 0, 0, 0 };
	g_context->OMSetBlendState(g_blendState.Get(), blendFactor, 0xFFFFFFFF);

	// 设置输入布局
	g_context->IASetInputLayout(g_inputLayout.Get());

	// 设置顶点缓冲区
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);

	// 设置索引缓冲区
	g_context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// 设置图元类型
	g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 设置着色器
	g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
	g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);

	// 绑定纹理和采样器
	g_context->PSSetShaderResources(0, 1, g_textureSRV.GetAddressOf());
	g_context->PSSetSamplers(0, 1, g_samplerState.GetAddressOf());


	g_context->VSSetConstantBuffers(0, 1, g_constantBuffer.GetAddressOf());
	

	auto currentTime = std::chrono::steady_clock::now();
	g_iTime = std::chrono::duration<float>(currentTime - g_StartTime).count();
	auto animationRatio = min(g_iTime / MaxAnimationTime, 1.0f);

	auto translateMatrix =  DirectX::XMMatrixTranslation(
		200.f * animationRatio,
		-200.f * animationRatio,
		0.0f
	);

	

	UpdateConstantBuffer(g_LocalToWorld * g_initialPositionTransform * translateMatrix * g_projection, g_iTime);


	g_context->DrawIndexed(6, 0, 0);


	g_swapChain->Present(1, 0);
}

void Cleanup() {
	g_samplerState.Reset();
	g_textureSRV.Reset();
	g_texture.Reset();
	g_indexBuffer.Reset();
	g_blendState.Reset();
	g_inputLayout.Reset();
	g_pixelShader.Reset();
	g_vertexShader.Reset();
	g_vertexBuffer.Reset();
	g_dcompVisual.Reset();
	g_dcompTarget.Reset();
	g_dcompDevice.Reset();
	g_swapChain.Reset();
	g_constantBuffer.Reset();
	g_context.Reset();
	g_device.Reset();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY || (msg == WM_KEYDOWN && wParam == VK_ESCAPE)) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
	CoInitialize(nullptr);

	// 注册窗口类
	WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = L"QuadWindow";
	RegisterClassExW(&wc);

	// 创建全屏透明窗口
	int w = GetSystemMetrics(SM_CXSCREEN);
	int h = GetSystemMetrics(SM_CYSCREEN);

	HWND hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP,
		L"QuadWindow", L"Textured Quad",
		WS_POPUP,
		0, 0, w, h,
		nullptr, nullptr, hInstance, nullptr
	);

	if (!hwnd) {
		CoUninitialize();
		return -1;
	}

	ShowWindow(hwnd, SW_SHOW);

	if (!Initialize(hwnd)) {
		MessageBoxW(nullptr, L"Failed to initialize", L"Error", MB_OK);
		CoUninitialize();
		return -1;
	}

	// 消息循环
	MSG msg = {};
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Render();
	}

	Cleanup();
	CoUninitialize();
	return 0;
}