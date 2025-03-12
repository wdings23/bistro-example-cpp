#pragma once

#include <render-driver/Device.h>
#include <render-driver/SwapChain.h>

#include <render-driver/Buffer.h>

#include <render/Renderer.h>

#import <Metal/Metal.h>

#include <thread>

struct PageInfo
{
    uint32_t    miCoordX;
    uint32_t    miCoordY;
    uint32_t    miHashIndex;
    uint32_t    miTextureID;
    uint32_t    miMIP;
    uint32_t    miPageIndex;
};

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
    
protected:
    void initRenderData(
        std::vector<char> const& acMaterialBuffer,
        std::vector<std::string> const& aAlbedoTextureNames,
        std::vector<std::string> const& aNormalTextureNames,
        std::vector<uint2> const& aAlbedoTextureDimensions,
        std::vector<uint2> const& aNormalTextureDimensions,
        uint32_t iNumAlbedoTextures,
        uint32_t iNumNormalTextures
    );
    
public:
    std::unique_ptr<RenderDriver::Common::CBuffer> mTexturePageQueueReadBackBuffer;
    std::unique_ptr<RenderDriver::Common::CBuffer> mTexturePageInfoReadBackBuffer;
    std::unique_ptr<RenderDriver::Common::CBuffer> mTexturePageCounterReadBackBuffer;
    
    std::unique_ptr<RenderDriver::Common::CBuffer> maTexturePageUploadBuffer[4];
    std::unique_ptr<RenderDriver::Common::CBuffer> maScatchPadUploadBuffer[4];
    
    std::vector<std::string> maAlbedoTextureNames;
    std::vector<std::string> maNormalTextureNames;
    std::vector<uint2> maAlbedoTextureDimensions;
    std::vector<uint2> maNormalTextureDimensions;
    uint32_t miNumAlbedoTextures = 0;
    uint32_t miNumNormalTextures = 0;
    uint32_t miCurrAlbedoPageLoaded = 0;
    uint32_t miCurrNormalPageLoaded = 0;
    uint32_t miCurrTotalPageLoaded = 0;
    std::map<std::string, std::vector<PageInfo>> maPageInfo;
    bool mbTexturePageLoadingStarted = false;
    
    uint32_t miNumThreads = 4;
    std::unique_ptr<std::thread> mTexturePageLoadingThread;
    std::unique_ptr<std::thread> maThreads[4];
    uint32_t miNumLoadingThreads;
    std::unique_ptr<RenderDriver::Common::CCommandQueue> mThreadCommandQueue;
    std::mutex mTexturePageThreadMutex;
    std::condition_variable mConditionVariable;
    std::vector<char> macTexturePageQueueData;
    std::vector<char> macTexturePageInfoData;
    std::vector<char> macCounterData;
    std::vector<std::pair<uint32_t, uint32_t>>      maiStartAndNumChecks;
    uint32_t miNumChecksPerThread;
    
    uint32_t maiThread[4];
    
    bool mbStartLoadPage = false;
    bool mbQuit = false;
    
    std::unique_ptr<RenderDriver::Common::CCommandBuffer> mThreadCommandBuffer;
    std::unique_ptr<RenderDriver::Common::CCommandAllocator> mThreadCommandAllocator;
};
