#include <render-driver/Vulkan/PipelineStateVulkan.h>
#include <render-driver/Vulkan/DeviceVulkan.h>

#include <serialize_utils.h>

#include <wtfassert.h>

namespace RenderDriver
{
	namespace Vulkan
	{
		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE CPipelineState::create(
			RenderDriver::Common::GraphicsPipelineStateDescriptor const& desc,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Vulkan::CPipelineState::GraphicsPipelineStateDescriptor const& vulkanPipelineDesc =
				static_cast<RenderDriver::Vulkan::CPipelineState::GraphicsPipelineStateDescriptor const&>(desc);

			RenderDriver::Common::CPipelineState::create(desc, device);

			RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
			mpNativeDevice = static_cast<VkDevice*>(deviceVulkan.getNativeDevice());

			// vertex shader
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = desc.miVertexShaderSize;
			createInfo.pCode = reinterpret_cast<const uint32_t*>(desc.mpVertexShader);
			VkResult ret = vkCreateShaderModule(*mpNativeDevice, &createInfo, nullptr, &mVertexNativeShaderModule);
			WTFASSERT(ret == VK_SUCCESS, "Error creating vertex shader: %d", ret);

			// fragment shader
			createInfo.codeSize = desc.miPixelShaderSize;
			createInfo.pCode = reinterpret_cast<const uint32_t*>(desc.mpPixelShader);
			ret = vkCreateShaderModule(*mpNativeDevice, &createInfo, nullptr, &mFragmentNativeShaderModule);
			WTFASSERT(ret == VK_SUCCESS, "Error creating vertex shader: %d", ret);

			// vertex stage
			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = mVertexNativeShaderModule;
			vertShaderStageInfo.pName = "VSMain";

			// fragment stage
			VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
			fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentShaderStageInfo.module = mFragmentNativeShaderModule;
			fragmentShaderStageInfo.pName = "PSMain";

			std::vector<VkPipelineShaderStageCreateInfo> aPipelineStages(2);
			aPipelineStages[0] = vertShaderStageInfo;
			aPipelineStages[1] = fragmentShaderStageInfo;

			// dynamic states
			std::vector<VkDynamicState> dynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			// input assembly
			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			// viewport
			//WTFASSERT(vulkanPipelineDesc.miImageWidth > 0 && vulkanPipelineDesc.miImageHeight > 0, "uninitialized image dimension: (%d, %d)",
			//	vulkanPipelineDesc.miImageWidth,
			//	vulkanPipelineDesc.miImageHeight);

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)vulkanPipelineDesc.miImageWidth;
			viewport.height = (float)vulkanPipelineDesc.miImageHeight;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkExtent2D extent = { vulkanPipelineDesc.miImageWidth , vulkanPipelineDesc.miImageHeight };
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = extent;
			
			// viewport, set to (0, 0), actual viewport extent will be set later on in draw function
			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			// rasterizer
			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = (desc.mRasterState.mfDepthBiasClamp > 0.0f) ? VK_TRUE : VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = SerializeUtils::Vulkan::convert(desc.mRasterState.mFillMode);
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = SerializeUtils::Vulkan::convert(desc.mRasterState.mCullMode);
			rasterizer.frontFace = (desc.mRasterState.mbFrontCounterClockwise) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = (desc.mRasterState.miDepthBias > 0) ? VK_TRUE : VK_FALSE;
			rasterizer.depthBiasConstantFactor = static_cast<float>(desc.mRasterState.miDepthBias); 
			rasterizer.depthBiasClamp = desc.mRasterState.mfDepthBiasClamp;
			rasterizer.depthBiasSlopeFactor = desc.mRasterState.mfSlopeScaledDepthBias;

			// multi-sampling
			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = (desc.mRasterState.mbMultisampleEnable) ? VK_TRUE : VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.minSampleShading = 1.0f; 
			multisampling.pSampleMask = nullptr; 
			multisampling.alphaToCoverageEnable = VK_FALSE;
			multisampling.alphaToOneEnable = VK_FALSE;

			// blend states 
			std::vector< VkPipelineColorBlendAttachmentState> aColorBlendAttachmentStates(desc.miNumRenderTarget);
			for(uint32_t iRenderTarget = 0; iRenderTarget < desc.miNumRenderTarget; iRenderTarget++)
			{
				RenderDriver::Common::BlendState const& blendState = desc.maRenderTargetBlendStates[iRenderTarget];
				if((uint32_t const&)blendState == UINT32_MAX)
				{
					continue;
				}

				aColorBlendAttachmentStates[iRenderTarget].colorWriteMask = SerializeUtils::Vulkan::convert(blendState.mWriteMask);
				aColorBlendAttachmentStates[iRenderTarget].blendEnable = (blendState.mbEnabled) ? VK_TRUE : VK_FALSE;
				aColorBlendAttachmentStates[iRenderTarget].srcColorBlendFactor = SerializeUtils::Vulkan::convert(blendState.mSrcColor);
				aColorBlendAttachmentStates[iRenderTarget].dstColorBlendFactor = SerializeUtils::Vulkan::convert(blendState.mDestColor);
				aColorBlendAttachmentStates[iRenderTarget].colorBlendOp = SerializeUtils::Vulkan::convert(blendState.mColorOp);
				aColorBlendAttachmentStates[iRenderTarget].srcAlphaBlendFactor = SerializeUtils::Vulkan::convert(blendState.mSrcAlpha);
				aColorBlendAttachmentStates[iRenderTarget].dstAlphaBlendFactor = SerializeUtils::Vulkan::convert(blendState.mDestAlpha);
				aColorBlendAttachmentStates[iRenderTarget].alphaBlendOp = SerializeUtils::Vulkan::convert(blendState.mAlphaOp);
			}

			// total blend states for render targets
			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = (desc.maRenderTargetBlendStates[0].mbLogicOpEnabled) ? VK_TRUE : VK_FALSE;
			colorBlending.logicOp = SerializeUtils::Vulkan::convert(desc.maRenderTargetBlendStates[0].mLogicOp);
			colorBlending.attachmentCount = desc.miNumRenderTarget;
			colorBlending.pAttachments = aColorBlendAttachmentStates.data();
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;

			// push constants
			VkPushConstantRange pushConstant;
			pushConstant.offset = 0;
			pushConstant.size = vulkanPipelineDesc.miNumRootConstants * sizeof(float);
			pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

			// pipeline layout
			RenderDriver::Vulkan::CDescriptorSet* pDescriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(vulkanPipelineDesc.mpDescriptor);
			std::vector<VkDescriptorSetLayout>& aDescriptorSetLayout = pDescriptorSetVulkan->getNativeDescriptorSetLayouts();
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = pDescriptorSetVulkan->getNumLayoutSets();
			pipelineLayoutInfo.pSetLayouts = aDescriptorSetLayout.data();
			pipelineLayoutInfo.pushConstantRangeCount = (pushConstant.size > 0) ? 1 : 0;
			pipelineLayoutInfo.pPushConstantRanges = (pushConstant.size > 0) ? &pushConstant : nullptr;

			ret = vkCreatePipelineLayout(
				*mpNativeDevice,
				&pipelineLayoutInfo, 
				nullptr, 
				&mNativePipelineLayout);
			WTFASSERT(ret == VK_SUCCESS, "Error creating pipeline layout: %d", ret);

			uint32_t iNumTotalAttachments = (desc.mDepthStencilState.mbDepthEnabled || desc.mbOutputPresent) ? desc.miNumRenderTarget + 1 : desc.miNumRenderTarget;

			VkImageLayout finalImageLayout = (desc.mbOutputPresent) ? 
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : 
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			
			// render pass
			// color attachments
			std::vector<VkAttachmentDescription> aColorAttachments(iNumTotalAttachments);
			for(uint32_t iRenderTarget = 0; iRenderTarget < desc.miNumRenderTarget; iRenderTarget++)
			{
				aColorAttachments[iRenderTarget].format = SerializeUtils::Vulkan::convert(desc.maRenderTargetFormats[iRenderTarget]);
				aColorAttachments[iRenderTarget].samples = VK_SAMPLE_COUNT_1_BIT;
				aColorAttachments[iRenderTarget].loadOp = SerializeUtils::Vulkan::convert(desc.maLoadOps[iRenderTarget]);
				aColorAttachments[iRenderTarget].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				aColorAttachments[iRenderTarget].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				aColorAttachments[iRenderTarget].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				aColorAttachments[iRenderTarget].initialLayout = (aColorAttachments[iRenderTarget].loadOp == VK_ATTACHMENT_LOAD_OP_LOAD) ? 
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : 
					VK_IMAGE_LAYOUT_UNDEFINED;
				aColorAttachments[iRenderTarget].finalLayout = finalImageLayout;
			}

			// depth attachments
			uint32_t iDepthAttachmentIndex = desc.miNumRenderTarget;
			if(desc.mDepthStencilState.mbDepthEnabled || desc.mbOutputPresent)
			{
				aColorAttachments[iDepthAttachmentIndex].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
				aColorAttachments[iDepthAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
				aColorAttachments[iDepthAttachmentIndex].loadOp = SerializeUtils::Vulkan::convert(desc.maLoadOps[iDepthAttachmentIndex]);
				aColorAttachments[iDepthAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				aColorAttachments[iDepthAttachmentIndex].stencilLoadOp = SerializeUtils::Vulkan::convert(desc.maLoadOps[iDepthAttachmentIndex]);
				aColorAttachments[iDepthAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				aColorAttachments[iDepthAttachmentIndex].initialLayout = (aColorAttachments[iDepthAttachmentIndex].stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD) ?
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
					VK_IMAGE_LAYOUT_UNDEFINED;
				aColorAttachments[iDepthAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			std::vector<VkAttachmentReference> aColorAttachmentRef(desc.miNumRenderTarget);
			for(uint32_t i = 0; i < desc.miNumRenderTarget; i++)
			{
				aColorAttachmentRef[i].attachment = i;
				aColorAttachmentRef[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			VkAttachmentReference depthAttachmentRef = {};
			if(desc.mDepthStencilState.mbDepthEnabled || desc.mbOutputPresent)
			{
				depthAttachmentRef.attachment = iDepthAttachmentIndex;
				depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = desc.miNumRenderTarget;
			subpass.pColorAttachments = aColorAttachmentRef.data();
			subpass.pDepthStencilAttachment = (desc.mDepthStencilState.mbDepthEnabled || desc.mbOutputPresent) ? &depthAttachmentRef : nullptr;

			VkSubpassDependency aSubPassDependencies[2] = {};
			aSubPassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			aSubPassDependencies[0].dstSubpass = 0;
			aSubPassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			aSubPassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			aSubPassDependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			aSubPassDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

			aSubPassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
			aSubPassDependencies[1].dstSubpass = 0;
			aSubPassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			aSubPassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			aSubPassDependencies[1].srcAccessMask = 0;
			aSubPassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = iNumTotalAttachments;
			renderPassInfo.pAttachments = aColorAttachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = sizeof(aSubPassDependencies) / sizeof(*aSubPassDependencies);
			renderPassInfo.pDependencies = aSubPassDependencies;
			ret = vkCreateRenderPass(
				*mpNativeDevice,
				&renderPassInfo, 
				nullptr, 
				&mNativeRenderPass);
			WTFASSERT(ret == VK_SUCCESS, "Error creating render pass: %d", ret);

			// depth stencil
			VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
			depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateInfo.pNext = nullptr;
			depthStencilStateInfo.flags = 0;
			depthStencilStateInfo.depthTestEnable = (desc.mDepthStencilState.mbDepthEnabled) ? VK_TRUE : VK_FALSE;
			depthStencilStateInfo.depthWriteEnable = (desc.mDepthStencilState.mDepthWriteMask == RenderDriver::Common::DepthWriteMask::All) ? VK_TRUE : VK_FALSE;
			depthStencilStateInfo.depthCompareOp = SerializeUtils::Vulkan::convert(desc.mDepthStencilState.mDepthFunc);
			depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
			depthStencilStateInfo.stencilTestEnable = (desc.mDepthStencilState.mbStencilEnabled) ? VK_TRUE : VK_FALSE;
			depthStencilStateInfo.front = SerializeUtils::Vulkan::convert(desc.mDepthStencilState.mFrontFace);
			depthStencilStateInfo.back = SerializeUtils::Vulkan::convert(desc.mDepthStencilState.mBackFace);
			depthStencilStateInfo.minDepthBounds = 0.0f;
			depthStencilStateInfo.maxDepthBounds = 1.0f;

			// vertex format
			uint32_t iOffset = 0;
			std::vector<VkVertexInputAttributeDescription> aInputElementDescs;
			for(uint32_t iMember = 0; iMember < desc.miNumVertexMembers; iMember++)
			{
				if((desc.mbFullTrianglePass || desc.mbOutputPresent) && iMember == 1)
				{
					iOffset += sizeof(float) * 4;

					VkVertexInputAttributeDescription inputElementDesc = {};
					inputElementDesc.binding = 0;
					inputElementDesc.format = SerializeUtils::Vulkan::convert(RenderDriver::Common::Format::R32G32B32A32_FLOAT);
					inputElementDesc.location = 1;
					inputElementDesc.offset = iOffset;
					aInputElementDescs.push_back(inputElementDesc);
					
					iOffset += sizeof(float) * 4;

					break;
				}

				VkVertexInputAttributeDescription inputElementDesc = SerializeUtils::Vulkan::convert(
					desc.maVertexFormats[iMember],
					iMember,
					iOffset);
				aInputElementDescs.push_back(inputElementDesc);
				iOffset += SerializeUtils::Common::getNumComponents(desc.maVertexFormats[iMember].mFormat) * sizeof(float);
			}
			
			// vertex input
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			bindingDescription.stride = iOffset;

			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(aInputElementDescs.size());
			vertexInputInfo.pVertexAttributeDescriptions = aInputElementDescs.data();

			// pipeline 
			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = static_cast<uint32_t>(aPipelineStages.size());
			pipelineInfo.pStages = aPipelineStages.data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = mNativePipelineLayout;
			pipelineInfo.renderPass = mNativeRenderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;
			pipelineInfo.flags = 0;

			ret = vkCreateGraphicsPipelines(
				*mpNativeDevice,
				VK_NULL_HANDLE, 
				1, 
				&pipelineInfo, 
				nullptr, 
				&mNativePipelineState);
			WTFASSERT(ret == VK_SUCCESS, "Error creating graphics pipeline: %d", ret);

			//vkDestroyShaderModule(*pNativeDevice, mVertexNativeShaderModule, nullptr);
			//vkDestroyShaderModeul(*pNativeDevice, mFragmentNativeShaderModule, nullptr);

			return mHandle;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE CPipelineState::create(
			RenderDriver::Common::ComputePipelineStateDescriptor const& desc,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Common::CPipelineState::create(desc, device);

			RenderDriver::Vulkan::CPipelineState::ComputePipelineStateDescriptor const& vulkanPipelineDesc =
				static_cast<RenderDriver::Vulkan::CPipelineState::ComputePipelineStateDescriptor const&>(desc);

			RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
			mpNativeDevice = static_cast<VkDevice*>(deviceVulkan.getNativeDevice());

			// compute shader
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = desc.miComputeShaderSize;
			createInfo.pCode = reinterpret_cast<const uint32_t*>(desc.mpComputeShader);
			VkResult ret = vkCreateShaderModule(*mpNativeDevice, &createInfo, nullptr, &mComputeNativeShaderModule);
			WTFASSERT(ret == VK_SUCCESS, "Error creating vertex shader: %d", ret);

			// compute stage
			VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
			computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			computeShaderStageInfo.module = mComputeNativeShaderModule;
			computeShaderStageInfo.pName = "CSMain";

			// push constants
			VkPushConstantRange pushConstant;
			pushConstant.offset = 0;
			pushConstant.size = vulkanPipelineDesc.miNumRootConstants * sizeof(float);
			pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

			// pipeline layout
			RenderDriver::Vulkan::CDescriptorSet* pDescriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(vulkanPipelineDesc.mpDescriptor);
			std::vector<VkDescriptorSetLayout>& aDescriptorSetLayout = pDescriptorSetVulkan->getNativeDescriptorSetLayouts();
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = pDescriptorSetVulkan->getNumLayoutSets();
			pipelineLayoutInfo.pSetLayouts = aDescriptorSetLayout.data();
			pipelineLayoutInfo.pushConstantRangeCount = (pushConstant.size > 0) ? 1 : 0;
			pipelineLayoutInfo.pPushConstantRanges = (pushConstant.size > 0) ? &pushConstant : nullptr;
			ret = vkCreatePipelineLayout(
				*mpNativeDevice,
				&pipelineLayoutInfo,
				nullptr,
				&mNativePipelineLayout);
			WTFASSERT(ret == VK_SUCCESS, "Error creating compute pipeline layout: %d", ret);

			// pipeline
			VkComputePipelineCreateInfo computePipelineCreateInfo = {};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = mNativePipelineLayout;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = computeShaderStageInfo;

			ret = vkCreateComputePipelines(
				*mpNativeDevice,
				VK_NULL_HANDLE,
				1,
				&computePipelineCreateInfo,
				nullptr,
				&mNativePipelineState);
			WTFASSERT(ret == VK_SUCCESS, "Error creating compute pipeline: %d", ret);

			return mHandle;
		}

		extern PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE CPipelineState::create(
			RenderDriver::Common::RayTracePipelineStateDescriptor const& desc,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Common::CPipelineState::create(desc, device);

			RenderDriver::Vulkan::CPipelineState::RayTracePipelineStateDescriptor const& vulkanPipelineDesc =
				static_cast<RenderDriver::Vulkan::CPipelineState::RayTracePipelineStateDescriptor const&>(desc);

			VkInstance& instance = *((VkInstance*)vulkanPipelineDesc.mpPlatformInstance);

			PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
				vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesKHR")
			);

			RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
			VkDevice& nativeDevice = *(static_cast<VkDevice*>(deviceVulkan.getNativeDevice()));

			std::vector<VkPipelineShaderStageCreateInfo> aShaderStages;
			std::vector<VkRayTracingShaderGroupCreateInfoKHR> aShaderGroups;

			// ray-gen
			VkPipelineShaderStageCreateInfo shaderStage = {};
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			VkShaderModuleCreateInfo moduleCreateInfo = {};
			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.codeSize = desc.miRayGenShaderSize;
			moduleCreateInfo.pCode = (uint32_t*)desc.mpRayGenShader;
			VkResult ret = vkCreateShaderModule(
				nativeDevice,
				&moduleCreateInfo,
				nullptr,
				&mRayGenShaderModule
			);
			WTFASSERT(ret == VK_SUCCESS, "Error %d creating ray gen shader module",
				ret
			);
			shaderStage.module = mRayGenShaderModule;
			shaderStage.pName = "rayGen";
			shaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			aShaderStages.push_back(shaderStage);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroup = {};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(aShaderStages.size()) - 1;
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			aShaderGroups.push_back(shaderGroup);

			// miss
			moduleCreateInfo.codeSize = desc.miMissShaderSize;
			moduleCreateInfo.pCode = (uint32_t*)desc.mpMissShader;
			ret = vkCreateShaderModule(
				nativeDevice,
				&moduleCreateInfo,
				nullptr,
				&mMissShaderModule
			);
			WTFASSERT(ret == VK_SUCCESS, "Error %d creating miss shader module",
				ret
			);
			shaderStage.module = mMissShaderModule;
			shaderStage.pName = "missShader";
			shaderStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			aShaderStages.push_back(shaderStage);

			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(aShaderStages.size()) - 1;
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			aShaderGroups.push_back(shaderGroup);

			// hit
			moduleCreateInfo.codeSize = desc.miHitShaderSize;
			moduleCreateInfo.pCode = (uint32_t*)desc.mpCloseHitShader;
			ret = vkCreateShaderModule(
				nativeDevice,
				&moduleCreateInfo,
				nullptr,
				&mClosestHitShaderModule
			);
			WTFASSERT(ret == VK_SUCCESS, "Error %d creating hit shader module",
				ret
			);
			shaderStage.module = mClosestHitShaderModule;
			shaderStage.pName = "hitTriangle";
			shaderStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			aShaderStages.push_back(shaderStage);

			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.closestHitShader = static_cast<uint32_t>(aShaderStages.size()) - 1;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			aShaderGroups.push_back(shaderGroup);

			// pipeline layout
			RenderDriver::Vulkan::CDescriptorSet* pDescriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(vulkanPipelineDesc.mpDescriptor);
			std::vector<VkDescriptorSetLayout>& aDescriptorSetLayout = pDescriptorSetVulkan->getNativeDescriptorSetLayouts();
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = pDescriptorSetVulkan->getNumLayoutSets();
			pipelineLayoutInfo.pSetLayouts = aDescriptorSetLayout.data();
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;
			ret = vkCreatePipelineLayout(
				nativeDevice,
				&pipelineLayoutInfo,
				nullptr,
				&mNativePipelineLayout);
			WTFASSERT(ret == VK_SUCCESS, "Error creating compute pipeline layout: %d", ret);

			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
			rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			rayTracingPipelineCI.stageCount = static_cast<uint32_t>(aShaderStages.size());
			rayTracingPipelineCI.pStages = aShaderStages.data();
			rayTracingPipelineCI.groupCount = static_cast<uint32_t>(aShaderGroups.size());
			rayTracingPipelineCI.pGroups = aShaderGroups.data();
			rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;
			rayTracingPipelineCI.layout = mNativePipelineLayout;
			rayTracingPipelineCI.pDynamicState = nullptr;
			ret = vkCreateRayTracingPipelinesKHR(
				nativeDevice,
				VK_NULL_HANDLE,
				VK_NULL_HANDLE,
				1,
				&rayTracingPipelineCI,
				nullptr,
				&mNativePipelineState);

			WTFASSERT(ret == VK_SUCCESS, "Error creating ray trace pipeline \"%s\"",
				mID.c_str()
			);

			return mHandle;
		}

		/*
		**
		*/
		void CPipelineState::setID(std::string const& id)
		{
			RenderDriver::Common::CObject::setID(id);

			VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
			objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativePipelineState);
			objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
			objectNameInfo.pObjectName = id.c_str();

			PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
			setDebugUtilsObjectNameEXT(
				*mpNativeDevice,
				&objectNameInfo);

			std::string layoutName = id + " Layout";
			objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativePipelineLayout);
			objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
			objectNameInfo.pObjectName = layoutName.c_str();
			setDebugUtilsObjectNameEXT(
				*mpNativeDevice,
				&objectNameInfo);

			if(mNativeRenderPass)
			{
				std::string renderPassName = id + " Render Pass";
				objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeRenderPass);
				objectNameInfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
				objectNameInfo.pObjectName = renderPassName.c_str();
				setDebugUtilsObjectNameEXT(
					*mpNativeDevice,
					&objectNameInfo);
			}
			
			if(mComputeNativeShaderModule)
			{
				std::string shaderName = id + " Compute Shader Module";
				objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mComputeNativeShaderModule);
				objectNameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
				objectNameInfo.pObjectName = shaderName.c_str();
				setDebugUtilsObjectNameEXT(
					*mpNativeDevice,
					&objectNameInfo);
			}

			if(mVertexNativeShaderModule)
			{
				std::string shaderName = id + " Vertex Shader Module";
				objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mVertexNativeShaderModule);
				objectNameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
				objectNameInfo.pObjectName = shaderName.c_str();
				setDebugUtilsObjectNameEXT(
					*mpNativeDevice,
					&objectNameInfo);
			}

			if(mFragmentNativeShaderModule)
			{
				std::string shaderName = id + " Fragment Shader Module";
				objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mFragmentNativeShaderModule);
				objectNameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
				objectNameInfo.pObjectName = shaderName.c_str();
				setDebugUtilsObjectNameEXT(
					*mpNativeDevice,
					&objectNameInfo);
			}
		}

		/*
		**
		*/
		void* CPipelineState::getNativePipelineState()
		{
			return &mNativePipelineState;
		}

		/*
		**
		*/
		void* CPipelineState::getNativeRenderPass()
		{
			return &mNativeRenderPass;
		}

	}   // Vulkan

} // RenderDriver