#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/CommandBufferVulkan.h>
#include <render-driver/Vulkan/CommandAllocatorVulkan.h>
#include <render-driver/Vulkan/DescriptorSetVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <wtfassert.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        PLATFORM_OBJECT_HANDLE CCommandBuffer::create(
            RenderDriver::Common::CommandBufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CCommandBuffer::create(desc, device);

            RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
            VkDevice& nativeDeviceVulkan = *(reinterpret_cast<VkDevice*>(deviceVulkan.getNativeDevice()));

            RenderDriver::Vulkan::CCommandAllocator* pCommandAllocatorVulkan = static_cast<RenderDriver::Vulkan::CCommandAllocator*>(desc.mpCommandAllocator);
            VkCommandPool* pCommandPool = reinterpret_cast<VkCommandPool*>(pCommandAllocatorVulkan->getNativeCommandAllocator());

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = *pCommandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            VkResult ret = vkAllocateCommandBuffers(nativeDeviceVulkan, &allocInfo, &mNativeCommandBuffer);
            WTFASSERT(ret == VK_SUCCESS, "Error creating command buffer: %s", Utils::getErrorCode(ret));

#if defined(_DEBUG)
            miNumCommands = 0;
#endif // _DEBUG 

            mpNativeDevice = &nativeDeviceVulkan;

            return mHandle;
        }

        /*
        **
        */
        void CCommandBuffer::setID(std::string const& name)
        {
            RenderDriver::Common::CObject::setID(name);

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeCommandBuffer);
            objectNameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
            objectNameInfo.pObjectName = name.c_str();

            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);
        }

        /*
        **
        */
        void CCommandBuffer::setPipelineState(
            RenderDriver::Common::CPipelineState& pipelineState,
            RenderDriver::Common::CDevice& device)
        {
            VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            if(pipelineState.getType() == PipelineType::COMPUTE_PIPELINE_TYPE)
            {
                bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            }
            else if(pipelineState.getType() == PipelineType::RAY_TRACE_PIPELINE_TYPE)
            {
                bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            }

            VkPipeline& nativePipeline = *(static_cast<VkPipeline*>(pipelineState.getNativePipelineState()));
            vkCmdBindPipeline(
                mNativeCommandBuffer,
                bindPoint,
                nativePipeline);

#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_PIPELINE_STATE);
#endif // _DEBUG 
        }

        /*
        **
        */
        void CCommandBuffer::setDescriptor(
            PipelineInfo const& pipelineInfo,
            PLATFORM_OBJECT_HANDLE descriptorHeap,
            RenderDriver::Common::CDevice& device)
        {
            int iDebug = 1;
        }

        /*
        **
        */
        void CCommandBuffer::dispatch(
            uint32_t iGroupX,
            uint32_t iGroupY,
            uint32_t iGroupZ,
            uint32_t iLocalX,
            uint32_t iLocalY,
            uint32_t iLocalZ)
        {
            mState = RenderDriver::Common::CommandBufferState::Executing;
            
            vkCmdDispatch(
                mNativeCommandBuffer,
                iGroupX,
                iGroupY,
                iGroupZ);

#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_DISPATCH);
#endif // _DEBUG 
        }

        /*
        **
        */
        void CCommandBuffer::drawIndexInstanced(
            uint32_t iIndexCountPerInstance,
            uint32_t iNumInstances,
            uint32_t iStartIndexLocation,
            uint32_t iBaseVertexLocation,
            uint32_t iStartInstanceLocation,
            RenderDriver::Common::CBuffer* pIndexBuffer)
        {
            mState = RenderDriver::Common::CommandBufferState::Executing;
            
            vkCmdDrawIndexed(
                mNativeCommandBuffer,
                iIndexCountPerInstance,
                iNumInstances,
                iStartIndexLocation,
                iBaseVertexLocation,
                iStartInstanceLocation);

#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_DRAW_INDEXED_INSTANCE);
#endif // _DEBUG 
        }

        /*
        **
        */
        void CCommandBuffer::setGraphicsConstantBufferView(
            uint64_t iConstantBufferAddress,
            uint32_t iDataSize)
        {
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_CONSTANT_BUFFER_VIEW);
#endif // _DEBUG 
        }

        /*
        **
        */
        void CCommandBuffer::setGraphicsConstantBufferView2(
            RenderDriver::Common::CBuffer& buffer,
            uint32_t iDataSize)
        {
        }

        /*
        **
        */
        void CCommandBuffer::setGraphicsConstantBufferView3(
            RenderDriver::Common::CBuffer& buffer,
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            uint32_t iSetIndex,
            uint32_t iBindingIndex,
            uint32_t iDataSize)
        {
            RenderDriver::Vulkan::CDescriptorSet& descriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet&>(descriptorSet);

            std::vector<VkDescriptorSet> const& aDescriptorSets = descriptorSetVulkan.getNativeDescriptorSets();
            VkDescriptorSet const& nativeDescriptorSet = aDescriptorSets[iSetIndex];
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = *(static_cast<VkBuffer*>(buffer.getNativeBuffer()));
            bufferInfo.offset = 0;
            bufferInfo.range = iDataSize;

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = nativeDescriptorSet;
            descriptorWrite.dstBinding = iBindingIndex;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(
                *static_cast<VkDevice*>(mpNativeDevice),
                1,
                &descriptorWrite,
                0,
                nullptr);

#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_CONSTANT_BUFFER_VIEW);
#endif // _DEBUG 
        }

        /*
        **
        */
        void CCommandBuffer::setIndexAndVertexBufferView(
            uint64_t iIndexBufferAddress,
            uint32_t iIndexBufferSize,
            uint32_t iFormat,
            uint64_t iVertexBufferAddress,
            uint32_t iVertexBufferSize,
            uint32_t iVertexStrideSizeInBytes,
            RenderDriver::Common::CBuffer& indexBuffer,
            RenderDriver::Common::CBuffer& vertexBuffer)
        {
            VkBuffer& nativeVertexBuffer = *(static_cast<VkBuffer*>(vertexBuffer.getNativeBuffer()));
            VkBuffer& nativeIndexBuffer = *(static_cast<VkBuffer*>(indexBuffer.getNativeBuffer()));
            
            VkDeviceSize aOffsets[1] = { iVertexBufferAddress };
            vkCmdBindVertexBuffers(
                mNativeCommandBuffer,
                0,
                1,
                &nativeVertexBuffer,
                aOffsets);
            
            vkCmdBindIndexBuffer(
                mNativeCommandBuffer,
                nativeIndexBuffer,
                iIndexBufferAddress,
                VK_INDEX_TYPE_UINT32);

#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_DRAW_BUFFERS);
#endif // _DEBUG 

        }

        /*
        **
        */
        void CCommandBuffer::setIndexAndMultipleVertexBufferViews(
            uint64_t iIndexBufferAddress,
            uint32_t iIndexBufferSize,
            uint32_t iFormat,
            uint64_t iVertexPositionBufferAddress,
            uint32_t iVertexPositionBufferSize,
            uint32_t iVertexPositionStrideSizeInBytes,
            uint64_t iVertexNormalBufferAddress,
            uint32_t iVertexNormalBufferSize,
            uint32_t iVertexNormalStrideSizeInBytes,
            uint64_t iVertexUVBufferAddress,
            uint32_t iVertexUVBufferSize,
            uint32_t iVertexUVStrideSizeInBytes)
        {
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_DRAW_BUFFERS);
#endif // _DEBUG 

        }

        /*
        **
        */
        void CCommandBuffer::clearRenderTarget(PLATFORM_OBJECT_HANDLE renderTarget)
        {
            mState = RenderDriver::Common::CommandBufferState::Executing;
        }

        /*
        **
        */
        void CCommandBuffer::drawIndirectCount(
            RenderDriver::Common::CBuffer& drawIndexBuffer,
            RenderDriver::Common::CBuffer& drawCountBuffer,
            RenderDriver::Common::CBuffer& meshIndexBuffer,
            uint32_t iCountBufferOffset
        )
        {
            vkCmdDrawIndexedIndirectCount(
                mNativeCommandBuffer,
                *((VkBuffer*)drawIndexBuffer.getNativeBuffer()),
                0,
                *((VkBuffer*)drawCountBuffer.getNativeBuffer()),
                iCountBufferOffset,
                4000,
                sizeof(uint32_t) * 5
            );

        }

        /*
        **
        */
        void CCommandBuffer::reset()
        {
            mState = RenderDriver::Common::CommandBufferState::Initialized;
            VkResult ret = vkResetCommandBuffer(mNativeCommandBuffer, 0);
            WTFASSERT(ret == VK_SUCCESS, "Error resetting command buffer: %s", Utils::getErrorCode(ret));

if(mID == "Upload Command Buffer")
{
    int iDebug = 1;
}

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr; // Optional

            ret = vkBeginCommandBuffer(mNativeCommandBuffer, &beginInfo);
            WTFASSERT(ret == VK_SUCCESS, "Error starting command buffer: %s", Utils::getErrorCode(ret));

#if defined(_DEBUG)
            miNumCommands = 0;
#endif // _DEBUG 

        }

        /*
        **
        */
        void CCommandBuffer::close()
        {
            mState = RenderDriver::Common::CommandBufferState::Closed;
            VkResult ret = vkEndCommandBuffer(mNativeCommandBuffer);
            WTFASSERT(ret == VK_SUCCESS, "Error closing command buffer: %s", Utils::getErrorCode(ret));
        }

        /*
        **
        */
        void* CCommandBuffer::getNativeCommandList()
        {
            return &mNativeCommandBuffer;
        }

    }   // Common

}   // RenderDriver
