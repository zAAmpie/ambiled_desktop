#include "d3d11scaler.h"

#ifdef _WIN32

#include <d3dcompiler.h>
#include <cstring>

// Fullscreen-triangle vertex shader using SV_VertexID — no vertex buffer needed.
// Three vertices form an oversized triangle that covers the entire viewport after clipping.
static const char* VS_SRC = R"(
struct VS_OUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
VS_OUT main(uint id : SV_VertexID) {
    VS_OUT o;
    float2 uv = float2((id & 1) * 2.0, (id >> 1) * 2.0);
    o.uv  = uv;
    o.pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return o;
}
)";

// Bilinear sample of the source texture.
static const char* PS_SRC = R"(
Texture2D    tex  : register(t0);
SamplerState samp : register(s0);
struct PS_IN {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(PS_IN i) : SV_TARGET {
    return tex.Sample(samp, i.uv);
}
)";

D3D11Scaler::D3D11Scaler(ID3D11Device* device, ID3D11DeviceContext* context, float scaleFactor)
    : pDevice(device), pContext(context), pScaleFactor(scaleFactor), pInitialised(false),
      pVertexShader(nullptr), pPixelShader(nullptr), pSamplerState(nullptr),
      pIntermediateTex(nullptr), pSRV(nullptr),
      pOutputTex(nullptr), pRTV(nullptr),
      pSourceWidth(0), pSourceHeight(0), pOutputWidth(0), pOutputHeight(0)
{
}

D3D11Scaler::~D3D11Scaler()
{
    cleanup();
}

void D3D11Scaler::setScaleFactor(float factor)
{
    if (factor != pScaleFactor)
    {
        pScaleFactor = factor;
        cleanupTextures(); // force recreation at new output dimensions
    }
}

ID3D11Texture2D* D3D11Scaler::scale(ID3D11Texture2D* source)
{
    if (!source)
        return nullptr;

    if (!pInitialised && !initialise())
        return nullptr;

    D3D11_TEXTURE2D_DESC srcDesc;
    source->GetDesc(&srcDesc);

    if (!ensureTextures(srcDesc.Width, srcDesc.Height, srcDesc.Format))
        return nullptr;

    // Copy source into the SRV-bindable intermediate (desktop duplication textures
    // have no bind flags so cannot be used as shader resources directly)
    pContext->CopyResource(pIntermediateTex, source);

    // Set up pipeline — no input layout or vertex buffer needed for SV_VertexID triangle
    D3D11_VIEWPORT viewport{};
    viewport.Width    = static_cast<float>(pOutputWidth);
    viewport.Height   = static_cast<float>(pOutputHeight);
    viewport.MaxDepth = 1.0f;

    pContext->IASetInputLayout(nullptr);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pContext->VSSetShader(pVertexShader, nullptr, 0);
    pContext->PSSetShader(pPixelShader, nullptr, 0);
    pContext->PSSetShaderResources(0, 1, &pSRV);
    pContext->PSSetSamplers(0, 1, &pSamplerState);
    pContext->OMSetRenderTargets(1, &pRTV, nullptr);
    pContext->RSSetViewports(1, &viewport);

    pContext->Draw(3, 0);

    // Unbind to leave context in a clean state
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ID3D11RenderTargetView*   nullRTV = nullptr;
    pContext->PSSetShaderResources(0, 1, &nullSRV);
    pContext->OMSetRenderTargets(1, &nullRTV, nullptr);

    return pOutputTex;
}

bool D3D11Scaler::initialise()
{
    // Compile vertex shader
    ID3DBlob* vsBlob  = nullptr;
    ID3DBlob* errBlob = nullptr;
    HRESULT hr = D3DCompile(VS_SRC, strlen(VS_SRC), nullptr, nullptr, nullptr,
                            "main", "vs_4_0", 0, 0, &vsBlob, &errBlob);
    if (errBlob) { errBlob->Release(); errBlob = nullptr; }
    if (FAILED(hr)) return false;

    hr = pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                     nullptr, &pVertexShader);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    // Compile pixel shader
    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(PS_SRC, strlen(PS_SRC), nullptr, nullptr, nullptr,
                    "main", "ps_4_0", 0, 0, &psBlob, &errBlob);
    if (errBlob) { errBlob->Release(); errBlob = nullptr; }
    if (FAILED(hr)) return false;

    hr = pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                    nullptr, &pPixelShader);
    psBlob->Release();
    if (FAILED(hr)) return false;

    // Bilinear sampler with clamped addressing
    D3D11_SAMPLER_DESC sampDesc{};
    sampDesc.Filter         = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
    hr = pDevice->CreateSamplerState(&sampDesc, &pSamplerState);
    if (FAILED(hr)) return false;

    pInitialised = true;
    return true;
}

bool D3D11Scaler::ensureTextures(UINT sourceWidth, UINT sourceHeight, DXGI_FORMAT format)
{
    // Round to nearest, clamp to minimum 1
    UINT outW = static_cast<UINT>(sourceWidth  * pScaleFactor + 0.5f);
    UINT outH = static_cast<UINT>(sourceHeight * pScaleFactor + 0.5f);
    if (outW < 1u) outW = 1u;
    if (outH < 1u) outH = 1u;

    // Reuse existing textures if nothing has changed
    if (pIntermediateTex
        && sourceWidth  == pSourceWidth  && sourceHeight == pSourceHeight
        && outW         == pOutputWidth  && outH         == pOutputHeight)
        return true;

    cleanupTextures();

    // Intermediate texture: same dimensions as source, SRV-bindable
    D3D11_TEXTURE2D_DESC intDesc{};
    intDesc.Width            = sourceWidth;
    intDesc.Height           = sourceHeight;
    intDesc.MipLevels        = 1;
    intDesc.ArraySize        = 1;
    intDesc.Format           = format;
    intDesc.SampleDesc.Count = 1;
    intDesc.Usage            = D3D11_USAGE_DEFAULT;
    intDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = pDevice->CreateTexture2D(&intDesc, nullptr, &pIntermediateTex);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format                    = format;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    hr = pDevice->CreateShaderResourceView(pIntermediateTex, &srvDesc, &pSRV);
    if (FAILED(hr)) return false;

    // Output texture: scaled dimensions, render-target bindable
    D3D11_TEXTURE2D_DESC outDesc{};
    outDesc.Width            = outW;
    outDesc.Height           = outH;
    outDesc.MipLevels        = 1;
    outDesc.ArraySize        = 1;
    outDesc.Format           = format;
    outDesc.SampleDesc.Count = 1;
    outDesc.Usage            = D3D11_USAGE_DEFAULT;
    outDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;

    hr = pDevice->CreateTexture2D(&outDesc, nullptr, &pOutputTex);
    if (FAILED(hr)) return false;

    hr = pDevice->CreateRenderTargetView(pOutputTex, nullptr, &pRTV);
    if (FAILED(hr)) return false;

    pSourceWidth  = sourceWidth;
    pSourceHeight = sourceHeight;
    pOutputWidth  = outW;
    pOutputHeight = outH;
    return true;
}

void D3D11Scaler::cleanupTextures()
{
    if (pRTV)             { pRTV->Release();             pRTV = nullptr; }
    if (pOutputTex)       { pOutputTex->Release();       pOutputTex = nullptr; }
    if (pSRV)             { pSRV->Release();             pSRV = nullptr; }
    if (pIntermediateTex) { pIntermediateTex->Release(); pIntermediateTex = nullptr; }
    pSourceWidth = pSourceHeight = pOutputWidth = pOutputHeight = 0;
}

void D3D11Scaler::cleanup()
{
    cleanupTextures();
    if (pSamplerState) { pSamplerState->Release(); pSamplerState = nullptr; }
    if (pPixelShader)  { pPixelShader->Release();  pPixelShader  = nullptr; }
    if (pVertexShader) { pVertexShader->Release(); pVertexShader = nullptr; }
    pInitialised = false;
}

#endif // _WIN32
