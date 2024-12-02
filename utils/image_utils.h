#pragma once

#include <vector>
#include <string>

namespace ImageUtils
{
    void getImageDimensions(
        uint32_t& iImageWidth,
        uint32_t& iImageHeight,
        uint32_t& iNumComp,
        std::string const& filePath);

    void loadImage(
        std::vector<unsigned char>& aImageData,
        uint32_t& iImageWidth,
        uint32_t& iImageHeight,
        uint32_t& iNumComp,
        std::string const& filePath);
}