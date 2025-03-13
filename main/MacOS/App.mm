#include "App.h"
#include "render-driver/Metal/DeviceMetal.h"
#include "render-driver/Metal/SwapChainMetal.h"
#include "render/Metal/RendererMetal.h"
//#include "SDF/Metal/SDFManagerMetal.h"

//#include <RenderCommand/RenderRequestHandler.h>

#include <utils/LogPrint.h>

#include <math/quaternion.h>
#include <math/mat4.h>

#include <filesystem>
#include <algorithm>
#include <sstream>


#include <stb_image.h>
#include <utils/wtfassert.h>

#define WINDOW_WIDTH    512
#define WINDOW_HEIGHT   512

static std::atomic<uint32_t> giCopyingTexturePage = 0;
static std::atomic<uint32_t> giNumFinished = 0;

struct ImageInfo
{
    int32_t    miImageWidth;
    int32_t    miImageHeight;
    stbi_uc* mpImageData;
};

void loadTexturePageThread(
    Render::Common::CRenderer* pRenderer,
    std::vector<std::string> const& aAlbedoTextureNames,
    std::vector<uint2> const& aAlbedoTextureDimensions,
    std::vector<std::string> const& aNormalTextureNames,
    std::vector<uint2> const& aNormalTextureDimensions,
    uint32_t& iCurrTotalPageLoaded,
    uint32_t& iCurrAlbedoPageLoaded,
    uint32_t& iCurrNormalPageLoaded,
    RenderDriver::Common::CCommandBuffer& commandBuffer,
    RenderDriver::Common::CCommandQueue& commandQueue,
    RenderDriver::Common::CBuffer& threadScratchPathUploadBuffer,
    RenderDriver::Common::CBuffer& texturePageUploadBuffer,
    uint32_t iStartIndex,
    uint32_t iNumChecksPerLoop,
    uint32_t& iLastCounterValue,
    std::vector<char>const & acTexturePageQueueData,
    std::vector<char>const & acTexturePageInfoData,
    std::mutex& threadMutex);

void getTexturePage(
    std::vector<char>& acTexturePageImage,
    stbi_uc const* pImageData,
    uint2 const& pageCoord,
    uint32_t const& iImageWidth,
    uint32_t const& iImageHeight,
    uint32_t const& iTexturePageSize);

/*
**
*/
char const* getSaveDir()
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationScriptsDirectory, NSUserDomainMask, YES);
    NSString* applicationSupportDirectory = [paths firstObject];
    char const* szDir = [applicationSupportDirectory UTF8String];
    //DEBUG_PRINTF("save directory: \"%s\"\n", szDir);
    
    return szDir;
}

/*
**
*/
void getAssetsDir(std::string& fullPath, std::string const& fileName)
{
    NSString* resourcePath = [[NSBundle mainBundle] bundlePath];
    char const* szBundleDir = [resourcePath UTF8String];
    //DEBUG_PRINTF("bundle directory: \"%s\"\n", szBundleDir);
    
    fullPath = std::string(szBundleDir) + "/Contents/Resources/assets/" + fileName;
}

/*
**
*/
char const* getWriteDir()
{
    NSFileManager* fileManager = [[NSFileManager alloc] init];
    NSString* bundleID = [[NSBundle mainBundle] bundleIdentifier];
    NSArray* urlPaths = [fileManager 
        URLsForDirectory: NSApplicationSupportDirectory
        inDomains: NSUserDomainMask];
    NSURL* appDirectory = [[urlPaths objectAtIndex: 0] URLByAppendingPathComponent: bundleID];
    if(![fileManager fileExistsAtPath: [appDirectory path]])
    {
        NSError* error = nil;
        [fileManager
         createDirectoryAtURL: appDirectory
         withIntermediateDirectories: NO
         attributes: nil
         error: &error];
        
        if(error != nil)
        {
            NSLog(@"error: %@", error);
        }
    }
    
    //char const* szRet = [appDirectory.absoluteString UTF8String];
    
    return [appDirectory.absoluteString UTF8String];
}

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
    std::string fullPath;
    getAssetsDir(fullPath, "bistro2.mat");
    
    FILE* fp = fopen(fullPath.c_str(), "rb");
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

    getAssetsDir(fullPath, "converted-dds-scaled/albedo-dimensions.txt");
    DEBUG_PRINTF("%s\n", fullPath.c_str());
    aAlbedoTextureDimensions.resize(iNumAlbedoTextures);
    fp = fopen(fullPath.c_str(), "rb");
    fread(aAlbedoTextureDimensions.data(), sizeof(uint2), iNumAlbedoTextures, fp);
    fclose(fp);
    
    getAssetsDir(fullPath, "converted-dds-scaled/normal-dimensions.txt");
    aNormalTextureDimensions.resize(iNumNormalTextures);
    fp = fopen(fullPath.c_str(), "rb");
    fread(aNormalTextureDimensions.data(), sizeof(uint2), iNumNormalTextures, fp);
    fclose(fp);
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
        
        std::string fullPath;
        getAssetsDir(fullPath, "bistro2.mat");
        FILE* fp = fopen(fullPath.c_str(), "rb");
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

        std::unique_ptr<RenderDriver::Common::CBuffer> buffer = std::make_unique<RenderDriver::Metal::CBuffer>();
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
        
        uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
        pRenderer->copyCPUToBuffer(
            pBuffer,
            aMaterials.data(),
            0,
            (uint32_t)iDataSize,
            iFlags
        );
    }

    // material id for texture atlas pages
    {
        std::string fullPath;
        getAssetsDir(fullPath, "bistro2.mid");
        FILE* fp = fopen(fullPath.c_str(), "rb");
        fseek(fp, 0, SEEK_END);
        size_t iFileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<char> acBuffer(iFileSize);
        fread(acBuffer.data(), sizeof(char), iFileSize, fp);
        fclose(fp);

        std::unique_ptr<RenderDriver::Common::CBuffer> buffer = std::make_unique<RenderDriver::Metal::CBuffer>();
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
        
        uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
        pRenderer->copyCPUToBuffer(
            pBuffer,
            acBuffer.data(),
            0,
            (uint32_t)iFileSize,
            iFlags
        );
    }

    // mesh triangle ranges
    {
        // load binary data
        std::string fullPath;
        getAssetsDir(fullPath, "bistro2-triangles.bin");
        FILE* fp = fopen(fullPath.c_str(), "rb");
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

        std::unique_ptr<RenderDriver::Common::CBuffer> buffer = std::make_unique<RenderDriver::Metal::CBuffer>();
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
        
        uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
        pRenderer->copyCPUToBuffer(
            pBuffer,
            aMeshRanges.data(),
            0,
            (uint32_t)iDataSize,
            iFlags
        );
    }
}

/*
**
*/
void CApp::init(AppDescriptor const& appDesc)
{
    std::unique_ptr<Render::Common::RendererDescriptor> pDesc = std::make_unique<Render::Metal::RendererDescriptor>();
    pDesc->mFormat = RenderDriver::Common::Format::R10G10B10A2_UNORM;
    pDesc->miScreenWidth = WINDOW_WIDTH;
    pDesc->miScreenHeight = WINDOW_HEIGHT;
    pDesc->miSamples = 1;
    pDesc->miSwapChainFlags = RenderDriver::Common::SwapChainFlagBits::FIFO;
    pDesc->miNumBuffers = 3;
    pDesc->miViewportWidth = WINDOW_WIDTH;
    pDesc->miViewportHeight = WINDOW_HEIGHT;
    pDesc->mfViewportMaxDepth = 1.0f;
    
    mpRenderer = std::make_unique<Render::Metal::CRenderer>();
    
    //Render::Common::gLightDirection = normalize(float3(-0.703f, 0.5403f, -0.461f));
    Render::Common::gLightDirection = normalize(float3(0.3f, 1.0f, 0.0f));
    Render::Common::gfEmissiveRadiance = 0.0f;
    Render::Common::gfClearReservoir = 0.0f;

    // TODO: move vertex and index buffer out of renderer
    
#if defined(_MSC_VER)
    pDesc->mRenderJobsFilePath = "/Users/dingwings/projects/bistro-example-cpp/render-jobs/bistro-example-render-jobs.json";
#else
    getAssetsDir(pDesc->mRenderJobsFilePath, "render-jobs/trimmed-ray-trace-render-jobs.json");
#endif // _MSC_VER

    mpRenderer->setup(*pDesc);
    
    Render::Common::CRenderer* pRenderer = mpRenderer.get();
    
    std::string trianglePath;
    getAssetsDir(trianglePath, "bistro2-triangles.bin");
    
    // number of meshes
    uint32_t miNumMeshes = 0;
    FILE* fp = fopen(trianglePath.c_str(), "rb");
    fread(&miNumMeshes, sizeof(uint32_t), 1, fp);
    fclose(fp);
    
    // materials, albedo and normal texture names, albedo and normal texture dimension
    std::vector<char> acMaterialBuffer;
    getMaterialInfo(
        acMaterialBuffer,
        maAlbedoTextureNames,
        maNormalTextureNames,
        maAlbedoTextureDimensions,
        maNormalTextureDimensions,
        miNumAlbedoTextures,
        miNumNormalTextures,
        mpRenderer.get());
    
    // set up buffers used for all the passes
    setupExternalDataBuffers(
        maBufferMap,
        pRenderer
    );
    
    char const* szSaveDir = getSaveDir();
    DEBUG_PRINTF("%s\n", szSaveDir);
    std::string fullPath = std::string(szSaveDir) + "/bistro2-triangles.bin";
    fp = fopen(fullPath.c_str(), "rb");
    fclose(fp);
    
    pRenderer->initData();
    pRenderer->loadRenderJobInfo(pDesc->mRenderJobsFilePath, maBufferMap);
    
    fullPath = std::string(szSaveDir) + "/LDR_RGBA_0.png";
    
    miBlueNoiseWidth = 512;
    miBlueNoiseHeight = 512;
    macBlueNoiseImageData.resize(miBlueNoiseWidth*miBlueNoiseHeight*4);
    
    mCameraLookAt = float3(-3.44f, 1.53f, 0.625f);
    mCameraPosition = float3(7.08f, 1.62f, 0.675f);
    
    initRenderData(
        acMaterialBuffer,
        maAlbedoTextureNames,
        maNormalTextureNames,
        maAlbedoTextureDimensions,
        maNormalTextureDimensions,
        miNumAlbedoTextures,
        miNumNormalTextures
    );
    
    pRenderer->prepareRenderJobData();
    
    miNumLoadingThreads = 1;
    
    maiStartAndNumChecks.resize(4);
    
    // create thread command queue
    mThreadCommandQueue = std::make_unique<RenderDriver::Metal::CCommandQueue>();
    RenderDriver::Common::CCommandQueue::CreateDesc commandQueueDesc = {};
    commandQueueDesc.mType = RenderDriver::Common::CCommandQueue::Type::Copy;
    commandQueueDesc.mpDevice = pRenderer->getDevice();
    mThreadCommandQueue->create(commandQueueDesc);
    mThreadCommandQueue->setID("Texture Page Thread Command Queue");

    // create thread command buffer
    mThreadCommandBuffer = std::make_unique<RenderDriver::Metal::CCommandBuffer>();
    RenderDriver::Metal::CommandBufferDescriptor commandBufferDesc = {};
    commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
    commandBufferDesc.mpNativeCommandQueue = (__bridge id<MTLCommandQueue>)mThreadCommandQueue->getNativeCommandQueue();
    mThreadCommandBuffer->create(commandBufferDesc, *pRenderer->getDevice());
    //((RenderDriver::Metal::CCommandBuffer*)mThreadCommandBuffer.get())->createNativeCommandBuffer();
    mThreadCommandBuffer->setID("Texture Page Thread Command Buffer");
    
    // read back buffer from gpu to get the texture page info
    auto& pTexturePageQueueBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Texture Page Queue MIP"];
    uint32_t iBufferSize = (uint32_t)pTexturePageQueueBuffer->getDescriptor().miSize;
    macTexturePageQueueData.resize(iBufferSize);
        
    auto& texturePageInfoBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];
    iBufferSize = (uint32_t)texturePageInfoBuffer->getDescriptor().miSize;
    macTexturePageInfoData.resize(iBufferSize);

    // get the number of pages
    auto& pTexturePageCountBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Counters"];
    macCounterData.resize(pTexturePageCountBuffer->getDescriptor().miSize);
    
    RenderDriver::Common::CDevice* pDevice = pRenderer->getDevice();
    
    // create readback buffers
    RenderDriver::Common::BufferDescriptor bufferDesc = {};
    bufferDesc.miSize = macTexturePageQueueData.size();
    bufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferSrc;
    mTexturePageQueueReadBackBuffer = std::make_unique<RenderDriver::Metal::CBuffer>();
    mTexturePageQueueReadBackBuffer->create(bufferDesc, *pDevice);
    mTexturePageQueueReadBackBuffer->setID("Texture Page Queue Read Back Buffer");
    
    bufferDesc.miSize = macTexturePageInfoData.size();
    mTexturePageInfoReadBackBuffer = std::make_unique<RenderDriver::Metal::CBuffer>();
    mTexturePageInfoReadBackBuffer->create(bufferDesc, *pDevice);
    mTexturePageInfoReadBackBuffer->setID("Texture Page Info Read Back Buffer");
    
    bufferDesc.miSize = macCounterData.size();
    mTexturePageCounterReadBackBuffer = std::make_unique<RenderDriver::Metal::CBuffer>();
    mTexturePageCounterReadBackBuffer->create(bufferDesc, *pDevice);
    mTexturePageCounterReadBackBuffer->setID("Texture Page Counter Read Back Buffer");
    
    // create texture page upload buffers
    for(uint32_t i = 0; i < 4; i++)
    {
        bufferDesc.miSize = 64 * 64 * 4;
        maTexturePageUploadBuffer[i] = std::make_unique<RenderDriver::Metal::CBuffer>();
        maTexturePageUploadBuffer[i]->create(bufferDesc, *pDevice);
        
        std::ostringstream name;
        name << "Texture Page Upload Buffer " << i;
        maTexturePageUploadBuffer[i]->setID(name.str());
        
        bufferDesc.miSize = 256;
        maScatchPadUploadBuffer[i] = std::make_unique<RenderDriver::Metal::CBuffer>();
        maScatchPadUploadBuffer[i]->create(bufferDesc, *pDevice);
        
        std::ostringstream scratchPadName;
        scratchPadName << "Thread Scratch Pad Upload Buffer " << i;
        maScatchPadUploadBuffer[i]->setID(scratchPadName.str());
    }
    
    miCurrTotalPageLoaded = 0;
    
    // post draw, get the texture pages needed
    pRenderer->mpfnPostDraw = std::make_unique<std::function<void(Render::Common::CRenderer*)>>();
    *pRenderer->mpfnPostDraw = [&](Render::Common::CRenderer* pRenderer)
    {
        if(mbTexturePageLoadingStarted == false)
        {
            mbTexturePageLoadingStarted = true;
            
            miNumChecksPerThread = 4000 / miNumLoadingThreads;
            for(uint32_t iThread = 0; iThread < miNumLoadingThreads; iThread++)
            {
                maThreads[iThread] = std::make_unique<std::thread>(
                    [&](uint32_t iThread)
                {
                    uint32_t iThisThreadID = iThread;
                    
                    uint32_t iLastCounterValue = 0;

                    uint32_t iStartIndex = iThisThreadID * miNumChecksPerThread;
                    uint32_t iNumChecksPerLoop = 100;
                    std::map<uint32_t, ImageInfo> aImageInfo;

                    uint32_t iStartThreadIndex = 0;
                    for(uint32_t iLoop = 0;; iLoop++)
                    {
                        @autoreleasepool
                        {
                            if(mbQuit)
                            {
                                mConditionVariable.notify_all();
                                break;
                            }
                            
                            // wait or calculate thread start and end indices
                            if(iThisThreadID == 0 && iLoop > 0)
                            {
                                // get the page queue, page info, and calculate the each thread's start index and num pages in queue to check
                                uint32_t iDesired = 0; //giCopyingTexturePage.load();
                                while(giCopyingTexturePage.compare_exchange_strong(iDesired, 1u, std::memory_order_acquire, std::memory_order_release) == false)
                                {
                                    usleep(10);
                                }
                                
                                mThreadCommandBuffer->reset();
                                
                                // get the texture pages needed
                                auto& pTexturePageQueueBuffer = mpRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Texture Page Queue MIP"];
                                uint32_t iBufferSize = (uint32_t)pTexturePageQueueBuffer->getDescriptor().miSize;
                                WTFASSERT(iBufferSize <= macTexturePageQueueData.size(), "Need larger buffer");
                                mpRenderer->copyBufferToCPUMemory3(
                                    pTexturePageQueueBuffer,
                                    macTexturePageQueueData.data(),
                                    0,
                                    iBufferSize,
                                    *mTexturePageQueueReadBackBuffer,
                                    *mThreadCommandBuffer,
                                    *mThreadCommandQueue
                                );
                                
                                auto& texturePageInfoBuffer = mpRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];
                                iBufferSize = (uint32_t)texturePageInfoBuffer->getDescriptor().miSize;
                                WTFASSERT(iBufferSize <= macTexturePageInfoData.size(), "Need larger buffer");
                                mpRenderer->copyBufferToCPUMemory3(
                                    texturePageInfoBuffer,
                                    macTexturePageInfoData.data(),
                                    0,
                                    iBufferSize,
                                    *mTexturePageInfoReadBackBuffer,
                                    *mThreadCommandBuffer,
                                    *mThreadCommandQueue
                                );
                                
                                auto& pTexturePageCountBuffer = mpRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Counters"];
                                WTFASSERT(256 <= macCounterData.size(), "Need larger buffer");
                                mpRenderer->copyBufferToCPUMemory3(
                                    pTexturePageCountBuffer,
                                    macCounterData.data(),
                                    0,
                                    128,
                                    *mTexturePageCounterReadBackBuffer,
                                    *mThreadCommandBuffer,
                                    *mThreadCommandQueue
                                );
                                
                                // finish upload/download from gpu
                                giCopyingTexturePage.store(0);
                                
                                uint32_t iNumChecks = std::min(*((uint32_t*)macCounterData.data()), 65535u);
                                for(uint32_t i = 0; i < miNumLoadingThreads; i++)
                                {
                                    maiStartAndNumChecks[i] = std::make_pair(i * (uint32_t)ceil((float)iNumChecks / (float)miNumLoadingThreads), (uint32_t)ceil((float)iNumChecks / (float)miNumLoadingThreads));
                                }
                                
                                while(giNumFinished.load() < miNumLoadingThreads)
                                {
                                    if(mbQuit)
                                    {
                                        break;
                                    }
                                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                                }
                                
                                giNumFinished.store(0);
                                
                                // wake up all other threads
                                mbStartLoadPage = true;
                                mConditionVariable.notify_all();
                                
                            }
                            
                            // start or continue with the queue entry index from last loop
                            if(iStartThreadIndex >= maiStartAndNumChecks[iThisThreadID].second || iStartThreadIndex < maiStartAndNumChecks[iThisThreadID].first)
                            {
                                iStartThreadIndex = maiStartAndNumChecks[iThisThreadID].first;
                            }
                            
                            // number of queue entries to check at one time, make sure to cap to the end
                            uint32_t iNumChecksCopy = 100;
                            if(iStartThreadIndex + iNumChecksCopy > maiStartAndNumChecks[iThisThreadID].second)
                            {
                                iNumChecksCopy = maiStartAndNumChecks[iThisThreadID].second - iStartIndex;
                            }
                            
                            WTFASSERT(iStartThreadIndex < 1000000, "wtf");
                            WTFASSERT(iNumChecksCopy < 10000000, "wtf");
                            
                            loadTexturePageThread(
                                mpRenderer.get(),
                                maAlbedoTextureNames,
                                maAlbedoTextureDimensions,
                                maNormalTextureNames,
                                maNormalTextureDimensions,
                                miCurrTotalPageLoaded,
                                miCurrAlbedoPageLoaded,
                                miCurrNormalPageLoaded,
                                *mThreadCommandBuffer,
                                *mThreadCommandQueue,
                                *maScatchPadUploadBuffer[iThisThreadID],
                                *maTexturePageUploadBuffer[iThisThreadID],
                                iStartThreadIndex,
                                iNumChecksCopy,
                                iLastCounterValue,
                                macTexturePageQueueData,
                                macTexturePageInfoData,
                                mTexturePageThreadMutex
                            );
                            
                            iStartThreadIndex += iNumChecksCopy;
                            if(iStartThreadIndex >= maiStartAndNumChecks[iThisThreadID].second)
                            {
                                iStartThreadIndex = 0;
                            }
                            
                            giNumFinished.fetch_add(1);
                            if(iThisThreadID != 0)
                            {
                                std::unique_lock<std::mutex> uniqueLock(mTexturePageThreadMutex);
                                mConditionVariable.wait(uniqueLock);
                            }
                        }
                    }
                },
                iThread);

                DEBUG_PRINTF("Thread %d id = %d\n", iThread, maThreads[iThread]->get_id());

            }   // for thread = 0 to num threads
            mbStartLoadPage = true;
        }

        if(mbQuit)
        {
            for(uint32_t i = 0; i < 4; i++)
            {
                maThreads[i]->join();
            }
        }
    };
    
}

/*
**
*/
void CApp::update(CGFloat time)
{
    //mCameraPosition.x += 0.01f;
    //mCameraLookAt.x += 0.01f;
    
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 diff = normalize(mCameraLookAt - mCameraPosition);
    if(fabs(diff.y) > 0.98f)
    {
        up = float3(1.0f, 0.0f, 0.0f);
    }

    float3 viewDir = normalize(mCameraLookAt - mCameraPosition);
    float3 tangent = cross(up, viewDir);
    float3 binormal = cross(tangent, viewDir);

    Render::Common::UpdateCameraDescriptor cameraDesc = {};
    cameraDesc.mfFar = 100.0f;
    cameraDesc.mfNear = 0.4f;
    cameraDesc.miCameraIndex = 0;
    cameraDesc.mUp = up;
    cameraDesc.mPosition = mCameraPosition;
    cameraDesc.mLookAt = mCameraLookAt;
    cameraDesc.mfFov = 3.14159f * 0.5f;

    uint32_t iFrameIndex = mpRenderer->getFrameIndex();
    uint32_t iBlueNoiseX = iFrameIndex % miBlueNoiseWidth;
    uint32_t iBlueNoiseY = (iFrameIndex / miBlueNoiseWidth) % miBlueNoiseHeight;
    uint32_t iImageIndex = (iBlueNoiseY * miBlueNoiseWidth + iBlueNoiseX) * 4;
    float fFactor = 0.001f;
    cameraDesc.mJitter.x = (((float)macBlueNoiseImageData[iImageIndex] / 255.0f) * 2.0f - 1.0f) * fFactor;
    cameraDesc.mJitter.y = (((float)macBlueNoiseImageData[iImageIndex+1] / 255.0f) * 2.0f - 1.0f) * fFactor;

    mpRenderer->updateCamera(cameraDesc);
    mpRenderer->updateRenderJobData();
    
    DEBUG_PRINTF("texture page loaded: %d\n", miCurrTotalPageLoaded);
}

/*
**
*/
void CApp::render()
{
    @autoreleasepool
    {
        mpRenderer->draw();
        mpRenderer->presentSwapChain();
        mpRenderer->postDraw();
    }
}

/*
**
*/
void CApp::nextDrawable(
    id<MTLDrawable> drawable,
    id<MTLTexture> drawableTexture,
    uint32_t iWidth,
    uint32_t iHeight)
{
    static_cast<RenderDriver::Metal::CSwapChain*>(mpRenderer->getSwapChain())->setDrawable(
        drawable,
        drawableTexture,
        iWidth,
        iHeight);
}

/*
**
*/
void* CApp::getNativeDevice()
{
    return mpRenderer->getDevice()->getNativeDevice();
}

/*
**
*/
void CApp::initRenderData(
    std::vector<char> const& acMaterialBuffer,
    std::vector<std::string> const& aAlbedoTextureNames,
    std::vector<std::string> const& aNormalTextureNames,
    std::vector<uint2> const& aAlbedoTextureDimensions,
    std::vector<uint2> const& aNormalTextureDimensions,
    uint32_t iNumAlbedoTextures,
    uint32_t iNumNormalTextures
)
{
    std::map<std::string, std::vector<PageInfo>> maPageInfo;

    // initialize data, set the albedo and normal texture dimensions
    mpRenderer->mpfnInitData = std::make_unique<std::function<void(Render::Common::CRenderer*)>>();
    *mpRenderer->mpfnInitData = [
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
            std::string fullPath;
            getAssetsDir(fullPath, std::string("converted-dds-scaled/") + textureName);
            int32_t iWidth = 0, iHeight = 0, iNumChannels = 0;
            stbi_uc* pImageData = stbi_load(
                fullPath.c_str(),
                &iWidth,
                &iHeight,
                &iNumChannels,
                4
            );
            WTFASSERT(pImageData != nullptr, "Can\'t open \"%s\"", fullPath.c_str());
            
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
    RenderDriver::Common::CCommandBuffer& commandBuffer,
    RenderDriver::Common::CCommandQueue& commandQueue,
    RenderDriver::Common::CBuffer& threadScratchPathUploadBuffer,
    RenderDriver::Common::CBuffer& texturePageUploadBuffer,
    uint32_t iStartIndex,
    uint32_t iNumChecksPerLoop,
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

    uint32_t const iTextureAtlasSize = 8192;
    uint32_t const iTexturePageSize = 64;
    uint32_t iNumPagesPerRow = iTextureAtlasSize / iTexturePageSize;

    uint32_t const iTexturePageDataSize = iTexturePageSize * iTexturePageSize * 4;

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
            std::max(1u, uint32_t(mipTextureDimension.x / iTexturePageSize)),
            std::max(1u, uint32_t(mipTextureDimension.y / iTexturePageSize))
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

        // texture name
        std::string textureName = aAlbedoTextureNames[iTextureID];
        if(texturePage.miTextureID >= 65536)
        {
            textureName = aNormalTextureNames[iTextureID];
        }
        
        std::string fullTexturePath;
        getAssetsDir(fullTexturePath, std::string("converted-dds-scaled/") + textureName);
        int32_t iImageWidth = 0, iImageHeight = 0, iNumChannels = 0;
        stbi_uc* pImageData = stbi_load(
            fullTexturePath.c_str(),
            &iImageWidth,
            &iImageHeight,
            &iNumChannels,
            4
        );
        WTFASSERT(pImageData != nullptr, "Can\'t open \"%s\"", fullTexturePath.c_str());
        
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
        stbi_image_free(pImageData);
        char const* pTexturePageData = acTexturePageImage.data();
auto durationUS = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
        
        // wait for other thread to finish gpu upload
        uint32_t iExpected = 0;
        while(giCopyingTexturePage.compare_exchange_strong(iExpected, 1u, std::memory_order_acquire, std::memory_order_release) == false)
        {
            usleep(10);
        }

auto start1 = std::chrono::high_resolution_clock::now();

        void* pUploadBuffer = texturePageUploadBuffer.getMemoryOpen(iTexturePageDataSize);
        WTFASSERT(pUploadBuffer, "Can\'t open texture page upload buffer");
        
        uint2 texturePageAtlasCoord = uint2(
            iThreadAlbedoTextureIndex % iNumPagesPerRow,
            (iThreadAlbedoTextureIndex / iNumPagesPerRow) % iNumPagesPerRow
        );
        
        // copy texture page to texture atlas, normal or albedo
        uint32_t iPageIndex = 0;
        if(texturePage.miTextureID >= 65536)
        {
            texturePageAtlasCoord.x = iThreadNormalTextureIndex % iNumPagesPerRow;
            texturePageAtlasCoord.y = (iThreadNormalTextureIndex / iNumPagesPerRow) % iNumPagesPerRow;
            
            auto& textureAtlas1 = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputImageAttachments["Texture Atlas 1"];
            memcpy(pUploadBuffer, pTexturePageData, iTexturePageDataSize);
            pRenderer->copyTexturePageToAtlas2(
                (char const*)pTexturePageData,
                textureAtlas1,
                texturePageAtlasCoord,
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
                    texturePageAtlasCoord,
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
                    texturePageAtlasCoord,
                    iTexturePageSize,
                    commandBuffer,
                    commandQueue,
                    texturePageUploadBuffer
                );
            }
            iPageIndex = iThreadAlbedoTextureIndex + 1;
        }
                
#if 0
        stbi_image_free(pTexturePageData);
#endif // #if 0
        
        auto elapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
        
        auto& texturePageInfoBuffer = pRenderer->mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["MIP Texture Page Hash Table"];
            
        hashEntry.miPageIndex = iPageIndex;
        hashEntry.miUpdateFrame = pRenderer->getFrameIndex() + 1;
        uint32_t iBufferOffset = texturePage.miHashIndex * sizeof(uint32_t) * 4;
        pRenderer->copyCPUToBuffer4(
            texturePageInfoBuffer,
            &hashEntry,
            iBufferOffset,
            sizeof(HashEntry),
            commandBuffer,
            commandQueue,
            threadScratchPathUploadBuffer);
        
        // finished uploading to gpu
        giCopyingTexturePage.store(0);
        
        // update the number of pages loaded
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
