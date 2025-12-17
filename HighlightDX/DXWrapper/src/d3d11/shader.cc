#include "shader.h"
#include <cstring>
#include <sstream>

using Microsoft::WRL::ComPtr;
namespace D3D11 {
static bool CompileShaderInternal(const char *source, const char *target,
                                  ID3DBlob **outBlob, const char* shaderType) {
  ComPtr<ID3DBlob> errorBlob;
  HRESULT hr = D3DCompile(
      source, std::strlen(source), nullptr, nullptr, nullptr, "main", target,
      D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, outBlob,
      &errorBlob);

  if (FAILED(hr)) {
    if (errorBlob) {
      // Output to debug console
      const char* errorMsg = static_cast<char*>(errorBlob->GetBufferPointer());
      
      // Format error message using ostringstream
      // Note: Can be replaced with std::format in C++20:
      //   std::string errorStr = std::format(
      //     "Failed to compile {} (Target: {})\n\nError Details:\n{}",
      //     shaderType, target, errorMsg
      //   );
      std::ostringstream oss;
      oss << "Failed to compile " << shaderType << " (Target: " << target << ")\n\n";
      oss << "Error Details:\n";
      oss << errorMsg;
      std::string errorStr = oss.str();
      
      // Convert to wide string for MessageBoxW
      int size = MultiByteToWideChar(CP_UTF8, 0, errorStr.c_str(), -1, nullptr, 0);
      std::wstring wideError(size, 0);
      MultiByteToWideChar(CP_UTF8, 0, errorStr.c_str(), -1, &wideError[0], size);
      
      MessageBoxW(nullptr, wideError.c_str(), L"Shader Compilation Error", 
                  MB_OK | MB_ICONERROR | MB_TOPMOST);
    }
    return false;
  }
  return true;
}

bool VertexShader::CompileFromSource(ID3D11Device *device, const char *source) {
  if (!CompileShaderInternal(source, "vs_5_0", &blob_, "Vertex Shader")) {
    return false;
  }

  return SUCCEEDED(device->CreateVertexShader(
      blob_->GetBufferPointer(), blob_->GetBufferSize(), nullptr, &shader_));
}

bool PixelShader::CompileFromSource(ID3D11Device *device, const char *source) {
  ComPtr<ID3DBlob> blob;
  if (!CompileShaderInternal(source, "ps_5_0", &blob, "Pixel Shader")) {
    return false;
  }

  return SUCCEEDED(device->CreatePixelShader(
      blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader_));
}
} // namespace D3D11

