#include <render/DX12/RendererDX12.h>

#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/PhysicalDeviceDX12.h>
#include <render-driver/DX12/SwapChainDX12.h>
#include <render-driver/DX12/UtilsDX12.h>
#include <render-driver/PipelineInfo.h>
#include <render-driver/DX12/FenceDX12.h>
#include <render-driver/DX12/CommandAllocatorDX12.h>
#include <mesh-clusters/DX12/MeshClusterManagerDX12.h>

#include <strsafe.h>
#include <ShlObj_core.h>

#include <LogPrint.h>
#include <wtfassert.h>

#include <functional>
#include <sstream>
#include <thread>
#include <mutex>

#include <stb_image.h>
#include <stb_image_write.h>

#include "pix3.h"

#include <renderdoc/renderdoc_app.h>

#include <imgui/backends/imgui_impl_dx12.h>

//#include <aftermath/include/GFSDK_Aftermath.h>
//#include <aftermath/include/GFSDK_Aftermath_GpuCrashDump.h>
//#include <aftermath/include/GFSDK_Aftermath_GpuCrashDumpDecoding.h>

#include <iomanip>
#include <sstream>
#include <filesystem>

//#define RENDERDOC_CAPTURE
//#define GPU_VALIDATION 

//#define ENABLE_DEBUG_LAYER 1

//#define GPU_PIX_MARKER 

//#define USE_AFTERMATH 1

using namespace Microsoft::WRL;

struct CameraInfo
{
    vec4    mNearPlane;
    vec4    mFarPlane;
    vec4    mTopPlane;
    vec4    mBottomPlane;
    vec4    mLeftPlane;
    vec4    mRightPlane;
};

struct IndirectDrawCommand
{
    // D3D12_VERTEX_BUFFER_VIEW
    //uint64_t        mVertexBufferLocation;  // GPU address
    //uint32_t        mTotalVertexSizeInBytes;
    //uint32_t        mStrideInBytes;

    // D3D12_INDEX_BUFFER_VIEW 
    //uint64_t        mIndexBufferLocation;    // GPU address
    //uint32_t        mTotalIndexSizeInBytes;
    //uint32_t        mFormat;                // DXGI_FORMAT
    //
    //// D3D12_DRAW_ARGUMENTS
    uint32_t        mIndexCounterPerInstance;
    uint32_t        mInstanceCount;
    uint32_t        mStartIndexLocation;
    int32_t         mBaseVertexLocation;
    uint32_t        mStartInstanceLocation;

    // D3D12_DRAW_ARGUMENTS
    //uint32_t        miVertexCountPerInstance;
    //uint32_t        miInstanceCount;
    //uint32_t        miStartVertexLocation;
    //uint32_t        miStartInstanceLocation;
};

struct MeshInfo
{
    vec3                mPosition;
    vec3                mMaxBound;
    vec3                mMinBound;
    float               mPadding[3];
};

struct ViewProjectionMatrices
{
    mat4        mViewMatrix;
    mat4        mProjectionMatrix;
};

ViewProjectionMatrices viewProjectionMatrices;

uint32_t giNumDrawCommands = 0;

static ComPtr<ID3D12Fence> sDeviceRemoveFence;
static HANDLE sDeviceRemovedEvent;
static HANDLE sDeviceRemovedWaitHandle;

static std::mutex sAfterMathMutex;

#if defined(USE_AFTERMATH)
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
[&]() {                                                                                                 \
    GFSDK_Aftermath_Result _result = FC;                                                                \
    if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
    {                                                                                                   \
        std::string errorStr;                                                                               \
        if(_result == GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported)                                \
        {                                                                                                   \
            errorStr = "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";     \
        }                                                                                                   \
        else if(_result == GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported)                      \
        {                                                                                                       \
            errorStr = "Aftermath is incompatible with D3D API interception, such as PIX or Nsight Graphics.";  \
        }                                                                                                       \
        else                                                                                                    \
        {                                                                                                       \
            std::stringstream stream;                                                                           \
            stream << std::setfill('0') << std::setw(2 * sizeof(_result)) << std::hex << _result;               \
            errorStr = stream.str();                                                                            \
        }                                                                                                       \
        MessageBoxA(0, errorStr.c_str(), "Aftermath Error", MB_OK);                                             \
        assert(0);                                                                                              \
    }                                                                                                           \
}()

/*
**
*/
void ShaderDebugInfoCallback(
    const void* pShaderDebugInfo,
    const uint32_t shaderDebugInfoSize,
    void* pUserData)
{
    std::lock_guard<std::mutex> lock(sAfterMathMutex);

    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
        GFSDK_Aftermath_Version_API,
        pShaderDebugInfo,
        shaderDebugInfoSize,
        &identifier));
    std::vector<uint8_t> data((uint8_t*)pShaderDebugInfo, (uint8_t*)pShaderDebugInfo + shaderDebugInfoSize);

    Render::DX12::CRenderer* pRenderer = reinterpret_cast<Render::DX12::CRenderer*>(pUserData);

    std::string shaderName = "default";
    
    std::string const* pShaderFilePath = pRenderer->getShaderFilePath(identifier.id[0]);
    if(pShaderFilePath)
    {
        uint64_t iStart = pShaderFilePath->find_last_of("\\");
        if(iStart == std::string::npos)
        {
            iStart = pShaderFilePath->find_last_of("//");
        }
        shaderName = pShaderFilePath->substr(iStart + 1);
    }
    std::string filePath = "c:\\Users\\Dingwings\\after-math-dumps\\shader-" + std::to_string(identifier.id[0]) + "-" + std::to_string(identifier.id[1]) + ".nvdbg";
    FILE* fp = fopen(filePath.c_str(), "wb");
    fwrite(pShaderDebugInfo, sizeof(char), shaderDebugInfoSize, fp);
    fclose(fp);

    pRenderer->setShaderDebugInfo(identifier.id[0], data);
}

/*
**
*/
void CrashDumpDescriptionCallback(
    PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription,
    void* pUserData)
{
    std::lock_guard<std::mutex> lock(sAfterMathMutex);

    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "RenderWithMe");
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, "crash dump");
}

/*
**
*/
void ResolveMarkerCallback(
    const void* pMarkerData, 
    const uint32_t markerDataSize, 
    void* pUserData, 
    void** ppResolvedMarkerData, 
    uint32_t* pResolvedMarkerDataSize)
{
    std::lock_guard<std::mutex> lock(sAfterMathMutex);

    int iDebug = 1;
}

/*
**
*/
void ShaderDebugInfoLookupCallback(
    const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
    PFN_GFSDK_Aftermath_SetData setShaderDebugInfo,
    void* pUserData)
{
    Render::DX12::CRenderer* pRenderer = reinterpret_cast<Render::DX12::CRenderer*>(pUserData);
    std::string const* pShaderFilePath = pRenderer->getShaderFilePath(pIdentifier->id[0]);
    std::vector<uint8_t> const* pShaderDebugInfo = pRenderer->getShaderDebugInfo(pIdentifier->id[0]);
    if(pShaderDebugInfo)
    {
        setShaderDebugInfo(pShaderDebugInfo->data(), uint32_t(pShaderDebugInfo->size()));
        DEBUG_PRINTF("crash shader: %s\n", pShaderFilePath->c_str());
    }
}

/*
**
*/
void ShaderLookupCallback(
    const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash,
    PFN_GFSDK_Aftermath_SetData setShaderBinary,
    void* pUserData)
{
    Render::DX12::CRenderer* pRenderer = reinterpret_cast<Render::DX12::CRenderer*>(pUserData);
    std::vector<uint8_t> const* shaderBinary = pRenderer->getShaderBinary(pShaderHash->hash);
    std::string const* pShaderFilePath = pRenderer->getShaderFilePath(pShaderHash->hash);
    if(shaderBinary != nullptr)
    {
        setShaderBinary(shaderBinary->data(), uint32_t(shaderBinary->size()));
    }
}

/*
**
*/
void ShaderSourceDebugInfoLookupCallback(
    const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
    PFN_GFSDK_Aftermath_SetData setShaderBinary,
    void* pUserData)
{
    std::lock_guard<std::mutex> lock(sAfterMathMutex);

    int iDebug = 1;
}

/*
**
*/
void GpuCrashDumpCallback(
    const void* pGpuCrashDump,
    const uint32_t gpuCrashDumpSize,
    void* pUserData)
{
    std::lock_guard<std::mutex> lock(sAfterMathMutex);

    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
        GFSDK_Aftermath_Version_API,
        pGpuCrashDump,
        gpuCrashDumpSize,
        &decoder));

    // Use the decoder object to read basic information, like application
    // name, PID, etc. from the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
    GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo);

    uint32_t applicationNameLength = 0;
    GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
        decoder,
        GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
        &applicationNameLength);

    std::vector<char> applicationName(applicationNameLength, '\0');
    GFSDK_Aftermath_GpuCrashDump_GetDescription(
        decoder,
        GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
        uint32_t(applicationName.size()),
        applicationName.data());

    uint32_t jsonSize = 0;
    GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
        decoder,
        GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
        GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
        ShaderDebugInfoLookupCallback,
        ShaderLookupCallback,
        ShaderSourceDebugInfoLookupCallback,
        pUserData,
        &jsonSize);

    std::vector<char> json(jsonSize);
    GFSDK_Aftermath_GpuCrashDump_GetJSON(
        decoder,
        uint32_t(json.size()),
        json.data());

    FILE* fp = fopen("c:\\Users\\Dingwings\\after-math-dumps\\crash-dump.nv-gpudmp", "wb");
    fwrite((char const*)pGpuCrashDump, sizeof(char), gpuCrashDumpSize, fp);
    fclose(fp);

    fp = fopen("c:\\Users\\Dingwings\\after-math-dumps\\crash-dump.json", "wb");
    fwrite((char const*)json.data(), sizeof(char), json.size() - 1, fp);
    fclose(fp);

    Render::DX12::CRenderer* pRenderer = reinterpret_cast<Render::DX12::CRenderer*>(pUserData);

    GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder);
}
#endif // #if USE_AFTERMATH


namespace Render
{
    namespace DX12
    {
        static std::wstring GetLatestWinPixGpuCapturerPath();
        
        /*
        **
        */
        void CRenderer::platformSetup(Render::Common::RendererDescriptor const& desc)
        {
#if !defined(USE_AFTERMATH)
            if(GetModuleHandleA("WinPixGpuCapturer.dll") == 0)
            {
                LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
            }
#endif // USE_AFTERMATH

            mpDevice = std::make_unique<RenderDriver::DX12::CDevice>();
            mpPhysicalDevice = std::make_unique<RenderDriver::DX12::CPhysicalDevice>();
            mpComputeCommandQueue = std::make_unique<RenderDriver::DX12::CCommandQueue>();
            mpGraphicsCommandQueue = std::make_unique<RenderDriver::DX12::CCommandQueue>();
            mpCopyCommandQueue = std::make_unique<RenderDriver::DX12::CCommandQueue>();
            mpGPUCopyCommandQueue = std::make_unique<RenderDriver::DX12::CCommandQueue>();
            mpSwapChain = std::make_unique<RenderDriver::DX12::CSwapChain>();

            mpUploadCommandAllocator = std::make_unique<RenderDriver::DX12::CCommandAllocator>();
            mpUploadFence = std::make_unique<RenderDriver::DX12::CFence>();
            mpUploadCommandBuffer = std::make_unique<RenderDriver::DX12::CCommandBuffer>();

            mpCopyTextureCommandAllocator = std::make_unique<RenderDriver::DX12::CCommandAllocator>();
            mpCopyTextureCommandBuffer = std::make_unique<RenderDriver::DX12::CCommandBuffer>();
            //mpCopyTextureFence = std::make_unique<RenderDriver::DX12::CFence>();

            mpReadBackBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();
            mpIntermediateProbeImageBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();

            mpTotalMeshesVertexBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();
            mpTotalMeshesIndexBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();

            RenderDriver::DX12::CDevice* nativeDevice = static_cast<RenderDriver::DX12::CDevice*>(mpDevice.get());

            //mpSerializer = std::make_unique<Render::DX12::Serializer>();

#if defined(ENABLE_DEBUG_LAYER) && !defined(USE_AFTERMATH)
            ComPtr<ID3D12Debug3> spDebugController;
            HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController));
            WTFASSERT(SUCCEEDED(hr), "Failed to get debug interface");
            spDebugController->SetEnableGPUBasedValidation(FALSE);
            spDebugController->EnableDebugLayer();
#endif // ENABLE_DEBUG_LAYER

#if defined(GPU_VALIDATION)
            {
                ComPtr<ID3D12Debug3> spDebugController;
                HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController));
                WTFASSERT(SUCCEEDED(hr), "Failed to get debug interface");
                spDebugController->SetEnableGPUBasedValidation(true);
                spDebugController->EnableDebugLayer();
                spDebugController->SetEnableSynchronizedCommandQueueValidation(true);
                spDebugController->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING);

                ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;
                hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings));
                WTFASSERT(SUCCEEDED(hr), "Failed to get DRED interface");
                pDredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                pDredSettings->SetWatsonDumpEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }

#endif // GPU_VALIDATION

#if defined(USE_AFTERMATH)
            GFSDK_Aftermath_EnableGpuCrashDumps(
                GFSDK_Aftermath_Version_API,
                GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
                GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default,
                GpuCrashDumpCallback,
                ShaderDebugInfoCallback,
                CrashDumpDescriptionCallback,
                ResolveMarkerCallback,
                this);
#endif // USE_AFTERMATH

            RenderDriver::Common::CPhysicalDevice::Descriptor physicalDeviceDesc = { RenderDriver::Common::CPhysicalDevice::AdapterType::Hardware };
            mpPhysicalDevice->create(physicalDeviceDesc);

            RenderDriver::Common::CDevice::CreateDesc createDesc;
            createDesc.mpPhyiscalDevice = mpPhysicalDevice.get();
            mpDevice->create(createDesc);
            mpDevice->setID("D3D12 Device");

#if defined(USE_AFTERMATH)
            // after math
            {
                // shader binary sources and debug info
                for(auto const& dirEntry : std::filesystem::recursive_directory_iterator("C:\\Users\\Dingwings\\test\\projects\\mesh_parameterization\\assets\\indirect_draw_pipeline\\output"))
                {
                    std::string const& pathStr = dirEntry.path().string();
                    if((pathStr.find(".cso") != std::string::npos || pathStr.find(".fso") != std::string::npos) && dirEntry.is_regular_file() && pathStr.find("spv") == std::string::npos)
                    {
                        FILE* fp = fopen(pathStr.c_str(), "rb");
                        fseek(fp, 0, SEEK_END);
                        uint64_t iSize = ftell(fp);
                        fseek(fp, 0, SEEK_SET);
                        std::vector<uint8_t> acBuffer(iSize);
                        fread(acBuffer.data(), sizeof(uint8_t), acBuffer.size(), fp);
                        fclose(fp);

                        uint64_t iStart = pathStr.find_last_of("\\");
                        if(iStart == std::string::npos)
                        {
                            iStart = pathStr.find_last_of("/");
                        }
                        std::string fileName = pathStr.substr(iStart + 1);
                        assert(iStart != std::string::npos);
                        uint64_t iEnd = pathStr.find(".");
                        assert(iEnd != std::string::npos);
                        std::string baseName = pathStr.substr(iStart + 1, iEnd - iStart - 1);
                        std::string directory = pathStr.substr(0, iStart);
                        std::string pdbFilePath = directory + "\\" + baseName + ".pdb";

                        fp = fopen(pdbFilePath.c_str(), "rb");
                        assert(fp != nullptr);
                        fseek(fp, 0, SEEK_END);
                        iSize = ftell(fp);
                        fseek(fp, 0, SEEK_SET);
                        std::vector<uint8_t> acPDBBuffer(iSize);
                        fread(acPDBBuffer.data(), sizeof(uint8_t), acPDBBuffer.size(), fp);
                        fclose(fp);

                        const D3D12_SHADER_BYTECODE shader{ acBuffer.data(), acBuffer.size() };
                        GFSDK_Aftermath_ShaderBinaryHash shaderHash;
                        GFSDK_Aftermath_GetShaderHash(
                            GFSDK_Aftermath_Version_API,
                            &shader,
                            &shaderHash);
                        mShaderBinariesDB[shaderHash.hash].swap(acBuffer);
                        mShaderBinaryFilePath[shaderHash.hash] = pathStr;
                        mShaderBinaryPDB[shaderHash.hash].swap(acPDBBuffer);
                    }
                }

                uint32_t const iAfterMathFlags =
                    GFSDK_Aftermath_FeatureFlags_EnableMarkers |
                    GFSDK_Aftermath_FeatureFlags_EnableResourceTracking |
                    GFSDK_Aftermath_FeatureFlags_CallStackCapturing |
                    GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo |
                    GFSDK_Aftermath_FeatureFlags_EnableShaderErrorReporting;

                GFSDK_Aftermath_Result ret = GFSDK_Aftermath_DX12_Initialize(
                    GFSDK_Aftermath_Version_API,
                    iAfterMathFlags,
                    reinterpret_cast<ID3D12Device*>(mpDevice->getNativeDevice()));

                DEBUG_PRINTF("ret = 0x%X\n", ret);

            }   // aftermath
#endif // USE_AFTERMATH

#if defined(ENABLE_DEBUG_LAYER) && !defined(USE_AFTERMATH)
            {
                ID3D12InfoQueue* pInfoQueue = nullptr;
                ID3D12Device* pDevice = static_cast<ID3D12Device*>(mpDevice->getNativeDevice());
                pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
#if !defined(PROFILE_BUILD)
                if(pInfoQueue)
                {
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                }
#endif // PROFILE_BUILD

                //pInfoQueue->Release();
            }
#endif // ENABLE_DEBUG_LAYER

            //{
            //    ComPtr<ID3D12DebugDevice1> pDebugDevice1;
            //    static_cast<ID3D12Device*>(mpDevice->getNativeDevice())->QueryInterface<ID3D12DebugDevice1>(&pDebugDevice1);
            //    D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS debugSetting =
            //    {
            //        256,
            //        D3D12_GPU_BASED_VALIDATION_SHADER_PATCH_MODE_GUARDED_VALIDATION,
            //        D3D12_GPU_BASED_VALIDATION_PIPELINE_STATE_CREATE_FLAG_FRONT_LOAD_CREATE_GUARDED_VALIDATION_SHADERS
            //    };
            //    pDebugDevice1->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, &debugSetting, sizeof(debugSetting));
            //    
            //    //ComPtr<ID3D12DebugDevice2> pDebugDevice2;
            //    //static_cast<ID3D12Device*>(mpDevice->getNativeDevice())->QueryInterface<ID3D12DebugDevice2>(&pDebugDevice2);
            //    //uint32_t iBits = D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS | D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING;
            //    //pDebugDevice2->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &iBits, sizeof(iBits));
            //
#if defined(GPU_VALIDATION)
            ComPtr<ID3D12InfoQueue> infoQueue;
            if(SUCCEEDED(static_cast<ID3D12Device*>(mpDevice->getNativeDevice())->QueryInterface(IID_PPV_ARGS(&infoQueue))))
            {
#if !defined(PROFILE_BUILD)
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
#endif // PROFILE_BUILD
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_COMMAND_ALLOCATOR_SYNC, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_COMMAND_ALLOCATOR_CANNOT_RESET, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_DEVICE_REMOVAL_PROCESS_AT_FAULT, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_DEVICE_REMOVAL_PROCESS_POSSIBLY_AT_FAULT, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_DEVICE_REMOVAL_PROCESS_NOT_AT_FAULT, TRUE);
            }
            //
            //}
            ComPtr<ID3D12DebugDevice1> pDeviceDebug;
            static_cast<ID3D12Device*>(mpDevice->getNativeDevice())->QueryInterface<ID3D12DebugDevice1>(&pDeviceDebug);
            D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS debugSetting =
            {
                256,
                D3D12_GPU_BASED_VALIDATION_SHADER_PATCH_MODE_UNGUARDED_VALIDATION,
                D3D12_GPU_BASED_VALIDATION_PIPELINE_STATE_CREATE_FLAG_FRONT_LOAD_CREATE_UNGUARDED_VALIDATION_SHADERS
            };
            pDeviceDebug->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, &debugSetting, sizeof(debugSetting));

            //ComPtr<ID3D12InfoQueue> infoQueue;
            if(SUCCEEDED(static_cast<ID3D12Device*>(mpDevice->getNativeDevice())->QueryInterface(IID_PPV_ARGS(&infoQueue))))
            {
#if !defined(PROFILE_BUILD)
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif // PROFILE_BUILD
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET, FALSE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_COMMAND_LIST_OPEN, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_COMMAND_LIST_CLOSED, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_COMMAND_ALLOCATOR_SYNC, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_COMMAND_ALLOCATOR_CANNOT_RESET, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_DEVICE_REMOVAL_PROCESS_AT_FAULT, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_DEVICE_REMOVAL_PROCESS_POSSIBLY_AT_FAULT, TRUE);
                infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_DEVICE_REMOVAL_PROCESS_NOT_AT_FAULT, TRUE);
            }
#endif // GPU_VALIDATION

            {
                HRESULT hr = static_cast<ID3D12Device*>(mpDevice->getNativeDevice())->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&sDeviceRemoveFence));
                assert(SUCCEEDED(hr));
                sDeviceRemovedEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                assert(sDeviceRemovedEvent != nullptr);
                RegisterWaitForSingleObject(
                    &sDeviceRemovedWaitHandle,
                    sDeviceRemovedEvent,
                    [](PVOID context, BOOLEAN)
                    {
                        ID3D12Device* pDevice = static_cast<ID3D12Device*>(context);
                        HRESULT hr = pDevice->GetDeviceRemovedReason();
                        DEBUG_PRINTF("device removed\n");
                    },
                    static_cast<ID3D12Device*>(mpDevice->getNativeDevice()),
                        INFINITE,
                        0);
                sDeviceRemoveFence->SetEventOnCompletion(UINT64_MAX, sDeviceRemovedEvent);
            }

            RenderDriver::Common::CCommandQueue::CreateDesc queueCreateDesc;
            queueCreateDesc.mpDevice = mpDevice.get();
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::Graphics;
            mpGraphicsCommandQueue->create(queueCreateDesc);
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::Compute;
            mpComputeCommandQueue->create(queueCreateDesc);
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::Copy;
            mpCopyCommandQueue->create(queueCreateDesc);
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::CopyGPU;
            mpGPUCopyCommandQueue->create(queueCreateDesc);

            mpGraphicsCommandQueue->setID("Graphics Command Queue");
            mpComputeCommandQueue->setID("Compute Command Queue");
            mpCopyCommandQueue->setID("Copy Command Queue");
            mpGPUCopyCommandQueue->setID("GPU Copy Command Queue");

            Render::DX12::RendererDescriptor const& dx12Desc = static_cast<Render::DX12::RendererDescriptor const&>(desc);

            // create swap chain
            RenderDriver::DX12::SwapChainDescriptor swapChainDesc = {};
            swapChainDesc.miWidth = dx12Desc.miScreenWidth;
            swapChainDesc.miHeight = dx12Desc.miScreenHeight;
            swapChainDesc.mFormat = dx12Desc.mFormat;
            swapChainDesc.miSamples = 1;
            swapChainDesc.miFlags = dx12Desc.miSwapChainFlags;
            swapChainDesc.miNumBuffers = dx12Desc.miNumBuffers;
            swapChainDesc.mpGraphicsCommandQueue = static_cast<RenderDriver::Common::CCommandQueue*>(mpGraphicsCommandQueue.get());
            swapChainDesc.mpPhysicalDevice = static_cast<RenderDriver::Common::CPhysicalDevice*>(mpPhysicalDevice.get());
            swapChainDesc.mHWND = dx12Desc.mHWND;
            swapChainDesc.mpDevice = mpDevice.get();

            // create render targets for swap chain
            mpSwapChain->create(swapChainDesc, *mpDevice);

            ID3D12Device* pDeviceDX12 = static_cast<ID3D12Device*>(nativeDevice->getNativeDevice());
            miDescriptorHeapOffset = pDeviceDX12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            Render::Common::InitializeVertexAndIndexBufferDescriptor initializeVBDesc =
            {
                /* .miVertexDataSize    */          1 << 27,
                /* .miInidexDataSize    */          1 << 27,
            };
            //initializeTotalVertexAndIndexBuffer(initializeVBDesc);

            mHWND = dx12Desc.mHWND;

#if defined(RENDERDOC_CAPTURE)
            if(GetModuleHandleA("renderdoc.dll") == 0)
            {
                LoadLibrary(L"./renderdoc.dll");
                HMODULE mod = GetModuleHandleA("renderdoc.dll");
            
                pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
                int32_t iRet = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void**)&mpRenderDocAPI);
                WTFASSERT(iRet == 1, "Can\'t initialize renderdoc");
            }
#endif // RENDERDOC_CAPTURE

            {
                RenderDriver::Common::FenceDescriptor fenceDesc = {};
                mpUploadFence->create(fenceDesc, *mpDevice);
                mpUploadFence->setID("Upload Fence");
            
                // upload command buffer allocator
                RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc = {};
                commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                mpUploadCommandAllocator->create(commandAllocatorDesc, *mpDevice);
                mpUploadCommandAllocator->setID("Upload Command Allocator");
            
                // upload command buffer
                RenderDriver::Common::CommandBufferDescriptor commandBufferDesc = {};
                commandBufferDesc.mpCommandAllocator = mpUploadCommandAllocator.get();
                commandBufferDesc.mpPipelineState = nullptr;
                commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                mpUploadCommandBuffer->create(commandBufferDesc, *mpDevice);
                mpUploadCommandBuffer->setID("Upload Command Buffer");
            }

            // Imgui
            {
                // shader resource descriptor heap
                RenderDriver::Common::DescriptorHeapDescriptor imguiDescriptorHeapDesc =
                {
                    /* DescriptorHeapType          mType                */  RenderDriver::Common::DescriptorHeapType::General,
                    /* DescriptorHeapFlag          mFlag                */  RenderDriver::Common::DescriptorHeapFlag::ShaderVisible,
                    /* uint32_t                    miNumDescriptors     */  64,
                };
                std::unique_ptr<RenderDriver::DX12::CDescriptorHeap> pImGuiDescriptorHeap = std::make_unique<RenderDriver::DX12::CDescriptorHeap>();
                PLATFORM_OBJECT_HANDLE imguiDescriptorHeapHandle = pImGuiDescriptorHeap->create(imguiDescriptorHeapDesc, *mpDevice);
                //mpImguiDescriptorHeap = static_cast<Render::DX12::Serializer*>(mpSerializer.get())->registerObject(
                //    pImGuiDescriptorHeap,
                //    imguiDescriptorHeapHandle);
                mpImguiDescriptorHeap->setID("Imgui Descriptor Heap");
                
            }   // Imgui

            //{
            //    mpSwapChainFence = std::make_unique<RenderDriver::DX12::CFence>();
            //    RenderDriver::Common::FenceDescriptor swapChainFenceDesc;
            //    mpSwapChainFence->create(swapChainFenceDesc, *mpDevice);
            //    mpSwapChainFence->setID("Swap Chain Fence");
            //}

            RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc;
            RenderDriver::Common::CommandBufferDescriptor commandBufferDesc;

            mPreRenderCopyJob.mCommandAllocator = std::make_shared<RenderDriver::DX12::CCommandAllocator>();
            commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
            mPreRenderCopyJob.mCommandAllocator->create(commandAllocatorDesc, *mpDevice);
            mPreRenderCopyJob.mCommandAllocator->setID("PreRender Copy Job Command Allocator");

            mPreRenderCopyJob.mCommandBuffer = std::make_shared<RenderDriver::DX12::CCommandBuffer>();
            commandBufferDesc.mpCommandAllocator = mPreRenderCopyJob.mCommandAllocator.get();
            commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
            commandBufferDesc.mpPipelineState = nullptr;
            mPreRenderCopyJob.mCommandBuffer->create(
                commandBufferDesc,
                *mpDevice);
            mPreRenderCopyJob.mCommandBuffer->setID("PreRender Copy Job Command Buffer");

            mapQueueCommandAllocators.resize(RenderDriver::Common::CCommandQueue::NumTypes);
            mapQueueCommandBuffers.resize(RenderDriver::Common::CCommandQueue::NumTypes);
            for(uint32_t i = 0; i < static_cast<uint32_t>(mapQueueCommandBuffers.size()); i++)
            {
                std::string name = "Graphics Queue";
                if(i == RenderDriver::Common::CCommandQueue::Type::Compute)
                {
                    name = "Compute Queue";
                }
                else if(i == RenderDriver::Common::CCommandQueue::Type::Copy)
                {
                    name = "Copy Queue";
                }

                mapQueueCommandAllocators[i] = std::make_shared<RenderDriver::DX12::CCommandAllocator>();
                mapQueueCommandBuffers[i] = std::make_shared<RenderDriver::DX12::CCommandBuffer>();
                
                commandAllocatorDesc.mType = static_cast<RenderDriver::Common::CommandBufferType>(i);
                mapQueueCommandAllocators[i]->create(commandAllocatorDesc, *mpDevice);
                mapQueueCommandAllocators[i]->setID(name + " Command Allocator");

                commandBufferDesc.mpCommandAllocator = mapQueueCommandAllocators[i].get();
                commandBufferDesc.mType = commandAllocatorDesc.mType;
                commandBufferDesc.mpPipelineState = nullptr;
                mapQueueCommandBuffers[i]->create(commandBufferDesc, *mpDevice);
                mapQueueCommandBuffers[i]->setID(name + " Command Buffer");

                mapQueueCommandBuffers[i]->close();
                mapQueueCommandAllocators[i]->reset();
                mapQueueCommandBuffers[i]->reset();
            }

            mpScratchBuffer0 = std::make_unique<RenderDriver::DX12::CBuffer>();

#if defined(RUN_TEST_LOCALLY)
            extern std::unique_ptr<RenderDriver::Common::CBuffer> gpCopyBuffer;
            gpCopyBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();
#endif // TEST_HLSL_LOCALLY

            mDefaultUniformBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();
            RenderDriver::Common::BufferDescriptor bufferCreationDesc = {};
            bufferCreationDesc.miSize = 1024;
            bufferCreationDesc.mBufferUsage = RenderDriver::Common::BufferUsage::UniformBuffer;
            bufferCreationDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            mDefaultUniformBuffer->create(
                bufferCreationDesc,
                *mpDevice
            );
            mDefaultUniformBuffer->setID("Default Uniform Buffer");
            mpDefaultUniformBuffer = mDefaultUniformBuffer.get();

            mRenderDriverType = Render::Common::RenderDriverType::DX12;
        }

#if 0
        /*
        **
        */
        void CRenderer::platformBeginRenderPass(Render::Common::RenderPassDescriptor& renderPassDesc)
        {
        }

        /*
        **
        */
        void CRenderer::platformEndRenderPass(Render::Common::RenderPassDescriptor& renderPassDesc)
        {
        }
#endif // #if 0

        /*
        **
        */
        void CRenderer::platformUploadResourceData(
            RenderDriver::Common::CBuffer& buffer,
            void* pRawSrcData,
            uint64_t iDataSize,
            uint64_t iDestDataOffset)
        {
            char szOutput[256];
            sprintf(szOutput, "Upload Resource %s (%lld bytes)\n",
                buffer.getID().c_str(),
                iDataSize);

            PIXBeginEvent(
#if defined(GPU_PIX_MARKER)
                static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue()),
#endif // GPU_PIX_MARKER
                0xff0000, 
                szOutput);

            RenderDriver::Common::Utils::TransitionBarrierInfo aBarrierInfo[1];
            aBarrierInfo[0].mpBuffer = &buffer;
            aBarrierInfo[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
            aBarrierInfo[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopyDestination;
            platformTransitionBarriers(
                aBarrierInfo,
                *mpUploadCommandBuffer,
                1,
                false);

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::DX12::CBuffer>());

            RenderDriver::DX12::CBuffer& uploadBuffer = static_cast<RenderDriver::DX12::CBuffer&>(*mapUploadBuffers.back());
            RenderDriver::Common::BufferDescriptor uploadBufferDesc = {};

            uploadBufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
            uploadBufferDesc.miSize = iDataSize;
            uploadBuffer.create(uploadBufferDesc, *mpDevice);
            uploadBuffer.setID("UploadBuffer");


            uint8_t* pData = nullptr;
            HRESULT hr = static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer())->Map(0, nullptr, reinterpret_cast<void**>(&pData));
            assert(SUCCEEDED(hr));
            memcpy(pData, pRawSrcData, iDataSize);
            static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer())->Unmap(0, nullptr);

            WTFASSERT(iDestDataOffset < UINT32_MAX, "Invalid destination data offset: %d\n", iDestDataOffset);

            static_cast<ID3D12GraphicsCommandList*>(mpUploadCommandBuffer->getNativeCommandList())->CopyBufferRegion(
                static_cast<ID3D12Resource*>(buffer.getNativeBuffer()),
                iDestDataOffset,
                static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer()),
                0,
                uploadBufferDesc.miSize);


#if defined(_DEBUG)
            mpUploadCommandBuffer->addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_COPY);
#endif // DEBUG

            platformTransitionBarriers(
                aBarrierInfo,
                *mpUploadCommandBuffer,
                1,
                true);

            PIXEndEvent(
#if defined(GPU_PIX_MARKER)
                static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue())
#endif // GPU_PIX_MARKER
            );
        }

        /*
        **
        */
        void CRenderer::platformSetComputeDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            ID3D12RootSignature* rootSignature = static_cast<ID3D12RootSignature*>(descriptorSet.getNativeDescriptorSet());
            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->SetComputeRootSignature(rootSignature);
        }

        /*
        **
        */
        void CRenderer::platformSetRayTraceDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
        }

        /*
        **
        */
        void CRenderer::platformSetGraphicsDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            ID3D12RootSignature* rootSignature = static_cast<ID3D12RootSignature*>(descriptorSet.getNativeDescriptorSet());
            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->SetGraphicsRootSignature(rootSignature);
        }

        /*
        **
        */
        void CRenderer::platformSetGraphicsRoot32BitConstant(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iValue,
            uint32_t iRootParameterIndex,
            uint32_t iOffset)
        {
            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->SetGraphicsRoot32BitConstant(iRootParameterIndex, iValue, iOffset);
        }

        /*
        **
        */
        void CRenderer::platformSetGraphicsRoot32BitConstants(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            void const* paValues,
            uint32_t iNumValues,
            uint32_t iRootParameterIndex,
            uint32_t iOffsetIn32Bits)
        {
            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->SetGraphicsRoot32BitConstants(iRootParameterIndex, iNumValues, paValues, iOffsetIn32Bits);
        }

        /*
        **
        */
        void CRenderer::platformSetComputeRoot32BitConstant(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iValue,
            uint32_t iRootParameterIndex,
            uint32_t iOffset)
        {
            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->SetComputeRoot32BitConstant(iRootParameterIndex, iValue, iOffset);
        }

        /*
        **
        */
        void CRenderer::platformSetComputeRoot32BitConstants(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState,
            void const* paValues,
            uint32_t iNumValues,
            uint32_t iRootParameterIndex,
            uint32_t iOffsetIn32Bits)
        {
            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->SetComputeRoot32BitConstants(iRootParameterIndex, iNumValues, paValues, iOffsetIn32Bits);
        }

        /*
        **
        */
        void CRenderer::platformSetDescriptorHeap(
            RenderDriver::Common::CDescriptorHeap& descriptorHeap,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            ID3D12DescriptorHeap* aDescriptorHeaps[] =
            {
                static_cast<ID3D12DescriptorHeap*>(descriptorHeap.getNativeDescriptorHeap()),
            };
            UINT iNumDescriptorHeaps = sizeof(aDescriptorHeaps) / sizeof(*aDescriptorHeaps);
            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->SetDescriptorHeaps(
                iNumDescriptorHeaps,
                aDescriptorHeaps);
        }

        /*
        **
        */
        void CRenderer::platformSetResourceViews(
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CDescriptorHeap& descriptorHeap,
            uint32_t iTripleBufferIndex,
            Render::Common::JobType jobType)
        {
            ID3D12GraphicsCommandList* commandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());

            // set resource views
            for(uint32_t i = 0; i < static_cast<uint32_t>(aShaderResources.size()); i++)
            {
                SerializeUtils::Common::ShaderResourceInfo const& shaderResourceInfo = aShaderResources[i];
                
                bool bIsFiller = (aShaderResources[i].mName.find("fillerShaderResource") != std::string::npos || aShaderResources[i].mName.find("fillerUnorderedAccessResource") != std::string::npos);
                ShaderResourceType shaderResourceType = (bIsFiller == true) ? ShaderResourceType::RESOURCE_TYPE_BUFFER_IN : aShaderResources[i].mType;
                
                bool bIsBuffer = (shaderResourceType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN ||
                    shaderResourceType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT ||
                    shaderResourceType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT);

                D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
                if(bIsBuffer)
                {
                    if(shaderResourceInfo.maHandles[iTripleBufferIndex] > 0)
                    {
                        //RenderDriver::Common::CBuffer& buffer = *mpSerializer->getBuffer(shaderResourceInfo.maHandles[iTripleBufferIndex]).get();
                        //gpuAddress = static_cast<ID3D12Resource*>(buffer.getNativeBuffer())->GetGPUVirtualAddress();
                    }
                    else
                    {
                        RenderDriver::DX12::CBuffer* pDX12Buffer = static_cast<RenderDriver::DX12::CBuffer*>(shaderResourceInfo.mExternalResource.mpBuffer);
                        if(pDX12Buffer == nullptr)
                        {
                            gpuAddress = 0;             // null descriptor
                        }
                        else
                        {
                            gpuAddress = static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer())->GetGPUVirtualAddress() + shaderResourceInfo.mExternalResource.miGPUAddressOffset;
                        }
                    }

                    if(shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::ConstantBufferView)
                    {
                        if(jobType == Render::Common::JobType::Compute)
                        {
                            commandList->SetComputeRootConstantBufferView(i, gpuAddress);
                        }
                        else
                        {
                            //commandList->SetGraphicsRootConstantBufferView(i, gpuAddress);
                        }
                    }
                    else if(shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::ShaderResourceView)
                    {
                        if(jobType == Render::Common::JobType::Compute)
                        {
                            commandList->SetComputeRootShaderResourceView(i, gpuAddress);
                        }
                        else
                        {
                            commandList->SetGraphicsRootShaderResourceView(i, gpuAddress);
                        }
                    }
                    else if(shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView)
                    {
                        if(jobType == Render::Common::JobType::Compute)
                        {
                            commandList->SetComputeRootUnorderedAccessView(i, gpuAddress);
                        }
                        else
                        {
                            commandList->SetGraphicsRootUnorderedAccessView(i, gpuAddress);
                        }
                    }
                }
                else
                {
                    ID3D12DescriptorHeap* pDX12DescriptorHeap = static_cast<ID3D12DescriptorHeap*>(descriptorHeap.getNativeDescriptorHeap());
                    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle = pDX12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
                    gpuDescriptorHandle.ptr += (i * miDescriptorHeapOffset);

                    if(jobType == Render::Common::JobType::Compute)
                    {
                        commandList->SetComputeRootDescriptorTable(i, gpuDescriptorHandle);
                    }
                    else
                    {
                        commandList->SetGraphicsRootDescriptorTable(i, gpuDescriptorHandle);
                    }
                }
            }
        }

        /*
        **
        */
        void CRenderer::platformSetViewportAndScissorRect(
            uint32_t iWidth,
            uint32_t iHeight,
            float fMaxDepth,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            D3D12_VIEWPORT viewPort;
            viewPort.Width = static_cast<float>(iWidth);
            viewPort.Height = static_cast<float>(iHeight);
            viewPort.MinDepth = 0.0f;
            viewPort.MaxDepth = fMaxDepth;
            viewPort.TopLeftX = 0.0f;
            viewPort.TopLeftY = 0.0f;

            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->RSSetViewports(1, &viewPort);

            D3D12_RECT scissorRect = {};
            scissorRect.right = iWidth;
            scissorRect.bottom = iHeight;
            dx12CommandList->RSSetScissorRects(1, &scissorRect);
        }

        /*
        **
        */
        void CRenderer::platformSetRenderTargetAndClear2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
            std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
            uint32_t iNumRenderTargetAttachments,
            std::vector<std::vector<float>> const& aafClearColors,
            std::vector<bool> const& abClear)
        {
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> aDescriptorHandles(iNumRenderTargetAttachments);
            for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
            {
                aDescriptorHandles[i] = static_cast<ID3D12DescriptorHeap*>(apRenderTargetDescriptorHeaps[i]->getNativeDescriptorHeap())->GetCPUDescriptorHandleForHeapStart();
            }

            D3D12_CPU_DESCRIPTOR_HANDLE depthStencilRTDescriptorHeapHandle = { 0 };
            if(apDepthStencilDescriptorHeaps.size() > 0)
            {
                depthStencilRTDescriptorHeapHandle = static_cast<ID3D12DescriptorHeap*>(apDepthStencilDescriptorHeaps[0]->getNativeDescriptorHeap())->GetCPUDescriptorHandleForHeapStart();
            }

            ID3D12GraphicsCommandList* dx12CommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            dx12CommandList->OMSetRenderTargets(
                iNumRenderTargetAttachments,
                aDescriptorHandles.data(),
                false,
                depthStencilRTDescriptorHeapHandle.ptr == 0 ? nullptr : &depthStencilRTDescriptorHeapHandle);

            for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
            {
                if(abClear[i])
                {
                    dx12CommandList->ClearRenderTargetView(
                        aDescriptorHandles[i],
                        aafClearColors[i].data(),
                        0,
                        nullptr);
                }
            }

            if(apDepthStencilDescriptorHeaps.size() > 0)
            {
                if(abClear[0])
                {
                    dx12CommandList->ClearDepthStencilView(
                        depthStencilRTDescriptorHeapHandle,
                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                        1.0f,
                        0,
                        0,
                        nullptr);
                }
            }
        }

        /*
        **
        */
        void CRenderer::platformCopyImageToCPUMemory(
            RenderDriver::Common::CImage* pGPUImage,
            std::vector<float>& afImageData)
        {
            RenderDriver::Common::ImageDescriptor const& desc = pGPUImage->getDescriptor();
            uint32_t iComponentSize = 4 * sizeof(char);
            switch(desc.mFormat)
            {
                case RenderDriver::Common::Format::R32G32B32A32_FLOAT:
                    iComponentSize = 4 * sizeof(float);
                    break;
                case RenderDriver::Common::Format::R32G32B32_FLOAT:
                    iComponentSize = 3 * sizeof(float);
                    break;
                case RenderDriver::Common::Format::R16G16B16A16_FLOAT:
                    iComponentSize = 4 * sizeof(int16_t);
                    break;
            }


            D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
            {
                static_cast<ID3D12Resource*>(pGPUImage->getNativeImage()),
                D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                0
            };

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::DX12::CBuffer>());
            auto& readBackBuffer = mapUploadBuffers.back();

            RenderDriver::Common::BufferDescriptor createBufferDesc =
            {
                /* uint64_t                    miSize;                                          */      1024 * 1024 * 4 * sizeof(float),
                /* Format                      mFormat;                                         */      RenderDriver::Common::Format::UNKNOWN,
                /* ResourceFlagBits            mFlags;                                          */      RenderDriver::Common::ResourceFlagBits::None,
                /* HeapType                    mHeapType = HeapType::Default;                   */      RenderDriver::Common::HeapType::ReadBack,
                /* ResourceStateFlagBits       mOverrideState = ResourceStateFlagBits::None;    */      RenderDriver::Common::ResourceStateFlagBits::CopyDestination,
                /* CBuffer* mpCounterBuffer = nullptr;                                          */      nullptr,
            };
            readBackBuffer->create(createBufferDesc, *mpDevice);

            uint32_t iTotalDataSize = desc.miWidth * desc.miHeight * iComponentSize;
            D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
            {
                static_cast<ID3D12Resource*>(readBackBuffer->getNativeBuffer()),             //ID3D12Resource* pResource;
                D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                {
                    0,
                    SerializeUtils::DX12::convert(desc.mFormat),              // DXGI_FORMAT Format;
                        desc.miWidth,                                        // UINT Width;
                        desc.miHeight,                                       // UINT Height;
                        1,                                                  // UINT Depth;
                        desc.miWidth * iComponentSize,                      // UINT RowPitch;
                }
            };

            static_cast<ID3D12GraphicsCommandList*>(mpUploadCommandBuffer->getNativeCommandList())->CopyTextureRegion(
                &destCopyLocation,
                0,
                0,
                0,
                &srcCopyLocation,
                nullptr);

            mpUploadCommandBuffer->close();
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);

            // wait until done
            RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
            placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
            placeFenceDesc.miFenceValue = miCopyCommandFenceValue++;
            mpUploadFence->place(placeFenceDesc);
            mpUploadFence->waitCPU(UINT64_MAX);

            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();

            D3D12_RANGE range;
            range.Begin = 0;
            range.End = iTotalDataSize;

            afImageData.resize(desc.miWidth * desc.miHeight * 4);
            void* pData = nullptr;
            static_cast<ID3D12Resource*>(readBackBuffer->getNativeBuffer())->Map(0, &range, &pData);
            memcpy(afImageData.data(), pData, iTotalDataSize);
            static_cast<ID3D12Resource*>(readBackBuffer->getNativeBuffer())->Unmap(0, nullptr);
        }

        std::mutex sMutex;

        /*
        **
        */
        void CRenderer::platformCopyBufferToCPUMemory(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize)
        {
            PIXBeginEvent(
#if defined(GPU_PIX_MARKER)
                static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue()),
#endif // GPU_PIX_MARKER
                0xff000000,
                "Copy GPU To CPU Data");

            RenderDriver::Common::Utils::TransitionBarrierInfo aBarrierInfo[1];
            aBarrierInfo[0].mpBuffer = pGPUBuffer;
            aBarrierInfo[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
            aBarrierInfo[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopySource;
            platformTransitionBarriers(
                aBarrierInfo,
                *mpUploadCommandBuffer,
                1,
                false);

            RenderDriver::DX12::CBuffer* pBufferDX12 = static_cast<RenderDriver::DX12::CBuffer*>(pGPUBuffer);
            static_cast<ID3D12GraphicsCommandList*>(mpUploadCommandBuffer->getNativeCommandList())->CopyBufferRegion(
                static_cast<ID3D12Resource*>(mpReadBackBuffer->getNativeBuffer()),
                0,
                static_cast<ID3D12Resource*>(pBufferDX12->getNativeBuffer()),
                iSrcOffset,
                iDataSize);

#if defined(_DEBUG)
            mpUploadCommandBuffer->addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_COPY);
#endif // DEBUG

            platformTransitionBarriers(
                aBarrierInfo,
                *mpUploadCommandBuffer,
                1,
                true);

            // exec command buffer
            mpUploadCommandBuffer->close();
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);

auto start = std::chrono::high_resolution_clock::now();

            // wait until done
            //RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
            //placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
            //placeFenceDesc.miFenceValue = miCopyCommandFenceValue++;
            //mpUploadFence->place(placeFenceDesc);
            //mpUploadFence->waitCPU(UINT64_MAX);
            mpUploadFence->signal(mpCopyCommandQueue.get(), ++miCopyCommandFenceValue);
            uint64_t iFenceValue = mpUploadFence->getFenceValue();
            while(iFenceValue < miCopyCommandFenceValue)
            {
                iFenceValue = mpUploadFence->getFenceValue();
            }
            
uint64_t iElapsed0 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

start = std::chrono::high_resolution_clock::now();

            D3D12_RANGE readbackRange = { 0, iDataSize };
            void* pData = nullptr;
            static_cast<ID3D12Resource*>(mpReadBackBuffer->getNativeBuffer())->Map(0, &readbackRange, &pData);

            memcpy(pCPUBuffer, pData, iDataSize);

            D3D12_RANGE emptyRange = { 0, 0 };
            static_cast<ID3D12Resource*>(mpReadBackBuffer->getNativeBuffer())->Unmap(0, &emptyRange);

uint64_t iElapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

            mpUploadCommandBuffer->reset();

            PIXEndEvent(
#if defined(GPU_PIX_MARKER)
                static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue())
#endif // GPU_PIX_MARKER
            );
        }

        /*
        **
        */
        void CRenderer::platformCopyBufferToCPUMemory2(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformUpdateTextureInArray(
            RenderDriver::Common::CImage& image,
            void const* pRawSrcData,
            uint32_t iImageWidth,
            uint32_t iImageHeight,
            RenderDriver::Common::Format const& format,
            uint32_t iTextureArrayIndex,
            uint32_t iBaseDataSize)
        {
            RenderDriver::DX12::CImage& imageDX12 = static_cast<RenderDriver::DX12::CImage&>(image);
        
            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::DX12::CBuffer>());
            RenderDriver::DX12::CBuffer& uploadBuffer = static_cast<RenderDriver::DX12::CBuffer&>(*mapUploadBuffers.back());
        
            uint32_t iNumBaseComponents = SerializeUtils::Common::getNumComponents(format);
            uint32_t iImageSize = iImageWidth * iImageHeight * iNumBaseComponents * iBaseDataSize;
        
            RenderDriver::Common::BufferDescriptor uploadBufferDesc =
            {
                /* .miSize          */      iImageSize,
                /* .mFormat         */      RenderDriver::Common::Format::UNKNOWN,
                /* .mFlags          */      RenderDriver::Common::ResourceFlagBits::None,
                /* .mHeapType       */      RenderDriver::Common::HeapType::Upload,
                /* .mOverrideState  */      RenderDriver::Common::ResourceStateFlagBits::None,
                /* .mpCounterBuffer */      nullptr,
            };
            uploadBuffer.create(uploadBufferDesc, *mpDevice);
            uploadBuffer.setID("Upload Image Buffer");
        
            uint8_t* pData = nullptr;
            HRESULT hr = static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer())->Map(0, nullptr, reinterpret_cast<void**>(&pData));
            assert(SUCCEEDED(hr));
            memcpy(pData, pRawSrcData, iImageSize);
            static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer())->Unmap(0, nullptr);
        
            D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
            {
                static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer()),             //ID3D12Resource* pResource;
                D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,      // D3D12_TEXTURE_COPY_TYPE Type;
                {
                    0,                                                      // UINT64 Offset
                    {                     
                        SerializeUtils::DX12::convert(format),              // DXGI_FORMAT Format;
                        iImageWidth,                                        // UINT Width;
                        iImageHeight,                                       // UINT Height;
                        1,                                                  // UINT Depth;
                        iImageWidth * iNumBaseComponents * iBaseDataSize     // UINT RowPitch;
                    }
                },
            };
        
            D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
            {
                static_cast<ID3D12Resource*>(imageDX12.getNativeImage()),             //ID3D12Resource* pResource;
                D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,      // D3D12_TEXTURE_COPY_TYPE Type;
                0 // iTextureArrayIndex
            };
        
            static_cast<ID3D12GraphicsCommandList*>(mpUploadCommandBuffer->getNativeCommandList())->CopyTextureRegion(
                &destCopyLocation,
                0,
                0,
                0,
                &srcCopyLocation,
                nullptr);

            mpUploadCommandBuffer->close();
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);

            RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
            placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
            placeFenceDesc.miFenceValue = ++miCopyCommandFenceValue;
            mpUploadFence->place(placeFenceDesc);
            mpUploadFence->waitCPU(UINT64_MAX);

            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();
        }

        /*
        **
        */
        void CRenderer::platformCopyImage2(
            RenderDriver::Common::CImage& destImage,
            RenderDriver::Common::CImage& srcImage,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            bool bSrcWritable)
        {
            RenderDriver::DX12::CImage& destImageDX12 = static_cast<RenderDriver::DX12::CImage&>(destImage);
            RenderDriver::DX12::CImage& srcImageDX12 = static_cast<RenderDriver::DX12::CImage&>(srcImage);

            ID3D12Resource* pSrcImageDX12 = static_cast<ID3D12Resource*>(srcImageDX12.getNativeImage());
            ID3D12Resource* pDestImageDX12 = static_cast<ID3D12Resource*>(destImageDX12.getNativeImage());

            WTFASSERT(pSrcImageDX12->GetDesc().Width == pDestImageDX12->GetDesc().Width, "Width do not match src \"%s\" %d != dest \"%s\" %d",
                srcImage.getID().c_str(),
                pSrcImageDX12->GetDesc().Width,
                destImage.getID().c_str(),
                pDestImageDX12->GetDesc().Width);
            WTFASSERT(pSrcImageDX12->GetDesc().Height == pDestImageDX12->GetDesc().Height, "Height do not match src \"%s\" %d != dest \"%s\" %d",
                srcImage.getID().c_str(),
                pSrcImageDX12->GetDesc().Height,
                destImage.getID().c_str(),
                pDestImageDX12->GetDesc().Height);



            D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
            {
                pSrcImageDX12,
                D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                0
            };

            D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
            {
                pDestImageDX12,             //ID3D12Resource* pResource;
                D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,      // D3D12_TEXTURE_COPY_TYPE Type;
                0
            };

            //ID3D12GraphicsCommandList* pCommandList = static_cast<ID3D12GraphicsCommandList*>(mpUploadCommandBuffer->getNativeCommandList());
            ID3D12GraphicsCommandList* pCommandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            pCommandList->CopyResource(pDestImageDX12, pSrcImageDX12);
        }

        /*
        **
        */
        void CRenderer::platformCopyBufferToBuffer(
            RenderDriver::Common::CBuffer* pDestBuffer,
            RenderDriver::Common::CBuffer* pSrcBuffer,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint64_t iDataSize)
        {
            RenderDriver::Common::ResourceStateFlagBits prevSrcBufferState = pSrcBuffer->getState();
            RenderDriver::DX12::Utils::transitionBarrier(
                *pSrcBuffer,
                *mpUploadCommandBuffer.get(),
                prevSrcBufferState,
                RenderDriver::Common::ResourceStateFlagBits::CopySource);
            
            RenderDriver::DX12::Utils::transitionBarrier(
                *pDestBuffer,
                *mpUploadCommandBuffer.get(),
                RenderDriver::Common::ResourceStateFlagBits::Common,
                RenderDriver::Common::ResourceStateFlagBits::CopyDestination);

            ID3D12GraphicsCommandList* pCommandList = static_cast<ID3D12GraphicsCommandList*>(mpUploadCommandBuffer->getNativeCommandList());
            pCommandList->CopyBufferRegion(
                static_cast<ID3D12Resource*>(pDestBuffer->getNativeBuffer()), 
                iDestOffset, 
                static_cast<ID3D12Resource*>(pSrcBuffer->getNativeBuffer()),
                iSrcOffset, 
                static_cast<uint64_t>(iDataSize));

#if defined(_DEBUG)
            mpUploadCommandBuffer->addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_COPY);
#endif // DEBUG

            RenderDriver::DX12::Utils::transitionBarrier(
                *pDestBuffer,
                *mpUploadCommandBuffer.get(),
                RenderDriver::Common::ResourceStateFlagBits::CopyDestination,
                RenderDriver::Common::ResourceStateFlagBits::Common);

            RenderDriver::DX12::Utils::transitionBarrier(
                *pSrcBuffer,
                *mpUploadCommandBuffer.get(),
                RenderDriver::Common::ResourceStateFlagBits::CopySource,
                prevSrcBufferState);

            mpUploadCommandBuffer->close();
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);

            // wait until done
            RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
            placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
            placeFenceDesc.miFenceValue = ++miCopyCommandFenceValue;
            mpUploadFence->place(placeFenceDesc);
            mpUploadFence->waitCPU(UINT64_MAX);

            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();
        }

        /*
        **
        */
        float3 CRenderer::platformGetWorldFromScreenPosition(
            uint32_t iX,
            uint32_t iY,
            uint32_t iScreenWidth,
            uint32_t iScreenHeight)
        {
#if 0
            Render::Common::RenderJobInfo& renderJob = mpSerializer->getRenderJob("Draw Mesh Graphics");
            PLATFORM_OBJECT_HANDLE handle = renderJob.maOutputRenderTargetAttachments[0];
            RenderDriver::Common::CRenderTarget& renderTarget = *mpSerializer->getRenderTarget(handle).get();

            D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
            {
                static_cast<ID3D12Resource*>(renderTarget.getImage()->getNativeImage()),
                D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                0
            };

            RenderDriver::DX12::CBuffer readBackBuffer;
            RenderDriver::Common::BufferDescriptor desc =
            {
                /* uint64_t                    miSize;                                          */      1024 * 1024 * 4 * sizeof(float),
                /* Format                      mFormat;                                         */      RenderDriver::Common::Format::UNKNOWN,
                /* ResourceFlagBits            mFlags;                                          */      RenderDriver::Common::ResourceFlagBits::None,
                /* HeapType                    mHeapType = HeapType::Default;                   */      RenderDriver::Common::HeapType::ReadBack,
                /* ResourceStateFlagBits       mOverrideState = ResourceStateFlagBits::None;    */      RenderDriver::Common::ResourceStateFlagBits::CopyDestination,
                /* CBuffer* mpCounterBuffer = nullptr;                                          */      nullptr,
            };
            readBackBuffer.create(desc, *mpDevice);

            D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
            {
                static_cast<ID3D12Resource*>(readBackBuffer.getNativeBuffer()),             //ID3D12Resource* pResource;
                D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                {
                    0,
                    SerializeUtils::DX12::convert(RenderDriver::Common::Format::R32G32B32A32_FLOAT),              // DXGI_FORMAT Format;
                        iScreenWidth,                                        // UINT Width;
                        iScreenHeight,                                       // UINT Height;
                        1,                                                  // UINT Depth;
                        iScreenWidth * 4 * sizeof(float),     // UINT RowPitch;
                }
            };

            static_cast<ID3D12GraphicsCommandList*>(mpUploadCommandBuffer->getNativeCommandList())->CopyTextureRegion(
                &destCopyLocation,
                0,
                0,
                0,
                &srcCopyLocation,
                nullptr);

            mpUploadCommandBuffer->close();
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);

            // wait until done
            RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
            placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
            placeFenceDesc.miFenceValue = miCopyCommandFenceValue++;
            mpUploadFence->place(placeFenceDesc);
            mpUploadFence->waitCPU(UINT64_MAX);

            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();

            D3D12_RANGE range;
            range.Begin = 0;
            range.End = iScreenWidth * iScreenHeight * 4 * sizeof(float);

            std::vector<float> afImageData(iScreenWidth * iScreenHeight * 4);
            void* pData = nullptr;
            static_cast<ID3D12Resource*>(readBackBuffer.getNativeBuffer())->Map(0, &range, &pData);
            memcpy(afImageData.data(), pData, iScreenWidth * iScreenHeight * 4 * sizeof(float));
            static_cast<ID3D12Resource*>(readBackBuffer.getNativeBuffer())->Unmap(0, nullptr);

            uint32_t iIndex = (iY * iScreenWidth + iX) * 4;
            float3 ret(afImageData[iIndex], afImageData[iIndex + 1], afImageData[iIndex + 2]);
#endif // #if 0
            float3 ret(0.0f);
            return ret;
        }

        /*
        **
        */
        uint64_t CRenderer::getTotalVertexBufferGPUAddress()
        {
            return static_cast<uint64_t>(static_cast<ID3D12Resource*>(mpTotalMeshesVertexBuffer->getNativeBuffer())->GetGPUVirtualAddress());
        }

        /*
        **
        */
        uint64_t CRenderer::getTotalIndexBufferGPUAddress()
        {
            return static_cast<uint64_t>(static_cast<ID3D12Resource*>(mpTotalMeshesIndexBuffer->getNativeBuffer())->GetGPUVirtualAddress());
        }

        /*
        **
        */
        void CRenderer::platformBeginRenderDebuggerCapture(std::string const& captureFilePath)
        {
            size_t charsNeeded = ::MultiByteToWideChar(
                CP_UTF8, 
                0,
                captureFilePath.data(), 
                (int)captureFilePath.size(), 
                NULL, 
                0);

            std::vector<wchar_t> buffer(charsNeeded);
            int charsConverted = ::MultiByteToWideChar(
                CP_UTF8, 
                0,
                captureFilePath.data(), 
                (int)captureFilePath.size(), 
                &buffer[0], 
                static_cast<int32_t>(buffer.size()));

            PIXCaptureParameters captureParameters;
            std::wstring captureFilePathWS = std::wstring(&buffer[0], charsConverted);
            captureParameters.GpuCaptureParameters.FileName = captureFilePathWS.c_str();
            HRESULT hr = PIXBeginCapture(PIX_CAPTURE_GPU, &captureParameters);
            WTFASSERT(hr == S_OK, "gpu capture error %d\n", hr);
        }

        /*
        **
        */
        void CRenderer::platformEndRenderDebuggerCapture()
        {
            while(PIXEndCapture(FALSE) == E_PENDING)
            {
                Sleep(500);
            }
        }

        /*
        **
        */
        void CRenderer::platformBeginRenderDocCapture(std::string const& filePath)
        {
            mpRenderDocAPI->SetCaptureFilePathTemplate(filePath.c_str());
            //mpRenderDocAPI->StartFrameCapture(mpDevice->getNativeDevice(), mHWND);
            mpRenderDocAPI->StartFrameCapture(nullptr, nullptr);
        }

        /*
        **
        */
        void CRenderer::platformEndRenderDocCapture()
        {
            mpRenderDocAPI->EndFrameCapture(nullptr, nullptr);
        }

        /*
        **
        */
        void CRenderer::platformTransitionBarriers(
            RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iNumBarrierInfo,
            bool bReverseState,
            void* szUserData)
        {
            RenderDriver::DX12::Utils::transitionBarriers(
                aBarrierInfo,
                commandBuffer,
                iNumBarrierInfo,
                bReverseState);
        }

        /*
        **
        */
        void CRenderer::platformCreateRenderJobFences()
        {
#if 0
            // create fences for each render jobs
            maaRenderJobFences.resize(RenderDriver::Common::CCommandQueue::NumTypes);
            for(uint32_t iQueue = 0; iQueue < RenderDriver::Common::CCommandQueue::NumTypes; iQueue++)
            {
                maaRenderJobFences[iQueue].resize(maRenderJobsByType[iQueue].size());
                for(uint32_t iRenderJob = 0; iRenderJob < static_cast<uint32_t>(maRenderJobsByType[iQueue].size()); iRenderJob++)
                {
                    maaRenderJobFences[iQueue][iRenderJob] = std::make_unique<RenderDriver::DX12::CFence>();
                    RenderDriver::Common::FenceDescriptor desc;
                    maaRenderJobFences[iQueue][iRenderJob]->create(desc, *mpDevice);

                    std::ostringstream oss;
                    oss << maRenderJobsByType[iQueue][iRenderJob].mpRenderJob->mName << " Fence";
                    maaRenderJobFences[iQueue][iRenderJob]->setID(oss.str());

                    //maRenderJobsByType[iQueue][iRenderJob].mpFence = maaRenderJobFences[iQueue][iRenderJob].get();
                }
            }
#endif // #if 0

            std::string aFenceNames[4] =
            {
                "Graphics Queue Fence",
                "Compute Queue Fence",
                "Copy Queue Fence",
                "Copy GPU Queue Fence",
            };

            mapCommandQueueFences.resize(RenderDriver::Common::CCommandQueue::NumTypes);
            for(uint32_t iFence = 0; iFence < static_cast<uint32_t>(mapCommandQueueFences.size()); iFence++)
            {
                mapCommandQueueFences[iFence] = std::make_unique<RenderDriver::DX12::CFence>();
                RenderDriver::Common::FenceDescriptor desc;
                mapCommandQueueFences[iFence]->create(desc, *mpDevice);
                mapCommandQueueFences[iFence]->setID(aFenceNames[iFence]);
            }
        }

        /*
        **
        */
        static std::wstring GetLatestWinPixGpuCapturerPath()
        {
            LPWSTR programFilesPath = nullptr;
            SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

            std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

            WIN32_FIND_DATA findData;
            bool foundPixInstallation = false;
            wchar_t newestVersionFound[MAX_PATH];

            HANDLE hFind = FindFirstFile(pixSearchPath.c_str(), &findData);
            if(hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if(((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
                        (findData.cFileName[0] != '.'))
                    {
                        if(!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
                        {
                            foundPixInstallation = true;
                            StringCchCopy(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
                        }
                    }
                } while(FindNextFile(hFind, &findData) != 0);
            }

            FindClose(hFind);

            if(!foundPixInstallation)
            {
                // TODO: Error, no PIX installation found
            }

            wchar_t output[MAX_PATH];
            StringCchCopy(output, pixSearchPath.length(), pixSearchPath.data());
            StringCchCat(output, MAX_PATH, &newestVersionFound[0]);
            StringCchCat(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

            return &output[0];
        }

        /*
        **
        */
        void CRenderer::platformPostSetup(std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>>& aExternalBufferMap)
        {
            {
                for(uint32_t i = 0; i < static_cast<uint32_t>(mapQueueGraphicsCommandAllocators.size()); i++)
                {
                    // graphics
                    mapQueueGraphicsCommandAllocators[i] = std::make_shared<RenderDriver::DX12::CCommandAllocator>();
                    mapQueueGraphicsCommandBuffers[i] = std::make_shared<RenderDriver::DX12::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor graphicsCommandAllocatorDesc;
                    graphicsCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                    mapQueueGraphicsCommandAllocators[i]->create(graphicsCommandAllocatorDesc, *mpDevice);

                    std::ostringstream graphicCommandAllocatorName;
                    graphicCommandAllocatorName << "Graphics Command Allocator " << i;
                    mapQueueGraphicsCommandAllocators[i]->setID(graphicCommandAllocatorName.str());
                    mapQueueGraphicsCommandAllocators[i]->reset();

                    RenderDriver::Common::CommandBufferDescriptor graphicsCommandBufferDesc;
                    graphicsCommandBufferDesc.mpCommandAllocator = mapQueueGraphicsCommandAllocators[i].get();
                    graphicsCommandBufferDesc.mpPipelineState = nullptr;
                    graphicsCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                    mapQueueGraphicsCommandBuffers[i]->create(graphicsCommandBufferDesc, *mpDevice);

                    std::ostringstream graphicCommandBufferName;
                    graphicCommandBufferName << "Graphics Command Buffer " << i;
                    mapQueueGraphicsCommandBuffers[i]->setID(graphicCommandBufferName.str());
                    mapQueueGraphicsCommandBuffers[i]->close();
                    mapQueueGraphicsCommandBuffers[i]->reset();

                    // compute
                    mapQueueComputeCommandAllocators[i] = std::make_shared<RenderDriver::DX12::CCommandAllocator>();
                    mapQueueComputeCommandBuffers[i] = std::make_shared<RenderDriver::DX12::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor computeCommandAllocatorDesc;
                    computeCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueComputeCommandAllocators[i]->create(computeCommandAllocatorDesc, *mpDevice);
                    mapQueueComputeCommandAllocators[i]->reset();

                    std::ostringstream computeCommandAllocatorName;
                    computeCommandAllocatorName << "Compute Command Allocator " << i;
                    mapQueueComputeCommandAllocators[i]->setID(computeCommandAllocatorName.str());

                    RenderDriver::Common::CommandBufferDescriptor computeCommandBufferDesc;
                    computeCommandBufferDesc.mpCommandAllocator = mapQueueComputeCommandAllocators[i].get();
                    computeCommandBufferDesc.mpPipelineState = nullptr;
                    computeCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueComputeCommandBuffers[i]->create(computeCommandBufferDesc, *mpDevice);

                    std::ostringstream computeCommandBufferName;
                    computeCommandBufferName << "Compute Command Buffer " << i;
                    mapQueueComputeCommandBuffers[i]->setID(computeCommandBufferName.str());
                    mapQueueComputeCommandBuffers[i]->close();
                    mapQueueComputeCommandBuffers[i]->reset();

                    // copy
                    mapQueueCopyCommandAllocators[i] = std::make_shared<RenderDriver::DX12::CCommandAllocator>();
                    mapQueueCopyCommandBuffers[i] = std::make_shared<RenderDriver::DX12::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor copyCommandAllocatorDesc;
                    copyCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    mapQueueCopyCommandAllocators[i]->create(copyCommandAllocatorDesc, *mpDevice);
                    mapQueueCopyCommandAllocators[i]->reset();

                    std::ostringstream copyCommandAllocatorName;
                    copyCommandAllocatorName << "Copy Command Allocator " << i;
                    mapQueueCopyCommandAllocators[i]->setID(copyCommandAllocatorName.str());

                    RenderDriver::Common::CommandBufferDescriptor copyCommandBufferDesc;
                    copyCommandBufferDesc.mpCommandAllocator = mapQueueCopyCommandAllocators[i].get();
                    copyCommandBufferDesc.mpPipelineState = nullptr;
                    copyCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    mapQueueCopyCommandBuffers[i]->create(copyCommandBufferDesc, *mpDevice);

                    std::ostringstream copyCommandBufferName;
                    copyCommandBufferName << "Copy Command Buffer " << i;
                    mapQueueCopyCommandBuffers[i]->setID(copyCommandBufferName.str());
                    mapQueueCopyCommandBuffers[i]->close();
                    mapQueueCopyCommandBuffers[i]->reset();

                    // copy gpu
                    mapQueueGPUCopyCommandAllocators[i] = std::make_shared<RenderDriver::DX12::CCommandAllocator>();
                    mapQueueGPUCopyCommandBuffers[i] = std::make_shared<RenderDriver::DX12::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor gpuCopyCommandAllocatorDesc;
                    gpuCopyCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueGPUCopyCommandAllocators[i]->create(gpuCopyCommandAllocatorDesc, *mpDevice);

                    std::ostringstream gpuCopyCommandAllocatorName;
                    gpuCopyCommandAllocatorName << "GPU Copy Command Allocator " << i;
                    mapQueueGPUCopyCommandAllocators[i]->setID(gpuCopyCommandAllocatorName.str());
                    mapQueueGPUCopyCommandAllocators[i]->reset();

                    RenderDriver::Common::CommandBufferDescriptor gpuCopyCommandBufferDesc;
                    gpuCopyCommandBufferDesc.mpCommandAllocator = mapQueueGPUCopyCommandAllocators[i].get();
                    gpuCopyCommandBufferDesc.mpPipelineState = nullptr;
                    gpuCopyCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueGPUCopyCommandBuffers[i]->create(gpuCopyCommandBufferDesc, *mpDevice);

                    std::ostringstream gpuCopyCommandBufferName;
                    gpuCopyCommandBufferName << "GPU Copy Command Buffer " << i;
                    mapQueueGPUCopyCommandBuffers[i]->setID(gpuCopyCommandBufferName.str());
                    mapQueueGPUCopyCommandBuffers[i]->close();
                    mapQueueGPUCopyCommandBuffers[i]->reset();
                }
            }
        }

        /*
        **
        */
        void CRenderer::platformSwapChainMoveToNextFrame()
        {
            //PIXBeginEvent(
            //    0x808000, 
            //    "Move To Next Frame");
            //
            //miCopyCommandFenceValue = 0;
            //mpUploadFence->reset(mpCopyCommandQueue.get());
            //mpUploadFence->signal(mpCopyCommandQueue.get(), 0);
            //
            //PIXEndEvent();
        }

        /*
        **
        */
        void CRenderer::platformCopyCPUToGPUBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CBuffer* pDestBuffer,
            RenderDriver::Common::CBuffer* pUploadBuffer,
            void* pCPUData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDataSize,
            uint32_t iFlag)
        {
            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::BEGIN_MARKER)) > 0)
            {
                PIXBeginEvent(
#if defined(GPU_PIX_MARKER)
                    static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue()),
#endif // GPU_PIX_MARKER
                    0x008080, 
                    std::string(std::string("Copy To ") + pDestBuffer->getID()).c_str());
            }

            WTFASSERT(pUploadBuffer->getDescriptor().miSize >= iDataSize,
                "upload buffer not big enough %d (%d)",
                pUploadBuffer->getDescriptor().miSize,
                pDestBuffer->getDescriptor().miSize);

            RenderDriver::Common::Utils::TransitionBarrierInfo aBarrierInfo[1];
            aBarrierInfo[0].mpBuffer = pDestBuffer;
            aBarrierInfo[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
            aBarrierInfo[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopyDestination;
            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                1,
                false);

            
            uint8_t* pData = nullptr;
            HRESULT hr = static_cast<ID3D12Resource*>(pUploadBuffer->getNativeBuffer())->Map(0, nullptr, reinterpret_cast<void**>(&pData));
            assert(SUCCEEDED(hr));
            memcpy(pData, pCPUData, iDataSize);
            static_cast<ID3D12Resource*>(pUploadBuffer->getNativeBuffer())->Unmap(0, nullptr);

            WTFASSERT(iDestOffset < UINT32_MAX, "Invalid destination data offset: %d\n", iDestOffset);

            static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList())->CopyBufferRegion(
                static_cast<ID3D12Resource*>(pDestBuffer->getNativeBuffer()),
                iDestOffset,
                static_cast<ID3D12Resource*>(pUploadBuffer->getNativeBuffer()),
                iSrcOffset,
                iDataSize);

#if defined(_DEBUG)
            commandBuffer.addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_COPY);
#endif // DEBUG


            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                1,
                true);

            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::END_MARKER)) > 0)
            {
                PIXEndEvent(
#if defined(GPU_PIX_MARKER)
                    static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue())
#endif // GPU_PIX_MARKER
                );
            }
        }

        /*
        **
        */
        void CRenderer::platformCopyCPUToGPUBuffer2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CBuffer* pDestBuffer,
            RenderDriver::Common::CBuffer* pUploadBuffer,
            void* pCPUData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDestDataSize,
            uint32_t iTotalDataSize,
            uint32_t iFlag)
        {
            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::BEGIN_MARKER)) > 0)
            {
                PIXBeginEvent(
#if defined(GPU_PIX_MARKER)
                    static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue()),
#endif // GPU_PIX_MARKER
                    0x008080, 
                    std::string(std::string("Copy To ") + pDestBuffer->getID()).c_str());
            }

            WTFASSERT(pUploadBuffer->getDescriptor().miSize >= pDestBuffer->getDescriptor().miSize,
                "upload buffer not big enough %d (%d)",
                pUploadBuffer->getDescriptor().miSize,
                pDestBuffer->getDescriptor().miSize);

            RenderDriver::Common::Utils::TransitionBarrierInfo aBarrierInfo[1];
            aBarrierInfo[0].mpBuffer = pDestBuffer;
            aBarrierInfo[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
            aBarrierInfo[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopyDestination;
            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                1,
                false);


            uint8_t* pData = nullptr;
            HRESULT hr = static_cast<ID3D12Resource*>(pUploadBuffer->getNativeBuffer())->Map(0, nullptr, reinterpret_cast<void**>(&pData));
            assert(SUCCEEDED(hr));
            memcpy(pData, pCPUData, iTotalDataSize);
            static_cast<ID3D12Resource*>(pUploadBuffer->getNativeBuffer())->Unmap(0, nullptr);

            WTFASSERT(iDestOffset < UINT32_MAX, "Invalid destination data offset: %d\n", iDestOffset);

            static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList())->CopyBufferRegion(
                static_cast<ID3D12Resource*>(pDestBuffer->getNativeBuffer()),
                iDestOffset,
                static_cast<ID3D12Resource*>(pUploadBuffer->getNativeBuffer()),
                iSrcOffset,
                iDestDataSize);

#if defined(_DEBUG)
            commandBuffer.addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_COPY);
#endif // DEBUG

            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                1,
                true);

            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::END_MARKER)) > 0)
            {
                PIXEndEvent(
#if defined(GPU_PIX_MARKER)
                    static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue())
#endif // GPU_PIX_MARKER
                );
            }
        }

        /*
        **
        */
        void CRenderer::platformCopyCPUToGPUBuffer3(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue,
            RenderDriver::Common::CBuffer* pDestBuffer,
            void* pCPUData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDataSize,
            RenderDriver::Common::CBuffer& uploadBuffer)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformExecuteCopyCommandBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iFlag)
        {
            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::BEGIN_MARKER)) > 0)
            {
                PIXBeginEvent(
#if defined(GPU_PIX_MARKER)
                    static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue()),
#endif // GPU_PIX_MARKER
                    0x008080, 
                    std::string(std::string("Execute ") + commandBuffer.getID()).c_str());
            }

            commandBuffer.close();

            //DEBUG_PRINTF("wait for fence value: %d\n", miCopyCommandFenceValue + 1);

            mpCopyCommandQueue->execCommandBuffer(commandBuffer, *mpDevice);
            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION)) > 0)
            {
                // wait until done
                RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
                placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
                placeFenceDesc.miFenceValue = ++miCopyCommandFenceValue;
                mpUploadFence->place(placeFenceDesc);
                

                //mpUploadFence->waitCPU(UINT64_MAX);
                mpUploadFence->waitCPU2(
                    UINT64_MAX,
                    mpCopyCommandQueue.get(),
                    placeFenceDesc.miFenceValue);

                uint64_t iCompletedValue = mpUploadFence->getFenceValue();
                //DEBUG_PRINTF("completed fence value: %d\n", iCompletedValue);
            }

            

            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::END_MARKER)))
            {
                PIXEndEvent(
#if defined(GPU_PIX_MARKER)
                    static_cast<ID3D12CommandQueue*>(mpCopyCommandQueue->getNativeCommandQueue())
#endif // GPU_PIX_MARKER
                );
            }
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker(
            std::string const& name,
            RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
#if defined(GPU_PIX_MARKER)
            if(pCommandQueue)
            {
                PIXBeginEvent(
                    static_cast<ID3D12CommandQueue*>(pCommandQueue->getNativeCommandQueue()),
                    0xff0000,
                    name.c_str());
            }
            else
#endif // GPU_PIX_MARKER
            {
                PIXBeginEvent(
                    0xff0000,
                    name.c_str());
            }
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker2(
            std::string const& name,
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
#if defined(GPU_PIX_MARKER)
            if(pCommandBuffer)
            {
                PIXBeginEvent(
                    static_cast<ID3D12GraphicsCommandList*>(pCommandBuffer->getNativeCommandList()),
                    0xff0000,
                    name.c_str());
            }
            else
#endif // GPU_PIX_MARKER
            {
                PIXBeginEvent(
                    0xff0000,
                    name.c_str());
            }
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker(
            RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
#if defined(GPU_PIX_MARKER)
            if(pCommandQueue)
            {
                PIXEndEvent(static_cast<ID3D12CommandQueue*>(pCommandQueue->getNativeCommandQueue()));
            }
            else
#endif // GPU_PIX_MARKER
            {
                PIXEndEvent();
            }
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker2(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
#if defined(GPU_PIX_MARKER)
            if(pCommandBuffer)
            {
                PIXEndEvent(static_cast<ID3D12GraphicsCommandList*>(pCommandBuffer->getNativeCommandList()));
            }
            else
#endif // GPU_PIX_MARKER
            {
                PIXEndEvent();
            }
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker3(
            std::string const& name,
            float4 const& color,
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            uint32_t iEncodedColor = 0;
            float4 convertedColor = color * 255.0f;
            uint32_t iColor = static_cast<uint32_t>(color.x) << 16 | static_cast<uint32_t>(color.y) << 8 | static_cast<uint32_t>(color.z);

#if defined(GPU_PIX_MARKER)
            if(pCommandBuffer)
            {
                PIXBeginEvent(
                    static_cast<ID3D12GraphicsCommandList*>(pCommandBuffer->getNativeCommandList()),
                    iColor,
                    name.c_str());
            }
            else
#endif // GPU_PIX_MARKER
            {
                PIXBeginEvent(
                    iColor,
                    name.c_str());
            }
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker3(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
#if defined(GPU_PIX_MARKER)
            if(pCommandBuffer)
            {
                PIXEndEvent(static_cast<ID3D12GraphicsCommandList*>(pCommandBuffer->getNativeCommandList()));
            }
            else
#endif // GPU_PIX_MARKER
            {
                PIXEndEvent();
            }
        }

        /*
        **
        */
        void CRenderer::platformInitializeRenderJobs(
            std::vector<std::string> const& aRenderJobNames)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCreateRenderJobCommandBuffers(
            std::vector<std::string> const& aRenderJobNames
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformBeginRenderPass2(
            Render::Common::RenderPassDescriptor2 const& desc
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformEndRenderPass2(
            Render::Common::RenderPassDescriptor2 const& desc
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCreateVertexAndIndexBuffer(
            std::string const& name,
            uint32_t iVertexBufferSize,
            uint32_t iIndexBufferSize
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformSetVertexAndIndexBuffers2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            std::string const& meshName)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCreateSwapChainCommandBuffer()
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformTransitionBarrier3(
            RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarriers,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iNumBarriers,
            RenderDriver::Common::CCommandQueue::Type const& queueType
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformTransitionOutputAttachments()
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCreateAccelerationStructures(
            std::vector<float4> const& aTrianglePositions,
            std::vector<uint32_t> const& aiTriangleIndices,
            std::vector<std::pair<uint32_t, uint32_t>> const& aMeshRanges,
            uint32_t iNumMeshes
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformRayTraceCommand(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iScreenWidth,
            uint32_t iScreenHeight
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformRayTraceShaderSetup(
            Render::Common::CRenderJob* pRenderJob
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCopyTexturePageToAtlas(
            char const* pImageData,
            RenderDriver::Common::CImage* pDestImage,
            uint2 const& pageCoord,
            uint32_t iTexturePageDimension)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCopyTexturePageToAtlas2(
            char const* pImageData,
            RenderDriver::Common::CImage* pDestImage,
            uint2 const& pageCoord,
            uint32_t iTexturePageDimension,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue,
            RenderDriver::Common::CBuffer& uploadBuffer)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCreateCommandBuffer(
            std::unique_ptr<RenderDriver::Common::CCommandAllocator>& threadCommandAllocator,
            std::unique_ptr<RenderDriver::Common::CCommandBuffer>& threadCommandBuffer)
        {
            threadCommandAllocator = std::make_unique<RenderDriver::DX12::CCommandAllocator>();
            threadCommandBuffer = std::make_unique<RenderDriver::DX12::CCommandBuffer>();
        }

        /*
        **
        */
        void CRenderer::platformCreateBuffer(
            std::unique_ptr<RenderDriver::Common::CBuffer>& buffer,
            uint32_t iSize)
        {
            buffer = std::make_unique<RenderDriver::DX12::CBuffer>();
            RenderDriver::Common::BufferDescriptor desc = {};
            desc.miSize = iSize;
            buffer->create(
                desc,
                *mpDevice);
        }

        /*
        **
        */
        void CRenderer::platformCreateCommandQueue(
            std::unique_ptr<RenderDriver::Common::CCommandQueue>& commandQueue,
            RenderDriver::Common::CCommandQueue::Type const& type)
        {
            commandQueue = std::make_unique<RenderDriver::DX12::CCommandQueue>();
        }

        /*
        **
        */
        void CRenderer::platformTransitionInputImageAttachments(
            Render::Common::CRenderJob* pRenderJob,
            std::vector<char>& acPlatformAttachmentInfo,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            bool bReverse)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformTransitionOutputAttachmentsRayTrace(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::setAttachmentImage(
            std::string const& dstRenderJobName,
            std::string const& dstAttachmentName,
            std::string const& srcRenderJobName,
            std::string const& srcAttachmentName)
        {
            WTFASSERT(0, "Implement me");
        }

    }   // DX12

}   // Render
