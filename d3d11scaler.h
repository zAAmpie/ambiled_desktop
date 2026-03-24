#pragma once

#ifdef _WIN32

#include <d3d11.h>

// GPU-side bilinear downscaler using a D3D11 fullscreen-triangle blit.
// Reduces bus traffic and downstream CPU work by scaling the captured frame
// on the GPU before readback. Textures are cached and only recreated on
// resolution or scale factor changes.
//
// Device and context are non-owning — the caller (capture class) owns them.
class D3D11Scaler
{
public:
    D3D11Scaler(ID3D11Device* device, ID3D11DeviceContext* context, float scaleFactor = 0.5f);
    ~D3D11Scaler();

    // Scale source texture. Returns scaled GPU texture owned by this class,
    // or nullptr if the scaler failed to initialise (caller should use source as-is).
    ID3D11Texture2D* scale(ID3D11Texture2D* source);

    float scaleFactor() const { return pScaleFactor; }
    void setScaleFactor(float factor);

private:
    bool initialise();
    bool ensureTextures(UINT sourceWidth, UINT sourceHeight, DXGI_FORMAT format);
    void cleanupTextures();
    void cleanup();

    ID3D11Device*           pDevice;        // non-owning
    ID3D11DeviceContext*    pContext;        // non-owning
    float                   pScaleFactor;
    bool                    pInitialised;

    // Shader pipeline — created once in initialise()
    ID3D11VertexShader*     pVertexShader;
    ID3D11PixelShader*      pPixelShader;
    ID3D11SamplerState*     pSamplerState;

    // Per-resolution textures — recreated when source size or scale factor changes
    ID3D11Texture2D*            pIntermediateTex;   // SRV-bindable copy of source
    ID3D11ShaderResourceView*   pSRV;
    ID3D11Texture2D*            pOutputTex;         // scaled render target (returned to caller)
    ID3D11RenderTargetView*     pRTV;
    UINT pSourceWidth;
    UINT pSourceHeight;
    UINT pOutputWidth;
    UINT pOutputHeight;
};

#endif // _WIN32
