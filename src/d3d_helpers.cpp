#include "d3d_helpers.h"

bool CreateDeviceD3D(HWND hWnd, D3DState &state) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel{};
    constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};

    HRESULT res =
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray,
                                      2, D3D11_SDK_VERSION, &sd, &state.swapChain, &state.device, &featureLevel,
                                      &state.deviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) {
        // Try WARP software driver if hardware is not available
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
                                            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &state.swapChain,
                                            &state.device, &featureLevel, &state.deviceContext);
    }
    if (res != S_OK) {
        return false;
    }

    CreateRenderTarget(state);
    return true;
}

void CleanupDeviceD3D(D3DState &state) {
    CleanupRenderTarget(state);
    if (state.swapChain) {
        state.swapChain->Release();
        state.swapChain = nullptr;
    }
    if (state.deviceContext) {
        state.deviceContext->Release();
        state.deviceContext = nullptr;
    }
    if (state.device) {
        state.device->Release();
        state.device = nullptr;
    }
}

void CreateRenderTarget(D3DState &state) {
    ID3D11Texture2D *backBuffer = nullptr;
    state.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) {
        state.device->CreateRenderTargetView(backBuffer, nullptr, &state.renderTargetView);
        backBuffer->Release();
    }
}

void CleanupRenderTarget(D3DState &state) {
    if (state.renderTargetView) {
        state.renderTargetView->Release();
        state.renderTargetView = nullptr;
    }
}
