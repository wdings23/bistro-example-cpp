#include "App.h"
#include "render-driver/Metal/DeviceMetal.h"
#include "render-driver/Metal/SwapChainMetal.h"
//#include "Renderer/Metal/RendererMetal.h"
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
char const* getAssetsDir()
{
    NSString* resourcePath = [[NSBundle mainBundle] bundlePath];
    char const* szBundleDir = [resourcePath UTF8String];
    DEBUG_PRINTF("bundle directory: \"%s\"\n", szBundleDir);
    
    return szBundleDir;
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

    aAlbedoTextureDimensions.resize(iNumAlbedoTextures);
    fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds-scaled\\albedo-dimensions.txt", "rb");
    fread(aAlbedoTextureDimensions.data(), sizeof(uint2), iNumAlbedoTextures, fp);
    fclose(fp);

    aNormalTextureDimensions.resize(iNumNormalTextures);
    fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds-scaled\\normal-dimensions.txt", "rb");
    fread(aNormalTextureDimensions.data(), sizeof(uint2), iNumNormalTextures, fp);
    fclose(fp);
}

/*
**
*/
void CApp::init(AppDescriptor const& appDesc)
{
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
