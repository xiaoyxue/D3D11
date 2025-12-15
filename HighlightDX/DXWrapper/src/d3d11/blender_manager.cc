#include "blender_manager.h"

using Microsoft::WRL::ComPtr;

namespace D3D11 {

  bool BlenderManager::CreateBlendStates(ID3D11Device* device)
  {
    D3D11_BLEND_DESC blendDesc = {};
    ComPtr<ID3D11BlendState> blend_state;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D11_COLOR_WRITE_ENABLE_ALL;

    if (FAILED(device->CreateBlendState(&blendDesc, &blend_state))) {
      return false;
    }
    blender_ = blend_state;
    return true;
  }

  Microsoft::WRL::ComPtr<ID3D11BlendState> BlenderManager::GetBlender()
  {
    if (blender_ != nullptr) {
      return blender_;
    }
  }

}
