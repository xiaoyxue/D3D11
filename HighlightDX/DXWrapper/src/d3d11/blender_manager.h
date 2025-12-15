#pragma once

#include "common/core.h"

namespace D3D11 {
  class BlenderManager {
  public:
    BlenderManager() = default;
    ~BlenderManager() = default;
    bool CreateBlendStates(ID3D11Device* device);
    Microsoft::WRL::ComPtr<ID3D11BlendState> GetBlender();
  private:
    Microsoft::WRL::ComPtr<ID3D11BlendState> blender_;
  };
}
