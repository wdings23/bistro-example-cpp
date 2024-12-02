#pragma once

#include <string>
#include <render-driver/Utils.h>
#include <render-driver/CommandBuffer.h>
#include <render-driver/Image.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <serialize_utils.h>


namespace RenderDriver
{
    namespace DX12
    {
        namespace Utils
        {
            std::wstring convertToLPTSTR(std::string const& str);

            void transitionBarrier(
                RenderDriver::Common::CBuffer& buffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::ResourceStateFlagBits before,
                RenderDriver::Common::ResourceStateFlagBits after);

            void transitionBarrier(
                RenderDriver::Common::CImage& image,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::ResourceStateFlagBits before,
                RenderDriver::Common::ResourceStateFlagBits after);

            uint32_t alignForUavCounter(UINT bufferSize);

            void copyBufferToTexture(
                RenderDriver::Common::CBuffer& srcBuffer,
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint64_t iSrcBufferOffset,
                uint32_t iWidth,
                uint32_t iHeight,
                uint32_t iNumChannels,
                uint32_t iChannelSize);

            void copyBuffer(
                RenderDriver::Common::CBuffer& srcBuffer,
                RenderDriver::Common::CBuffer& destBuffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint64_t iSrcBufferOffset,
                uint64_t iDestBufferOffset,
                uint64_t iSize);

            void copyTextureToBuffer(
                RenderDriver::Common::CImage& srcImage,
                RenderDriver::Common::CBuffer& destBuffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint64_t iDestBufferOffset,
                uint32_t iWidth,
                uint32_t iHeight,
                uint32_t iNumChannels,
                uint32_t iChannelSize);

            void transitionBarriers(
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iNumBarrierInfo,
                bool bReverseState);

            void showDebugBreadCrumbs(RenderDriver::DX12::CDevice& device);
        }
    }
}