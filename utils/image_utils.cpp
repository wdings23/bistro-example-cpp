#include "image_utils.h"

#include <math.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <filesystem>

namespace ImageUtils
{
    /*
    **
    */
    void getImageDimensions(
        uint32_t& iImageWidth,
        uint32_t& iImageHeight,
        uint32_t& iNumComp,
        std::string const& filePath)
    {
        // geometry image
        int32_t iWidth = 0, iHeight = 0, iComp = 0;
        stbi_uc* pImageData = stbi_load(
            filePath.c_str(),
            &iWidth,
            &iHeight,
            &iComp,
            4);

        iImageWidth = (uint32_t)iWidth;
        iImageHeight = (uint32_t)iHeight;
        iNumComp = (uint32_t)iComp;

        stbi_image_free(pImageData);
    }

    /*
    **
    */
    void loadImage(
        std::vector<unsigned char>& aImageData,
        uint32_t& iImageWidth,
        uint32_t& iImageHeight,
        uint32_t& iNumComp,
        std::string const& filePath)
    {
        // geometry image
        int32_t iWidth = 0, iHeight = 0, iComp = 0;
        stbi_uc* pImageData = stbi_load(
            filePath.c_str(),
            &iWidth,
            &iHeight,
            &iComp,
            4);

        iImageWidth = (uint32_t)iWidth;
        iImageHeight = (uint32_t)iHeight;
        iNumComp = (uint32_t)iComp;

        aImageData.resize(iWidth * iHeight * iComp);
        memcpy(aImageData.data(), pImageData, iWidth * iHeight * iNumComp);

        stbi_image_free(pImageData);
    }



    /*
    **
    */
    void rescaleImages(
        std::vector<std::string> const& aTextureNames,
        std::string const& srcDirectory,
        std::string const& destDirectory,
        uint32_t iMaxImageWidth,
        uint32_t iMaxImageHeight,
        std::string const& imageDimensionFileName
    )
    {
        if(!std::filesystem::exists(destDirectory))
        {
            std::filesystem::create_directory(destDirectory);
        }

        struct uint2
        {
            uint32_t        x;
            uint32_t        y;

            uint2(uint32_t iX, uint32_t iY) { x = iX; y = iY; }
        };
        std::vector<uint2> aDimensions;
        for(auto const& textureName : aTextureNames)
        {
            auto directoryEnd = textureName.find_last_of("\\");
            if(directoryEnd == std::string::npos)
            {
                directoryEnd = textureName.find_last_of("/");
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

            std::string directoryName = textureName.substr(0, directoryEnd);

            auto fileExtensionStart = textureName.find_last_of(".");
            if(fileExtensionStart == std::string::npos)
            {
                fileExtensionStart = textureName.length();
            }

            std::string baseName = textureName.substr(directoryEnd, fileExtensionStart - directoryEnd);
            std::string srcFullPath = srcDirectory + "/" + baseName + ".png";

            int32_t iWidth = 0, iHeight = 0, iNumChannels = 0;
            stbi_uc* pOrigImageData = stbi_load(
                srcFullPath.c_str(),
                &iWidth,
                &iHeight,
                &iNumChannels,
                4
            );

            int32_t iMaxWidth = std::min(iWidth, (int32_t)iMaxImageWidth);
            int32_t iMaxHeight = std::min(iHeight, (int32_t)iMaxImageHeight);

            std::vector<char> acScaledImageData(iMaxWidth * iMaxHeight * 4);

            float fScaleX = (float)iWidth / (float)iMaxWidth;
            float fScaleY = (float)iHeight / (float)iMaxHeight;
            float fX = 0.0f, fY = 0.0f;
            for(int32_t iY = 0; iY < iMaxWidth; iY++)
            {
                int32_t iSampleY = int32_t(fY);
                fY += fScaleY;
                fX = 0.0f;
                for(int32_t iX = 0; iX < iMaxHeight; iX++)
                {
                    int32_t iSampleX = int32_t(fX);
                    fX += fScaleX;

                    int32_t iOrigIndex = (iSampleY * iWidth + iSampleX) * 4;
                    int32_t iScaleIndex = (iY * iMaxWidth + iX) * 4;

                    acScaledImageData[iScaleIndex] = pOrigImageData[iOrigIndex];
                    acScaledImageData[iScaleIndex+1] = pOrigImageData[iOrigIndex+1];
                    acScaledImageData[iScaleIndex+2] = pOrigImageData[iOrigIndex+2];
                    acScaledImageData[iScaleIndex+3] = pOrigImageData[iOrigIndex+3];
                }
            }

            std::string destFullPath = destDirectory + "/" + baseName + ".png";
            stbi_write_png(
                destFullPath.c_str(),
                iMaxWidth,
                iMaxHeight,
                4,
                acScaledImageData.data(),
                iMaxWidth * 4
            );

            aDimensions.push_back(uint2(iMaxWidth, iMaxHeight));
        }

        std::string dimensionFullPath = destDirectory + "/" + imageDimensionFileName + ".txt";
        FILE* fp = fopen(dimensionFullPath.c_str(), "wb");
        fwrite(aDimensions.data(), sizeof(uint2), aDimensions.size(), fp);
        fclose(fp);
    }

    /*
    **
    */
    /*
    **
    */
    void convertNormalImages(
        std::vector<std::string> const& aTextureNames,
        std::string const& srcDirectory,
        std::string const& destDirectory
    )
    {
        for(auto const& textureName : aTextureNames)
        {
            auto directoryEnd = textureName.find_last_of("\\");
            if(directoryEnd == std::string::npos)
            {
                directoryEnd = textureName.find_last_of("/");
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

            std::string directoryName = textureName.substr(0, directoryEnd);

            auto fileExtensionStart = textureName.find_last_of(".");
            if(fileExtensionStart == std::string::npos)
            {
                fileExtensionStart = textureName.length();
            }

            std::string baseName = textureName.substr(directoryEnd, fileExtensionStart - directoryEnd);
            std::string srcFullPath = srcDirectory + "/" + baseName + ".png";

            int32_t iWidth = 0, iHeight = 0, iNumChannels = 0;
            stbi_uc* pOrigImageData = stbi_load(
                srcFullPath.c_str(),
                &iWidth,
                &iHeight,
                &iNumChannels,
                4
            );

            std::vector<unsigned char> acImageData(iWidth * iHeight * 4);
            unsigned char* pImageData = acImageData.data();
            for(int32_t iY = 0; iY < iHeight; iY++)
            {
                for(int32_t iX = 0; iX < iWidth; iX++)
                {
                    int32_t iIndex = (iY * iWidth + iX) * 4;
                    float fX = (float)pOrigImageData[iIndex] / 255.0f;
                    float fY = (float)pOrigImageData[iIndex+1] / 255.0f;

                    float fPhi = (fX * 2.0f - 1.0f) * 3.14159f;
                    float fTheta = fY * 2.0f * 3.14159f;

                    float fLocalNormalX = (sinf(fPhi) * cosf(fTheta)) * 0.5f + 0.5f;
                    float fLocalNormalY = (sinf(fPhi) * sinf(fTheta)) * 0.5f + 0.5f;
                    float fLocalNormalZ = cosf(fPhi) * 0.5f + 0.5f;

                    pImageData[iIndex] = (unsigned char)(fLocalNormalX * 255.0f);
                    pImageData[iIndex+1] = (unsigned char)(fLocalNormalY * 255.0f);
                    pImageData[iIndex+2] = (unsigned char)(fLocalNormalZ * 255.0f);
                    pImageData[iIndex+3] = 255;
                }
            }

            std::string destFullPath = destDirectory + "/" + baseName + ".png";
            stbi_write_png(
                destFullPath.c_str(),
                iWidth,
                iHeight,
                4,
                pImageData,
                iWidth * 4
            );
        }
    }

}
