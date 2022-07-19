#include <d3d9.h>
#include <d3d9caps.h>
#include <d3dx9core.h>
#include <d3dx9.h>
#include <d3d9helper.h>
#include <d3d9types.h>
#include <Windows.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )

IDirect3DPixelShader9* m_pGlitchShader = nullptr;
IDirect3DTexture9* m_pGlitchTexture = nullptr;
IDirect3DDevice9* m_pDevice = nullptr; //fill on initialize render
IDirect3DSurface9* m_pBackupRenderTarget = nullptr; //backbuffer surface


/// initialize part ///
bool initialize_shader()
{
    if (m_pDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(blur_shader_data), &m_pBlurShader) != D3D_OK) //blur_shader_data - array of bytes imgui-glitch-shader.o 
        return false;
  
    if (m_pDevice->CreateTexture(m_width, m_height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pGlitchTexture, nullptr) != D3D_OK) // m_width and m_height - screen size
	return false;
    
    return true;
}

void on_lost_device
{
    if (m_pGlitchShader)
    {
        m_pGlitchShader->Release();
        m_pGlitchShader = nullptr;
    }

    if (m_pGlitchTexture)
    {
        m_pGlitchTexture->Release();
        m_pGlitchTexture = nullptr;
    }
}


/// imgui implementation part ///

void glitch_begin(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    IDirect3DSurface9* m_pBackupBuffer;
    m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackupBuffer);	 //get backbuffer 	
    m_pDevice->GetRenderTarget(0, &m_pBackupRenderTarget);
		
    IDirect3DSurface9* m_pSurface;

    m_pGlitchTexture->GetSurfaceLevel(0, &m_pSurface); //get glitch texture surface 
    m_pDevice->StretchRect(m_pBackupBuffer, NULL, m_pSurface, NULL, D3DTEXF_NONE); //copy backbuffer texture to glitch texture
    m_pDevice->SetRenderTarget(0, m_pSurface); //set new render target(glitch)
  
    m_pBackupBuffer->Release();
    m_pSurface->Release();

    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

void glitch_texture(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    static float time = 0.f;

    m_pDevice->SetPixelShader(m_pGlitchShader); //set glitch shader
  
    const float params_size[2] = { 1.f, 1.f }; 
    const float params_time[1] = { time }; 

    m_pDevice->SetPixelShaderConstantF(0, params_size, 2); //set screen size (1.f, 1.f) - fullscreen
    m_pDevice->SetPixelShaderConstantF(1, params_time, 1); //set time
	
    time += ImGui::GetIO().DeltaTime * 0.25f; //update time counter
}

void glitch_end(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    //restore render target, restore states
    m_pDevice->SetRenderTarget(0, m_pBackupRenderTarget);
    m_pBackupRenderTarget->Release();

    m_pDevice->SetPixelShader(nullptr);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP); 
}

/// render shader texture where u want ///

void glitch_draw(ImDrawList* drawList)
{
    auto& io = ImGui::GetIO();

    drawList->AddCallback(glitch_begin, 0); //prepare texture

    drawList->AddCallback(glitch_texture, 0); //draw glitch
    drawList->AddImage(m_pGlitchTexture, ImVec2(0.f, 0.f), ImVec2(io.DisplaySize.x, io.DisplaySize.y));

    drawList->AddCallback(glitch_end, 0); //restore texture

    drawList->AddImage(m_pGlitchTexture, ImVec2(0.f, 0.f), ImVec2(io.DisplaySize.x, io.DisplaySize.y)); //render texture
}

