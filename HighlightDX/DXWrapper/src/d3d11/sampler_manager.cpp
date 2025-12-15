#include "sampler_manager.h"

using Microsoft::WRL::ComPtr;

namespace D3D11 {
  
  bool SamplerManager::CreateSamplerStates(ID3D11Device* device)
  {
    {
      D3D11_SAMPLER_DESC samplerDesc = {};
      ComPtr<ID3D11SamplerState> sampler_state;
      samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
      if (FAILED(device->CreateSamplerState(&samplerDesc, &sampler_state))) {
        return false;
      }
      samplers_[SamplerType::LinearClamp] = sampler_state;
    }
    {
      D3D11_SAMPLER_DESC samplerDesc = {};
      ComPtr<ID3D11SamplerState> sampler_state;
      samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
      samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
      samplerDesc.MaxAnisotropy = 16;
      samplerDesc.MinLOD = 0.0f;
      samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
      samplerDesc.MipLODBias = 0.0f;

      samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

      samplerDesc.BorderColor[1] = 0.0f;
      samplerDesc.BorderColor[2] = 0.0f;
      samplerDesc.BorderColor[3] = 0.0f;

      if (FAILED(device->CreateSamplerState(&samplerDesc, &sampler_state))) {
        return false;
      }
      samplers_[SamplerType::Anisotropic] = sampler_state;
    }
  }

  Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerManager::GetSampler(SamplerType type)
  {
    if (samplers_.find(type) != samplers_.end()) {
      return samplers_[type];
    }
  }

}
