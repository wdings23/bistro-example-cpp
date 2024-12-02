#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/AccelerationStructureVulkan.h>
#include <render-driver/Vulkan/BufferVulkan.h>


namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CAccelerationStructure::create(
            AccelerationStructureDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CAccelerationStructure::create(desc, device);

#if 0
            VkInstance& vulkanInstance = *((VkInstance*)desc.mpInstance);

            PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
                vkGetInstanceProcAddr(
                    vulkanInstance,
                    "vkGetBufferDeviceAddressKHR")
                );

            PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
                vkGetInstanceProcAddr(
                    vulkanInstance,
                    "vkCreateAccelerationStructureKHR")
                );

            PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
                vkGetInstanceProcAddr(
                    vulkanInstance,
                    "vkGetAccelerationStructureBuildSizesKHR")
                );

            PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
                vkGetInstanceProcAddr(
                    vulkanInstance,
                    "vkCmdBuildAccelerationStructuresKHR")
                );

            PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
                vkGetInstanceProcAddr(
                    vulkanInstance,
                    "vkGetAccelerationStructureDeviceAddressKHR")
                );

            // We flip the matrix [1][1] = -1.0f to accomodate for the glTF up vector
            VkTransformMatrixKHR transformMatrix =
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f
            };

            std::unique_ptr<RenderDriver::Vulkan::CBuffer> mTransformBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor transformBufferDesc;
            transformBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage((uint32_t)(
                uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly) |
                uint32_t(RenderDriver::Common::BufferUsage::TransferDest))
            );
            transformBufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            transformBufferDesc.miSize = sizeof(VkAccelerationStructureInstanceKHR);
            mTransformBuffer->create(
                transformBufferDesc,
                *mpDevice);

            platformUploadResourceData(
                *mTransformBuffer,
                &transformMatrix,
                sizeof(transformMatrix),
                0
            );

            VkBufferDeviceAddressInfoKHR bufferAddressInfoDesc = {};
            bufferAddressInfoDesc.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bufferAddressInfoDesc.buffer = *((VkBuffer*)maIndexBuffers["bistro"]->getNativeBuffer());
            VkDeviceAddress triangleIndexBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &bufferAddressInfoDesc
            );

            bufferAddressInfoDesc.buffer = *((VkBuffer*)maTrianglePositionBuffers["bistro"]->getNativeBuffer());
            VkDeviceAddress trianglePositionBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &bufferAddressInfoDesc
            );

            bufferAddressInfoDesc.buffer = *((VkBuffer*)mTransformBuffer->getNativeBuffer());
            VkDeviceAddress transformDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &bufferAddressInfoDesc
            );

            VkDeviceAddress mBottomAccelerationStructureHandle;
            {
                VkDevice& nativeDevice = *((VkDevice*)mpDevice->getNativeDevice());

                // geometries for all the meshes
                std::vector<VkAccelerationStructureGeometryKHR> aGeometries;
                std::vector<VkAccelerationStructureBuildRangeInfoKHR> aBuildRangeInfo;
                std::vector<uint32_t> aiMaxPrimitiveCounts;
                for(auto const& meshRange : aMeshRanges)
                {
                    VkDeviceOrHostAddressConstKHR trianglePositionAddress;
                    trianglePositionAddress.deviceAddress = trianglePositionBufferDeviceAddress;

                    VkDeviceOrHostAddressConstKHR triangleIndexAddress;
                    triangleIndexAddress.deviceAddress = triangleIndexBufferDeviceAddress;

                    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress;
                    transformBufferDeviceAddress.deviceAddress = transformDeviceAddress;

                    VkAccelerationStructureGeometryKHR geometry = {};
                    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
                    geometry.geometry.triangles.vertexData = trianglePositionAddress;
                    geometry.geometry.triangles.maxVertex = (uint32_t)aTrianglePositions.size();

                    geometry.geometry.triangles.vertexStride = sizeof(float4);
                    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
                    geometry.geometry.triangles.indexData = triangleIndexAddress;
                    geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

                    aGeometries.push_back(geometry);
                    aiMaxPrimitiveCounts.push_back((meshRange.second - meshRange.first) / 3);

                    uint32_t iNumTriangles = (meshRange.second - meshRange.first) / 3;
                    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {};
                    buildRangeInfo.firstVertex = 0;
                    buildRangeInfo.primitiveOffset = meshRange.first * sizeof(uint32_t);
                    buildRangeInfo.primitiveCount = iNumTriangles;
                    buildRangeInfo.transformOffset = 0;
                    aBuildRangeInfo.push_back(buildRangeInfo);
                }

                // primitive ranges
                std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos;
                for(auto& rangeInfo : aBuildRangeInfo) {
                    pBuildRangeInfos.push_back(&rangeInfo);
                }

                // Get size info
                VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
                accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
                accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(aGeometries.size());
                accelerationStructureBuildGeometryInfo.pGeometries = aGeometries.data();

                // acceleration structure build size to allocate buffers (acceleration buffer, build scratch buffer)
                VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
                accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
                vkGetAccelerationStructureBuildSizesKHR(
                    nativeDevice,
                    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                    &accelerationStructureBuildGeometryInfo,
                    aiMaxPrimitiveCounts.data(),
                    &accelerationStructureBuildSizesInfo);

                // bottom acceleration buffer
                std::unique_ptr<RenderDriver::Vulkan::CBuffer> mpBottomLevelAccelerationStructureBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
                RenderDriver::Common::BufferDescriptor bottomLASDesc;
                bottomLASDesc.miSize = accelerationStructureBuildSizesInfo.accelerationStructureSize;
                bottomLASDesc.mBufferUsage = (RenderDriver::Common::BufferUsage)(
                    uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                    uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly)
                    );
                mpBottomLevelAccelerationStructureBuffer->create(
                    bottomLASDesc,
                    *mpDevice
                );

                // bottom acceleration structure handle
                VkAccelerationStructureKHR mBottomLevelASHandle;
                VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
                accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
                accelerationStructureCreateInfo.buffer = *((VkBuffer*)mpBottomLevelAccelerationStructureBuffer->getNativeBuffer());
                accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
                accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                vkCreateAccelerationStructureKHR(
                    nativeDevice,
                    &accelerationStructureCreateInfo,
                    nullptr,
                    &mBottomLevelASHandle);

                // Create a small scratch buffer used during build of the bottom level acceleration structure
                std::unique_ptr<RenderDriver::Vulkan::CBuffer> mScratcuBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
                RenderDriver::Common::BufferDescriptor scratchBufferDesc;
                scratchBufferDesc.miSize = accelerationStructureBuildSizesInfo.buildScratchSize;
                scratchBufferDesc.mBufferUsage = (RenderDriver::Common::BufferUsage)(
                    uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                    uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly)
                    );
                mScratcuBuffer->create(
                    scratchBufferDesc,
                    *mpDevice
                );
                bufferAddressInfoDesc.buffer = *((VkBuffer*)mScratcuBuffer->getNativeBuffer());
                VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                    *((VkDevice*)mpDevice->getNativeDevice()),
                    &bufferAddressInfoDesc
                );

                // Build the acceleration structure on the device via a one-time command buffer submission
                // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
                accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                accelerationStructureBuildGeometryInfo.dstAccelerationStructure = mBottomLevelASHandle;
                accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;
                RenderDriver::Vulkan::CCommandBuffer* pCommandBuffer = maRenderJobCommandBuffers["Atmosphere Graphics"].get();
                pCommandBuffer->reset();
                VkCommandBuffer& nativeCommandBuffer = *((VkCommandBuffer*)pCommandBuffer->getNativeCommandList());
                vkCmdBuildAccelerationStructuresKHR(
                    nativeCommandBuffer,
                    1,
                    &accelerationStructureBuildGeometryInfo,
                    pBuildRangeInfos.data());
                pCommandBuffer->close();
                mpGraphicsCommandQueue->execCommandBuffer(
                    *pCommandBuffer,
                    *mpDevice
                );
                vkQueueWaitIdle(
                    *((VkQueue*)mpGraphicsCommandQueue->getNativeCommandQueue())
                );

                bufferAddressInfoDesc.buffer = *((VkBuffer*)mpBottomLevelAccelerationStructureBuffer->getNativeBuffer());
                VkDeviceAddress bottomASDeviceAddress = vkGetBufferDeviceAddressKHR(
                    *((VkDevice*)mpDevice->getNativeDevice()),
                    &bufferAddressInfoDesc
                );

                VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo = {};
                accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
                accelerationDeviceAddressInfo.accelerationStructure = mBottomLevelASHandle;
                mBottomAccelerationStructureHandle = vkGetAccelerationStructureDeviceAddressKHR(
                    nativeDevice,
                    &accelerationDeviceAddressInfo
                );

                mScratcuBuffer->releaseNativeBuffer();
                mScratcuBuffer.reset();
            }



            {
                VkAccelerationStructureGeometryTrianglesDataKHR triangles = {};
                triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                triangles.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
                triangles.vertexData.deviceAddress = trianglePositionBufferDeviceAddress;
                triangles.vertexStride = sizeof(float4);
                triangles.indexType = VK_INDEX_TYPE_UINT32;
                triangles.indexData.deviceAddress = triangleIndexBufferDeviceAddress;
                triangles.maxVertex = (uint32_t)aTrianglePositions.size() - 1;

                VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
                accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
                accelerationStructureGeometry.geometry.triangles = triangles;

                VkAccelerationStructureBuildRangeInfoKHR offset;
                offset.firstVertex = 0;
                offset.primitiveCount = (uint32_t)aiTriangleIndices.size() / 3;
                offset.primitiveOffset = 0;
                offset.transformOffset = 0;

            }

            VkDevice& nativeDevice = *((VkDevice*)mpDevice->getNativeDevice());



            VkAccelerationStructureInstanceKHR instance = {};
            instance.transform = transformMatrix;
            instance.instanceCustomIndex = 0;
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.accelerationStructureReference = mBottomAccelerationStructureHandle;

            std::unique_ptr<RenderDriver::Vulkan::CBuffer> mpInstanceBuffer =
                std::make_unique<RenderDriver::Vulkan::CBuffer>();

            RenderDriver::Common::BufferDescriptor instanceBufferDesc;
            instanceBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage((uint32_t)(
                uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly))
            );
            instanceBufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            instanceBufferDesc.miSize = sizeof(VkAccelerationStructureInstanceKHR);
            mpInstanceBuffer->create(
                instanceBufferDesc,
                *mpDevice);
            VkDeviceMemory& instanceDataDeviceMemory = *((VkDeviceMemory*)mpInstanceBuffer->getNativeDeviceMemory());

            VkBufferDeviceAddressInfoKHR bufferDeviceAI = {};
            bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bufferDeviceAI.buffer = *((VkBuffer*)mpInstanceBuffer->getNativeBuffer());
            VkDeviceAddress deviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &bufferDeviceAI
            );

            VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress = {};
            instanceDataDeviceAddress.deviceAddress = deviceAddress;

            VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
            accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
            accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

            // build geometry
            VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
            accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            accelerationStructureBuildGeometryInfo.geometryCount = 1;
            accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

            uint32_t iNumPrimitives = 1;
            VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
            accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
            vkGetAccelerationStructureBuildSizesKHR(
                nativeDevice,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &accelerationStructureBuildGeometryInfo,
                &iNumPrimitives,
                &accelerationStructureBuildSizesInfo
            );

            // TLAS buffer
            std::unique_ptr<RenderDriver::Vulkan::CBuffer> mTLASBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor tlasDesc;
            tlasDesc.mBufferUsage = RenderDriver::Common::BufferUsage((uint32_t)(
                uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit))
            );
            tlasDesc.miSize = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            tlasDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            mTLASBuffer->create(
                tlasDesc,
                *mpDevice
            );

            // TLAS Acceleration Structure
            VkAccelerationStructureKHR mTLASHandle;
            VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
            accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            accelerationStructureCreateInfo.buffer = *((VkBuffer*)mTLASBuffer->getNativeBuffer());
            accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            vkCreateAccelerationStructureKHR(
                nativeDevice,
                &accelerationStructureCreateInfo,
                nullptr,
                &mTLASHandle);

            // Create a small scratch buffer used during build of the top level acceleration structure
            std::unique_ptr<RenderDriver::Vulkan::CBuffer> mScratchBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor scratchBufferDesc;
            scratchBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage((uint32_t)(
                uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer))
            );
            scratchBufferDesc.miSize = accelerationStructureBuildSizesInfo.buildScratchSize;
            scratchBufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            mScratchBuffer->create(
                scratchBufferDesc,
                *mpDevice
            );
            VkBufferDeviceAddressInfoKHR scratchBufferAddressInfoDesc = {};
            scratchBufferAddressInfoDesc.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            scratchBufferAddressInfoDesc.buffer = *((VkBuffer*)mScratchBuffer->getNativeBuffer());
            VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &scratchBufferAddressInfoDesc
            );

            // build acceleration structure
            VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
            accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            accelerationBuildGeometryInfo.dstAccelerationStructure = mTLASHandle;
            accelerationBuildGeometryInfo.geometryCount = 1;
            accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
            accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;
            VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {};
            accelerationStructureBuildRangeInfo.primitiveCount = 1;
            accelerationStructureBuildRangeInfo.primitiveOffset = 0;
            accelerationStructureBuildRangeInfo.firstVertex = 0;
            accelerationStructureBuildRangeInfo.transformOffset = 0;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos =
            {&accelerationStructureBuildRangeInfo};
            RenderDriver::Vulkan::CCommandBuffer* pCommandBuffer = maRenderJobCommandBuffers["Atmosphere Graphics"].get();
            pCommandBuffer->reset();
            VkCommandBuffer& nativeCommandBuffer = *((VkCommandBuffer*)pCommandBuffer->getNativeCommandList());
            vkCmdBuildAccelerationStructuresKHR(
                nativeCommandBuffer,
                1,
                &accelerationBuildGeometryInfo,
                accelerationBuildStructureRangeInfos.data()
            );
            pCommandBuffer->close();
            mpGraphicsCommandQueue->execCommandBuffer(
                *maRenderJobCommandBuffers["Atmosphere Graphics"],
                *mpDevice
            );
            vkQueueWaitIdle(
                *((VkQueue*)mpGraphicsCommandQueue->getNativeCommandQueue())
            );

            mScratchBuffer->releaseNativeBuffer();
            mScratchBuffer.reset();

            mpInstanceBuffer->releaseNativeBuffer();
            mpInstanceBuffer.reset();
#endif // 3if 0

            return mHandle;
        }

        /*
        **
        */
        void CAccelerationStructure::setNativeAccelerationStructure(void* pData)
        {
            VkAccelerationStructureKHR* pAccelerationStructure = (VkAccelerationStructureKHR*)pData;
            mTopLevelAccelerationStructure = *pAccelerationStructure;
        }

        /*
        **
        */
        void* CAccelerationStructure::getNativeAccelerationStructure()
        {
            return &mTopLevelAccelerationStructure;
        }

    }   // Vulkan

}   // RenderDriver