#pragma once

#include <render-driver/Device.h>
#include <render-driver/SwapChain.h>

#include <render/Renderer.h>

#import <Metal/Metal.h>

class CApp
{
public:
    struct AppDescriptor
    {
        
    };
    
public:
    CApp() = default;
    virtual ~CApp() = default;
    
    void init(AppDescriptor const& appDesc);
    
    void update(CGFloat time);
    void render();
    
    void* getNativeDevice();
    
    void nextDrawable(
        id<MTLDrawable> drawable,
        id<MTLTexture> drawableTexture,
        uint32_t iWidth,
        uint32_t iHeight);
    
protected:
    std::unique_ptr<RenderDriver::Common::CDevice>           mpDevice;
    std::unique_ptr<RenderDriver::Common::CSwapChain>        mpSwapChain;
    std::unique_ptr<Render::Common::CRenderer>               mpRenderer;
    
    std::vector<unsigned char> macBlueNoiseImageData;
    uint32_t miBlueNoiseWidth;
    uint32_t miBlueNoiseHeight;
    
    float3 mCameraPosition;
    float3 mCameraLookAt;
    
    std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>> maBufferMap;
};
