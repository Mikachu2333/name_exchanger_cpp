#pragma once

#include <d3d11.h>
#include <windows.h>

// D3D11 global state
struct D3DState {
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *deviceContext = nullptr;
    IDXGISwapChain *swapChain = nullptr;
    ID3D11RenderTargetView *renderTargetView = nullptr;
    bool swapChainOccluded = false;
    UINT resizeWidth = 0;
    UINT resizeHeight = 0;
};

// Create D3D11 device and swap chain
bool CreateDeviceD3D(HWND hWnd, D3DState &state);

// Release all D3D resources
void CleanupDeviceD3D(D3DState &state);

// Create the render target view from the swap chain back buffer
void CreateRenderTarget(D3DState &state);

// Release the render target view
void CleanupRenderTarget(D3DState &state);
