#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "mikudancestudio/model_alpha_pass.hpp"
#include "mikudancestudio/transparent_triangles.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace {
void Check(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
struct Vertex { float x, y, z, rhw; DWORD color; float u, v; };
void Quad(IDirect3DDevice9* device, float z, DWORD color) {
    const Vertex v[] = {
        {-0.5f,-0.5f,z,1,color,0,0}, {15.5f,-0.5f,z,1,color,1,0},
        {-0.5f,15.5f,z,1,color,0,1}, {15.5f,15.5f,z,1,color,1,1}};
    Check(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(Vertex))), "draw quad");
}
}

int main() {
    HWND window = CreateWindowW(L"STATIC", L"Alpha regression", WS_POPUP,
                                0,0,16,16,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Check(window != nullptr, "create hidden render target window");
    auto* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    Check(d3d != nullptr, "create D3D9");
    D3DPRESENT_PARAMETERS pp{};
    pp.Windowed=TRUE; pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.BackBufferWidth=16; pp.BackBufferHeight=16; pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    pp.EnableAutoDepthStencil=TRUE; pp.AutoDepthStencilFormat=D3DFMT_D16;
    IDirect3DDevice9* device=nullptr;
    Check(SUCCEEDED(d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&device)), "create real render device");
    device->SetRenderState(D3DRS_LIGHTING,FALSE);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
    device->SetRenderState(D3DRS_ALPHAREF,1);
    device->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    IDirect3DTexture9* texture=nullptr;
    Check(SUCCEEDED(device->CreateTexture(4,1,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr)), "alpha texture");
    D3DLOCKED_RECT lock{};
    texture->LockRect(0,&lock,nullptr,0);
    auto* pixels=static_cast<DWORD*>(lock.pBits);
    pixels[0]=0x00000000; pixels[1]=0x40000000; pixels[2]=0x80000000; pixels[3]=0xFF000000;
    texture->UnlockRect(0);
    unsigned char tag[4]{};
    mikudancestudio::CacheTextureColorAndAlpha(texture, tag);
    Check((tag[3] & mikudancestudio::kTextureUsesAlpha) != 0, "texture alpha classification");
    device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);
    device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    IDirect3DSurface9* back=nullptr; device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    IDirect3DSurface9* read=nullptr;
    Check(SUCCEEDED(device->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)), "readback");
    for (bool split : {false,true}) {
        device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFFFFFFFF,1,0);
        device->BeginScene();
        int frontDraws=0, rearDraws=0;
        const auto draw = [&](mikudancestudio::ModelAlphaPass pass) {
            if (mikudancestudio::ModelMaterialMatchesPass(pass,true)) {
                device->SetTexture(0,texture); Quad(device,0.25f,0xFFFFFFFF);
                ++frontDraws;
            }
            if (mikudancestudio::ModelMaterialMatchesPass(pass,false)) {
                device->SetTexture(0,nullptr); Quad(device,0.75f,0xFF0000FF);
                ++rearDraws;
            }
        };
        if (split) {
            draw(mikudancestudio::ModelAlphaPass::Opaque);
            draw(mikudancestudio::ModelAlphaPass::Translucent);
        } else draw(mikudancestudio::ModelAlphaPass::Original);
        Check(frontDraws==1 && rearDraws==1, "each material is submitted only once");
        // An inverted-hull outline behind the front hair must not paint over
        // its soft alpha pixels. With translucent ZWRITE disabled this is black.
        device->SetTexture(0,nullptr); Quad(device,0.5f,0xFF000000);
        device->EndScene();
        Check(SUCCEEDED(device->GetRenderTargetData(back,read)), "read rendered pixels");
        read->LockRect(&lock,nullptr,D3DLOCK_READONLY);
        const auto* row=reinterpret_cast<const DWORD*>(static_cast<const char*>(lock.pBits)+8*lock.Pitch);
        Check((row[1]&0xFFFFFF)==0, "zero-alpha pixels do not write depth over rear geometry");
        Check((row[14]&0xFFFFFF)==0, "opaque black surface still occludes rear surface");
        if (split) {
            Check((row[6]&0xFFFF00)==0 && (row[10]&0xFFFF00)==0,
                  "partial alpha blends with rear blue instead of white clear color");
            Check((row[6]&255)>170 && (row[10]&255)>110,
                  "soft alpha coverage retained and rear outline remains occluded");
        } else Check((row[6]&0xFF0000)!=0, "old single pass reproduces white fringe");
        read->UnlockRect();
        DWORD value=0; device->GetRenderState(D3DRS_ZWRITEENABLE,&value);
        Check(value==TRUE,"depth-write state preserved");
        device->GetRenderState(D3DRS_ALPHAREF,&value); Check(value==1,"alpha threshold preserved");
    }
    // The clothing's front and rear surfaces share one material/texture. A
    // material-only partition cannot fix this: its near triangles may be first.
    texture->LockRect(0,&lock,nullptr,0);
    static_cast<DWORD*>(lock.pBits)[3]=0xFFFFFFFF;
    texture->UnlockRect(0);
    const Vertex layers[] = {
        {-.5f,-.5f,.25f,1,0xFFFFFFFF,.375f,0}, {15.5f,-.5f,.25f,1,0xFFFFFFFF,.375f,0},
        {-.5f,15.5f,.25f,1,0xFFFFFFFF,.375f,0}, {15.5f,15.5f,.25f,1,0xFFFFFFFF,.375f,0},
        {-.5f,-.5f,.75f,1,0xFF0000FF,.875f,0}, {15.5f,-.5f,.75f,1,0xFF0000FF,.875f,0},
        {-.5f,15.5f,.75f,1,0xFF0000FF,.875f,0}, {15.5f,15.5f,.75f,1,0xFF0000FF,.875f,0}};
    D3DMATRIX worldView{};
    worldView._11=worldView._22=worldView._33=worldView._44=1;
    for (bool sort : {false,true}) {
        unsigned short indices[]={0,1,2,2,1,3,4,5,6,6,5,7};
        if (sort) Check(mikudancestudio::SortTransparentTriangles(layers,sizeof(Vertex),8,
            indices,12,worldView), "sort transparent triangles");
        device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xFFFFFFFF,1,0);
        device->BeginScene();
        device->SetTexture(0,texture);
        Check(SUCCEEDED(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,8,4,
            indices,D3DFMT_INDEX16,layers,sizeof(Vertex))), "draw overlapping surfaces in one material");
        device->SetTexture(0,nullptr); Quad(device,.5f,0xFF000000);
        device->EndScene();
        Check(SUCCEEDED(device->GetRenderTargetData(back,read)), "read same-material alpha result");
        read->LockRect(&lock,nullptr,D3DLOCK_READONLY);
        const auto* row=reinterpret_cast<const DWORD*>(static_cast<const char*>(lock.pBits)+8*lock.Pitch);
        if (sort) Check((row[8]&0xFFFF00)==0 && (row[8]&255)>170,
            "same-material white fringe removed without exposing rear black outline");
        else Check((row[8]&0xFF0000)!=0, "unsorted same-material triangles reproduce white fringe");
        read->UnlockRect();
    }
    unsigned int wideIndices[]={0,1,2,4,5,6};
    Check(mikudancestudio::SortTransparentTriangles(layers,sizeof(Vertex),8,wideIndices,6,worldView)
          && wideIndices[0]==4, "32-bit indices sort far surface first");
    worldView._33=-1;
    Check(mikudancestudio::SortTransparentTriangles(layers,sizeof(Vertex),8,wideIndices,6,worldView)
          && wideIndices[0]==0, "camera reversal changes triangle order");
    wideIndices[0]=8;
    Check(!mikudancestudio::SortTransparentTriangles(layers,sizeof(Vertex),8,wideIndices,6,worldView),
          "invalid vertex index rejected before reading vertex data");
    texture->LockRect(0,&lock,nullptr,0);
    pixels=static_cast<DWORD*>(lock.pBits);
    for (int i=0;i<4;++i) pixels[i]=0xFF112233;
    texture->UnlockRect(0);
    mikudancestudio::CacheTextureColorAndAlpha(texture,tag);
    Check(tag[0]==0x11 && tag[1]==0x22 && tag[2]==0x33, "toon color metadata preserved");
    Check(tag[3]==mikudancestudio::kTextureAlphaKnown, "reload updates opaque texture classification");
    DWORD paddedPixels[]={0xFFFFFFFF,0xFFFFFFFF,0,0xFFFFFFFF,0x80FFFFFF,0};
    D3DLOCKED_RECT padded{3*sizeof(DWORD),paddedPixels};
    Check(mikudancestudio::TexturePixelsUseAlpha(padded,2,2), "alpha scan respects pitch and later rows");
    paddedPixels[4]=0xFFFFFFFF;
    Check(!mikudancestudio::TexturePixelsUseAlpha(padded,2,2), "padding is not mistaken for alpha texels");
    read->Release(); back->Release(); texture->Release(); device->Release(); d3d->Release();
    DestroyWindow(window);
    std::puts("Transparent model pixel regressions passed");
}
