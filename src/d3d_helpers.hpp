#pragma once

#include <d3d11.h>
#include <windows.h>

struct D3DState {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
    bool swapChainOccluded = false;
    UINT resizeWidth = 0;
    UINT resizeHeight = 0;
};

[[nodiscard]] bool CreateDeviceD3D(HWND hWnd, D3DState& state);
void CleanupDeviceD3D(D3DState& state) noexcept;
[[nodiscard]] bool CreateRenderTarget(D3DState& state);
void CleanupRenderTarget(D3DState& state) noexcept;
