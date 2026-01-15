#pragma once
#include <d3d11.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../external/stb_image.h"

#include "ui/secureloader_icon.h"

namespace render {
    
    // Logo texture
    inline ID3D11ShaderResourceView* g_pLogoTexture = nullptr;
    
    // Load PNG from memory into D3D11 texture
    inline bool LoadTextureFromMemory(
        ID3D11Device* pDevice,
        const unsigned char* data,
        unsigned int dataSize,
        ID3D11ShaderResourceView** outTexture,
        int* outWidth = nullptr,
        int* outHeight = nullptr)
    {
        if (!pDevice || !data || dataSize == 0) return false;
        
        // Decode PNG
        int width, height, channels;
        unsigned char* imageData = stbi_load_from_memory(data, dataSize, &width, &height, &channels, 4);
        if (!imageData) return false;
        
        // Create texture
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        
        D3D11_SUBRESOURCE_DATA subResource = {};
        subResource.pSysMem = imageData;
        subResource.SysMemPitch = width * 4;
        
        ID3D11Texture2D* pTexture = nullptr;
        HRESULT hr = pDevice->CreateTexture2D(&texDesc, &subResource, &pTexture);
        
        stbi_image_free(imageData);
        
        if (FAILED(hr)) return false;
        
        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        
        hr = pDevice->CreateShaderResourceView(pTexture, &srvDesc, outTexture);
        pTexture->Release();
        
        if (FAILED(hr)) return false;
        
        if (outWidth) *outWidth = width;
        if (outHeight) *outHeight = height;
        
        return true;
    }
    
    // Initialize logo texture
    inline bool InitializeLogo(ID3D11Device* pDevice) {
        if (g_pLogoTexture) return true;  // Already loaded
        
        return LoadTextureFromMemory(
            pDevice,
            secureloader_logo_png,
            secureloader_logo_size,
            &g_pLogoTexture
        );
    }
    
    // Release logo texture
    inline void ReleaseLogo() {
        if (g_pLogoTexture) {
            g_pLogoTexture->Release();
            g_pLogoTexture = nullptr;
        }
    }
}
