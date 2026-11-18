#include "d3d_helpers.hpp"

#include <iterator>

bool CreateDeviceD3D(HWND hWnd, D3DState& state) {
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferCount = 2;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.OutputWindow = hWnd;
    description.SampleDesc.Count = 1;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel{};
    constexpr D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION,
        &description, &state.swapChain, &state.device, &featureLevel, &state.deviceContext);
    if (result == DXGI_ERROR_UNSUPPORTED) {
        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels,
                                               static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION, &description,
                                               &state.swapChain, &state.device, &featureLevel, &state.deviceContext);
    }
    if (FAILED(result) || FAILED(CreateRenderTarget(state))) {
        CleanupDeviceD3D(state);
        return false;
    }
    return true;
}

void CleanupDeviceD3D(D3DState& state) noexcept {
    if (state.deviceContext) state.deviceContext->ClearState();
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
    state = {};
}

HRESULT CreateRenderTarget(D3DState& state) {
    CleanupRenderTarget(state);
    if (!state.swapChain || !state.device) return E_UNEXPECTED;

    ID3D11Texture2D* backBuffer = nullptr;
    const HRESULT getResult = state.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(getResult)) return getResult;

    const HRESULT viewResult = state.device->CreateRenderTargetView(backBuffer, nullptr, &state.renderTargetView);
    backBuffer->Release();
    return viewResult;
}

void CleanupRenderTarget(D3DState& state) noexcept {
    if (state.renderTargetView) {
        state.renderTargetView->Release();
        state.renderTargetView = nullptr;
    }
}
