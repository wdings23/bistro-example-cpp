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

#define STRINGIFY_CASE(X)            \
    case X:                     \
        formatStr = #X;         \
        break;                  \

namespace RenderDriver
{
    namespace Metal
    {
        namespace Utils
        {
            /*
            **
            */
            void getFormatString(std::string& formatStr, RenderDriver::Common::Format format)
            {
                MTLPixelFormat formatMetal = convert(format);
                switch(formatMetal)
                {
                    STRINGIFY_CASE(MTLPixelFormatInvalid)
                    STRINGIFY_CASE(MTLPixelFormatA8Unorm)
                    STRINGIFY_CASE(MTLPixelFormatR8Unorm)
                    STRINGIFY_CASE(MTLPixelFormatR8Unorm_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatR8Snorm)
                    STRINGIFY_CASE(MTLPixelFormatR8Uint)
                    STRINGIFY_CASE(MTLPixelFormatR8Sint)
                    STRINGIFY_CASE(MTLPixelFormatR16Unorm)
                    STRINGIFY_CASE(MTLPixelFormatR16Snorm)
                    STRINGIFY_CASE(MTLPixelFormatR16Uint)
                    STRINGIFY_CASE(MTLPixelFormatR16Sint)
                    STRINGIFY_CASE(MTLPixelFormatR16Float)
                    STRINGIFY_CASE(MTLPixelFormatRG8Unorm)
                    STRINGIFY_CASE(MTLPixelFormatRG8Unorm_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatRG8Snorm)
                    STRINGIFY_CASE(MTLPixelFormatRG8Uint)
                    STRINGIFY_CASE(MTLPixelFormatRG8Sint)
                    STRINGIFY_CASE(MTLPixelFormatB5G6R5Unorm)
                    STRINGIFY_CASE(MTLPixelFormatA1BGR5Unorm)
                    STRINGIFY_CASE(MTLPixelFormatABGR4Unorm)
                    STRINGIFY_CASE(MTLPixelFormatBGR5A1Unorm)
                    STRINGIFY_CASE(MTLPixelFormatR32Uint)
                    STRINGIFY_CASE(MTLPixelFormatR32Sint)
                    STRINGIFY_CASE(MTLPixelFormatR32Float)
                    STRINGIFY_CASE(MTLPixelFormatRG16Unorm)
                    STRINGIFY_CASE(MTLPixelFormatRG16Snorm)
                    STRINGIFY_CASE(MTLPixelFormatRG16Uint)
                    STRINGIFY_CASE(MTLPixelFormatRG16Sint)
                    STRINGIFY_CASE(MTLPixelFormatRG16Float)
                    STRINGIFY_CASE(MTLPixelFormatRGBA8Unorm)
                    STRINGIFY_CASE(MTLPixelFormatRGBA8Unorm_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatRGBA8Snorm)
                    STRINGIFY_CASE(MTLPixelFormatRGBA8Uint)
                    STRINGIFY_CASE(MTLPixelFormatRGBA8Sint)
                    STRINGIFY_CASE(MTLPixelFormatBGRA8Unorm)
                    STRINGIFY_CASE(MTLPixelFormatBGRA8Unorm_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatRGB10A2Uint)
                    STRINGIFY_CASE(MTLPixelFormatRG11B10Float)
                    STRINGIFY_CASE(MTLPixelFormatRGB9E5Float)
                    STRINGIFY_CASE(MTLPixelFormatBGR10A2Unorm)
                    STRINGIFY_CASE(MTLPixelFormatBGR10_XR)
                    STRINGIFY_CASE(MTLPixelFormatBGR10_XR_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatRG32Uint)
                    STRINGIFY_CASE(MTLPixelFormatRG32Sint)
                    STRINGIFY_CASE(MTLPixelFormatRG32Float)
                    STRINGIFY_CASE(MTLPixelFormatRGBA16Unorm)
                    STRINGIFY_CASE(MTLPixelFormatRGBA16Snorm)
                    STRINGIFY_CASE(MTLPixelFormatRGBA16Uint)
                    STRINGIFY_CASE(MTLPixelFormatRGBA16Sint)
                    STRINGIFY_CASE(MTLPixelFormatRGBA16Float)
                    STRINGIFY_CASE(MTLPixelFormatBGRA10_XR)
                    STRINGIFY_CASE(MTLPixelFormatBGRA10_XR_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatRGBA32Uint)
                    STRINGIFY_CASE(MTLPixelFormatRGBA32Sint)
                    STRINGIFY_CASE(MTLPixelFormatRGBA32Float)
                    STRINGIFY_CASE(MTLPixelFormatRGB10A2Unorm)
                    STRINGIFY_CASE(MTLPixelFormatBC1_RGBA)
                    STRINGIFY_CASE(MTLPixelFormatBC1_RGBA_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatBC2_RGBA)
                    STRINGIFY_CASE(MTLPixelFormatBC2_RGBA_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatBC3_RGBA)
                    STRINGIFY_CASE(MTLPixelFormatBC3_RGBA_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatBC4_RUnorm)
                    STRINGIFY_CASE(MTLPixelFormatBC4_RSnorm)
                    STRINGIFY_CASE(MTLPixelFormatBC5_RGUnorm)
                    STRINGIFY_CASE(MTLPixelFormatBC5_RGSnorm)
                    STRINGIFY_CASE(MTLPixelFormatBC6H_RGBFloat)
                    STRINGIFY_CASE(MTLPixelFormatBC6H_RGBUfloat)
                    STRINGIFY_CASE(MTLPixelFormatBC7_RGBAUnorm)
                    STRINGIFY_CASE(MTLPixelFormatBC7_RGBAUnorm_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGB_2BPP)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGB_2BPP_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGB_4BPP)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGB_4BPP_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGBA_2BPP)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGBA_2BPP_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGBA_4BPP)
                    STRINGIFY_CASE(MTLPixelFormatPVRTC_RGBA_4BPP_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatEAC_R11Unorm)
                    STRINGIFY_CASE(MTLPixelFormatEAC_R11Snorm)
                    STRINGIFY_CASE(MTLPixelFormatEAC_RG11Unorm)
                    STRINGIFY_CASE(MTLPixelFormatEAC_RG11Snorm)
                    STRINGIFY_CASE(MTLPixelFormatEAC_RGBA8)
                    STRINGIFY_CASE(MTLPixelFormatEAC_RGBA8_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatETC2_RGB8)
                    STRINGIFY_CASE(MTLPixelFormatETC2_RGB8_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatETC2_RGB8A1)
                    STRINGIFY_CASE(MTLPixelFormatETC2_RGB8A1_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_4x4_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_5x4_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_5x5_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_6x5_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_6x6_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x5_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x6_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x8_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x5_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x6_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x8_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x10_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_12x10_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_12x12_sRGB)
                    STRINGIFY_CASE(MTLPixelFormatASTC_4x4_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_5x4_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_5x5_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_6x5_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_6x6_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x5_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x6_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x8_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x5_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x6_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x8_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x10_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_12x10_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_12x12_LDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_4x4_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_5x4_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_5x5_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_6x5_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_6x6_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x5_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x6_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_8x8_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x5_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x6_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x8_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_10x10_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_12x10_HDR)
                    STRINGIFY_CASE(MTLPixelFormatASTC_12x12_HDR)
                    STRINGIFY_CASE(MTLPixelFormatGBGR422)
                    STRINGIFY_CASE(MTLPixelFormatBGRG422)
                    STRINGIFY_CASE(MTLPixelFormatDepth16Unorm)
                    STRINGIFY_CASE(MTLPixelFormatDepth32Float)
                    STRINGIFY_CASE(MTLPixelFormatStencil8)
                    STRINGIFY_CASE(MTLPixelFormatDepth24Unorm_Stencil8)
                    STRINGIFY_CASE(MTLPixelFormatDepth32Float_Stencil8)
                    STRINGIFY_CASE(MTLPixelFormatX32_Stencil8)
                    STRINGIFY_CASE(MTLPixelFormatX24_Stencil8)
                }
            }
        
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
