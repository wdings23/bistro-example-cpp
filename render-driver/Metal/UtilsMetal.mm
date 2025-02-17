#include <render-driver/Metal/UtilsMetal.h>

#include <utils/wtfassert.h>

#define CASE(X, Y)    \
case X:            \
{                \
ret = Y;    \
break;        \
}
        
#define CASE2(X, Y)    \
case Y:            \
{                \
ret = X;    \
break;        \
}

namespace RenderDriver
{
    namespace Metal
    {
        namespace Utils
        {
            /*
            **
            */
            MTLPixelFormat convert(RenderDriver::Common::Format format)
            {
                MTLPixelFormat ret = MTLPixelFormatInvalid;
                
                switch(format)
                {
                        CASE(RenderDriver::Common::Format::UNKNOWN, MTLPixelFormatInvalid)
                        CASE(RenderDriver::Common::Format::R32G32B32A32_TYPELESS, MTLPixelFormatRGBA32Float)
                        CASE(RenderDriver::Common::Format::R32G32B32A32_FLOAT, MTLPixelFormatRGBA32Float)
                        CASE(RenderDriver::Common::Format::R32G32B32A32_UINT, MTLPixelFormatRGBA32Uint)
                        CASE(RenderDriver::Common::Format::R32G32B32A32_SINT, MTLPixelFormatRGBA32Sint)
                        CASE(RenderDriver::Common::Format::R16G16B16A16_TYPELESS, MTLPixelFormatRGBA16Float)
                        CASE(RenderDriver::Common::Format::R16G16B16A16_FLOAT, MTLPixelFormatRGBA16Float)
                        CASE(RenderDriver::Common::Format::R16G16B16A16_UNORM, MTLPixelFormatRGBA16Unorm)
                        CASE(RenderDriver::Common::Format::R16G16B16A16_UINT, MTLPixelFormatRGBA16Uint)
                        CASE(RenderDriver::Common::Format::R16G16B16A16_SNORM, MTLPixelFormatRGBA16Snorm)
                        CASE(RenderDriver::Common::Format::R16G16B16A16_SINT, MTLPixelFormatRGBA16Sint)
                        CASE(RenderDriver::Common::Format::R32G32_TYPELESS, MTLPixelFormatRG32Float)
                        CASE(RenderDriver::Common::Format::R32G32_FLOAT, MTLPixelFormatRG32Float)
                        CASE(RenderDriver::Common::Format::R32G32_UINT, MTLPixelFormatRG32Uint)
                        CASE(RenderDriver::Common::Format::R32G32_SINT, MTLPixelFormatRG32Sint)
                        CASE(RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT, MTLPixelFormatDepth32Float_Stencil8)
                        CASE(RenderDriver::Common::Format::R32_FLOAT_X8X24_TYPELESS, MTLPixelFormatDepth32Float_Stencil8)
                        CASE(RenderDriver::Common::Format::R10G10B10A2_TYPELESS, MTLPixelFormatBGR10A2Unorm)
                        CASE(RenderDriver::Common::Format::R10G10B10A2_UNORM, MTLPixelFormatBGR10A2Unorm)
                        CASE(RenderDriver::Common::Format::R10G10B10A2_UINT, MTLPixelFormatRGB10A2Uint)
                        CASE(RenderDriver::Common::Format::R8G8B8A8_TYPELESS, MTLPixelFormatRGBA8Sint)
                        CASE(RenderDriver::Common::Format::R8G8B8A8_UNORM, MTLPixelFormatRGBA8Unorm)
                        CASE(RenderDriver::Common::Format::R8G8B8A8_UNORM_SRGB, MTLPixelFormatRGBA8Unorm_sRGB)
                        CASE(RenderDriver::Common::Format::R8G8B8A8_UINT, MTLPixelFormatRGBA8Uint)
                        CASE(RenderDriver::Common::Format::R8G8B8A8_SNORM, MTLPixelFormatRGBA8Snorm)
                        CASE(RenderDriver::Common::Format::R8G8B8A8_SINT, MTLPixelFormatRGBA8Sint)
                        CASE(RenderDriver::Common::Format::B8G8R8A8_UNORM, MTLPixelFormatBGRA8Unorm)
                        CASE(RenderDriver::Common::Format::R16G16_TYPELESS, MTLPixelFormatRG16Float)
                        CASE(RenderDriver::Common::Format::R16G16_FLOAT, MTLPixelFormatRG16Float)
                        CASE(RenderDriver::Common::Format::R16G16_UNORM, MTLPixelFormatRG16Unorm)
                        CASE(RenderDriver::Common::Format::R16G16_UINT, MTLPixelFormatRG16Uint)
                        CASE(RenderDriver::Common::Format::R16G16_SNORM, MTLPixelFormatRG16Snorm)
                        CASE(RenderDriver::Common::Format::R16G16_SINT, MTLPixelFormatRG16Sint)
                        CASE(RenderDriver::Common::Format::R32_TYPELESS, MTLPixelFormatR32Float)
                        CASE(RenderDriver::Common::Format::D32_FLOAT, MTLPixelFormatDepth32Float)
                        CASE(RenderDriver::Common::Format::R32_FLOAT, MTLPixelFormatR32Float)
                        CASE(RenderDriver::Common::Format::R32_UINT, MTLPixelFormatR32Uint)
                        CASE(RenderDriver::Common::Format::R32_SINT, MTLPixelFormatR32Sint)
                        CASE(RenderDriver::Common::Format::D24_UNORM_S8_UINT, MTLPixelFormatDepth24Unorm_Stencil8)
                        CASE(RenderDriver::Common::Format::R24_UNORM_X8_TYPELESS, MTLPixelFormatDepth24Unorm_Stencil8)
                        CASE(RenderDriver::Common::Format::X24_TYPELESS_G8_UINT, MTLPixelFormatX24_Stencil8)
                        CASE(RenderDriver::Common::Format::R8G8_TYPELESS, MTLPixelFormatRG8Snorm)
                        CASE(RenderDriver::Common::Format::R8G8_UNORM, MTLPixelFormatRG8Unorm)
                        CASE(RenderDriver::Common::Format::R8G8_UINT, MTLPixelFormatRG8Uint)
                        CASE(RenderDriver::Common::Format::R8G8_SNORM, MTLPixelFormatRG8Snorm)
                        CASE(RenderDriver::Common::Format::R8G8_SINT, MTLPixelFormatRG8Sint)
                        CASE(RenderDriver::Common::Format::R16_TYPELESS, MTLPixelFormatR16Float)
                        CASE(RenderDriver::Common::Format::R16_FLOAT, MTLPixelFormatR16Float)
                        CASE(RenderDriver::Common::Format::D16_UNORM, MTLPixelFormatDepth16Unorm)
                        CASE(RenderDriver::Common::Format::R16_UNORM, MTLPixelFormatR16Unorm)
                        CASE(RenderDriver::Common::Format::R16_UINT, MTLPixelFormatR16Uint)
                        CASE(RenderDriver::Common::Format::R16_SNORM, MTLPixelFormatR16Snorm)
                        CASE(RenderDriver::Common::Format::R16_SINT, MTLPixelFormatR16Sint)
                        CASE(RenderDriver::Common::Format::R8_TYPELESS, MTLPixelFormatR8Snorm)
                        CASE(RenderDriver::Common::Format::R8_UNORM, MTLPixelFormatR8Unorm)
                        CASE(RenderDriver::Common::Format::R8_UINT, MTLPixelFormatR8Uint)
                        CASE(RenderDriver::Common::Format::R8_SNORM, MTLPixelFormatR8Snorm)
                        CASE(RenderDriver::Common::Format::R8_SINT, MTLPixelFormatR8Sint)
                        CASE(RenderDriver::Common::Format::A8_UNORM, MTLPixelFormatA8Unorm)
                    default:
                        WTFASSERT(0, "no such format: %d\n", format);
                }
                
                return ret;
            }
        
            /*
            **
            */
            MTLCompareFunction convert(RenderDriver::Common::ComparisonFunc compareFunc)
            {
                MTLCompareFunction ret = MTLCompareFunctionNever;
                switch(compareFunc)
                {
                        CASE(RenderDriver::Common::ComparisonFunc::Never, MTLCompareFunctionNever)
                        CASE(RenderDriver::Common::ComparisonFunc::Less, MTLCompareFunctionLess)
                        CASE(RenderDriver::Common::ComparisonFunc::Equal, MTLCompareFunctionEqual)
                        CASE(RenderDriver::Common::ComparisonFunc::LessEqual, MTLCompareFunctionLessEqual)
                        CASE(RenderDriver::Common::ComparisonFunc::Greater, MTLCompareFunctionGreater)
                        CASE(RenderDriver::Common::ComparisonFunc::NotEqual, MTLCompareFunctionNotEqual)
                        CASE(RenderDriver::Common::ComparisonFunc::GreaterEqual, MTLCompareFunctionGreaterEqual)
                        CASE(RenderDriver::Common::ComparisonFunc::Always, MTLCompareFunctionAlways)
                    default:
                        WTFASSERT(0, "no such compare function: %d", compareFunc);
                }
                
                return ret;
            }
        
        }   // Utils
        
    }   // Metal
    
}   // SerializeUtils

#undef CASE
#undef CASE2
