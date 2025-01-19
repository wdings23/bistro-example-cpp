#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <assert.h>

#include <Windows.h>

#include <render/DX12/RendererDX12.h>
#include <render/Vulkan/RendererVulkan.h>

#include <render-driver/Vulkan/ImageViewVulkan.h>

#include <skeletal-animation/AnimationManager.h>
#include <LogPrint.h>
#include <wtfassert.h>

#include <chrono>
#include <sstream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <tinyexr.h>

//#define USE_IMGUI       1

#include <stb_image.h>
#include <stb_image_write.h>

#include <image_utils.h>

#include <mutex>
std::mutex gMutex;

#define WINDOW_WIDTH    512
#define WINDOW_HEIGHT   512

#define PI 3.14159f

HWND _initWindow(
    HINSTANCE hInstance,
    WNDPROC windowProc,
    uint32_t iWidth,
    uint32_t iHeight);

LRESULT CALLBACK _windowProc(
    HWND windowHandle,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam);


//static float3 sCameraPosition(6.8f, 2.3f, 4.8f);
//static float3 sCameraLookAt(0.0f, 0.0f, 0.0f);

//static float3 sCameraLookAt(-3.44f, 1.53f, 0.625f);
//static float3 sCameraPosition(-7.08f, 1.62f, 0.675f);
//static float3 sInitialCameraPosition(1.0f, 1.0f, 0.0f);
//static float3 sInitialCameraLookAt(0.0f, 1.0f, 0.0f);
static float3 sInitialCameraPosition(-6.623f, 1.35f, -0.1312f);
static float3 sInitialCameraLookAt(-5.695f, 1.35f, 0.2434f);
static float3 sCameraPosition(1.0f, 1.0f, 0.0f);
static float3 sCameraLookAt(0.0f, 0.0f, 0.0f);

static float3 sCameraUp(0.0f, 1.0f, 0.0f);
static float4 sBallPosition(0.0f, -0.2f, 0.0f, 1.0f);

float2 sCameraAngle(0.0f, 0.0f);
int32_t siLastX = -1;
int32_t siLastY = -1;

static float3 sUpperEyeTranslation(0.0f, 0.0f, 0.0f);
static float sfHeading = 0.0f;
static bool sbMoving = false;
static float3 sWalkDir(0.0f, 0.0f, 0.0f);

static POINT sMouseClickPos = { -1, -1 };
static float3 sMouseClickWorldPos(0.0f, 0.0f, 0.0f);

static float gfCameraNear = 0.8f;
static float gfCameraFar = 100.0f;
static float gfFOV = 3.14159f * 0.5f;

static POINT sMiddleClickPos;
static POINT sLastPt;
static POINT sLastRightMousePt;
static POINT sLastLeftMousePt;

static bool sbMiddleMouseDown = false;
static bool sbRightMouseDown = false;
static bool sbLeftMouseDown = false;

static float3 sSunColor = float3(15.0f, 15.0f, 15.0f);

Render::Common::CRenderer* gpRenderer;
int32_t giNumOutputRenderTargets = 0;
std::vector<std::pair<std::string, std::string>> gaRenderTargetNames;
int32_t giCurrRenderJobOutput = 0;

static ImGuiIO* spImguiIO = nullptr;
void initImgui(
    HWND hWnd,
    ID3D12Device* pDevice,
    ID3D12DescriptorHeap* pShaderResourceHeap,
    DXGI_FORMAT outputFormat,
    uint32_t iNumFramesInFlight);

void imguiLayout(Render::Common::CRenderer* pRenderer);

/*
**
*/
extern "C" char const* getSaveDir()
{
    return "C:\\Users\\Dingwings\\Logs";
}

static Render::Common::RenderDriverType sRenderDriverType = Render::Common::RenderDriverType::DX12;


#include "quaternion.h"
#include "mat4.h"

/*
**
*/
void getMaterialInfo(
    std::vector<char>& acMaterialBuffer,
    std::vector<std::string>& aAlbedoTextureNames,
    std::vector<std::string>& aNormalTextureNames,
    std::vector<uint2>& aAlbedoTextureDimensions,
    std::vector<uint2>& aNormalTextureDimensions,
    uint32_t& iNumAlbedoTextures,
    uint32_t& iNumNormalTextures,
    Render::Common::CRenderer* pRenderer
)
{
    FILE* fp = fopen("d:\\downloads\\Bistro_v4\\bistro2.mat", "rb");
    fseek(fp, 0, SEEK_END);
    size_t iFileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<char> acBuffer(iFileSize);
    fread(acBuffer.data(), sizeof(char), iFileSize, fp);
    fclose(fp);

    // material id
    char* pacBuffer = acBuffer.data();
    uint32_t iCurrPos = 0;
    uint32_t iNumMaterials = 0;
    for(;;)
    {
        uint32_t iMaterialID = *((uint32_t*)(pacBuffer + iCurrPos + 16 * 3));
        if(iMaterialID >= 99999)
        {
            break;
        }

        iCurrPos += 16 * 4;
        iNumMaterials += 1;
    }
    iCurrPos += 16 * 4;

    acMaterialBuffer.resize(iCurrPos);
    memcpy(acMaterialBuffer.data(), acBuffer.data(), iCurrPos);

    // albedo textures
    iNumAlbedoTextures = *((uint32_t*)(pacBuffer + iCurrPos));
    iCurrPos += 4;
    for(uint32_t i = 0; i < iNumAlbedoTextures; i++)
    {
        std::string albedoTextureName = "";
        for(;;)
        {
            char cChar = *(pacBuffer + iCurrPos);
            if(cChar == '\n')
            {
                // parse to base name and png extension
                auto fileExtensionStart = albedoTextureName.find_last_of(".");
                auto directoryEnd = albedoTextureName.find_last_of("\\");
                if(directoryEnd == std::string::npos)
                {
                    directoryEnd = albedoTextureName.find_last_of("/");
                    if(directoryEnd == std::string::npos)
                    {
                        directoryEnd = 0;
                    }
                    else
                    {
                        directoryEnd += 1;
                    }
                }
                else
                {
                    directoryEnd += 1;
                }
                std::string parsed = albedoTextureName.substr(directoryEnd, fileExtensionStart - directoryEnd) + ".png";
                aAlbedoTextureNames.push_back(parsed);
                break;
            }
            albedoTextureName += cChar;
            iCurrPos += 1;
        }
        iCurrPos += 1;
    }

    // normal texture
    iNumNormalTextures = *((uint32_t*)(pacBuffer + iCurrPos));
    iCurrPos += 4;
    for(uint32_t i = 0; i < iNumNormalTextures; i++)
    {
        std::string normalTextureName = "";
        for(;;)
        {
            char cChar = *(pacBuffer + iCurrPos);
            if(cChar == '\n')
            {
                // parse to base name and png extension
                auto fileExtensionStart = normalTextureName.find_last_of(".");
                auto directoryEnd = normalTextureName.find_last_of("\\");
                if(directoryEnd == std::string::npos)
                {
                    directoryEnd = normalTextureName.find_last_of("/");
                    if(directoryEnd == std::string::npos)
                    {
                        directoryEnd = 0;
                    }
                    else
                    {
                        directoryEnd += 1;
                    }
                }
                else
                {
                    directoryEnd += 1;
                }
                std::string parsed = normalTextureName.substr(directoryEnd, fileExtensionStart - directoryEnd) + ".png";
                aNormalTextureNames.push_back(parsed);
                break;
            }
            normalTextureName += cChar;
            iCurrPos += 1;
        }
        iCurrPos += 1;
    }

    //ImageUtils::rescaleImages(
    //    aAlbedoTextureNames,
    //    "d:\\Downloads\\bistro_v4\\converted-dds",
    //    "d:\\Downloads\\bistro_v4\\converted-dds-initial",
    //    64,
    //    64,
    //    "albedo-dimensions"
    //);

#if 0
    ImageUtils::rescaleImages(
        aAlbedoTextureNames,
        "d:\\Downloads\\bistro_v4\\converted-dds",
        "d:\\Downloads\\bistro_v4\\converted-dds-scaled",
        512,
        512,
        "albedo-dimensions"
    );

    ImageUtils::rescaleImages(
        aNormalTextureNames,
        "d:\\Downloads\\bistro_v4\\converted-dds",
        "d:\\Downloads\\bistro_v4\\converted-dds-scaled",
        512,
        512,
        "normal-dimensions"
    );

    ImageUtils::convertNormalImages(
        aNormalTextureNames,
        "d:\\Downloads\\bistro_v4\\converted-dds-scaled",
        "d:\\Downloads\\bistro_v4\\converted-dds-scaled");

    ImageUtils::rescaleImages(
        aAlbedoTextureNames,
        "d:\\Downloads\\bistro_v4\\converted-dds-scaled",
        "d:\\Downloads\\bistro_v4\\converted-dds-initial",
        64,
        64,
        "albedo-dimensions"
    );

    ImageUtils::rescaleImages(
        aNormalTextureNames,
        "d:\\Downloads\\bistro_v4\\converted-dds-scaled",
        "d:\\Downloads\\bistro_v4\\converted-dds-initial",
        64,
        64,
        "normal-dimensions"
    );

#endif // #if 0

    aAlbedoTextureDimensions.resize(iNumAlbedoTextures);
    fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds-scaled\\albedo-dimensions.txt", "rb");
    fread(aAlbedoTextureDimensions.data(), sizeof(uint2), iNumAlbedoTextures, fp);
    fclose(fp);

    aNormalTextureDimensions.resize(iNumNormalTextures);
    fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds-scaled\\normal-dimensions.txt", "rb");
    fread(aNormalTextureDimensions.data(), sizeof(uint2), iNumNormalTextures, fp);
    fclose(fp);
}

//void loadTexturePage(
//    std::vector<char>& acPageData,
//    std::string const& imageFileName,
//    uint2 const& pageCoord,
//    uint32_t iPageDimension);

struct PageInfo
{
    uint32_t    miCoordX;
    uint32_t    miCoordY;
    uint32_t    miHashIndex;
    uint32_t    miTextureID;
    uint32_t    miMIP;
    uint32_t    miPageIndex;
};

struct ImageInfo
{
    int32_t    miImageWidth;
    int32_t    miImageHeight;
    stbi_uc* mpImageData;
};

void outputTexturePages(
    std::string const& imageFileName,
    uint32_t iPageDimension);

void loadTexturePage(
    Render::Common::CRenderer* pRenderer,
    std::vector<std::string> const& aAlbedoTextureNames,
    std::vector<uint2> const& aAlbedoTextureDimensions,
    std::vector<std::string> const& aNormalTextureNames,
    std::vector<uint2> const& aNormalTextureDimensions,
    uint32_t& iCurrTotalPageLoaded,
    uint32_t& iCurrAlbedoPageLoaded,
    uint32_t& iCurrNormalPageLoaded,
    std::map<std::string, std::vector<PageInfo>>& aPageInfo);

void loadTexturePageThread(
    Render::Common::CRenderer* pRenderer,
    std::vector<std::string> const& aAlbedoTextureNames,
    std::vector<uint2> const& aAlbedoTextureDimensions,
    std::vector<std::string> const& aNormalTextureNames,
    std::vector<uint2> const& aNormalTextureDimensions,
    uint32_t& iCurrTotalPageLoaded,
    uint32_t& iCurrAlbedoPageLoaded,
    uint32_t& iCurrNormalPageLoaded,
    //std::map<std::string, std::vector<PageInfo>>& aPageInfo,
    RenderDriver::Common::CCommandBuffer& commandBuffer,
    RenderDriver::Common::CCommandQueue& commandQueue,
    RenderDriver::Common::CBuffer& uploadBuffer,
    uint32_t iStartIndex,
    uint32_t iNumChecksPerLoop,
    //std::map<uint32_t, ImageInfo>& aImageInfo,
    RenderDriver::Common::CBuffer& texturePageUploadBuffer,
    void* pUploadBuffer,
    uint32_t& iLastCounterValue,
    std::vector<char> const& acTexturePageQueueData,
    std::vector<char> const& acTexturePageInfoData,
    std::mutex& threadMutex);

void setupExternalDataBuffers(
    std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>>& bufferMap,
    Render::Common::CRenderer* pRenderer);

std::mutex threadMutex;
std::mutex updateCheckMutex;
bool gbQuit = false;

/*
**
*/
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR pCommandLine,
    _In_ int iCommandShow)
{

    //std::this_thread::sleep_for(std::chrono::seconds(5));

    if(strstr(pCommandLine, "--render=vulkan"))
    {
        sRenderDriverType = Render::Common::RenderDriverType::Vulkan;
        printf("Using Vulkan\n");
    }
    else
    {
        printf("Using DX12\n");
    }

    AllocConsole();
    FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
    freopen_s(&fpstdin, "CONIN$", "r", stdin);
    freopen_s(&fpstdout, "CONOUT$", "w", stdout);
    freopen_s(&fpstderr, "CONOUT$", "w", stderr);

    HWND window = _initWindow(
        hInstance,
        _windowProc,
        500,
        500);

    // for attaching debugger
    //Sleep(20000);

    std::unique_ptr<Render::Common::RendererDescriptor> pDesc = nullptr;
    if(strstr(pCommandLine, "--render=vulkan"))
    {
        pDesc = std::make_unique<Render::Vulkan::RendererDescriptor>();
        reinterpret_cast<Render::Vulkan::RendererDescriptor*>(pDesc.get())->mHWND = window;
    }
    else
    {
        pDesc = std::make_unique<Render::DX12::RendererDescriptor>();
        reinterpret_cast<Render::DX12::RendererDescriptor*>(pDesc.get())->mHWND = window;
    }

    pDesc->mFormat = RenderDriver::Common::Format::R10G10B10A2_UNORM;
    pDesc->miScreenWidth = WINDOW_WIDTH;
    pDesc->miScreenHeight = WINDOW_HEIGHT;
    pDesc->miSamples = 1;
    pDesc->miSwapChainFlags = RenderDriver::Common::SwapChainFlagBits::FIFO;
    pDesc->miNumBuffers = 3;
    pDesc->miViewportWidth = WINDOW_WIDTH;
    pDesc->miViewportHeight = WINDOW_HEIGHT;
    pDesc->mfViewportMaxDepth = 1.0f;

    std::unique_ptr<Render::DX12::CRenderer> dx12Renderer = nullptr;
    std::unique_ptr<Render::Vulkan::CRenderer> vulkanRenderer = nullptr;
    Render::Common::CRenderer* pRenderer = nullptr;
    if(strstr(pCommandLine, "--render=vulkan"))
    {
        pDesc->mFormat = RenderDriver::Common::Format::B8G8R8A8_UNORM;
        vulkanRenderer = std::make_unique<Render::Vulkan::CRenderer>();
        pRenderer = vulkanRenderer.get();
    }
    else
    {
        dx12Renderer = std::make_unique<Render::DX12::CRenderer>();
        pRenderer = dx12Renderer.get();
    }

    gpRenderer = pRenderer;

    Render::Common::gLightDirection = normalize(float3(0.5f, 1.0f, 0.0f));

    // TODO: move vertex and index buffer out of renderer

    pDesc->mRenderJobsFilePath = "d:\\test\\bistro-example-cpp\\render-jobs\\bistro-example-render-jobs.json";
    pRenderer->setup(*pDesc);
    
    // number of meshes
    uint32_t miNumMeshes = 0;
    FILE* fp = fopen("d:\\Downloads\\Bistro_v4\\bistro2-triangles.bin", "rb");
    fread(&miNumMeshes, sizeof(uint32_t), 1, fp);
    fclose(fp);

    // materials, albedo and normal texture names, albedo and normal texture dimension
    std::vector<char> acMaterialBuffer;
    std::vector<std::string> aAlbedoTextureNames;
    std::vector<std::string> aNormalTextureNames;
    std::vector<uint2> aAlbedoTextureDimensions;
    std::vector<uint2> aNormalTextureDimensions;
    uint32_t iNumAlbedoTextures = 0;
    uint32_t iNumNormalTextures = 0;
    getMaterialInfo(
        acMaterialBuffer,
        aAlbedoTextureNames,
        aNormalTextureNames,
        aAlbedoTextureDimensions,
        aNormalTextureDimensions,
        iNumAlbedoTextures,
        iNumNormalTextures,
        pRenderer);

    // set up buffers used for all the passes
    std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>> aBufferMap;
    setupExternalDataBuffers(
        aBufferMap,
        pRenderer
    );

    // has to ptr of the map into data lambda or else it's a copy which unique_ptr disallow
    std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>>* paBufferMap = &aBufferMap;

#if 0
    for(auto const& fileName : aAlbedoTextureNames)
    {
        outputTexturePages(fileName, 64);
    }
#endif // #if 0

    uint32_t iCurrAlbedoPageLoaded = 0, iCurrNormalPageLoaded = 0, iCurrTotalPageLoaded = 0;

    // set up uniform buffer ptrs
    std::unique_ptr<std::function<void(Render::Common::CRenderJob*)>> pfnSetVertexBuffer = std::make_unique<std::function<void(Render::Common::CRenderJob*)>>();
    *pfnSetVertexBuffer = [pRenderer](Render::Common::CRenderJob* pRenderJob)
    {
        Render::Vulkan::CRenderer* pRendererVulkan = (Render::Vulkan::CRenderer*)pRenderer;
        pRenderJob->mapUniformBuffers["vertexBuffer"] = pRendererVulkan->getVertexBuffer("bistro");
        pRenderJob->mapUniformBuffers["indexBuffer"] = pRendererVulkan->getIndexBuffer("bistro");
    };
    pRenderer->mapfnRenderJobData["Test Compute"] = pfnSetVertexBuffer.get();

#if defined(USE_RAY_TRACING)
    std::unique_ptr<std::function<void(Render::Common::CRenderJob*)>> pfnSetBuffers = std::make_unique<std::function<void(Render::Common::CRenderJob*)>>();
    *pfnSetBuffers = [paBufferMap, pRenderer](Render::Common::CRenderJob* pRenderJob)
    {
        Render::Vulkan::CRenderer* pRendererVulkan = (Render::Vulkan::CRenderer*)pRenderer;
        pRenderJob->mapUniformBuffers["vertexBuffer"] = pRendererVulkan->getVertexBuffer("bistro");
        pRenderJob->mapUniformBuffers["indexBuffer"] = pRendererVulkan->getIndexBuffer("bistro");

        std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>>& aBufferMap = *paBufferMap;
        pRenderJob->mapUniformBuffers["materialData"] = aBufferMap["Material Data"].get();
        pRenderJob->mapUniformBuffers["materialID"] = aBufferMap["Material ID"].get();
        if(pRenderJob->maShaderResourceInfo.find("meshTriangleRangeData") != pRenderJob->maShaderResourceInfo.end())
        {
            pRenderJob->mapUniformBuffers["meshTriangleRangeData"] = aBufferMap["Mesh Triangle Index Ranges"].get();
        }
    };

    pRenderer->mapfnRenderJobData["Temporal Restir Diffuse Ray Trace"] = pfnSetBuffers.get();
    pRenderer->mapfnRenderJobData["Build Irradiance Cache Ray Trace"] = pfnSetBuffers.get();
#endif // USE_RAY_TRACING

    std::map<std::string, std::vector<PageInfo>> aPageInfo;

    // initialize data, set the albedo and normal texture dimensions 
    pRenderer->mpfnInitData = std::make_unique<std::function<void(Render::Common::CRenderer*)>>();
    *pRenderer->mpfnInitData = [
        aAlbedoTextureDimensions, 
        aNormalTextureDimensions,
        aAlbedoTextureNames,
        aNormalTextureNames,
        acMaterialBuffer
    ](Render::Common::CRenderer* pRenderer)
    {
        // copy texture dimension data over
        auto& pTextureDimensionBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapUniformBuffers["Texture Sizes"];
        pRenderer->copyCPUToBuffer(
            pTextureDimensionBuffer,
            (void *)aAlbedoTextureDimensions.data(),
            0,
            uint32_t(aAlbedoTextureDimensions.size() * sizeof(uint2))
        );

        pTextureDimensionBuffer = pRenderer->mapRenderJobs["Texture Atlas Graphics"]->mapUniformBuffers["Texture Sizes"];
        pRenderer->copyCPUToBuffer(
            pTextureDimensionBuffer,
            (void*)aAlbedoTextureDimensions.data(),
            0,
            uint32_t(aAlbedoTextureDimensions.size() * sizeof(uint2))
        );

        uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
        auto& pNormalTextureDimensionBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapUniformBuffers["Normal Texture Sizes"];
        pRenderer->copyCPUToBuffer(
            pNormalTextureDimensionBuffer,
            (void *)aNormalTextureDimensions.data(),
            0,
            uint32_t(aNormalTextureDimensions.size() * sizeof(uint2)),
            iFlags
        );

        pNormalTextureDimensionBuffer = pRenderer->mapRenderJobs["Texture Atlas Graphics"]->mapUniformBuffers["Normal Texture Sizes"];
        pRenderer->copyCPUToBuffer(
            pNormalTextureDimensionBuffer,
            (void*)aNormalTextureDimensions.data(),
            0,
            uint32_t(aNormalTextureDimensions.size() * sizeof(uint2)),
            iFlags
        );

        auto& pMaterialBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapUniformBuffers["materials"];
        pRenderer->copyCPUToBuffer(
            pMaterialBuffer,
            (void *)acMaterialBuffer.data(),
            0,
            (uint32_t)acMaterialBuffer.size()
        );

        // inital texture pages
        uint32_t const iInitialAtlasSize = 512;
        uint32_t const iInitialPageSize = 16;
        constexpr uint32_t iNumPagesPerRow = iInitialAtlasSize / iInitialPageSize;
        uint32_t iPage = 0;
        for(auto const& textureName : aAlbedoTextureNames)
        {
            std::string fullPath = "d:\\Downloads\\Bistro_v4\\converted-dds-scaled\\" + textureName;
            int32_t iWidth = 0, iHeight = 0, iNumChannels = 0;
            stbi_uc* pImageData = stbi_load(
                fullPath.c_str(),
                &iWidth,
                &iHeight,
                &iNumChannels,
                4
            );
            
            std::vector<unsigned char> acScaledImage(iInitialPageSize * iInitialPageSize * 4);
            //if(iWidth < iPageSize || iHeight < iPageSize)
            {
                float fScaleX = (float)iWidth / (float)iInitialPageSize;
                float fScaleY = (float)iHeight / (float)iInitialPageSize;

                float fY = 0.0f;
                for(uint32_t iY = 0; iY < iInitialPageSize; iY++)
                {
                    uint32_t iSampleY = uint32_t(fY);
                    fY += fScaleY;
                    float fX = 0.0f;
                    for(uint32_t iX = 0; iX < iInitialPageSize; iX++)
                    {
                        uint32_t iSampleX = uint32_t(fX);
                        fX += fScaleX;

                        uint32_t iSampleIndex = (iSampleY * iWidth + iSampleX) * 4;
                        uint32_t iIndex = (iY * iInitialPageSize + iX) * 4;
                        acScaledImage[iIndex] = pImageData[iSampleIndex];
                        acScaledImage[iIndex+1] = pImageData[iSampleIndex+1];
                        acScaledImage[iIndex+2] = pImageData[iSampleIndex+2];
                        acScaledImage[iIndex+3] = pImageData[iSampleIndex+3];

                    }
                }
                pImageData = acScaledImage.data();
            }

            uint32_t iPageX = iPage % iNumPagesPerRow;
            uint32_t iPageY = iPage / iNumPagesPerRow;
            uint2 pageCoord(iPageX, iPageY);
            auto & textureAtlas3 = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputImageAttachments["Initial Texture Atlas"];
            pRenderer->copyTexturePageToAtlas(
                (char const*)pImageData,
                textureAtlas3,
                pageCoord,
                iInitialPageSize
            );

            ++iPage;
        }
    };

    float3 diff = normalize(sInitialCameraLookAt - sInitialCameraPosition);
    sInitialCameraLookAt = diff * 0.25f + sInitialCameraPosition;

    sCameraPosition = sInitialCameraPosition;
    sCameraLookAt = sInitialCameraLookAt;

    if(strstr(pCommandLine, "--render=vulkan"))
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        static_cast<Render::Vulkan::CRenderer*>(pRenderer)->initImgui();
    }

    pRenderer->initData();
    pRenderer->loadRenderJobInfo(pDesc->mRenderJobsFilePath);
    pRenderer->prepareRenderJobData();

    {
        for(auto const& renderJob : gpRenderer->mapRenderJobs)
        {
            if(renderJob.first == "Swap Chain Graphics"/* || renderJob.first == "Copy Render Targets"*/)
            {
                continue;
            }

            for(auto const& outputAttachment : renderJob.second->mapOutputImageAttachments)
            {
                if(outputAttachment.second != nullptr)
                {
                    gaRenderTargetNames.push_back(std::make_pair(outputAttachment.first, renderJob.first));
                    giNumOutputRenderTargets += 1;
                }
            }
        }

        int iDebug = 1;
    }

    ShowWindow(window, iCommandShow);

#if defined(USE_IMGUI)
    if(strstr(pCommandLine, "--render=vulkan"))
    {
        ImGui_ImplWin32_Init(window);

        
        spImguiIO = &ImGui::GetIO();
    }
    else
    {
        initImgui(
            window,
            reinterpret_cast<ID3D12Device*>(pRenderer->getDevice()->getNativeDevice()),
            reinterpret_cast<ID3D12DescriptorHeap*>(pRenderer->getImguiDescriptorHeap()->getNativeDescriptorHeap()),
            SerializeUtils::DX12::convert(pDesc->mFormat),
            pDesc->miNumBuffers);
    }
#endif // USE_IMGUI

    bool bStarted = false;
    std::unique_ptr<std::thread> texturePageLoadingThread;

    uint32_t const iNumThreads = 4;
    std::unique_ptr<std::thread> aThreads[4];

    std::unique_ptr<RenderDriver::Common::CCommandQueue> threadCommandQueue;
    pRenderer->createCommandQueue(
        threadCommandQueue,
        RenderDriver::Common::CCommandQueue::Type::CopyGPU
    );
    threadCommandQueue->setID("Thread Command Queue");

    auto& pTexturePageQueueBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Texture Page Queue MIP"];
    uint32_t iBufferSize = (uint32_t)pTexturePageQueueBuffer->getDescriptor().miSize;
    std::vector<char> acTexturePageQueueData(iBufferSize);
    //std::unique_ptr<RenderDriver::Common::CBuffer> texturePageQueueReadBackBuffer;
    //pRenderer->createBuffer(
    //    texturePageQueueReadBackBuffer,
    //    iBufferSize
    //);

    auto& texturePageInfoBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];
    iBufferSize = (uint32_t)texturePageInfoBuffer->getDescriptor().miSize;
    std::vector<char> acTexturePageInfoData(iBufferSize);
    //std::unique_ptr<RenderDriver::Common::CBuffer> texturePageHashTableReadBackBuffer;
    //pRenderer->createBuffer(
    //    texturePageHashTableReadBackBuffer,
    //    iBufferSize
    //);

    // get the number of pages
    auto& pTexturePageCountBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Counters"];
    std::vector<char> acCounterData(256);
    //std::unique_ptr<RenderDriver::Common::CBuffer> counterReadBackBuffer;
    //pRenderer->createBuffer(
    //    counterReadBackBuffer,
    //    256
    //);

    uint32_t iNumFinished = 0;
    bool bStartLoad = false;
    std::mutex texturePageThreadMutex;

    std::condition_variable conditionVariable;

    std::vector<std::pair<uint32_t, uint32_t>>      aiStartAndNumChecks(iNumThreads);

    // post draw, get the texture pages needed
    pRenderer->mpfnPostDraw = std::make_unique<std::function<void(Render::Common::CRenderer*)>>();
    *pRenderer->mpfnPostDraw = [
        aAlbedoTextureNames,
            aNormalTextureNames,
            aAlbedoTextureDimensions,
            aNormalTextureDimensions,
            &iCurrTotalPageLoaded,
            &iCurrAlbedoPageLoaded,
            &iCurrNormalPageLoaded,
            &aPageInfo,
            &bStarted,
            &texturePageLoadingThread,
            &aThreads,
            &threadCommandQueue,
            iNumThreads,
            &iNumFinished,
            &texturePageThreadMutex,
            &acTexturePageQueueData,
            &acTexturePageInfoData,
            &acCounterData,
            &bStartLoad,
            &aiStartAndNumChecks,
            &conditionVariable
    ](Render::Common::CRenderer* pRenderer)
    {
        if(bStarted == false)
        {
            uint32_t iNumChecksPerThread = 4000 / iNumThreads;
            for(uint32_t iThread = 0; iThread < iNumThreads; iThread++)
            {
                aThreads[iThread] = std::make_unique<std::thread>(
                    [pRenderer,
                    aAlbedoTextureNames,
                    aNormalTextureNames,
                    aAlbedoTextureDimensions,
                    aNormalTextureDimensions,
                    &iCurrTotalPageLoaded,
                    &iCurrAlbedoPageLoaded,
                    &iCurrNormalPageLoaded,
                    &aPageInfo,
                    &threadCommandQueue,
                    iThread,
                    iNumThreads,
                    &iNumChecksPerThread,
                    &iNumFinished,
                    &texturePageThreadMutex,
                    &acTexturePageQueueData,
                    &acTexturePageInfoData,
                    &acCounterData,
                    &bStartLoad,
                    &aiStartAndNumChecks,
                    &conditionVariable
                    ]()
                {
                    std::unique_ptr<RenderDriver::Common::CCommandAllocator> threadCommandAllocator;
                    std::unique_ptr<RenderDriver::Common::CCommandBuffer> threadCommandBuffer;

                    pRenderer->createCommandBuffer(
                        threadCommandAllocator,
                        threadCommandBuffer
                    );

                    RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc = {};
                    commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    threadCommandAllocator->create(
                        commandAllocatorDesc,
                        *pRenderer->getDevice()
                    );
                    threadCommandAllocator->setID("Texture Page Thread Command Allocator");

                    RenderDriver::Common::CommandBufferDescriptor commandBufferDesc = {};
                    commandBufferDesc.mpCommandAllocator = threadCommandAllocator.get();
                    commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    threadCommandBuffer->create(
                        commandBufferDesc,
                        *pRenderer->getDevice()
                    );
                    threadCommandBuffer->setID("Texture Page Thread Command Buffer");

                    std::unique_ptr<RenderDriver::Common::CBuffer> texturePageUploadBuffer;
                    pRenderer->createBuffer(
                        texturePageUploadBuffer,
                        64 * 64 * 4
                    );
                    texturePageUploadBuffer->setID("Texture Page Thread Command Buffer");

                    void* pUploadBuffer = texturePageUploadBuffer->getMemoryOpen(64 * 64 * 4);

                    uint32_t iLastCounterValue = 0;

                    uint32_t iStartIndex = iThread * iNumChecksPerThread;
                    uint32_t iNumChecksPerLoop = 100;
                    std::map<uint32_t, ImageInfo> aImageInfo;

                    std::unique_ptr<RenderDriver::Common::CBuffer> threadUploadBuffer;
                    pRenderer->createBuffer(threadUploadBuffer, 256);

                    for(;;)
                    {
                        if(gbQuit)
                        {
                            conditionVariable.notify_all();
                            break;
                        }

                        // wait or caldulate thread start and end indices
                        if(iThread == 0)
                        {
                            // get the page queue, page info, and calculate the each thread's start index and num pages in queue to check 

                            std::lock_guard<std::mutex> lock(texturePageThreadMutex);

                            // get the texture pages needed
                            auto& pTexturePageQueueBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Texture Page Queue MIP"];
                            uint32_t iBufferSize = (uint32_t)pTexturePageQueueBuffer->getDescriptor().miSize;
                            WTFASSERT(iBufferSize <= acTexturePageQueueData.size(), "Need larger buffer");
                            pRenderer->copyBufferToCPUMemory2(
                                pTexturePageQueueBuffer,
                                acTexturePageQueueData.data(),
                                0,
                                iBufferSize,
                                *threadCommandBuffer,
                                *threadCommandQueue
                            );

                            auto& texturePageInfoBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];
                            iBufferSize = (uint32_t)texturePageInfoBuffer->getDescriptor().miSize;
                            WTFASSERT(iBufferSize <= acTexturePageInfoData.size(), "Need larger buffer");
                            pRenderer->copyBufferToCPUMemory2(
                                texturePageInfoBuffer,
                                acTexturePageInfoData.data(),
                                0,
                                iBufferSize,
                                *threadCommandBuffer,
                                *threadCommandQueue
                            );

                            auto& pTexturePageCountBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Counters"];
                            WTFASSERT(256 <= acCounterData.size(), "Need larger buffer");
                            pRenderer->copyBufferToCPUMemory2(
                                pTexturePageCountBuffer,
                                acCounterData.data(),
                                0,
                                256,
                                *threadCommandBuffer,
                                *threadCommandQueue
                            );

                            // get the size of the queue and compute the start and end indices to check for each threads
                            uint32_t iNumChecks = min(*((uint32_t*)acCounterData.data()), 65535);
                            iNumChecksPerThread = (uint32_t)ceil((float)iNumChecks / (float)iNumThreads);
                            for(uint32_t i = 0; i < iNumThreads; i++)
                            {
                                aiStartAndNumChecks[i] = std::make_pair(i * iNumChecksPerThread, iNumChecksPerThread);
                            }

                            // wake up all other threads
                            bStartLoad = true;
                            
                        }
                        else
                        {
                            // wait for thread 0 to compute the start and end indices to check in the queue
                            while(!bStartLoad)
                            {
                                std::this_thread::sleep_for(std::chrono::microseconds(10));
                            }
                            
                        }

                        uint32_t iStartThreadIndex = aiStartAndNumChecks[iThread].first;
                        uint32_t iNumChecksCopy = aiStartAndNumChecks[iThread].second;

                        WTFASSERT(iStartThreadIndex < 1000000, "wtf");
                        WTFASSERT(iNumChecksCopy < 10000000, "wtf");

                        loadTexturePageThread(
                            pRenderer,
                            aAlbedoTextureNames,
                            aAlbedoTextureDimensions,
                            aNormalTextureNames,
                            aNormalTextureDimensions,
                            iCurrTotalPageLoaded,
                            iCurrAlbedoPageLoaded,
                            iCurrNormalPageLoaded,
                            //aPageInfo,
                            *threadCommandBuffer,
                            *threadCommandQueue,
                            *threadUploadBuffer,
                            iStartThreadIndex,
                            iNumChecksCopy,
                            //aImageInfo,
                            *texturePageUploadBuffer,
                            pUploadBuffer,
                            iLastCounterValue,
                            acTexturePageQueueData,
                            acTexturePageInfoData,
                            threadMutex
                        );
                        WTFASSERT(iNumChecksCopy < 100000, "wtf");

                        {
                            std::lock_guard<std::mutex> lock(texturePageThreadMutex);
                            iNumFinished += 1;
                        }

                        // wait for all the threads to finish
                        if(iThread == 0)
                        {
                            while(iNumFinished != iNumThreads)
                            {
                                if(gbQuit)
                                {
                                    break;
                                }
                                std::this_thread::sleep_for(std::chrono::microseconds(10));
                            }

                            // re-initialize and wake up all the other threads
                            iNumFinished = 0;
                            bStartLoad = false;
                            conditionVariable.notify_all();
                        }
                        else
                        {
                            std::unique_lock<std::mutex> uniqueLock(texturePageThreadMutex);
                            if(iNumFinished != 0)
                            {
                                conditionVariable.wait(uniqueLock);
                            }
                        }
                    }

                    texturePageUploadBuffer->releaseNativeBuffer();
                    texturePageUploadBuffer.reset();
                    texturePageUploadBuffer = nullptr;
                }
                );

                DEBUG_PRINTF("Thread %d id = %d\n", iThread, aThreads[iThread]->get_id());

            }   // for thread = 0 to num threads
            bStarted = true;
        }

        if(gbQuit)
        {
            for(uint32_t iThread = 0; iThread < 8; iThread++)
            {
                aThreads[iThread]->join();
            }
        }
    };

    
    auto lastTime = std::chrono::high_resolution_clock::now();
    uint64_t iCurrTimeMS = 0;
    
    MSG msg = {};
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            continue;
        }

auto start = std::chrono::high_resolution_clock::now();
#if defined(USE_IMGUI)
        // imgui begin frame
        {
            if(sRenderDriverType == Render::Common::RenderDriverType::DX12)
            {
                ImGui_ImplDX12_NewFrame();
            }
            else if(sRenderDriverType == Render::Common::RenderDriverType::Vulkan)
            {
                ImGui_ImplVulkan_NewFrame();
            }

            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }
#endif // USE_IMGUI

#if defined(USE_IMGUI)
        // imgui layout
        pRenderer->beginDebugMarker("ImGUI Layout");
        imguiLayout(pRenderer);
        pRenderer->endDebugMarker();
#endif // USE_IMGUI
       
        float3 up = float3(0.0f, 1.0f, 0.0f);
        float3 diff = normalize(sCameraLookAt - sCameraPosition);
        if(fabs(diff.y) > 0.98f)
        {
            up = float3(1.0f, 0.0f, 0.0f);
        }

        float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
        float3 tangent = cross(up, viewDir);
        float3 binormal = cross(tangent, viewDir);

        //sCameraPosition = sCameraPosition + tangent * 0.003f;
        //sCameraLookAt = sCameraLookAt + tangent * 0.003f;

        Render::Common::UpdateCameraDescriptor cameraDesc = {};
        cameraDesc.mfFar = 100.0f;
        cameraDesc.mfNear = 0.4f;
        cameraDesc.miCameraIndex = 0;
        cameraDesc.mUp = up;
        cameraDesc.mPosition = sCameraPosition;
        cameraDesc.mLookAt = sCameraLookAt;
        cameraDesc.mfFov = 3.14159f * 0.5f;

        pRenderer->updateCamera(cameraDesc);
        pRenderer->updateRenderJobData();
        pRenderer->draw();
        pRenderer->postDraw();
    }

    // signal quit to threads
    gbQuit = true;
    for(uint32_t iThread = 0; iThread < iNumThreads; iThread++)
    {
        if(aThreads[iThread]->joinable())
        {
            aThreads[iThread]->join();
        }
    }

    return 0;
}

/*
**
*/
HWND _initWindow(
    HINSTANCE hInstance,
    WNDPROC windowProc,
    uint32_t iWidth,
    uint32_t iHeight)
{
    HWND windowHandle = 0;
    WNDCLASSEX windowClass = { 0 };

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = (sRenderDriverType == Render::Common::RenderDriverType::DX12) ? L"RenderWithMe (DX12)" : L"RenderWithMe (Vulkan)";

    auto result = RegisterClassEx(&windowClass);
    assert(result);

    int iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    DWORD dwExStyle, dwStyle;
    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    windowHandle = CreateWindow(
        windowClass.lpszClassName,
        windowClass.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    assert(windowHandle);

    return windowHandle;
}

/*
**
*/
void handleCameraMousePan(
    int32_t iX,
    int32_t iY,
    int32_t iLastX,
    int32_t iLastY)
{
    float const fSpeed = 0.01f;

    float fDiffX = float(iX - iLastX);
    float fDiffY = float(iY - iLastY);

    float3 viewDir = sCameraLookAt - sCameraPosition;
    float3 normalizedViewDir = normalize(viewDir);

    float3 tangent = cross(sCameraUp, normalizedViewDir);
    float3 binormal = cross(tangent, normalizedViewDir);

    sCameraPosition = sCameraPosition + binormal * -fDiffY * fSpeed + tangent * -fDiffX * fSpeed;
    sCameraLookAt = sCameraLookAt + binormal * -fDiffY * fSpeed + tangent * -fDiffX * fSpeed;
}

/*
**
*/
void handleCameraMouseRotate(
    int32_t iX,
    int32_t iY,
    int32_t iLastX,
    int32_t iLastY)
{
    if(siLastX < 0)
    {
        siLastX = iX;
    }

    if(siLastY < 0)
    {
        siLastY = iY;
    }

    float fDiffX = float(iX - siLastX) * -1.0f;
    float fDiffY = float(iY - siLastY);

    float fRotationSpeed = 0.3f;
    float fDeltaX = (2.0f * 3.14159f) / 512.0f;
    float fDeltaY = (2.0f * 3.14159f) / 512.0f;

    sCameraAngle.y += fDiffX * fRotationSpeed * fDeltaY;
    sCameraAngle.x += fDiffY * fRotationSpeed * fDeltaX;

    if(sCameraAngle.y < 0.0f)
    {
        sCameraAngle.y = 2.0f * 3.14159f + sCameraAngle.y;
    }
    if(sCameraAngle.y > 2.0f * 3.14159f)
    {
        sCameraAngle.y = sCameraAngle.y - 2.0f * 3.14159f;
    }

    if(sCameraAngle.x < -PI * 0.5f)
    {
        sCameraAngle.x = -PI * 0.5f;
    }
    if(sCameraAngle.x > PI * 0.5f)
    {
        sCameraAngle.x = PI * 0.5f;
    }


    float4x4 rotateX = rotateMatrixX(sCameraAngle.x);
    float4x4 rotateY = rotateMatrixY(sCameraAngle.y);
    float4x4 totalMatrix = rotateY * rotateX;

    float3 diff = sInitialCameraPosition - sInitialCameraLookAt;
    
    float4 xformEyePosition = totalMatrix * float4(diff, 1.0f);
    xformEyePosition.x += sCameraLookAt.x;
    xformEyePosition.y += sCameraLookAt.y;
    xformEyePosition.z += sCameraLookAt.z;
    sCameraPosition = xformEyePosition;

    siLastX = iX;
    siLastY = iY;
}

static float3 sSunDirection(0.0f, 1.0f, 0.0f);
static float3 sSunRotation(0.0f, 0.0f, 0.0f);

static float2 sSunAngle(0.0f, 0.0f);
static int32_t siLastSunDirectionX = -1;
static int32_t siLastSunDirectionY = -1;

/*
**
*/
void handleRotateSunDirection(
    int32_t iX,
    int32_t iY,
    int32_t iLastX,
    int32_t iLastY)
{
    float const fSpeed = 0.01f;

    if(siLastSunDirectionX < 0)
    {
        siLastSunDirectionX = iX;
    }
    if(siLastSunDirectionY < 0)
    {
        siLastSunDirectionY = iY;
    }

    float fDiffX = float(iX - siLastSunDirectionX);
    float fDiffY = float(iY - siLastSunDirectionY);

    sSunAngle.x += fDiffY * fSpeed;
    sSunAngle.y += fDiffX * fSpeed;

    if(sSunAngle.y < -PI * 0.5f)
    {
        sSunAngle.y = -PI * 0.5f;
    }
    if(sSunAngle.y > PI * 0.5f)
    {
        sSunAngle.y = PI * 0.5f;
    }

    if(sSunAngle.x < -PI * 0.5f)
    {
        sSunAngle.x = -PI * 0.5f;
    }
    if(sSunAngle.x > PI * 0.5f)
    {
        sSunAngle.x = PI * 0.5f;
    }

    float4x4 rotateX = rotateMatrixX(sSunAngle.x);
    float4x4 rotateY = rotateMatrixY(sSunAngle.y);
    float4x4 rotationMatrix = rotateY * rotateX;
    Render::Common::gLightDirection = normalize(mul(float4(0.0f, 1.0f, 0.0f, 1.0f), rotationMatrix));

    siLastSunDirectionX = iX;
    siLastSunDirectionY = iY;

}

#if defined(USE_IMGUI)
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI

/*
**
*/
LRESULT CALLBACK _windowProc(
    HWND windowHandle,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam)
{
#if defined(USE_IMGUI)
    if(ImGui_ImplWin32_WndProcHandler(windowHandle, uMessage, wParam, lParam))
    {
        return true;
    }
#endif // USE_IMGUI

    switch(uMessage)
    {

    case WM_CLOSE:
        DestroyWindow(windowHandle);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        break;
    };

#if defined(USE_IMGUI)
    if(spImguiIO)
    {
        if(spImguiIO->WantCaptureKeyboard || spImguiIO->WantCaptureMouse)
        {
            return true;
        }
    }
#endif // USE_IMGUI


    switch(uMessage)
    {
    case WM_MBUTTONDOWN:
        GetCursorPos(&sMiddleClickPos);
        ScreenToClient(windowHandle, &sMiddleClickPos);
        sLastPt = sMiddleClickPos;
        sbMiddleMouseDown = true;
        break;

    case WM_LBUTTONUP:
        sbLeftMouseDown = false;
        siLastX = -1;
        siLastY = -1;
        break;

    case WM_MBUTTONUP:
        sbMiddleMouseDown = false;
        siLastSunDirectionX = -1;
        siLastSunDirectionY = -1;
        break;

    case WM_RBUTTONUP:
        sbRightMouseDown = false;
        break;

    case WM_LBUTTONDOWN:
    {
        //GetCursorPos(&sMouseClickPos);
        //ScreenToClient(windowHandle, &sMouseClickPos);
        //DEBUG_PRINTF("mouse (%d, %d)\n", sMouseClickPos.x, sMouseClickPos.y);
        sbLeftMouseDown = true;
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(windowHandle, &pt);

        sLastLeftMousePt = pt;
        
        break;
    }
    case WM_RBUTTONDOWN:
    {
        sbRightMouseDown = true;
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(windowHandle, &pt);

        sLastRightMousePt = pt;
        break;
    }
    case WM_MOUSEMOVE:
    {
        if(sbMiddleMouseDown)
        {
            SetCapture(windowHandle);

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(windowHandle, &pt);
            handleRotateSunDirection(pt.x, pt.y, sLastLeftMousePt.x, sLastLeftMousePt.y);

            sLastPt = pt;
        }
        else if(sbRightMouseDown)
        {
            SetCapture(windowHandle);

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(windowHandle, &pt);
            handleCameraMousePan(pt.x, pt.y, sLastRightMousePt.x, sLastRightMousePt.y);
            sLastRightMousePt = pt;
        }
        else if(sbLeftMouseDown)
        {
            SetCapture(windowHandle);

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(windowHandle, &pt);
            handleCameraMouseRotate(pt.x, pt.y, sLastPt.x, sLastPt.y);
            sLastLeftMousePt = pt;
        }
        break;
    }
    case WM_MOUSEWHEEL:
    {
        int32_t iDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if(iDelta < 0)
        {
            float const fSpeed = 0.1f;
            float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
            sCameraPosition += viewDir * -fSpeed;
            sCameraLookAt += viewDir * -fSpeed;

            DEBUG_PRINTF("delta forward: %d\n", iDelta);
        }
        else if(iDelta > 0)
        {
            float const fSpeed = 0.1f;
            float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
            sCameraPosition += viewDir * fSpeed;
            sCameraLookAt += viewDir * fSpeed;
            
            DEBUG_PRINTF("delta backward: %d\n", iDelta);
        }

        break;
    }
    case WM_KEYDOWN:
        switch(wParam)
        {
            case VK_F1:
            {
                float4x4 const& viewMatrix = Render::Common::gaCameras[0].getViewMatrix();
                float4x4 const& projectionMatrix = Render::Common::gaCameras[0].getProjectionMatrix();

                std::vector<char> acBuffer0(1 << 16);
                memset(acBuffer0.data(), 0, acBuffer0.size());
                sprintf(acBuffer0.data(), "float viewMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};\n",
                    viewMatrix._m00, viewMatrix._m01, viewMatrix._m02, viewMatrix._m03,
                    viewMatrix._m10, viewMatrix._m11, viewMatrix._m12, viewMatrix._m13,
                    viewMatrix._m20, viewMatrix._m21, viewMatrix._m22, viewMatrix._m23,
                    viewMatrix._m30, viewMatrix._m31, viewMatrix._m32, viewMatrix._m33);

                std::vector<char> acBuffer1(1 << 16);
                memset(acBuffer1.data(), 0, acBuffer1.size());
                sprintf(acBuffer1.data(), "float projectionMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};",
                    projectionMatrix._m00, projectionMatrix._m01, projectionMatrix._m02, projectionMatrix._m03,
                    projectionMatrix._m10, projectionMatrix._m11, projectionMatrix._m12, projectionMatrix._m13,
                    projectionMatrix._m20, projectionMatrix._m21, projectionMatrix._m22, projectionMatrix._m23,
                    projectionMatrix._m30, projectionMatrix._m31, projectionMatrix._m32, projectionMatrix._m33);

                std::string totalContent = std::string(acBuffer0.data()) + std::string(acBuffer1.data());
                FILE* fp = fopen("c:\\Users\\Dingwings\\deferred-screenshots-3\\matrices.txt", "w");
                fwrite(totalContent.c_str(), sizeof(char), totalContent.length(), fp);
                fclose(fp);

                DEBUG_PRINTF("float viewMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};\n",
                    viewMatrix._m00, viewMatrix._m01, viewMatrix._m02, viewMatrix._m03,
                    viewMatrix._m10, viewMatrix._m11, viewMatrix._m12, viewMatrix._m13,
                    viewMatrix._m20, viewMatrix._m21, viewMatrix._m22, viewMatrix._m23,
                    viewMatrix._m30, viewMatrix._m31, viewMatrix._m32, viewMatrix._m33);

                DEBUG_PRINTF("float projectionMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};",
                    projectionMatrix._m00, projectionMatrix._m01, projectionMatrix._m02, projectionMatrix._m03,
                    projectionMatrix._m10, projectionMatrix._m11, projectionMatrix._m12, projectionMatrix._m13,
                    projectionMatrix._m20, projectionMatrix._m21, projectionMatrix._m22, projectionMatrix._m23,
                    projectionMatrix._m30, projectionMatrix._m31, projectionMatrix._m32, projectionMatrix._m33);

                break;
            }

            case VK_F2:
            {
                break;
            }

            case VK_UP:
            {
                break;
            }

            case VK_DOWN:
            {
                break;
            }

            case VK_LEFT:
            {
                break;
            }

            case VK_RIGHT:
            {
                
                break;
            }

            case VK_F11:
            {
                break;
            }
        }
    case WM_CHAR:
        switch(wParam)
        {
            case 'w':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                sCameraPosition += viewDir * fSpeed;
                sCameraLookAt += viewDir * fSpeed;

                break;
            }

            case 's':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                sCameraPosition += viewDir * -fSpeed;
                sCameraLookAt += viewDir * -fSpeed;

                break;
            }

            case 'a':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * -fSpeed;
                sCameraLookAt += tangent * -fSpeed;

                break;
            }

            case 'd':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                break;
            }

            case '-':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfCameraNear -= 0.05f;

                break;
            }

            case '=':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfCameraNear += 0.05f;

                break;
            }

            case '9':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfFOV -= 0.01f;

                break;
            }

            case '0':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfFOV += 0.01f;

                break;
            }

            case ',':
            {
                Render::Common::CRenderJob* pSwapChainRenderJob = gpRenderer->mapRenderJobs["Swap Chain Graphics"];

                giCurrRenderJobOutput = ((giCurrRenderJobOutput - 1) + giNumOutputRenderTargets) % giNumOutputRenderTargets;
                auto const& renderJobNameInfo = gaRenderTargetNames[giCurrRenderJobOutput];
                
                gpRenderer->setAttachmentImage(
                    "Swap Chain Graphics",
                    "Output",
                    renderJobNameInfo.second,
                    renderJobNameInfo.first
                );

                DEBUG_PRINTF("swap chain output: \"%s\" orig render job: \"%s\"\n",
                    renderJobNameInfo.first.c_str(),
                    renderJobNameInfo.second.c_str());

                break;
            }

            case '.':
            {
                Render::Common::CRenderJob* pSwapChainRenderJob = gpRenderer->mapRenderJobs["Swap Chain Graphics"];

                giCurrRenderJobOutput = (giCurrRenderJobOutput + 1) % giNumOutputRenderTargets;
                auto const& renderJobNameInfo = gaRenderTargetNames[giCurrRenderJobOutput];

                gpRenderer->setAttachmentImage(
                    "Swap Chain Graphics",
                    "Output",
                    renderJobNameInfo.second,
                    renderJobNameInfo.first
                );

                DEBUG_PRINTF("swap chain output: \"%s\" orig render job: \"%s\"\n",
                    renderJobNameInfo.first.c_str(),
                    renderJobNameInfo.second.c_str());

                break;
            }

            case '/':
            {
                sSunColor += float3(-0.1f, -0.1f, -0.1f);
                break;
            }

            case '*':
            {
                sSunColor += float3(0.1f, 0.1f, 0.1f);
                break;
            }

            case '1':
            {
                break;
            }

            case '2':
            {
                break;
            }

            case '3':
            {
                break;
            }
        }
    }

    return DefWindowProc(windowHandle, uMessage, wParam, lParam);
}

#if defined(USE_IMGUI)
/*
**
*/
void initImgui(
    HWND hWnd,
    ID3D12Device* pDevice,
    ID3D12DescriptorHeap* pShaderResourceHeap,
    DXGI_FORMAT outputFormat,
    uint32_t iNumFramesInFlight)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    spImguiIO = &ImGui::GetIO();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX12_Init(
        pDevice,
        iNumFramesInFlight,
        outputFormat,
        pShaderResourceHeap,
        pShaderResourceHeap->GetCPUDescriptorHandleForHeapStart(),
        pShaderResourceHeap->GetGPUDescriptorHandleForHeapStart());
}

#endif // USE_IMGUI

static int32_t siSelectedOutputAttachment = 0;
static uint32_t siRestirType = 1;
static float sfWorldSpaceBlurRadius = 0.5f;
static float sfLastWorldSpaceBlurRadius = 0.5f;
static int32_t siWorldSpaceBlurSamples = 16;
static int32_t siLastWorldSpaceBlurSamples = 16;
static float sfSunIntensity = 15.0f;
static float sfLastSunIntensity = 15.0f;
static int32_t siSelectedOutputAttachmentRenderJob = 0;
static int32_t siSelectedInputAttachmentRenderJob = 0;
static float sfSpatialRestirRadius0 = 32.0f;
static float sfSpatialRestirRadius1 = 16.0f;
static float sfLastSpatialRestirRadius0 = 2.0f;
static float sfLastSpatialRestirRadius1 = 1.0f;
static int32_t siSpatialRestirSampleCount0 = 8;
static int32_t siSpatialRestirSampleCount1 = 6;
static int32_t siLastSpatialRestirSampleCount0 = 8;
static int32_t siLastSpatialRestirSampleCount1 = 6;
static uint64_t siSelectedOutputAttachmentTexturePtr = UINT64_MAX;
static uint64_t siSelectedInputAttachmentTexturePtr = UINT64_MAX;
static bool sbShowDebugOptionsUI = false;
static bool sbShowOutputAttachmentUI = false;
static bool sbShowInputAttachmentUI = false;
static bool sbShowRenderJobStatsUI = false;
static float sfBilateralBlurRadius = 2.0f;
static float sfBilateralBlurScale = 2.0f;
static float sfBilateralBlurColorSensitivty = 30.0f;
static float sfLastBilateralBlurRadius = 2.0f;
static float sfLastBilateralBlurScale = 2.0f;
static float sfLastBilateralBlurColorSensitivty = 30.0f;
static float sfAmbientOcclusionBlurRadius = 1.0f;
static float sfLastAmbientOcclusionBlurRadius = 1.0f;
static float sfAmbientOcclusionMaxDistance = 1.5f;
static float sfLastAmbientOcclusionMaxDistance = 1.5f;
static float sfMaterialRoughness = 0.5f;
static float sfLastMaterialRoughness = 0.5f;
static float sfMaterialMetalness = 0.0f;
static float sfLastMaterialMetalness = 0.0f;
static float sfUpscaleRadius = 1.0f;
static float sfLastUpscaleRadius = 1.0f;
static float3 sLocalLightColor = float3(5.0f, 2.5f, 0.3f);
static float sfLocalLightIntensity = 0.0f;
static float sfLastLocalLightIntensity = 5.0f;

static std::map<uint64_t, VkDescriptorSet>  saImageDescriptorSets;

/*
**
*/
void outputAttachmentLayout(Render::Common::CRenderer* pRenderer)
{
#if 0
    Render::Common::Serializer* pSerializer = pRenderer->getSerializer();
    auto const& aRenderJobs = pSerializer->getRenderJobs();

    ImGui::Begin("Render Job Output Attachments");
    {
        if(ImGui::ListBoxHeader("Render Jobs"))
        {
            std::vector<char[256]> aRenderJobNames(aRenderJobs.size());
            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                strcpy(aRenderJobNames[i], aRenderJobs[i].mName.c_str());
            }

            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                bool bSelected = (i == siSelectedOutputAttachmentRenderJob) ? true : false;
                if(ImGui::Selectable(aRenderJobNames.data()[i], &bSelected))
                {
                    siSelectedOutputAttachmentRenderJob = i;
                }
            }
            ImGui::ListBoxFooter();
        }
    }
    ImGui::End();

    // output attachment images
    std::string outputAttachmentName = "";
    std::string outputAttachmentTitle = "Output Attachments (" + aRenderJobs[siSelectedOutputAttachmentRenderJob].mName + ")";
    PLATFORM_OBJECT_HANDLE renderTargetHandle = 0;
    ImGui::Begin(outputAttachmentTitle.c_str());
    {
        ImGui::Text(aRenderJobs[siSelectedOutputAttachmentRenderJob].mName.c_str());

        auto& renderJob = pSerializer->getRenderJob(aRenderJobs[siSelectedOutputAttachmentRenderJob].mName);
        renderTargetHandle = 0;
        uint32_t iNumAttachments = min(static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size()), 32);
        for(uint32_t iOutputAttachment = 0; iOutputAttachment < iNumAttachments; iOutputAttachment++)
        {
            if(renderJob.maOutputRenderTargetAttachments[iOutputAttachment] <= 0)
            {
                continue;
            }

            RenderDriver::Common::CImage* pImage = nullptr;
            if(renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mType == Render::Common::AttachmentType::TextureOut ||
                renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mType == Render::Common::AttachmentType::TextureInOut)
            {
                bool bIsImage = pSerializer->isImage(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]);
                if(bIsImage)
                {
                    pImage = pSerializer->getImage(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]).get();
                }
                else
                {
                    auto& renderTarget = pSerializer->getRenderTarget(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]);
                    pImage = renderTarget->getImage().get();
                }
            }

            if(pImage)
            {
                uint64_t iTexturePtr = 0;
                if(pRenderer->getRenderDriverType() == Render::Common::RenderDriverType::Vulkan)
                {
                    // descriptor_set = ImGui_ImplVulkan_AddTexture(...), need image view for image
                    // 
                    // equivalent to:
                    // fill out VkWriteDescriptorSet descriptorWrite = {};
                    // fill out VkDescriptorImageInfo -> sampler,  imageView, imageLayout
                    // call vkUpdateDescriptorSet
                
                    VkImage& nativeImage = *(static_cast<VkImage*>(pImage->getNativeImage()));
                    uint64_t iID = reinterpret_cast<uint64_t>(nativeImage);
                    if(saImageDescriptorSets.find(iID) == saImageDescriptorSets.end())
                    {
                        // first time creation of the image view and descriptor set used as ImTextureID 

                        Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(pRenderer);

                        // create image view for AddTexture()
                        std::unique_ptr<RenderDriver::Vulkan::CImageView> imageView = std::make_unique<RenderDriver::Vulkan::CImageView>();
                        RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
                        imageViewDesc.mpImage = pImage;
                        imageViewDesc.mFormat = pImage->getDescriptor().mFormat;
                        imageViewDesc.miNumImages = 1;
                        PLATFORM_OBJECT_HANDLE handle = imageView->create(imageViewDesc, *(pRenderer->getDevice()));

                        //Render::Vulkan::Serializer* pSerializer = static_cast<Render::Vulkan::Serializer*>(pRenderer->getSerializer());
                        //RenderDriver::Vulkan::CImageView* pImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pSerializer->registerObject(imageView, handle));

                        // descriptor set for the image, use as input for texture id used in ImageButton
                        // pass descriptor_set to , ie.  ImGui::Image((ImTextureID)descriptor_set, ...)
                        //VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
                        //    pRendererVulkan->getNativeLinearSampler(),
                        //    pImageView->getNativeImageView(),
                        //    VK_IMAGE_LAYOUT_GENERAL);

                        // save descriptor set
                        //iTexturePtr = reinterpret_cast<uint64_t>(descriptorSet);
                        //saImageDescriptorSets[iID] = std::move(descriptorSet);
                    }
                    else
                    {
                        iTexturePtr = reinterpret_cast<uint64_t>(saImageDescriptorSets[iID]);
                    }
                }
                else
                {
                    // existing texture
                    pRenderer->getImguiDescriptorHeap()->setImageView(pImage, iOutputAttachment + 1, *pRenderer->getDevice());
                    iTexturePtr = pRenderer->getImguiDescriptorHeap()->getGPUHandle(iOutputAttachment + 1, *pRenderer->getDevice());
                }

                renderTargetHandle = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mAttachmentHandle;
                std::string const& attachmentName = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mName.c_str();

                std::ostringstream oss;
                auto const& imageDesc = pImage->getDescriptor();
                oss << attachmentName.c_str() << " (" << imageDesc.miWidth << " x " << imageDesc.miHeight << ")";
                ImGui::Text(oss.str().c_str());

                if(ImGui::ImageButton((ImTextureID)iTexturePtr, ImVec2(64, 64)))
                {
                    siSelectedOutputAttachment = iOutputAttachment;
                    outputAttachmentName = attachmentName;
                    siSelectedOutputAttachmentTexturePtr = iTexturePtr;
                }
            }
            //else
            //{
            //    std::string const& attachmentName = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mName.c_str();
            //    if(ImGui::Button(attachmentName.c_str()))
            //    {
            //        siSelectedOutputAttachment = iOutputAttachment;
            //        outputAttachmentName = attachmentName;
            //        siSelectedOutputAttachmentTexturePtr = UINT64_MAX;
            //    }
            //}
        }
    }
    ImGui::End();

    if(outputAttachmentName.length() <= 0)
    {
        outputAttachmentName = "Output Attachment";
    }

    // Selected Attachment
    ImGui::Begin(outputAttachmentName.c_str());
    {
        if(siSelectedOutputAttachmentTexturePtr != UINT64_MAX)
        {
            ImGui::Image((ImTextureID)siSelectedOutputAttachmentTexturePtr, ImVec2(768, 768));
        }
    }
    ImGui::End();
#endif // #if 0
}

/*
**
*/
void inputAttachmentLayout(Render::Common::CRenderer* pRenderer)
{
#if 0
    Render::Common::Serializer* pSerializer = pRenderer->getSerializer();
    auto const& aRenderJobs = pSerializer->getRenderJobs();

    auto getImageTexturePtrVulkan = [&](RenderDriver::Common::CImage* pImage)
    {
        uint64_t iTexturePtr = 0;

        // descriptor_set = ImGui_ImplVulkan_AddTexture(...), need image view for image
        // 
        // equivalent to:
        // fill out VkWriteDescriptorSet descriptorWrite = {};
        // fill out VkDescriptorImageInfo -> sampler,  imageView, imageLayout
        // call vkUpdateDescriptorSet

        VkImage& nativeImage = *(static_cast<VkImage*>(pImage->getNativeImage()));
        uint64_t iID = reinterpret_cast<uint64_t>(nativeImage);
        if(saImageDescriptorSets.find(iID) == saImageDescriptorSets.end())
        {
            // first time creation of the image view and descriptor set used as ImTextureID 

            Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(pRenderer);

            // create image view for AddTexture()
            std::unique_ptr<RenderDriver::Vulkan::CImageView> imageView = std::make_unique<RenderDriver::Vulkan::CImageView>();
            RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
            imageViewDesc.mpImage = pImage;
            imageViewDesc.mFormat = pImage->getDescriptor().mFormat;
            imageViewDesc.miNumImages = 1;
            PLATFORM_OBJECT_HANDLE handle = imageView->create(imageViewDesc, *(pRenderer->getDevice()));

            //Render::Vulkan::Serializer* pSerializer = static_cast<Render::Vulkan::Serializer*>(pRenderer->getSerializer());
            //RenderDriver::Vulkan::CImageView* pImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pSerializer->registerObject(imageView, handle));
            //
            //// descriptor set for the image, use as input for texture id used in ImageButton
            //// pass descriptor_set to , ie.  ImGui::Image((ImTextureID)descriptor_set, ...)
            //VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
            //    pRendererVulkan->getNativeLinearSampler(),
            //    pImageView->getNativeImageView(),
            //    VK_IMAGE_LAYOUT_GENERAL);
            //
            //// save descriptor set
            //iTexturePtr = reinterpret_cast<uint64_t>(descriptorSet);
            //saImageDescriptorSets[iID] = std::move(descriptorSet);
        }
        else
        {
            iTexturePtr = reinterpret_cast<uint64_t>(saImageDescriptorSets[iID]);
        }

        return iTexturePtr;
    };

    ImGui::Begin("Render Job Input Attachments");
    {
        if(ImGui::ListBoxHeader("Render Jobs"))
        {
            std::vector<char[256]> aRenderJobNames(aRenderJobs.size());
            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                strcpy(aRenderJobNames[i], aRenderJobs[i].mName.c_str());
            }

            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                bool bSelected = (i == siSelectedInputAttachmentRenderJob) ? true : false;
                if(ImGui::Selectable(aRenderJobNames.data()[i], &bSelected))
                {
                    siSelectedInputAttachmentRenderJob = i;
                }
            }
            ImGui::ListBoxFooter();
        }
    }
    ImGui::End();

    // output attachment images
    std::string inputAttachmentName = "";
    std::string inputAttachmentTitle = "Input Attachments (" + aRenderJobs[siSelectedInputAttachmentRenderJob].mName + ")";
    ImGui::Begin(inputAttachmentTitle.c_str());
    {
        ImGui::Text(aRenderJobs[siSelectedInputAttachmentRenderJob].mName.c_str());

        auto& renderJob = pSerializer->getRenderJob(aRenderJobs[siSelectedInputAttachmentRenderJob].mName);
        uint32_t iNumInputAttachments = min(static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size()), 32);
        for(uint32_t iInputAttachment = 0; iInputAttachment < iNumInputAttachments; iInputAttachment++)
        {
            if(renderJob.maInputRenderTargetAttachments[iInputAttachment] <= 0)
            {
                continue;
            }

            bool bSeparateImage = false;
            for(uint32_t i = 0; i < static_cast<uint32_t>(renderJob.maInputRenderTargetAttachmentBufferAsImages.size()); i++)
            {
                if(renderJob.maInputRenderTargetAttachmentBufferAsImages[i] == renderJob.maInputRenderTargetAttachments[iInputAttachment])
                {
                    bSeparateImage = true;
                    break;
                }
            }

            std::ostringstream oss;
            uint64_t iTexturePtr = 0x0;
            std::string attachmentName = "";
            if(bSeparateImage)
            {
                RenderDriver::Common::CImage* pImage = pSerializer->getImage(renderJob.maInputRenderTargetAttachments[iInputAttachment]).get();
                
                if(pRenderer->getRenderDriverType() == Render::Common::RenderDriverType::Vulkan)
                {
                    iTexturePtr = getImageTexturePtrVulkan(pImage);
                }
                else
                {
                    // + 32 to offset the descriptor heap index from output's maximum number of textures
                    pRenderer->getImguiDescriptorHeap()->setImageView(pImage, iInputAttachment + 32 + 1, *pRenderer->getDevice());
                    iTexturePtr = pRenderer->getImguiDescriptorHeap()->getGPUHandle(iInputAttachment + 32 + 1, *pRenderer->getDevice());
                }

                auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInputAttachment]];
                attachmentName = attachmentInfo.mName;
                std::string attachmentOutput = attachmentName + " (" + attachmentInfo.mParentJobName + ")";

                auto const& imageDesc = pImage->getDescriptor();
                oss << attachmentOutput.c_str() << " (" << imageDesc.miWidth << " x " << imageDesc.miHeight << ")";

            }
            else
            {
                bool bIsImage = pSerializer->isImage(renderJob.maInputRenderTargetAttachments[iInputAttachment]);
                RenderDriver::Common::CImage* pImage = nullptr;
                if(bIsImage)
                {
                    pImage = pSerializer->getImage(renderJob.maInputRenderTargetAttachments[iInputAttachment]).get();
                }
                else
                {
                    if(pSerializer->hasRenderTarget(renderJob.maInputRenderTargetAttachments[iInputAttachment]))
                    {
                        auto& renderTarget = pSerializer->getRenderTarget(renderJob.maInputRenderTargetAttachments[iInputAttachment]);
                        if(renderTarget.get())
                        {
                            pImage = renderTarget->getImage().get();
                        }
                    }
                }
                if(pImage)
                {
                    if(pRenderer->getRenderDriverType() == Render::Common::RenderDriverType::Vulkan)
                    {
                        iTexturePtr = getImageTexturePtrVulkan(pImage);
                    }
                    else
                    {
                        // + 32 to offset the descriptor heap index from output's maximum number of textures
                        pRenderer->getImguiDescriptorHeap()->setImageView(pImage, iInputAttachment + 32 + 1, *pRenderer->getDevice());
                        iTexturePtr = pRenderer->getImguiDescriptorHeap()->getGPUHandle(iInputAttachment + 32 + 1, *pRenderer->getDevice());
                    }

                    auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInputAttachment]];
                    attachmentName = attachmentInfo.mName;
                    std::string attachmentOutput = attachmentName + " (" + attachmentInfo.mParentJobName + ")";

                    auto const& imageDesc = pImage->getDescriptor();
                    oss << attachmentOutput.c_str() << " (" << imageDesc.miWidth << " x " << imageDesc.miHeight << ")";
                }
                else
                {
                    auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInputAttachment]];
                    attachmentName = attachmentInfo.mName;
                    std::string attachmentOutput = attachmentName + " (" + attachmentInfo.mParentJobName + ")";
                    oss << attachmentOutput.c_str() << " buffer";
                }
            }
            
            ImGui::Text(oss.str().c_str());

            if(iTexturePtr > 0x0)
            {
                if(ImGui::ImageButton((ImTextureID)iTexturePtr, ImVec2(64, 64)))
                {
                    siSelectedOutputAttachment = iInputAttachment;
                    inputAttachmentName = attachmentName;
                    siSelectedInputAttachmentTexturePtr = iTexturePtr;
                }
            }
            else
            {
                if(ImGui::Button(oss.str().c_str()))
                {
                    siSelectedOutputAttachment = iInputAttachment;
                    inputAttachmentName = attachmentName;
                    siSelectedInputAttachmentTexturePtr = iTexturePtr;
                }
            }
        }
    }
    ImGui::End();

    if(inputAttachmentName.length() <= 0)
    {
        inputAttachmentName = "Input Attachment";
    }

    // Selected Attachment
    ImGui::Begin(inputAttachmentName.c_str());
    {
        if(siSelectedInputAttachmentTexturePtr != UINT64_MAX)
        {
            ImGui::Image((ImTextureID)siSelectedInputAttachmentTexturePtr, ImVec2(768, 768));
        }
    }
    ImGui::End();
#endif // #if 0
}

/*
**
*/
void debugOptionsLayout(Render::Common::CRenderer* pRenderer)
{
    char const* szTitle = "Restir Type";
    ImGui::Begin("Debug Options");
    {
        char const* aszItems[] = { "Temporal", "Spatial 0", "Spatial 1" };
        ImGui::Text(aszItems[siRestirType]);

        static char const* pItem = aszItems[0];
        if(ImGui::ListBoxHeader("Restir Pass"))
        {
            for(uint32_t i = 0; i < 3; i++)
            {
                bool bSelected = (i == siRestirType) ? true : false;
                if(ImGui::Selectable(aszItems[i], &bSelected))
                {
                    
                    siRestirType = i;
                }
            }
            ImGui::ListBoxFooter();
        }

        ImGui::SliderFloat("World Space Blur Radius", &sfWorldSpaceBlurRadius, 0.0f, 5.0f);
        if(sfWorldSpaceBlurRadius != sfLastWorldSpaceBlurRadius)
        {
            sfLastWorldSpaceBlurRadius = sfWorldSpaceBlurRadius;
        }

        ImGui::SliderInt("World Space Samples", &siWorldSpaceBlurSamples, 4, 64);
        if(siLastWorldSpaceBlurSamples != siWorldSpaceBlurSamples)
        {
            siLastWorldSpaceBlurSamples = siWorldSpaceBlurSamples;
        }

        ImGui::SliderFloat("Sky Light Intensity", &sfSunIntensity, 0.0f, 300.0f);
        if(sfLastSunIntensity != sfSunIntensity)
        {
            sfLastSunIntensity = sfSunIntensity;
        }

        ImGui::SliderFloat("Local Light Intensity", &sfLocalLightIntensity, 0.0f, 300.0f);
        if(sfLastLocalLightIntensity != sfLocalLightIntensity)
        {
            sfLastLocalLightIntensity = sfLocalLightIntensity;
        }

        ImGui::SliderFloat("Spatial Restir Radius 0", &sfSpatialRestirRadius0, 0.1f, 50.0f);
        if(sfLastSpatialRestirRadius0 != sfSpatialRestirRadius0)
        {
            sfLastSpatialRestirRadius0 = sfSpatialRestirRadius0;
        }

        ImGui::SliderFloat("Spatial Restir Radius 1", &sfSpatialRestirRadius1, 0.1f, 50.0f);
        if(sfLastSpatialRestirRadius1 != sfSpatialRestirRadius1)
        {
            sfLastSpatialRestirRadius1 = sfSpatialRestirRadius1;
        }

        ImGui::SliderInt("Spatial Restir Sample Count 0", &siSpatialRestirSampleCount0, 1, 64);
        if(siLastSpatialRestirSampleCount0 != siSpatialRestirSampleCount0)
        {
            siLastSpatialRestirSampleCount0 = siSpatialRestirSampleCount0;
        }

        ImGui::SliderInt("Spatial Restir Sample Count 1", &siSpatialRestirSampleCount1, 1, 64);
        if(siLastSpatialRestirSampleCount1 != siSpatialRestirSampleCount1)
        {
            siLastSpatialRestirSampleCount1 = siSpatialRestirSampleCount1;
        }

        ImGui::SliderFloat("Upscale Radius", &sfUpscaleRadius, 0.1f, 5.0f);
        if(sfLastUpscaleRadius != sfUpscaleRadius)
        {
            sfLastUpscaleRadius = sfUpscaleRadius;
        }

        ImGui::SliderFloat("Bilateral Blur Radius", &sfBilateralBlurRadius, 1.0f, 16.0f);
        if(sfLastBilateralBlurRadius != sfBilateralBlurRadius)
        {
           sfLastBilateralBlurRadius = sfBilateralBlurRadius;
        }

        ImGui::SliderFloat("Bilateral Blur Scale", &sfBilateralBlurScale, 1.0f, 5.0f);
        if(sfLastBilateralBlurScale != sfLastBilateralBlurScale)
        {
            sfLastBilateralBlurScale = sfLastBilateralBlurScale;
        }

        ImGui::SliderFloat("Bilateral Blur Color Sensitivity", &sfBilateralBlurColorSensitivty, 1.0f, 64.0f);
        if(sfLastBilateralBlurColorSensitivty != sfBilateralBlurColorSensitivty)
        {
            sfLastBilateralBlurColorSensitivty = sfBilateralBlurColorSensitivty;
        }

        ImGui::SliderFloat("Ambient Occlusion Blur Radius", &sfAmbientOcclusionBlurRadius, 1.0f, 5.0f);
        if(sfLastAmbientOcclusionBlurRadius != sfAmbientOcclusionBlurRadius)
        {
            sfLastAmbientOcclusionBlurRadius = sfAmbientOcclusionBlurRadius;
        }

        ImGui::SliderFloat("Ambient Occlusion Maximum Distance", &sfAmbientOcclusionMaxDistance, 0.01f, 5.0f);
        if(sfLastAmbientOcclusionMaxDistance != sfAmbientOcclusionMaxDistance)
        {
            sfLastAmbientOcclusionMaxDistance = sfAmbientOcclusionMaxDistance;
        }

        ImGui::SliderFloat("Material Roughness", &sfMaterialRoughness, 0.01f, 1.0f);
        if(sfLastMaterialRoughness != sfMaterialRoughness)
        {
            sfLastMaterialRoughness = sfMaterialRoughness;
        }


        ImGui::SliderFloat("Material Metalness", &sfMaterialMetalness, 0.0f, 1.0f);
        if(sfLastMaterialMetalness != sfMaterialMetalness)
        {
            sfLastMaterialMetalness = sfMaterialMetalness;
        }
    }
    ImGui::End();
}

/*
**
*/
void renderJobStatsLayout(Render::Common::CRenderer* pRenderer)
{
#if 0
    Render::Common::Serializer* pSerializer = pRenderer->getSerializer();
    auto const& aRenderJobs = pSerializer->getRenderJobs();

    ImGui::Begin("Render Job Stats");
    {
        auto const& aRenderJobExecTimes = pRenderer->getRenderJobExecTimes();
        auto const& aRenderJobs = pRenderer->getSerializer()->getRenderJobs();
        for(auto const& renderJob : aRenderJobs)
        {
            uint64_t iTimeElapsed = 0;
            for(auto const& keyValue : aRenderJobExecTimes)
            {
                if(keyValue.first == renderJob.mName)
                {
                    iTimeElapsed = keyValue.second.miTimeDuration;
                    break;
                }
            }

            std::ostringstream renderJobTimeOutput;
            renderJobTimeOutput << 
                renderJob.mName << 
                " " << 
                (float(iTimeElapsed) * 0.001f) << 
                " milliseconds " << 
                ((float(iTimeElapsed) / float(pRenderer->getTotalRenderJobsTime())) * 100.0f) <<
                " %%";
            ImGui::LabelText("", renderJobTimeOutput.str().c_str());
        }

        std::ostringstream renderJobTime;
        renderJobTime << "Total Render Jobs Time " << (float(pRenderer->getTotalRenderJobsTime()) * 0.001f) << " msec";
        ImGui::LabelText("", renderJobTime.str().c_str());
    }
    ImGui::End();
#endif // #if 0
}

/*
**
*/
void imguiLayout(Render::Common::CRenderer* pRenderer)
{
    std::ostringstream oss;
    
    pRenderer->beginDebugMarker("Top Window UI");
    ImGui::Begin("Toggle UI");
    {
        if(ImGui::Button("Toggle Debug Options UI"))
        {
            sbShowDebugOptionsUI = !sbShowDebugOptionsUI;
        }

        if(ImGui::Button("Toggle Render Job Input Attachments UI"))
        {
            sbShowInputAttachmentUI = !sbShowInputAttachmentUI;
        }

        if(ImGui::Button("Toggle Render Job Output Attachments UI"))
        {
            sbShowOutputAttachmentUI = !sbShowOutputAttachmentUI;
        }

        if(ImGui::Button("Toggle Render Job Stats"))
        {
            sbShowRenderJobStatsUI = !sbShowRenderJobStatsUI;
        }
    }
    ImGui::End();
    pRenderer->endDebugMarker();

    if(sbShowDebugOptionsUI)
    {
        pRenderer->beginDebugMarker("Debug Options UI");
        debugOptionsLayout(pRenderer);
        pRenderer->endDebugMarker();
    }

    if(sbShowOutputAttachmentUI)
    {
        pRenderer->beginDebugMarker("Output Attachment UI");
        outputAttachmentLayout(pRenderer);
        pRenderer->endDebugMarker();
    }

    if(sbShowInputAttachmentUI)
    {
        pRenderer->beginDebugMarker("Input Attachment UI");
        inputAttachmentLayout(pRenderer);
        pRenderer->endDebugMarker();
    }
    
    if(sbShowRenderJobStatsUI)
    {
        pRenderer->beginDebugMarker("Stats UI");
        renderJobStatsLayout(pRenderer);
        pRenderer->endDebugMarker();
    }
}

#if 0
/*
**
*/
void loadTexturePage(
    std::vector<char>& acPageData,
    std::string const& imageFileName,
    uint2 const& pageCoord,
    uint32_t iPageDimension)
{
    std::string fullPath = "d:\\Downloads\\Bistro_v4\\converted-dds\\" + imageFileName;
    int32_t iImageWidth = 0, iImageHeight = 0, iNumChannels = 0;
    stbi_uc* pImageData = stbi_load(
        fullPath.c_str(),
        &iImageWidth,
        &iImageHeight,
        &iNumChannels,
        4);

    for(uint32_t iY = 0; iY < iPageDimension; iY++)
    {
        uint32_t iStart = (((pageCoord.y * iPageDimension) + iY) * iImageWidth + pageCoord.x * iPageDimension) * 4;
        uint32_t iIndex = iY * iPageDimension * 4;
        memcpy(
            &acPageData[iIndex],
            &pImageData[iStart],
            iPageDimension * 4
        );
    }
    stbi_image_free(pImageData);

    stbi_write_png(
        "d:\\Downloads\\Bistro_v4\\texture-pages\\test.png",
        iPageDimension,
        iPageDimension,
        4,
        acPageData.data(),
        iPageDimension * 4
    );
}
#endif // if 0

/*
**
*/
void outputTexturePages(
    std::string const& imageFileName,
    uint32_t iPageDimension)
{
    std::string fullPath = "d:\\Downloads\\Bistro_v4\\converted-dds-scaled\\" + imageFileName;
    int32_t iImageWidth = 0, iImageHeight = 0, iNumChannels = 0;
    stbi_uc* pImageData = stbi_load(
        fullPath.c_str(),
        &iImageWidth,
        &iImageHeight,
        &iNumChannels,
        4);

    uint2 numPages(
        max(iImageWidth / iPageDimension, 1),
        max(iImageHeight / iPageDimension, 1)
    );

    auto fileExtensionStart = imageFileName.find_last_of(".");
    std::string baseName = imageFileName.substr(0, fileExtensionStart);
    std::vector<char> acPageImageData(iPageDimension * iPageDimension * 4);

    if(numPages.x > 1 && numPages.y > 1)
    {
        for(uint32_t iPageY = 0; iPageY < numPages.y; iPageY++)
        {
            for(uint32_t iPageX = 0; iPageX < numPages.x; iPageX++)
            {
                for(uint32_t iY = 0; iY < iPageDimension; iY++)
                {
                    uint32_t iStart = (((iPageY * iPageDimension) + iY) * iImageWidth + iPageX * iPageDimension) * 4;
                    uint32_t iIndex = iY * iPageDimension * 4;
                    memcpy(
                        &acPageImageData[iIndex],
                        &pImageData[iStart],
                        iPageDimension * 4
                    );
                }

                std::ostringstream oss;
                oss << "d:\\Downloads\\Bistro_v4\\texture-pages\\" << baseName << "-" << iPageX << "-" << iPageY << ".png";
                stbi_write_png(
                    oss.str().c_str(),
                    iPageDimension,
                    iPageDimension,
                    4,
                    acPageImageData.data(),
                    iPageDimension * 4
                );
            }
        }
    }
    else
    {
        float2 scale(
            (float)iImageWidth / (float)iPageDimension,
            (float)iImageHeight / (float)iPageDimension
        );

        float fX = 0.0f, fY = 0.0f;
        for(int32_t iY = 0; iY < (int32_t)iPageDimension; iY++)
        {
            int32_t iSampleY = int32_t(fY);
            fY += scale.y;
            if(iSampleY >= iImageHeight)
            {
                iSampleY = iImageHeight - 1;
            }
            for(int32_t iX = 0; iX < (int32_t)iPageDimension; iX++)
            {
                int32_t iSampleX = int32_t(fX);
                fX += scale.x;
                if(iSampleX >= iImageWidth)
                {
                    iSampleX = iImageWidth - 1;
                }

                uint32_t iSampleIndex = (iSampleY * iImageWidth + iSampleX) * 4;
                char cSampleRed = pImageData[iSampleIndex];
                char cSampleGreen = pImageData[iSampleIndex+1];
                char cSampleBlue = pImageData[iSampleIndex+2];
                char cSampleAlpha = pImageData[iSampleIndex+3];

                uint32_t iIndex = (iY * iPageDimension + iX) * 4;
                acPageImageData[iIndex] = cSampleRed;
                acPageImageData[iIndex+1] = cSampleGreen;
                acPageImageData[iIndex+2] = cSampleBlue;
                acPageImageData[iIndex+3] = cSampleAlpha;
            }
        }

        std::ostringstream oss;
        oss << "d:\\Downloads\\Bistro_v4\\texture-pages\\" << baseName << "-0-0.png";
        stbi_write_png(
            oss.str().c_str(),
            iPageDimension,
            iPageDimension,
            4,
            acPageImageData.data(),
            iPageDimension * 4
        );
    }

    stbi_image_free(pImageData);
}

/*
**
*/
void loadTexturePage(
    Render::Common::CRenderer* pRenderer,
    std::vector<std::string> const& aAlbedoTextureNames,
    std::vector<uint2> const& aAlbedoTextureDimensions,
    std::vector<std::string> const& aNormalTextureNames,
    std::vector<uint2> const& aNormalTextureDimensions,
    uint32_t& iCurrTotalPageLoaded,
    uint32_t& iCurrAlbedoPageLoaded,
    uint32_t& iCurrNormalPageLoaded,
    std::map<std::string, std::vector<PageInfo>>& aPageInfo)
{
    // get the texture pages needed
    auto& pTexturePageQueueBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Texture Page Queue MIP"];
    uint32_t iBufferSize = (uint32_t)pTexturePageQueueBuffer->getDescriptor().miSize;
    std::vector<char> acTexturePageQueueData(iBufferSize);
    pRenderer->copyBufferToCPUMemory(
        pTexturePageQueueBuffer,
        acTexturePageQueueData.data(),
        0,
        iBufferSize
    );

    // get the number of pages
    auto& pTexturePageCountBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Counters"];
    std::vector<char> acCounter(256);
    pRenderer->copyBufferToCPUMemory(
        pTexturePageCountBuffer,
        acCounter.data(),
        0,
        256
    );

    uint32_t const iTextureAtlasSize = 8192;
    uint32_t const iTexturePageSize = 64;
    uint32_t iNumPagesPerRow = iTextureAtlasSize / iTexturePageSize;

    // texture page info
    struct TexturePage
    {
        int32_t miPageUV;
        int32_t miTextureID;
        int32_t miHashIndex;

        int32_t miMIP;
    };
    uint32_t iNumRequests = *((uint32_t*)acCounter.data());
    for(uint32_t i = 0; i < 10; i++)
    {
        if(iCurrTotalPageLoaded >= iNumRequests)
        {
            break;
        }

        char* acData = acTexturePageQueueData.data() + iCurrTotalPageLoaded * sizeof(TexturePage);
        TexturePage texturePage = *((TexturePage*)acData);
        uint32_t iTextureID = texturePage.miTextureID;
        if(iTextureID >= 65536)
        {
            iTextureID -= 65536;

            // TODO: have normal texture page images
            iCurrTotalPageLoaded += 1;
            continue;
        }

        int32_t iPageY = (texturePage.miPageUV >> 16);
        int32_t iPageX = (texturePage.miPageUV & 0x0000ffff);

        uint32_t iMipDenom = (uint32_t)pow(2, texturePage.miMIP);
        uint2 textureDimension = aAlbedoTextureDimensions[iTextureID];
        if(texturePage.miTextureID >= 65536)
        {
            textureDimension = aNormalTextureDimensions[iTextureID];
        }
        uint2 mipTextureDimension(
            textureDimension.x / iMipDenom,
            textureDimension.y / iMipDenom
        );

        uint2 numDivs(
            max(1, mipTextureDimension.x / iTexturePageSize),
            max(1, mipTextureDimension.y / iTexturePageSize)
        );

        // texture page coordinate
        uint2 pageCoord(
            (uint32_t)abs(iPageX) % numDivs.x,
            (uint32_t)abs(iPageY) % numDivs.y
        );

        // wrap around for negative coordinates
        if(iPageX < 0)
        {
            pageCoord.x = numDivs.x - pageCoord.x;
            if(pageCoord.x >= numDivs.x)
            {
                pageCoord.x = pageCoord.x % numDivs.x;
            }
        }
        if(iPageY < 0)
        {
            pageCoord.y = numDivs.y - pageCoord.y;
            if(pageCoord.y >= numDivs.y)
            {
                pageCoord.y = pageCoord.y % numDivs.y;
            }
        }

        // save page info
        PageInfo pageInfo;
        pageInfo.miCoordX = pageCoord.x;
        pageInfo.miCoordY = pageCoord.y;
        pageInfo.miHashIndex = texturePage.miHashIndex;
        pageInfo.miMIP = texturePage.miMIP;
        pageInfo.miTextureID = texturePage.miTextureID;
        pageInfo.miPageIndex = (texturePage.miTextureID >= 65536) ? iCurrNormalPageLoaded : iCurrAlbedoPageLoaded;

        std::ostringstream keyStream;
        keyStream << texturePage.miTextureID << "-" << texturePage.miMIP;
        aPageInfo[keyStream.str()].push_back(pageInfo);

        std::string textureName = aAlbedoTextureNames[iTextureID];
        if(texturePage.miTextureID >= 65536)
        {
            textureName = aNormalTextureNames[iTextureID];
        }

        // load texture page image data
        auto fileExtensionStart = textureName.find_last_of(".");
        std::string baseName = textureName.substr(0, fileExtensionStart);
        std::vector<char> acPageData(iTexturePageSize * iTexturePageSize * 4);
        std::ostringstream oss;
        oss << "d:\\Downloads\\Bistro_v4\\texture-pages\\" << baseName << "-" << pageCoord.x << "-" << pageCoord.y << ".png";
        int32_t iWidth = 0, iHeight = 0, iNumChannels = 0;
        stbi_uc* pTexturePageData = stbi_load(
            oss.str().c_str(),
            &iWidth,
            &iHeight,
            &iNumChannels,
            4
        );
        WTFASSERT(pTexturePageData, "Can\'t find texture page \"%s\"", oss.str().c_str());

        // copy texture page to texture atlas, normal or albedo
        uint32_t iPageIndex = 0;
        if(texturePage.miTextureID >= 65536)
        {
            auto& textureAtlas1 = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputImageAttachments["Texture Atlas 1"];
            pRenderer->copyTexturePageToAtlas(
                (char const*)pTexturePageData,
                textureAtlas1,
                uint2(iCurrNormalPageLoaded % iNumPagesPerRow, (iCurrNormalPageLoaded / iNumPagesPerRow) % iNumPagesPerRow),
                iTexturePageSize
            );

            iPageIndex = iCurrNormalPageLoaded + 1;
        }
        else
        {
            auto& textureAtlas0 = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputImageAttachments["Texture Atlas 0"];
            pRenderer->copyTexturePageToAtlas(
                (char const*)pTexturePageData,
                textureAtlas0,
                uint2(iCurrAlbedoPageLoaded % iNumPagesPerRow, (iCurrAlbedoPageLoaded / iNumPagesPerRow) % iNumPagesPerRow),
                iTexturePageSize
            );

            iPageIndex = iCurrAlbedoPageLoaded + 1;
        }

        // update the page info to point to the page index in the atlas
        uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
        uint32_t iBufferOffset = texturePage.miHashIndex * sizeof(uint32_t) * 4 + sizeof(uint32_t);
        auto& texturePageInfoBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];
        pRenderer->copyCPUToBuffer(
            texturePageInfoBuffer,
            &iPageIndex,
            iBufferOffset,
            sizeof(uint32_t)
        );

        // update frame index entry
        uint32_t iFrameIndex = pRenderer->getFrameIndex() + 1;
        iBufferOffset = texturePage.miHashIndex * sizeof(uint32_t) * 4 + sizeof(uint32_t) * 3;
        pRenderer->copyCPUToBuffer(
            texturePageInfoBuffer,
            &iFrameIndex,
            iBufferOffset,
            sizeof(uint32_t),
            iFlags
        );

        if(texturePage.miTextureID >= 65536)
        {
            iCurrNormalPageLoaded += 1;
        }
        else
        {
            iCurrAlbedoPageLoaded += 1;
        }

        iCurrTotalPageLoaded += 1;
    }
}

/*
**
*/
void getTexturePage(
    std::vector<char>& acTexturePageImage,
    stbi_uc const* pImageData,
    uint2 const& pageCoord,
    uint32_t const& iImageWidth,
    uint32_t const& iImageHeight,
    uint32_t const& iTexturePageSize)
{
    if(iImageWidth >= iTexturePageSize && iImageHeight >= iTexturePageSize)
    {
        for(uint32_t iY = 0; iY < iTexturePageSize; iY++)
        {
            uint32_t iStartPageIndex = iY * iTexturePageSize * 4;
            uint32_t iStartIndex = (pageCoord.y * iTexturePageSize + iY) * iImageWidth * 4;

            for(uint32_t iX = 0; iX < iTexturePageSize; iX++)
            {
                uint32_t iPageIndex = iStartPageIndex + iX * 4;
                uint32_t iIndex = iStartIndex + (pageCoord.x * iTexturePageSize + iX) * 4;

                acTexturePageImage[iPageIndex] = pImageData[iIndex];
                acTexturePageImage[iPageIndex + 1] = pImageData[iIndex + 1];
                acTexturePageImage[iPageIndex + 2] = pImageData[iIndex + 2];
                acTexturePageImage[iPageIndex + 3] = pImageData[iIndex + 3];
            }
        }
    }
    else
    {
        float2 scale(
            (float)iTexturePageSize / (float)iImageWidth,
            (float)iTexturePageSize / (float)iImageHeight
        );

        float fX = 0.0f, fY = 0.0f;
        for(uint32_t iY = 0; iY < iTexturePageSize; iY++)
        {
            uint32_t iTotalY = uint32_t(fY);
            fY += scale.y;
            
            uint32_t iStartIndex = iTotalY * iImageWidth * 4;
            uint32_t iStartPageIndex = iY * iTexturePageSize * 4;

            for(uint32_t iX = 0; iX < iTexturePageSize; iX++)
            {
                uint32_t iPageIndex = iStartPageIndex + iX * 4;

                uint32_t iTotalX = uint32_t(fX);
                fX += scale.x;
                uint32_t iIndex = iStartIndex + iX * 4;

                acTexturePageImage[iPageIndex] = pImageData[iIndex];
                acTexturePageImage[iPageIndex + 1] = pImageData[iIndex + 1];
                acTexturePageImage[iPageIndex + 2] = pImageData[iIndex + 2];
                acTexturePageImage[iPageIndex + 3] = pImageData[iIndex + 3];
            }
        }
    }
}

/*
**
*/
void loadTexturePageThread(
    Render::Common::CRenderer* pRenderer,
    std::vector<std::string> const& aAlbedoTextureNames,
    std::vector<uint2> const& aAlbedoTextureDimensions,
    std::vector<std::string> const& aNormalTextureNames,
    std::vector<uint2> const& aNormalTextureDimensions,
    uint32_t& iCurrTotalPageLoaded,
    uint32_t& iCurrAlbedoPageLoaded,
    uint32_t& iCurrNormalPageLoaded,
    //std::map<std::string, std::vector<PageInfo>>& aPageInfo,
    RenderDriver::Common::CCommandBuffer& commandBuffer,
    RenderDriver::Common::CCommandQueue& commandQueue,
    RenderDriver::Common::CBuffer& uploadBuffer,
    uint32_t iStartIndex,
    uint32_t iNumChecksPerLoop,
    //std::map<uint32_t, ImageInfo>& aImageInfo,
    RenderDriver::Common::CBuffer& texturePageUploadBuffer,
    void* pUploadBuffer,
    uint32_t& iLastCounterValue,
    std::vector<char>const & acTexturePageQueueData,
    std::vector<char>const & acTexturePageInfoData,
    std::mutex& threadMutex)
{
auto totalStart = std::chrono::high_resolution_clock::now();

    // texture page info
    struct TexturePage
    {
        int32_t miPageUV;
        int32_t miTextureID;
        int32_t miHashIndex;

        int32_t miMIP;
    };

    struct HashEntry
    {
        uint32_t miPageCoordinate;
        uint32_t miPageIndex;
        uint32_t miTextureIDAndMIP;
        uint32_t miUpdateFrame;
    };

    uint32_t iOffset = iStartIndex * sizeof(TexturePage);
    uint32_t iCopyBufferSize = iNumChecksPerLoop * sizeof(TexturePage);

    // get the texture pages needed
    //auto& pTexturePageQueueBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Texture Page Queue MIP"];
    ////uint32_t iBufferSize = (uint32_t)pTexturePageQueueBuffer->getDescriptor().miSize;
    //std::vector<char> acTexturePageQueueData(iCopyBufferSize);
    //pRenderer->copyBufferToCPUMemory2(
    //    pTexturePageQueueBuffer,
    //    acTexturePageQueueData.data(),
    //    iOffset,
    //    iCopyBufferSize,
    //    commandBuffer,
    //    commandQueue
    //);
    //
    //iOffset = iStartIndex * sizeof(HashEntry);
    //iCopyBufferSize = iNumChecksPerLoop * sizeof(HashEntry);

    //auto& texturePageInfoBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];
    //uint32_t iBufferSize = (uint32_t)texturePageInfoBuffer->getDescriptor().miSize;
    //std::vector<char> acTexturePageInfoData(iBufferSize);
    //pRenderer->copyBufferToCPUMemory2(
    //    texturePageInfoBuffer,
    //    acTexturePageInfoData.data(),
    //    0,
    //    iBufferSize,
    //    commandBuffer,
    //    commandQueue
    //);

    // get the number of pages
    //auto& pTexturePageCountBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Counters"];
    //std::vector<char> acCounter(256);
    //pRenderer->copyBufferToCPUMemory2(
    //    pTexturePageCountBuffer,
    //    acCounter.data(),
    //    0,
    //    256,
    //    commandBuffer,
    //    commandQueue
    //);
    //uint32_t iNumQueueEntries = *((uint32_t*)acCounter.data());
    //iLastCounterValue = iNumQueueEntries;

    uint32_t const iTextureAtlasSize = 8192;
    uint32_t const iTexturePageSize = 64;
    uint32_t iNumPagesPerRow = iTextureAtlasSize / iTexturePageSize;

    uint32_t const iTexturePageDataSize = iTexturePageSize * iTexturePageSize * 4;

    auto& texturePageInfoBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];

    uint32_t iThreadAlbedoTextureIndex = 0, iThreadNormalTextureIndex = 0;
    uint32_t i = 0;
    for(i = iStartIndex; i < iStartIndex + iNumChecksPerLoop; i++)
    //for(i = 0; i < iNumChecksPerLoop; i++)
    {
        char const* acData = acTexturePageQueueData.data() + i * sizeof(TexturePage);
        TexturePage texturePage = *((TexturePage const*)acData);
        
        // check for valid entry
        if(texturePage.miHashIndex == 0 && texturePage.miPageUV == 0 && texturePage.miTextureID == 0)
        {
            break;
        }

#if 0
        uint32_t iBufferSize = sizeof(HashEntry);
        std::vector<char> acTexturePageInfoData(iBufferSize);
        pRenderer->copyBufferToCPUMemory2(
            texturePageInfoBuffer,
            acTexturePageInfoData.data(),
            texturePage.miHashIndex * sizeof(HashEntry),
            iBufferSize,
            commandBuffer,
            commandQueue
        );
        HashEntry hashEntry = *((HashEntry*)acTexturePageInfoData.data());
        if(hashEntry.miPageIndex != 0xffffffff)
        {
            continue;
        }
#endif // #if 0

        // check if the page has already been loaded
        char const* acPageInfoData = acTexturePageInfoData.data() + texturePage.miHashIndex * sizeof(HashEntry);
        HashEntry hashEntry = *((HashEntry const*)acPageInfoData);
        if(hashEntry.miPageIndex != 0xffffffff)
        {
            continue;
        }

        // reserve albedo page index for this thread
        {
            std::lock_guard<std::mutex> lock(threadMutex);
            iThreadAlbedoTextureIndex = iCurrAlbedoPageLoaded;
            iCurrAlbedoPageLoaded += 1;
        }

        uint32_t iTextureID = texturePage.miTextureID;
        if(iTextureID >= 65536)
        {
            std::lock_guard<std::mutex> lock(threadMutex);
            iTextureID -= 65536;

            // TODO: have normal texture page images
            iThreadNormalTextureIndex = iCurrNormalPageLoaded;
            iCurrNormalPageLoaded += 1;
            iCurrTotalPageLoaded += 1;
            
            continue;
        }

#if 0
        // open the image, storing the file ptr in map
        stbi_uc* pImageData = nullptr;
        int32_t iImageWidth = 0, iImageHeight = 0, iNumChannels = 0;
        if(aImageInfo[texturePage.miTextureID].mpImageData == nullptr)
        {
            std::string fullPath = "d:\\Downloads\\Bistro_v4\\converted-dds-scaled\\" + aAlbedoTextureNames[texturePage.miTextureID];
            aImageInfo[texturePage.miTextureID].mpImageData = stbi_load(
                fullPath.c_str(),
                &iImageWidth,
                &iImageHeight,
                &iNumChannels,
                4
            );
            aImageInfo[texturePage.miTextureID].miImageWidth = iImageWidth;
            aImageInfo[texturePage.miTextureID].miImageHeight = iImageHeight;
            pImageData = aImageInfo[texturePage.miTextureID].mpImageData;
        }
        else
        {
            pImageData = aImageInfo[texturePage.miTextureID].mpImageData;
            iImageWidth = aImageInfo[texturePage.miTextureID].miImageWidth;
            iImageHeight = aImageInfo[texturePage.miTextureID].miImageHeight;
        }
#endif // #if 0

        int32_t iPageY = (texturePage.miPageUV >> 16);
        int32_t iPageX = (texturePage.miPageUV & 0x0000ffff);

        uint32_t iMipDenom = (uint32_t)pow(2, texturePage.miMIP);
        uint2 textureDimension = aAlbedoTextureDimensions[iTextureID];
        if(texturePage.miTextureID >= 65536)
        {
            textureDimension = aNormalTextureDimensions[iTextureID];
        }
        uint2 mipTextureDimension(
            textureDimension.x / iMipDenom,
            textureDimension.y / iMipDenom
        );

        uint2 numDivs(
            max(1, mipTextureDimension.x / iTexturePageSize),
            max(1, mipTextureDimension.y / iTexturePageSize)
        );

        // texture page coordinate
        uint2 pageCoord(
            (uint32_t)abs(iPageX) % numDivs.x,
            (uint32_t)abs(iPageY) % numDivs.y
        );

        // wrap around for negative coordinates
        if(iPageX < 0)
        {
            pageCoord.x = numDivs.x - pageCoord.x;
            if(pageCoord.x >= numDivs.x)
            {
                pageCoord.x = pageCoord.x % numDivs.x;
            }
        }
        if(iPageY < 0)
        {
            pageCoord.y = numDivs.y - pageCoord.y;
            if(pageCoord.y >= numDivs.y)
            {
                pageCoord.y = pageCoord.y % numDivs.y;
            }
        }

#if 0
        // save page info
        PageInfo pageInfo;
        pageInfo.miCoordX = pageCoord.x;
        pageInfo.miCoordY = pageCoord.y;
        pageInfo.miHashIndex = texturePage.miHashIndex;
        pageInfo.miMIP = texturePage.miMIP;
        pageInfo.miTextureID = texturePage.miTextureID;
        pageInfo.miPageIndex = (texturePage.miTextureID >= 65536) ? iThreadNormalTextureIndex : iThreadAlbedoTextureIndex;

        std::ostringstream keyStream;
        keyStream << texturePage.miTextureID << "-" << texturePage.miMIP;
        aPageInfo[keyStream.str()].push_back(pageInfo);
#endif // #if 0

        // texture name
        std::string textureName = aAlbedoTextureNames[iTextureID];
        if(texturePage.miTextureID >= 65536)
        {
            textureName = aNormalTextureNames[iTextureID];
        }

#if 0
        // get the texture page data
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<char> acTexturePageImage(iTexturePageSize * iTexturePageSize * 4);
        getTexturePage(
            acTexturePageImage,
            pImageData,
            pageCoord,
            iImageWidth,
            iImageHeight,
            iTexturePageSize
        );
        char const* pTexturePageData = acTexturePageImage.data();
        auto durationUS = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
#endif // #if 0

        auto start = std::chrono::high_resolution_clock::now();

        // load texture page image data
        auto fileExtensionStart = textureName.find_last_of(".");
        std::string baseName = textureName.substr(0, fileExtensionStart);
        std::vector<char> acPageData(iTexturePageSize * iTexturePageSize * 4);
        std::ostringstream oss;
        oss << "d:\\Downloads\\Bistro_v4\\texture-pages\\" << baseName << "-" << pageCoord.x << "-" << pageCoord.y << ".png";
        int32_t iWidth = 0, iHeight = 0, iNumChannels = 0;
        stbi_uc* pTexturePageData = stbi_load(
            oss.str().c_str(),
            &iWidth,
            &iHeight,
            &iNumChannels,
            4
        );
        WTFASSERT(pTexturePageData, "Can\'t find texture page \"%s\"", oss.str().c_str());
        auto durationUS = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

        auto start1 = std::chrono::high_resolution_clock::now();

        // copy texture page to texture atlas, normal or albedo
        uint32_t iPageIndex = 0;
        if(texturePage.miTextureID >= 65536)
        {
            auto& textureAtlas1 = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputImageAttachments["Texture Atlas 1"];
            memcpy(pUploadBuffer, pTexturePageData, iTexturePageDataSize);
            pRenderer->copyTexturePageToAtlas2(
                (char const*)pTexturePageData,
                textureAtlas1,
                uint2(iThreadNormalTextureIndex % iNumPagesPerRow, (iThreadNormalTextureIndex / iNumPagesPerRow) % iNumPagesPerRow),
                iTexturePageSize,
                commandBuffer,
                commandQueue,
                texturePageUploadBuffer
            );

            iPageIndex = iThreadNormalTextureIndex + 1;
        }
        else
        {
            if(iThreadAlbedoTextureIndex >= iNumPagesPerRow * iNumPagesPerRow)
            {
                auto& textureAtlas0 = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputImageAttachments["Texture Atlas 2"];
                memcpy(pUploadBuffer, pTexturePageData, iTexturePageDataSize);
                pRenderer->copyTexturePageToAtlas2(
                    (char const*)pTexturePageData,
                    textureAtlas0,
                    uint2(iThreadAlbedoTextureIndex % iNumPagesPerRow, (iThreadAlbedoTextureIndex / iNumPagesPerRow) % iNumPagesPerRow),
                    iTexturePageSize,
                    commandBuffer,
                    commandQueue,
                    texturePageUploadBuffer
                );
            }
            else
            {
                auto& textureAtlas0 = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputImageAttachments["Texture Atlas 0"];
                memcpy(pUploadBuffer, pTexturePageData, iTexturePageDataSize);
                pRenderer->copyTexturePageToAtlas2(
                    (char const*)pTexturePageData,
                    textureAtlas0,
                    uint2(iThreadAlbedoTextureIndex% iNumPagesPerRow, (iThreadAlbedoTextureIndex / iNumPagesPerRow) % iNumPagesPerRow),
                    iTexturePageSize,
                    commandBuffer,
                    commandQueue,
                    texturePageUploadBuffer
                );
            }

            iPageIndex = iThreadAlbedoTextureIndex + 1;
        }

        stbi_image_free(pTexturePageData);

        auto elapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

        // update the page info to point to the page index in the atlas
        uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
        uint32_t iBufferOffset = texturePage.miHashIndex * sizeof(uint32_t) * 4 + sizeof(uint32_t);
        pRenderer->copyCPUToBuffer4(
            texturePageInfoBuffer,
            &iPageIndex,
            iBufferOffset,
            sizeof(uint32_t),
            commandBuffer,
            commandQueue,
            uploadBuffer
        );

        // update frame index entry
        uint32_t iFrameIndex = pRenderer->getFrameIndex() + 1;
        iBufferOffset = texturePage.miHashIndex * sizeof(uint32_t) * 4 + sizeof(uint32_t) * 3;
        pRenderer->copyCPUToBuffer4(
            texturePageInfoBuffer,
            &iFrameIndex,
            iBufferOffset,
            sizeof(uint32_t),
            commandBuffer,
            commandQueue,
            uploadBuffer
        );

        {
            std::lock_guard<std::mutex> lock(threadMutex);
            iCurrTotalPageLoaded += 1;
        }
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - totalStart).count();

    int iDebug = 1;
}

/*
**
*/
void setupExternalDataBuffers(
    std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>>& bufferMap,
    Render::Common::CRenderer* pRenderer)
{
    RenderDriver::Common::CDevice* pDevice = pRenderer->getDevice();

    // material without the texture map path
    {
        struct OutputMaterialInfo
        {
            float4              mDiffuse;
            float4              mSpecular;
            float4              mEmissive;
            uint32_t            miID;
            uint32_t            miAlbedoTextureID;
            uint32_t            miNormalTextureID;
            uint32_t            miSpecularTextureID;
        };

        FILE* fp = fopen("d:\\downloads\\Bistro_v4\\bistro2.mat", "rb");
        fseek(fp, 0, SEEK_END);
        size_t iFileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<char> acBuffer(iFileSize);
        fread(acBuffer.data(), sizeof(char), iFileSize, fp);
        fclose(fp);

        // material id
        char* pacBuffer = acBuffer.data();
        uint32_t iCurrPos = 0;
        uint32_t iNumMaterials = 0;
        OutputMaterialInfo* pOutputMaterial = (OutputMaterialInfo *)pacBuffer;
        std::vector<OutputMaterialInfo> aMaterials;
        for(;;)
        {
            OutputMaterialInfo& materialInfo = *pOutputMaterial;
            aMaterials.push_back(materialInfo);
            if(materialInfo.miID >= 99999)
            {
                break;
            }
            ++pOutputMaterial;
        }

        uint32_t iDataSize = (uint32_t)aMaterials.size() * sizeof(OutputMaterialInfo);

        std::unique_ptr<RenderDriver::Common::CBuffer> buffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
        bufferMap["Material Data"] = std::move(buffer);
        RenderDriver::Common::CBuffer* pBuffer = bufferMap["Material Data"].get();
        RenderDriver::Common::BufferDescriptor desc;
        desc.miSize = iDataSize;
        desc.mBufferUsage = RenderDriver::Common::BufferUsage(
            uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) | 
            uint32_t(RenderDriver::Common::BufferUsage::TransferDest));
        pBuffer->create(
            desc,
            *pDevice
        );
        pBuffer->setID("Material Data");

        pRenderer->copyCPUToBuffer(
            pBuffer,
            aMaterials.data(),
            0,
            (uint32_t)iDataSize
        );
    }

    // material id for texture atlas pages
    {
        FILE* fp = fopen("d:\\downloads\\Bistro_v4\\bistro2.mid", "rb");
        fseek(fp, 0, SEEK_END);
        size_t iFileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<char> acBuffer(iFileSize);
        fread(acBuffer.data(), sizeof(char), iFileSize, fp);
        fclose(fp);

        std::unique_ptr<RenderDriver::Common::CBuffer> buffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
        bufferMap["Material ID"] = std::move(buffer);
        RenderDriver::Common::CBuffer* pBuffer = bufferMap["Material ID"].get();

        RenderDriver::Common::BufferDescriptor desc;
        desc.miSize = iFileSize;
        desc.mBufferUsage = RenderDriver::Common::BufferUsage(
            uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
            uint32_t(RenderDriver::Common::BufferUsage::TransferDest));
        pBuffer->create(
            desc,
            *pDevice
        );
        pBuffer->setID("Material ID");
        
        pRenderer->copyCPUToBuffer(
            pBuffer,
            acBuffer.data(),
            0,
            (uint32_t)iFileSize
        );
    }

    // mesh triangle ranges
    {
        // load binary data
        FILE* fp = fopen("d:\\Downloads\\Bistro_v4\\bistro2-triangles.bin", "rb");
        fseek(fp, 0, SEEK_END);
        size_t iFileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<char> acMeshBuffer(iFileSize);
        fread(acMeshBuffer.data(), sizeof(char), iFileSize, fp);
        fclose(fp);

        uint32_t const* paiMeshBuffer = (uint32_t*)acMeshBuffer.data();

        // info header
        uint32_t iNumMeshes = *paiMeshBuffer++;
        uint32_t iNumTotalVertices = *paiMeshBuffer++;
        uint32_t iNumTotalTriangles = *paiMeshBuffer++;
        uint32_t iVertexSize = *paiMeshBuffer++;
        uint32_t iTriangleStartOffset = *paiMeshBuffer++;

        struct MeshRange
        {
            uint32_t    miStart;
            uint32_t    miEnd;
        };

        // mesh ranges
        MeshRange* pMeshRange = (MeshRange*)paiMeshBuffer;
        std::vector<MeshRange> aMeshRanges;
        for(uint32_t iMesh = 0; iMesh < iNumMeshes; iMesh++)
        {
            
            MeshRange const& meshRange = *pMeshRange;
            aMeshRanges.push_back(meshRange);
            ++pMeshRange;
        }

        uint32_t iDataSize = (uint32_t)aMeshRanges.size() * sizeof(MeshRange);

        std::unique_ptr<RenderDriver::Common::CBuffer> buffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
        bufferMap["Mesh Triangle Index Ranges"] = std::move(buffer);
        RenderDriver::Common::CBuffer* pBuffer = bufferMap["Mesh Triangle Index Ranges"].get();

        RenderDriver::Common::BufferDescriptor desc;
        desc.miSize = iDataSize;
        desc.mBufferUsage = RenderDriver::Common::BufferUsage(
            uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
            uint32_t(RenderDriver::Common::BufferUsage::TransferDest));
        pBuffer->create(
            desc,
            *pDevice
        );
        pBuffer->setID("Mesh Triangle Index Ranges");

        pRenderer->copyCPUToBuffer(
            pBuffer,
            aMeshRanges.data(),
            0,
            (uint32_t)iDataSize
        );

    }



}