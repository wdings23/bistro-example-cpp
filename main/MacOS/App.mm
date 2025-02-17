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

#define WINDOW_WIDTH    1024
#define WINDOW_HEIGHT   1024

/*
**
*/
char const* getSaveDir()
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationScriptsDirectory, NSUserDomainMask, YES);
    NSString* applicationSupportDirectory = [paths firstObject];
    char const* szDir = [applicationSupportDirectory UTF8String];
    DEBUG_PRINTF("save directory: \"%s\"\n", szDir);
    
    return szDir;
}

/*
**
*/
void getAssetsDir(std::string& fullPath, std::string const& fileName)
{
    NSString* resourcePath = [[NSBundle mainBundle] bundlePath];
    char const* szBundleDir = [resourcePath UTF8String];
    DEBUG_PRINTF("bundle directory: \"%s\"\n", szBundleDir);
    
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

        pRenderer->copyCPUToBuffer(
            pBuffer,
            aMaterials.data(),
            0,
            (uint32_t)iDataSize
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

        pRenderer->copyCPUToBuffer(
            pBuffer,
            aMeshRanges.data(),
            0,
            (uint32_t)iDataSize,
            uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY)
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
    
    Render::Common::gLightDirection = normalize(float3(-0.703f, 0.5403f, -0.461f));
    Render::Common::gfEmissiveRadiance = 0.0f;
    Render::Common::gfClearReservoir = 0.0f;

    // TODO: move vertex and index buffer out of renderer
    
    pDesc->mRenderJobsFilePath = "/Users/dingwings/projects/bistro-example-cpp/render-jobs/bistro-example-render-jobs.json";
    mpRenderer->setup(*pDesc);
    
    std::string trianglePath;
    getAssetsDir(trianglePath, "bistro2-triangles.bin");
    
    // number of meshes
    uint32_t miNumMeshes = 0;
    FILE* fp = fopen(trianglePath.c_str(), "rb");
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
        mpRenderer.get());
    
    // set up buffers used for all the passes
    std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>> aBufferMap;
    setupExternalDataBuffers(
        aBufferMap,
        mpRenderer.get()
    );
}

/*
**
*/
void CApp::update(CGFloat time)
{
    
}

/*
**
*/
void CApp::render()
{
    
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
    
}

/*
**
*/
void* CApp::getNativeDevice()
{
    return mpRenderer->getDevice()->getNativeDevice();
}
