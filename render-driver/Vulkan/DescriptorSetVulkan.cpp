#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/DescriptorSetVulkan.h>
#include <render-driver/Vulkan/BufferVulkan.h>
#include <render-driver/Vulkan/ImageVulkan.h>
#include <render-driver/Vulkan/ImageViewVulkan.h>
#include <render-driver/Vulkan/AccelerationStructureVulkan.h>

#include <render-driver/Vulkan/UtilsVulkan.h>
#include <LogPrint.h>
#include <sstream>

#include <wtfassert.h>

#define DEBUG_PRINT_OUT 1


namespace RenderDriver
{
	namespace Vulkan
	{
		PLATFORM_OBJECT_HANDLE CDescriptorSet::create(
			RenderDriver::Common::DescriptorSetDescriptor const& desc,
			RenderDriver::Common::CDevice& device)
		{
			// have at most 2 sets for each pipeline
			//		set 0 => vertex shader resources
			//		set 1 => fragment shader resources
			//		set 0 => compute shader resources

			RenderDriver::Common::CDescriptorSet::create(desc, device);

			RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
			mpNativeDevice = static_cast<VkDevice*>(deviceVulkan.getNativeDevice());

			// other way of creating descriptor set
			if(desc.mpaShaderResources == nullptr)
			{
				return mHandle;
			}

			std::vector<VkDescriptorSetLayoutBinding> aVertexShaderLayoutBindings;
			std::vector<VkDescriptorSetLayoutBinding> aFragmentShaderLayoutBindings;
			std::vector<VkDescriptorSetLayoutBinding> aComputeShaderLayoutBindings;

			struct DescriptorBufferInfo
			{
				uint32_t						miShaderResourceIndex;
				uint32_t						miSetIndex;
				uint32_t						miBindingIndex;
				ShaderResourceType				mResourceType;
				VkDescriptorBufferInfo			mDescriptorBufferInfo;
			};

			//std::vector<VkDescriptorBufferInfo>		aDescriptorBufferInfo;
			std::vector<VkDescriptorImageInfo>		aDescriptorImageInfo;
			std::vector<VkDescriptorType>			aDescriptorTypes;

			std::vector<DescriptorBufferInfo>	aDescriptorBufferInfo;

			RenderDriver::Common::ShaderType shaderType = RenderDriver::Common::ShaderType::Fragment;

			// layout and keep count of different resources for pool size
			uint32_t iNumReadTextures = 0, iNumWriteTextures = 0, iNumReadBuffers = 0, iNumWriteBuffers = 0;
			std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = *desc.mpaShaderResources;
			uint32_t iShaderIndex = 0;
			
			// for saving layout info
			maLayout.resize(aShaderResources.size());

			for(auto const& shaderResource : aShaderResources)
			{
				// check for filler resource, this is needed to set the correct resource type for null descriptor update
				bool bFillerResource = (
					aShaderResources[iShaderIndex].mName.find("fillerShaderResource") != std::string::npos ||
					aShaderResources[iShaderIndex].mName.find("fillerUnorderedAccessResource") != std::string::npos);

				uint32_t iLayoutSet = UINT32_MAX;
				if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT ||
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT)
				{
					if(shaderResource.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView)
					{
						// write texture

						++iNumWriteTextures;

						VkDescriptorSetLayoutBinding layoutBinding = {};
						layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						layoutBinding.descriptorCount = 1;
						layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

						// set layout binding and layout set based on the shader type
						switch(shaderResource.mShaderType)
						{
							case RenderDriver::Common::ShaderType::Vertex:
								//layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aVertexShaderLayoutBindings.size());
								aVertexShaderLayoutBindings.push_back(layoutBinding);
								iLayoutSet = 0;
								break;
							case RenderDriver::Common::ShaderType::Fragment:
							{
								iLayoutSet = 1;
								//layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aFragmentShaderLayoutBindings.size());
								aFragmentShaderLayoutBindings.push_back(layoutBinding);
								
								break;
							}
							case RenderDriver::Common::ShaderType::Compute:
								layoutBinding.binding = static_cast<uint32_t>(aComputeShaderLayoutBindings.size());
								//layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
								aComputeShaderLayoutBindings.push_back(layoutBinding);
								shaderType = RenderDriver::Common::ShaderType::Compute;
								iLayoutSet = 0;
								
								break;
						}

						aDescriptorTypes.push_back(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

						// filler for now
						DescriptorBufferInfo descriptorBufferInfo = {};
						VkDescriptorBufferInfo bufferInfo = {};
						bufferInfo.offset = 0; 
						bufferInfo.range = VK_WHOLE_SIZE;
						descriptorBufferInfo.miShaderResourceIndex = iShaderIndex;
						descriptorBufferInfo.mDescriptorBufferInfo = bufferInfo;
						descriptorBufferInfo.mResourceType = shaderResource.mType;
						descriptorBufferInfo.miSetIndex = iLayoutSet;
						descriptorBufferInfo.miBindingIndex = layoutBinding.binding;

						aDescriptorBufferInfo.push_back(descriptorBufferInfo);

						maLayout[iShaderIndex].miSetIndex = iLayoutSet;
						maLayout[iShaderIndex].miBindingIndex = layoutBinding.binding;
						maLayout[iShaderIndex].miShaderResourceIndex = iShaderIndex;
						maLayout[iShaderIndex].mType = shaderResource.mType;

						DEBUG_PRINTF("\tset %d binding %d \"%s\" write texture\n", 
							iLayoutSet,
							layoutBinding.binding,
							shaderResource.mName.c_str());
					}
					else
					{
						// read-only texture

						++iNumReadTextures;

						// layout binding, type, count and shader stage
						VkDescriptorSetLayoutBinding layoutBinding = {};
						layoutBinding.descriptorType = (bFillerResource) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						layoutBinding.descriptorCount = 1;
						layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
						
						// set layout binding and layout set based on the shader type
						switch(shaderResource.mShaderType)
						{
							case RenderDriver::Common::ShaderType::Vertex:
								//layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aVertexShaderLayoutBindings.size());
								aVertexShaderLayoutBindings.push_back(layoutBinding);
								iLayoutSet = 0;
								break;
							case RenderDriver::Common::ShaderType::Fragment:
							{
								iLayoutSet = 1;
								//layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aFragmentShaderLayoutBindings.size());
								aFragmentShaderLayoutBindings.push_back(layoutBinding);
								
								break;
							}
							case RenderDriver::Common::ShaderType::Compute:
								layoutBinding.binding = static_cast<uint32_t>(aComputeShaderLayoutBindings.size());
								//layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
								aComputeShaderLayoutBindings.push_back(layoutBinding);
								shaderType = RenderDriver::Common::ShaderType::Compute;
								iLayoutSet = 0;
								break;
						}

						aDescriptorTypes.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

						// filler for now
						DescriptorBufferInfo descriptorBufferInfo = {};
						VkDescriptorBufferInfo bufferInfo = {};
						bufferInfo.offset = 0;
						bufferInfo.range = VK_WHOLE_SIZE;
						descriptorBufferInfo.miShaderResourceIndex = iShaderIndex;
						descriptorBufferInfo.mDescriptorBufferInfo = bufferInfo;
						descriptorBufferInfo.mResourceType = shaderResource.mType;
						descriptorBufferInfo.miSetIndex = iLayoutSet;
						descriptorBufferInfo.miBindingIndex = layoutBinding.binding;

						aDescriptorBufferInfo.push_back(descriptorBufferInfo);

						maLayout[iShaderIndex].miSetIndex = iLayoutSet;
						maLayout[iShaderIndex].miBindingIndex = layoutBinding.binding;
						maLayout[iShaderIndex].miShaderResourceIndex = iShaderIndex;
						maLayout[iShaderIndex].mType = shaderResource.mType;

						DEBUG_PRINTF("\tset %d binding %d \"%s\" read texture\n",
							iLayoutSet,
							layoutBinding.binding,
							shaderResource.mName.c_str());
					}
				}
				else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN || 
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT ||
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT)
				{
					if(shaderResource.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView)
					{
						// write buffer

						++iNumWriteBuffers;

						// layout binding, type, count and shader stage
						VkDescriptorSetLayoutBinding layoutBinding = {};
						layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						layoutBinding.descriptorCount = 1;
						layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
						
						// set binding and layout set
						switch(shaderResource.mShaderType)
						{
							case RenderDriver::Common::ShaderType::Vertex:
								//layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aVertexShaderLayoutBindings.size());
								aVertexShaderLayoutBindings.push_back(layoutBinding);
								iLayoutSet = 0;
								break;
							case RenderDriver::Common::ShaderType::Fragment:
							{
								iLayoutSet = 1;
								//layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aFragmentShaderLayoutBindings.size());
								aFragmentShaderLayoutBindings.push_back(layoutBinding);
								
								break;
							}
							case RenderDriver::Common::ShaderType::Compute:
								layoutBinding.binding = static_cast<uint32_t>(aComputeShaderLayoutBindings.size());
								//layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
								aComputeShaderLayoutBindings.push_back(layoutBinding);
								shaderType = RenderDriver::Common::ShaderType::Compute;
								iLayoutSet = 0;
								
								break;

							default:
								layoutBinding.binding = static_cast<uint32_t>(aComputeShaderLayoutBindings.size());
								aComputeShaderLayoutBindings.push_back(layoutBinding);
								iLayoutSet = 0;
								break;
						}

						if(shaderResource.mExternalResource.mpBuffer)
						{
							// valid buffer

							VkDescriptorBufferInfo bufferInfo = {};
							bufferInfo.buffer = *(static_cast<VkBuffer*>(shaderResource.mExternalResource.mpBuffer->getNativeBuffer()));
							bufferInfo.offset = shaderResource.mExternalResource.miGPUAddressOffset;
							bufferInfo.range = shaderResource.mDesc.miWidth;

							DescriptorBufferInfo descriptorBufferInfo = {};
							descriptorBufferInfo.miShaderResourceIndex = iShaderIndex;
							descriptorBufferInfo.mDescriptorBufferInfo = bufferInfo;
							descriptorBufferInfo.mResourceType = shaderResource.mType;
							descriptorBufferInfo.miSetIndex = iLayoutSet;
							descriptorBufferInfo.miBindingIndex = layoutBinding.binding;

							aDescriptorBufferInfo.push_back(descriptorBufferInfo);
						}
						else
						{
							// filler descriptor 

							DescriptorBufferInfo descriptorBufferInfo = {};
							VkDescriptorBufferInfo bufferInfo = {};
							bufferInfo.offset = 0;
							bufferInfo.range = VK_WHOLE_SIZE;
							descriptorBufferInfo.miShaderResourceIndex = iShaderIndex;
							descriptorBufferInfo.mDescriptorBufferInfo = bufferInfo;
							descriptorBufferInfo.mResourceType = shaderResource.mType;
							descriptorBufferInfo.miSetIndex = iLayoutSet;
							descriptorBufferInfo.miBindingIndex = layoutBinding.binding;

							aDescriptorBufferInfo.push_back(descriptorBufferInfo);
						}

						aDescriptorTypes.push_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

						maLayout[iShaderIndex].miSetIndex = iLayoutSet;
						maLayout[iShaderIndex].miBindingIndex = layoutBinding.binding;
						maLayout[iShaderIndex].miShaderResourceIndex = iShaderIndex;
						maLayout[iShaderIndex].mType = shaderResource.mType;

						DEBUG_PRINTF("\tset %d binding %d \"%s\" write buffer, descriptor type: %s\n",
							iLayoutSet,
							layoutBinding.binding,
							shaderResource.mName.c_str(),
							(layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ? "\"uniform buffer\"" : "\"storage buffer\"");
					}
					else
					{
						// read-only buffer

						++iNumReadBuffers;

						// binding, buffer >= 65536 size is storage, otherwise uniform
						VkDescriptorSetLayoutBinding layoutBinding = {};
						layoutBinding.descriptorType = (shaderResource.mDesc.miWidth >= 65536) ?
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : 
							VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						layoutBinding.descriptorCount = 1;
						layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
						
						// binding index and layout set 
						switch(shaderResource.mShaderType)
						{
							case RenderDriver::Common::ShaderType::Vertex:
								//layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aVertexShaderLayoutBindings.size());
								aVertexShaderLayoutBindings.push_back(layoutBinding);
								iLayoutSet = 0;
								break;
							case RenderDriver::Common::ShaderType::Fragment:
							{
								iLayoutSet = 1;
								//layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
								layoutBinding.binding = static_cast<uint32_t>(aFragmentShaderLayoutBindings.size());
								aFragmentShaderLayoutBindings.push_back(layoutBinding);
								
								break;
							}
							case RenderDriver::Common::ShaderType::Compute:
								layoutBinding.binding = static_cast<uint32_t>(aComputeShaderLayoutBindings.size());
								//layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
								aComputeShaderLayoutBindings.push_back(layoutBinding);
								shaderType = RenderDriver::Common::ShaderType::Compute;
								iLayoutSet = 0;
								
								break;
						}

						if(shaderResource.mExternalResource.mpBuffer)
						{
							// valid buffer

							VkDescriptorBufferInfo bufferInfo = {};
							bufferInfo.buffer = *(static_cast<VkBuffer*>(shaderResource.mExternalResource.mpBuffer->getNativeBuffer()));
							bufferInfo.offset = shaderResource.mExternalResource.miGPUAddressOffset;
							bufferInfo.range = shaderResource.mDesc.miWidth;

							DescriptorBufferInfo descriptorBufferInfo = {};
							descriptorBufferInfo.miShaderResourceIndex = iShaderIndex;
							descriptorBufferInfo.mDescriptorBufferInfo = bufferInfo;
							descriptorBufferInfo.mResourceType = shaderResource.mType;
							descriptorBufferInfo.miSetIndex = iLayoutSet;
							descriptorBufferInfo.miBindingIndex = layoutBinding.binding;

							aDescriptorBufferInfo.push_back(descriptorBufferInfo);
						}
						else
						{
							// filler buffer

							DescriptorBufferInfo descriptorBufferInfo = {};
							VkDescriptorBufferInfo bufferInfo = {};
							bufferInfo.offset = 0;
							bufferInfo.range = VK_WHOLE_SIZE;
							descriptorBufferInfo.miShaderResourceIndex = iShaderIndex;
							descriptorBufferInfo.mDescriptorBufferInfo = bufferInfo;
							descriptorBufferInfo.mResourceType = shaderResource.mType;
							descriptorBufferInfo.miSetIndex = iLayoutSet;
							descriptorBufferInfo.miBindingIndex = layoutBinding.binding;

							aDescriptorBufferInfo.push_back(descriptorBufferInfo);
						}

						aDescriptorTypes.push_back(layoutBinding.descriptorType);

						maLayout[iShaderIndex].miSetIndex = iLayoutSet;
						maLayout[iShaderIndex].miBindingIndex = layoutBinding.binding;
						maLayout[iShaderIndex].miShaderResourceIndex = iShaderIndex;
						maLayout[iShaderIndex].mType = shaderResource.mType;

						DEBUG_PRINTF("\tset %d binding %d \"%s\" read buffer, descriptor type: %s\n",
							iLayoutSet,
							layoutBinding.binding,
							shaderResource.mName.c_str(),
							(layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ? "\"uniform buffer\"" : "\"storage buffer\"");
					}
				}
				else
				{
					WTFASSERT(0, "shader resource not handled");
				}

				++iShaderIndex;
			}

			// add samplers at the end of the set if it's a graphics pipeline
			if((iNumWriteTextures > 0 || iNumReadTextures > 0) && shaderType != RenderDriver::Common::ShaderType::Compute)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.binding = static_cast<uint32_t>(aFragmentShaderLayoutBindings.size());
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
				aFragmentShaderLayoutBindings.push_back(layoutBinding);

				DEBUG_PRINTF("\tset 1 binding %d sampler\n",
					layoutBinding.binding);

				layoutBinding.binding = static_cast<uint32_t>(aFragmentShaderLayoutBindings.size());
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
				aFragmentShaderLayoutBindings.push_back(layoutBinding);

				DEBUG_PRINTF("\tset 1 binding %d sampler\n",
					layoutBinding.binding);
			}

			miNumLayoutSets = (shaderType != RenderDriver::Common::ShaderType::Compute) ? 2 : 1;
			maNativeLayoutSets.resize(miNumLayoutSets);

			// extensions for layout
			VkDescriptorBindingFlags bindingFlags = 
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
				VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

			VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo = {};
			extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			extendedInfo.bindingCount = 1u;
			extendedInfo.pBindingFlags = &bindingFlags;

			// create sets of layouts
			for(uint32_t iSet = 0; iSet < miNumLayoutSets; iSet++)
			{
				VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCreateInfo.pNext = &extendedInfo;
				if(iSet == 0)
				{
					if(miNumLayoutSets > 1)
					{
						layoutCreateInfo.bindingCount = static_cast<uint32_t>(aVertexShaderLayoutBindings.size());
						layoutCreateInfo.pBindings = aVertexShaderLayoutBindings.data();
					}
					else
					{
						layoutCreateInfo.bindingCount = static_cast<uint32_t>(aComputeShaderLayoutBindings.size());
						layoutCreateInfo.pBindings = aComputeShaderLayoutBindings.data();
					}
				}
				else
				{
					layoutCreateInfo.bindingCount = static_cast<uint32_t>(aFragmentShaderLayoutBindings.size());
					layoutCreateInfo.pBindings = aFragmentShaderLayoutBindings.data();
				}
				
				layoutCreateInfo.pNext = nullptr;
				VkResult ret = vkCreateDescriptorSetLayout(
					*mpNativeDevice, 
					&layoutCreateInfo, 
					nullptr, 
					&maNativeLayoutSets[iSet]);
				WTFASSERT(ret == VK_SUCCESS, "Error creating descriptor set layout: %d", ret);
			}

			// pool sizes for textures and buffers
			std::vector<VkDescriptorPoolSize> aPoolSizes;
			if(iNumWriteTextures > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				poolSize.descriptorCount = iNumWriteTextures;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumReadTextures > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				poolSize.descriptorCount = iNumReadTextures;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumWriteBuffers > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				poolSize.descriptorCount = iNumWriteBuffers;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumReadBuffers > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolSize.descriptorCount = iNumReadBuffers;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumReadTextures > 0 || iNumWriteTextures > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
				poolSize.descriptorCount = 2;
				aPoolSizes.push_back(poolSize);
			}

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(aPoolSizes.size());
			poolInfo.pPoolSizes = aPoolSizes.data();
			poolInfo.maxSets = 3;
			VkResult ret = vkCreateDescriptorPool(*mpNativeDevice, &poolInfo, nullptr, &mNativeDescriptorPool);
			WTFASSERT(ret == VK_SUCCESS, "Error creating descriptor pool: %d", ret);

			maNativeDescriptorSets.resize(miNumLayoutSets);
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = mNativeDescriptorPool;
			allocInfo.descriptorSetCount = miNumLayoutSets;
			allocInfo.pSetLayouts = maNativeLayoutSets.data();
			ret = vkAllocateDescriptorSets(*mpNativeDevice, &allocInfo, maNativeDescriptorSets.data());
			WTFASSERT(ret == VK_SUCCESS, "Error creating descriptor set: %d", ret);

			// update buffer descriptors and fill descriptors, textures will be updated later on
			for(uint32_t i = 0; i < static_cast<uint32_t>(aDescriptorBufferInfo.size()); i++)
			{
				uint32_t iShaderIndex = aDescriptorBufferInfo[i].miShaderResourceIndex;
				auto const& shaderResource = aShaderResources[iShaderIndex];
				uint32_t iSetIndex = aDescriptorBufferInfo[i].miSetIndex;
				uint32_t iBindingIndex = aDescriptorBufferInfo[i].miBindingIndex;

				bool bFillerResource = (
					aShaderResources[iShaderIndex].mName.find("fillerShaderResource") != std::string::npos || 
					aShaderResources[iShaderIndex].mName.find("fillerUnorderedAccessResource") != std::string::npos);

				VkWriteDescriptorSet descriptorWrite = {};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = maNativeDescriptorSets[iSetIndex];
				descriptorWrite.dstBinding = iBindingIndex;
				descriptorWrite.dstArrayElement = 0;
				descriptorWrite.descriptorType = aDescriptorTypes[iShaderIndex];
				descriptorWrite.descriptorCount = 1;
				descriptorWrite.pBufferInfo = &aDescriptorBufferInfo[i].mDescriptorBufferInfo;
				
				bool bTexture = (aDescriptorBufferInfo[i].mResourceType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
					aDescriptorBufferInfo[i].mResourceType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT ||
					aDescriptorBufferInfo[i].mResourceType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT);

				//if(descriptorWrite.pBufferInfo->buffer != nullptr)
				if(!bTexture)
				{
					vkUpdateDescriptorSets(
						*mpNativeDevice,
						1,
						&descriptorWrite,
						0,
						nullptr);
				}
				else
				{
					// update filler with null descriptor
					if(bFillerResource)
					{
						descriptorWrite.pBufferInfo = nullptr;

						VkDescriptorImageInfo fillerImageInfo = {};
						fillerImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						fillerImageInfo.imageView = VK_NULL_HANDLE;
						fillerImageInfo.sampler = VK_NULL_HANDLE;

						descriptorWrite.descriptorType = aDescriptorTypes[iShaderIndex];
						descriptorWrite.pImageInfo = &fillerImageInfo;

						vkUpdateDescriptorSets(
							*mpNativeDevice,
							1,
							&descriptorWrite,
							0,
							nullptr);
					}
				}
			}

			return mHandle;
		}

		/*
		**
		*/
		void CDescriptorSet::setID(std::string const& id)
		{
			RenderDriver::Common::CObject::setID(id);

			for(uint32_t i = 0; i < static_cast<uint32_t>(maNativeDescriptorSets.size()); i++)
			{
				VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
				objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
				objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maNativeDescriptorSets[i]);
				objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
				objectNameInfo.pObjectName = id.c_str();

				PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
				setDebugUtilsObjectNameEXT(
					*mpNativeDevice,
					&objectNameInfo);

				std::string layoutName = id + " Layout";
				objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maNativeLayoutSets[i]);
				objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
				objectNameInfo.pObjectName = layoutName.c_str();
				setDebugUtilsObjectNameEXT(
					*mpNativeDevice,
					&objectNameInfo);

				if(i == 0)
				{
					std::string poolName = id + " Pool";
					objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeDescriptorPool);
					objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
					objectNameInfo.pObjectName = poolName.c_str();
					setDebugUtilsObjectNameEXT(
						*mpNativeDevice,
						&objectNameInfo);
				}
			}


		}

		/*
		**
		*/
		void CDescriptorSet::setImageLayout(
			uint32_t iShaderResourceIndex,
			RenderDriver::Common::ImageLayout const& imageLayout)
		{
			maImageLayouts[iShaderResourceIndex] = imageLayout;
		}

		/*
		**
		*/
		void* CDescriptorSet::getNativeDescriptorSet()
		{
			return maNativeLayoutSets.data();
		}

		/*
		**
		*/
		void CDescriptorSet::addImage(
			RenderDriver::Common::CImage* pImage,
			RenderDriver::Common::CImageView* pImageView,
			uint32_t iBindingIndex,
			uint32_t iGroup,
			bool bReadOnly)
		{
			if(maaLayoutBindings.size() <= iGroup)
			{
				maaLayoutBindings.resize(iGroup + 1);
			}

			if(maaLayoutBindings[iGroup].size() <= iBindingIndex)
			{
				maaLayoutBindings[iGroup].resize(iBindingIndex + 1);
			}

			maaLayoutBindings[iGroup][iBindingIndex].descriptorType = (bReadOnly ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			maaLayoutBindings[iGroup][iBindingIndex].descriptorCount = 1;
			maaLayoutBindings[iGroup][iBindingIndex].stageFlags = 
				VK_SHADER_STAGE_VERTEX_BIT | 
				VK_SHADER_STAGE_FRAGMENT_BIT | 
				VK_SHADER_STAGE_COMPUTE_BIT | 
				VK_SHADER_STAGE_RAYGEN_BIT_KHR | 
				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | 
				VK_SHADER_STAGE_MISS_BIT_KHR;
			maaLayoutBindings[iGroup][iBindingIndex].binding = iBindingIndex;

			// filler, need this to keep track of the valid buffers
			if(maapBuffers.size() <= iGroup)
			{
				maapBuffers.resize(iGroup + 1);
			}
			if(maapBuffers[iGroup].size() <= iBindingIndex)
			{
				maapBuffers[iGroup].resize(iBindingIndex + 1);
			}
			maapBuffers[iGroup][iBindingIndex] = nullptr;

			if(maapImages.size() <= iGroup)
			{
				maapImages.resize(iGroup + 1);
			}
			if(maapImages[iGroup].size() <= iBindingIndex)
			{
				maapImages[iGroup].resize(iBindingIndex + 1);
			}
			maapImages[iGroup][iBindingIndex] = pImage;

			if(maapImageViews.size() <= iGroup)
			{
				maapImageViews.resize(iGroup + 1);
			}
			if(maapImageViews[iGroup].size() <= iBindingIndex)
			{
				maapImageViews[iGroup].resize(iBindingIndex + 1);
			}
			maapImageViews[iGroup][iBindingIndex] = pImageView;

			if(maapAccelerationStructures.size() <= iGroup)
			{
				maapAccelerationStructures.resize(iGroup + 1);
			}
			if(maapAccelerationStructures[iGroup].size() <= iBindingIndex)
			{
				maapAccelerationStructures[iGroup].resize(iBindingIndex + 1);
			}
			maapAccelerationStructures[iGroup][iBindingIndex] = nullptr;

		}

		/*
		**
		*/
		void CDescriptorSet::addBuffer(
			RenderDriver::Common::CBuffer* pBuffer,
			uint32_t iBindingIndex,
			uint32_t iGroup, 
			bool bReadOnly)
		{
			if(maaLayoutBindings.size() <= iGroup)
			{
				maaLayoutBindings.resize(iGroup + 1);
			}

			if(maaLayoutBindings[iGroup].size() <= iBindingIndex)
			{
				maaLayoutBindings[iGroup].resize(iBindingIndex + 1);
			}

			maaLayoutBindings[iGroup][iBindingIndex].descriptorType = (bReadOnly ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			maaLayoutBindings[iGroup][iBindingIndex].descriptorCount = 1;
			maaLayoutBindings[iGroup][iBindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			maaLayoutBindings[iGroup][iBindingIndex].binding = iBindingIndex;

			if(maapBuffers.size() <= iGroup)
			{
				maapBuffers.resize(iGroup + 1);
			}
			if(maapBuffers[iGroup].size() <= iBindingIndex)
			{
				maapBuffers[iGroup].resize(iBindingIndex + 1);
			}
			maapBuffers[iGroup][iBindingIndex] = pBuffer;

			if(maapImages.size() <= iGroup)
			{
				maapImages.resize(iGroup + 1);
			}
			if(maapImages[iGroup].size() <= iBindingIndex)
			{
				maapImages[iGroup].resize(iBindingIndex + 1);
			}
			maapImages[iGroup][iBindingIndex] = nullptr;

			if(maapImageViews.size() <= iGroup)
			{
				maapImageViews.resize(iGroup + 1);
			}
			if(maapImageViews[iGroup].size() <= iBindingIndex)
			{
				maapImageViews[iGroup].resize(iBindingIndex + 1);
			}
			maapImageViews[iGroup][iBindingIndex] = nullptr;

			if(maapAccelerationStructures.size() <= iGroup)
			{
				maapAccelerationStructures.resize(iGroup + 1);
			}
			if(maapAccelerationStructures[iGroup].size() <= iBindingIndex)
			{
				maapAccelerationStructures[iGroup].resize(iBindingIndex + 1);
			}
			maapAccelerationStructures[iGroup][iBindingIndex] = nullptr;
		}

		/*
		**
		*/
		void CDescriptorSet::addAccelerationStructure(
			RenderDriver::Common::CAccelerationStructure* pAccelerationStructure,
			uint32_t iBindingIndex,
			uint32_t iGroup)
		{
			if(maaLayoutBindings.size() <= iGroup)
			{
				maaLayoutBindings.resize(iGroup + 1);
			}

			if(maaLayoutBindings[iGroup].size() <= iBindingIndex)
			{
				maaLayoutBindings[iGroup].resize(iBindingIndex + 1);
			}

			maaLayoutBindings[iGroup][iBindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			maaLayoutBindings[iGroup][iBindingIndex].descriptorCount = 1;
			maaLayoutBindings[iGroup][iBindingIndex].stageFlags = 
				VK_SHADER_STAGE_VERTEX_BIT | 
				VK_SHADER_STAGE_FRAGMENT_BIT | 
				VK_SHADER_STAGE_COMPUTE_BIT | 
				VK_SHADER_STAGE_RAYGEN_BIT_KHR | 
				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			maaLayoutBindings[iGroup][iBindingIndex].binding = iBindingIndex;

			if(maapBuffers.size() <= iGroup)
			{
				maapBuffers.resize(iGroup + 1);
			}
			if(maapBuffers[iGroup].size() <= iBindingIndex)
			{
				maapBuffers[iGroup].resize(iBindingIndex + 1);
			}
			maapBuffers[iGroup][iBindingIndex] = nullptr;

			if(maapImages.size() <= iGroup)
			{
				maapImages.resize(iGroup + 1);
			}
			if(maapImages[iGroup].size() <= iBindingIndex)
			{
				maapImages[iGroup].resize(iBindingIndex + 1);
			}
			maapImages[iGroup][iBindingIndex] = nullptr;

			if(maapImageViews.size() <= iGroup)
			{
				maapImageViews.resize(iGroup + 1);
			}
			if(maapImageViews[iGroup].size() <= iBindingIndex)
			{
				maapImageViews[iGroup].resize(iBindingIndex + 1);
			}
			maapImageViews[iGroup][iBindingIndex] = nullptr;

			if(maapAccelerationStructures.size() <= iGroup)
			{
				maapAccelerationStructures.resize(iGroup + 1);
			}
			if(maapAccelerationStructures[iGroup].size() <= iBindingIndex)
			{
				maapAccelerationStructures[iGroup].resize(iBindingIndex + 1);
			}
			maapAccelerationStructures[iGroup][iBindingIndex] = pAccelerationStructure;
		}

		/*
		**
		*/
		void CDescriptorSet::finishLayout(
			void* aSamplers
		)
		{
			miNumLayoutSets = (uint32_t)maaLayoutBindings.size();

			// get the number of different images and buffers for descriptor pool
			uint32_t iNumWriteTextures = 0, iNumReadTextures = 0, iNumWriteBuffers = 0, iNumReadBuffers = 0, iNumAccelerationStructures = 0;
			for(uint32_t i = 0; i < (uint32_t)maaLayoutBindings.size(); i++)
			{
				for(uint32_t j = 0; j < (uint32_t)maaLayoutBindings[i].size(); j++)
				{
					if(maaLayoutBindings[i][j].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					{
						++iNumWriteTextures;
					}
					else if(maaLayoutBindings[i][j].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
					{
						++iNumReadTextures;
					}
					else if(maaLayoutBindings[i][j].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
					{
						++iNumWriteBuffers;
					}
					else if(maaLayoutBindings[i][j].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
					{
						++iNumReadBuffers;
					}
					else if(maaLayoutBindings[i][j].descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
					{
						++iNumAccelerationStructures;
					}
				}
			}

			// add samplers at the end of the set if it's a graphics pipeline
			if(iNumWriteTextures > 0 || iNumReadTextures > 0)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.binding = static_cast<uint32_t>(maaLayoutBindings[0].size());
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = (mDesc.mPipelineType == PipelineType::COMPUTE_PIPELINE_TYPE) ? 
					VK_SHADER_STAGE_COMPUTE_BIT : 
					VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
				maaLayoutBindings[0].push_back(layoutBinding);

				DEBUG_PRINTF("\tset 0 binding %d sampler\n",
					layoutBinding.binding);

				layoutBinding.binding = static_cast<uint32_t>(maaLayoutBindings[0].size());
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = (mDesc.mPipelineType == PipelineType::COMPUTE_PIPELINE_TYPE) ? 
					VK_SHADER_STAGE_COMPUTE_BIT : 
					VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
				maaLayoutBindings[0].push_back(layoutBinding);

				DEBUG_PRINTF("\tset 0 binding %d sampler\n",
					layoutBinding.binding);
			}

			// extensions for layout
			VkDescriptorBindingFlags bindingFlags =
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
				VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

			VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo = {};
			extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			extendedInfo.bindingCount = 1u;
			extendedInfo.pBindingFlags = &bindingFlags;

			// create sets of layouts
			uint32_t iNumBindingGroups = (uint32_t)maaLayoutBindings.size();
			maNativeLayoutSets.resize(iNumBindingGroups);
			for(uint32_t iGroup = 0; iGroup < iNumBindingGroups; iGroup++)
			{
				VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCreateInfo.pNext = &extendedInfo;

				layoutCreateInfo.bindingCount = (uint32_t)maaLayoutBindings[iGroup].size();
				layoutCreateInfo.pBindings = maaLayoutBindings[iGroup].data();

				layoutCreateInfo.pNext = nullptr;
				VkResult ret = vkCreateDescriptorSetLayout(
					*mpNativeDevice,
					&layoutCreateInfo,
					nullptr,
					&maNativeLayoutSets[iGroup]);
				WTFASSERT(ret == VK_SUCCESS, "Error creating descriptor set layout: %d", ret);
			}

			// pool sizes for textures and buffers
			std::vector<VkDescriptorPoolSize> aPoolSizes;
			if(iNumWriteTextures > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				poolSize.descriptorCount = iNumWriteTextures;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumReadTextures > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				poolSize.descriptorCount = iNumReadTextures;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumWriteBuffers > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				poolSize.descriptorCount = iNumWriteBuffers;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumReadBuffers > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolSize.descriptorCount = iNumReadBuffers;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumReadTextures > 0 || iNumWriteTextures > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
				poolSize.descriptorCount = 2;
				aPoolSizes.push_back(poolSize);
			}

			if(iNumAccelerationStructures > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				poolSize.descriptorCount = iNumAccelerationStructures;
				aPoolSizes.push_back(poolSize);
			}

			// descriptor pool
			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(aPoolSizes.size());
			poolInfo.pPoolSizes = aPoolSizes.data();
			poolInfo.maxSets = 3;
			VkResult ret = vkCreateDescriptorPool(*mpNativeDevice, &poolInfo, nullptr, &mNativeDescriptorPool);
			WTFASSERT(ret == VK_SUCCESS, "Error creating descriptor pool: %d", ret);

			// descriptor set
			maNativeDescriptorSets.resize(iNumBindingGroups);
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = mNativeDescriptorPool;
			allocInfo.descriptorSetCount = miNumLayoutSets;
			allocInfo.pSetLayouts = maNativeLayoutSets.data();

			//VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo = {};
			//if(iNumAccelerationStructures > 0)
			//{
			//	uint32_t variableDescCounts[] = { 1 };
			//	variableDescriptorCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
			//	variableDescriptorCountAllocInfo.descriptorSetCount = miNumLayoutSets;
			//	variableDescriptorCountAllocInfo.pDescriptorCounts = variableDescCounts;
			//
			//	allocInfo.pNext = &variableDescriptorCountAllocInfo;
			//}
			
			ret = vkAllocateDescriptorSets(*mpNativeDevice, &allocInfo, maNativeDescriptorSets.data());
			WTFASSERT(ret == VK_SUCCESS, "Error creating descriptor set: %d", ret);

			// update image and buffer descriptors
			for(uint32_t i = 0; i < (uint32_t)maapBuffers.size(); i++)
			{
				for(uint32_t j = 0; j < (uint32_t)maapBuffers[i].size(); j++)
				{
					VkDescriptorBufferInfo bufferInfo;
					if(maapBuffers[i][j] != nullptr)
					{
						bufferInfo.buffer = *((VkBuffer *)maapBuffers[i][j]->getNativeBuffer());
						bufferInfo.offset = 0;
						bufferInfo.range = VK_WHOLE_SIZE;
					}

					VkDescriptorImageInfo imageInfo;
					if(maapImageViews[i][j] != nullptr)
					{
						imageInfo.imageView = ((RenderDriver::Vulkan::CImageView*)maapImageViews[i][j])->getNativeImageView();;
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						//imageInfo.sampler = *((VkSampler*)aSamplers);
					}
					
					VkWriteDescriptorSet descriptorWrite = {};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = maNativeDescriptorSets[i];
					descriptorWrite.dstBinding = j;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = maaLayoutBindings[i][j].descriptorType;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pBufferInfo = (maapBuffers[i][j] != nullptr) ? &bufferInfo : VK_NULL_HANDLE;
					descriptorWrite.pImageInfo = (maapImageViews[i][j] != nullptr) ?  &imageInfo : VK_NULL_HANDLE;

					if(descriptorWrite.pBufferInfo != nullptr || descriptorWrite.pImageInfo != nullptr)
					{
						vkUpdateDescriptorSets(
							*mpNativeDevice,
							1,
							&descriptorWrite,
							0,
							nullptr);
					}
					else
					{
						if(maapAccelerationStructures[i][j] != nullptr)
						{
							VkAccelerationStructureKHR* pTopLevelAccelerationStructure = (VkAccelerationStructureKHR*)((RenderDriver::Vulkan::CAccelerationStructure*)maapAccelerationStructures[i][j])->getNativeAccelerationStructure();

							VkAccelerationStructureKHR aAccelerationStructures[1] = {*pTopLevelAccelerationStructure};
							VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
							descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
							descriptorAccelerationStructureInfo.pNext = nullptr;
							descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
							descriptorAccelerationStructureInfo.pAccelerationStructures = aAccelerationStructures;

							VkWriteDescriptorSet descriptorWrite = {};
							descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							// The specialized acceleration structure descriptor has to be chained
							descriptorWrite.pNext = &descriptorAccelerationStructureInfo;
							descriptorWrite.dstSet = maNativeDescriptorSets[i];
							descriptorWrite.dstBinding = j;
							descriptorWrite.descriptorCount = 1;
							descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

							vkUpdateDescriptorSets(
								*mpNativeDevice,
								1,
								&descriptorWrite,
								0,
								nullptr);
						}
						else
						{
							WTFASSERT(0, "Should not be here");
						}
					}
				}
			}

			// update samplers
			if(iNumReadTextures > 0)
			{
				VkDescriptorImageInfo samplerInfo = {};
				samplerInfo.sampler = ((VkSampler*)aSamplers)[0];

				VkWriteDescriptorSet sampleDescriptorWrite = {};
				sampleDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				sampleDescriptorWrite.dstSet = maNativeDescriptorSets[0];
				sampleDescriptorWrite.dstBinding = (uint32_t)maapBuffers[0].size();
				sampleDescriptorWrite.dstArrayElement = 0;
				sampleDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				sampleDescriptorWrite.descriptorCount = 1;
				sampleDescriptorWrite.pImageInfo = &samplerInfo;
				vkUpdateDescriptorSets(
					*mpNativeDevice,
					1,
					&sampleDescriptorWrite,
					0,
					nullptr
				);

				samplerInfo.sampler = ((VkSampler*)aSamplers)[1];
				sampleDescriptorWrite.dstBinding = (uint32_t)maapBuffers[0].size() + 1;
				vkUpdateDescriptorSets(
					*mpNativeDevice,
					1,
					&sampleDescriptorWrite,
					0,
					nullptr
				);
			}
		}

	}   // Vulkan

}   // RenderDriver