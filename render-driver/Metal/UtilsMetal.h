#pragma once

#include <stdint.h>

#include <render-driver/Image.h>

#include <Metal/Metal.h>

namespace RenderDriver
{
    namespace Metal
    {
        namespace Utils
        {
            struct ImageLayoutTransition
            {
                RenderDriver::Common::CImage*           mpImage = nullptr;
                RenderDriver::Common::ImageLayout       mBefore = RenderDriver::Common::ImageLayout::UNDEFINED;
                RenderDriver::Common::ImageLayout       mAfter = RenderDriver::Common::ImageLayout::UNDEFINED;
            };

            MTLPixelFormat convert(RenderDriver::Common::Format format);
            MTLCompareFunction convert(RenderDriver::Common::ComparisonFunc compareFunc);

        }   // Utils

    }   // Metal

}   // RenderDriver