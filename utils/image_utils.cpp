#include "image_utils.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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


}