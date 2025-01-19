#pragma once

#include <serialize_utils.h>

namespace RenderDriver
{
   namespace Common
    {
       namespace Utils
       {
           struct TransitionBarrierInfo
           {
               RenderDriver::Common::CBuffer*                       mpBuffer = nullptr;
               RenderDriver::Common::CImage*                        mpImage = nullptr;

               RenderDriver::Common::ResourceStateFlagBits          mBefore;
               RenderDriver::Common::ResourceStateFlagBits          mAfter;

               bool                                                 mbBeginOnly = false;
               bool                                                 mbEndOnly = false;
           
               std::string                                          mParentJobName;

               bool                                                 mbWriteableBefore = false;
               bool                                                 mbWriteableAfter = false;

               RenderDriver::Common::CommandBufferType              mCommandBufferType = RenderDriver::Common::CommandBufferType::Graphics;
           };

           void transitionBarrier(
               ShaderResourceInfo& shaderResource,
               RenderDriver::Common::CCommandBuffer& commandBuffer,
               RenderDriver::Common::ResourceStateFlagBits before,
               RenderDriver::Common::ResourceStateFlagBits after);

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
               TransitionBarrierInfo const* aBarrierInfo,
               RenderDriver::Common::CCommandBuffer& commandBuffer,
               uint32_t iNumBarrierInfo,
               bool bReverseState);
       }
    }
    
    
}