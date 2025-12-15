#pragma once
#include "common/core.h"

namespace D3D11 {
  enum class SamplerType {
    LinearClamp,
    Anisotropic
  };

  class SamplerManager {
  public:
    SamplerManager() = default;
    bool CreateSamplerStates(ID3D11Device* device);
    Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSampler(SamplerType type);
  private:
    std::unordered_map<SamplerType, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers_;
  };
}
