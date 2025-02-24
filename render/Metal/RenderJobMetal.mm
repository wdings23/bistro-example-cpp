#include "RenderJobMetal.h"
#include <render-driver/Metal/CommandAllocatorMetal.h>
#include <render-driver/Metal/CommandBufferMetal.h>
//#include <render-driver/Metal/FrameBufferMetal.h>
#include <render-driver/Metal/PipelineStateMetal.h>
#include <render-driver/Metal/SwapChainMetal.h>

#include <utils/LogPrint.h>
#include <utils/wtfassert.h>

#include <iostream>
#include <stdio.h>

#include <filesystem>

#if !defined(_MSC_VER)
    extern void getAssetsDir(std::string& fullPath, std::string const& fileName);
    extern char const* getSaveDir();
#endif // _MSC_VER

namespace Render
{
    namespace Metal
    {
		CRenderJob::CRenderJob() {}
		CRenderJob::~CRenderJob() {}

		/*
		**
		*/
		void CRenderJob::platformCreateAttachmentImage(
			std::string const& name,
			uint32_t iWidth,
			uint32_t iHeight,
			RenderDriver::Common::Format const& format
		)
		{
			maOutputImageAttachments[name] = std::make_unique<RenderDriver::Metal::CImage>();
			
			RenderDriver::Metal::CImage* pImage = maOutputImageAttachments[name].get();
			
			float afDefaultClearColor[4] = {0.0f, 0.0f, 0.3f, 0.0f};
			RenderDriver::Common::ImageDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(format);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(format);
			desc.miWidth = iWidth;
			desc.miHeight = iHeight;
			desc.miNumImages = 1;
			desc.mFormat = format;
			desc.mResourceFlags = RenderDriver::Common::ResourceFlagBits(
                uint32_t(RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess) |
                uint32_t(RenderDriver::Common::ResourceFlagBits::AllowRenderTarget)
            );
			desc.mafClearColor = afDefaultClearColor;
			desc.mImageLayout = RenderDriver::Common::ImageLayout::ATTACHMENT_OPTIMAL;
			pImage->create(
				desc, 
				*mpDevice
			);
			pImage->setID(name);
			pImage->getDescriptor().miWidth = iWidth;
			pImage->getDescriptor().miHeight = iHeight;
			pImage->getDescriptor().miNumImages = 1;
			pImage->getDescriptor().mFormat = desc.mFormat;
			pImage->setID(name + " Image");
			mapImageAttachments[name] = pImage;
			
			// set transition info

		}

		/*
		**
		*/
		void CRenderJob::platformCreateAttachmentBuffer(
			std::string const& name,
			uint32_t iBufferSize,
			RenderDriver::Common::Format const& format,
			RenderDriver::Common::BufferUsage const& usage
		)
		{
			maOutputBufferAttachments[name] = std::make_unique<RenderDriver::Metal::CBuffer>();
			RenderDriver::Metal::CBuffer* pBuffer = maOutputBufferAttachments[name].get();
			
			RenderDriver::Common::BufferDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(format);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(format);

			// data size
			desc.miSize = iBufferSize;
			
			// usage
			desc.mBufferUsage = usage;

			// format and writeable flag
			desc.mFormat = format;
			uint32_t iUsage = (uint32_t)usage;
			uint32_t iStorageBuffer = (uint32_t)RenderDriver::Common::BufferUsage::StorageBuffer;
			uint32_t iStorageTexel = (uint32_t)RenderDriver::Common::BufferUsage::StorageTexel;
			if((iUsage & iStorageBuffer) > 0 || (iUsage & iStorageTexel) > 0)
			{
				desc.mFlags = RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess;
			}

			// create and register to global buffer dictionary
			pBuffer->create(desc, *mpDevice);
			pBuffer->setID(name);
			mapBufferAttachments[name] = pBuffer;
		}

		/*
		**
		*/
		RenderDriver::Common::CBuffer* CRenderJob::platformCreateBuffer(
			std::string const& name,
			RenderDriver::Common::BufferDescriptor const& desc
		)
		{
			mapBuffers[name] = std::make_unique<RenderDriver::Metal::CBuffer>();
			
			RenderDriver::Metal::CBuffer* pBuffer = mapBuffers[name].get();
			
			pBuffer->create(desc, *mpDevice);
			pBuffer->setID(name);

			RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
			{
				/* .mFormat				*/  desc.mFormat,
				/* .mpBuffer			*/  pBuffer,
				/* .mpCounterBuffer		*/  nullptr,
				/* .miSize				*/  (uint32_t)desc.miSize,
				/* .miStructStrideSize	*/  1,
				/* .mFlags				*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
				/* .mViewType			*/  RenderDriver::Common::ResourceViewType::ShaderResourceView,
				/* .mpDescriptorHeap	*/  nullptr,
				/* .miDescriptorOffset	*/  0,
				/* .mpDescriptorSet		*/  nullptr,
			};

			return pBuffer;
		}

		/*
		**
		*/
		RenderDriver::Common::CImage* CRenderJob::platformCreateImage(
			std::string const& name,
			RenderDriver::Common::ImageDescriptor const& desc
		)
		{
            WTFASSERT(0, "Implement me");
            
			mapImages[name] = std::make_unique<RenderDriver::Metal::CImage>();

			RenderDriver::Metal::CImage* pImage = maOutputImageAttachments[name].get();
			mapImages[name]->create(desc, *mpDevice);

			return pImage;
		}

		/*
		**
		*/
		RenderDriver::Common::CImage* CRenderJob::platformCreateImageWithData(
			std::string const& name,
			unsigned char const* pImageData,
			uint32_t iTextureWidth,
			uint32_t iTextureHeight,
			RenderDriver::Common::Format const& format,
			RenderDriver::Common::CCommandQueue* pCommandQueue
		)
		{
            WTFASSERT(0, "Implement me");
            
			mapImages[name] = std::make_unique<RenderDriver::Metal::CImage>();

			RenderDriver::Metal::CImage* pImage = mapImages[name].get();

			float afDefaultClearColor[4] = {0.0f, 0.0f, 0.3f, 0.0f};
			RenderDriver::Common::ImageDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(format);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(format);
			desc.miWidth = iTextureWidth;
			desc.miHeight = iTextureHeight;
			desc.miNumImages = 1;
			desc.mFormat = format;
			desc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess;
			desc.mafClearColor = afDefaultClearColor;
			pImage->create(desc, *mpDevice);
			pImage->setID(name);

			// upload command buffer allocator
			RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc = {};
			commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
			RenderDriver::Metal::CCommandAllocator commandAllocator;
			commandAllocator.create(commandAllocatorDesc, *mpDevice);
			
			// upload command buffer
			RenderDriver::Common::CommandBufferDescriptor commandBufferDesc = {};
			commandBufferDesc.mpCommandAllocator = &commandAllocator;
			commandBufferDesc.mpPipelineState = nullptr;
			commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
			RenderDriver::Metal::CCommandBuffer commandBuffer;
			commandBuffer.create(commandBufferDesc, *mpDevice);
			commandBuffer.reset();

			// upload buffer 
			RenderDriver::Metal::CBuffer uploadBuffer;
			uint32_t iSize = iTextureWidth * iTextureHeight * 4;
			RenderDriver::Common::BufferDescriptor bufferDesc;
			bufferDesc.miSize = iSize;
			bufferDesc.mFormat = format;
			bufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
			uploadBuffer.create(bufferDesc, *mpDevice);

			// set image data
			uploadBuffer.setData(
				(void *)pImageData,
				iSize);

			// copy to image
			//uploadBuffer.copyToImage(
			//	uploadBuffer,
			//	*pImage,
			//	commandBuffer,
			//	iTextureWidth,
			//	iTextureHeight
			//);

			// execute command 
			pCommandQueue->execCommandBuffer(
				commandBuffer,
				*mpDevice
			);

			return pImage;
		}

		/*
		**
		*/
		void CRenderJob::platformInitDescriptorSet()
		{
			mDescriptorSet = std::make_unique<RenderDriver::Metal::CDescriptorSet>();
			RenderDriver::Common::DescriptorSetDescriptor desc = {};
			desc.mpaShaderResources = nullptr;

			desc.mPipelineType = PipelineType::GRAPHICS_PIPELINE_TYPE;
			if(mType == Render::Common::JobType::Compute)
				desc.mPipelineType = PipelineType::COMPUTE_PIPELINE_TYPE;
			else if(mType == Render::Common::JobType::Copy)
				desc.mPipelineType = PipelineType::COPY_PIPELINE_TYPE;

			mDescriptorSet->create(desc, *mpDevice);
			mpDescriptorSet = mDescriptorSet.get();
		}

		/*
		**
		*/
		void CRenderJob::platformInitPipelineState()
		{
			mPipelineState = std::make_unique<RenderDriver::Metal::CPipelineState>();
			mpPipelineState = mPipelineState.get();

			mSignalFence = std::make_unique<RenderDriver::Metal::CFence>();
			
			RenderDriver::Common::FenceDescriptor desc = {};
			mSignalFence->create(
				desc, 
				*mpDevice);
			mpSignalFence = mSignalFence.get();
		}

		/*
		**
		*/
		RenderDriver::Common::GraphicsPipelineStateDescriptor* CRenderJob::platformInitGraphicsPipelineStateDescriptor()
		{
			mGraphicsPipelineStateDesc = std::make_unique<RenderDriver::Metal::CPipelineState::GraphicsPipelineStateDescriptor>();
			return mGraphicsPipelineStateDesc.get();
		}

		/*
		**
		*/
		RenderDriver::Common::ComputePipelineStateDescriptor* CRenderJob::platformInitComputePipelineStateDescriptor()
		{
            mComputePipelineStateDesc = std::make_unique<RenderDriver::Metal::CPipelineState::ComputePipelineStateDescriptor>();
			return mComputePipelineStateDesc.get();
		}

		/*
		**
		*/
		RenderDriver::Common::RayTracePipelineStateDescriptor* CRenderJob::platformInitRayTracePipelineStateDescriptor()
    {
            WTFASSERT(0, "Implement me");
            
            //mRayTracePipelineStateDesc = std::make_unique<RenderDriver::Metal::CPipelineState::RayTracePipelineStateDescriptor>();
            //return mRayTracePipelineStateDesc.get();
            return nullptr;
        }

		/*
		**
		*/
		RenderDriver::Common::GraphicsPipelineStateDescriptor* CRenderJob::platformFillOutGraphicsPipelineDescriptor(
			RenderDriver::Common::GraphicsPipelineStateDescriptor* pDesc,
			std::vector<char>& acShaderBufferVS,
			std::vector<char>& acShaderBufferFS,
			std::string const& shaderPath,
            std::string const& pipelineName
		)
		{
DEBUG_PRINTF("%s\n", shaderPath.c_str());
            
            RenderDriver::Metal::CPipelineState::GraphicsPipelineStateDescriptor* pMetalDesc = (RenderDriver::Metal::CPipelineState::GraphicsPipelineStateDescriptor*)pDesc;
            //DEBUG_PRINTF("%s\n", shaderPath.c_str());
            
            pMetalDesc->mVertexEntryName = "VSMain";
            pMetalDesc->mFragementEntryName = "PSMain";
            
            std::string shaderOutputPath = std::string(getSaveDir()) + "/shader-output";
            
            auto fileNameEnd = shaderPath.find_last_of(".");
            auto directoryEnd = shaderPath.find_last_of("\\");
            std::string directory = shaderPath.substr(0, directoryEnd);
            std::string outputDirectory = directory + "-output";
            std::string fileName = shaderPath.substr(directoryEnd + 1, (fileNameEnd - (directoryEnd + 1)));
            std::string metalShaderFileName = fileName + ".metallib";
            
            std::string filePath = shaderOutputPath + "/" + metalShaderFileName;
            FILE* fp = fopen(filePath.c_str(), "rb");
            fseek(fp, 0, SEEK_END);
            size_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            acShaderBufferFS.resize(iFileSize);
            fread(acShaderBufferFS.data(), sizeof(char), iFileSize, fp);
            fclose(fp);
            
            // vertex shader
            if(pMetalDesc->mbFullTrianglePass)
            {
                filePath = shaderOutputPath + "/full-triangle-vertex-shader.metallib";
            }
            else
            {
                filePath = shaderOutputPath + "/draw-mesh-vertex-shader.metallib";
            }
            
            fp = fopen(filePath.c_str(), "rb");
            fseek(fp, 0, SEEK_END);
            iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            acShaderBufferVS.resize(iFileSize);
            fread(acShaderBufferVS.data(), sizeof(char), iFileSize, fp);
            fclose(fp);
            
            pMetalDesc->mLibraryFilePath = shaderOutputPath + "/" + metalShaderFileName;
            pMetalDesc->mVertexShaderLibraryFilePath = filePath;
            
            pDesc->miFlags = 0;
            pDesc->miPixelShaderSize = (uint32_t)iFileSize;
            pDesc->miVertexShaderSize = (uint32_t)iFileSize;
            pDesc->mpVertexShader = (uint8_t*)acShaderBufferVS.data();
            pDesc->mpPixelShader = (uint8_t*)acShaderBufferFS.data();
            pDesc->mpDescriptor = mpDescriptorSet;

            RenderDriver::Metal::CDescriptorSet* pDescriptorSetMetal = (RenderDriver::Metal::CDescriptorSet*)mpDescriptorSet;
            
            std::string bindingFileName = pipelineName + "-bindings.json";
            std::string bindingFilePath = shaderOutputPath + "/" + bindingFileName;
            rapidjson::Document doc;
            {
                std::vector<char> acBuffer;
                FILE* fp = fopen(bindingFilePath.c_str(), "rb");
                fseek(fp, 0, SEEK_END);
                size_t iFileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                acBuffer.resize(iFileSize + 1);
                memset(acBuffer.data(), 0, iFileSize + 1);
                fread(acBuffer.data(), sizeof(char), iFileSize, fp);
                doc.Parse(acBuffer.data());
            }
            auto aBindingInfo = doc.GetArray();
            for(auto const& bindingInfo : aBindingInfo)
            {
                std::string name = bindingInfo["name"].GetString();
                std::string shaderName = bindingInfo["shader-name"].GetString();
                uint32_t iBindingIndex = bindingInfo["index"].GetInt();
                uint32_t iBindingSet = bindingInfo["set"].GetInt();
                std::string type = bindingInfo["type"].GetString();
                
                SerializeUtils::Common::ShaderResourceInfo shaderResourceInfo;
                shaderResourceInfo.mName = name;
                shaderResourceInfo.mShaderResourceName = shaderName;
                shaderResourceInfo.miResourceIndex = iBindingIndex;
                shaderResourceInfo.miResourceSet = iBindingSet;
                
                // set the image or buffer ptr
                if(mapImageAttachments.find(name) != mapImageAttachments.end())
                {
                    shaderResourceInfo.mExternalResource.mpImage = mapImageAttachments[name];
                }
                else if(mapBufferAttachments.find(name) != mapBufferAttachments.end())
                {
                    shaderResourceInfo.mExternalResource.mpBuffer = mapBufferAttachments[name];
                }
                else if(mapUniformImages.find(name) != mapUniformImages.end())
                {
                    shaderResourceInfo.mExternalResource.mpImage = mapUniformImages[name];
                }
                else if(mapUniformBuffers.find(name) != mapUniformBuffers.end())
                {
                    shaderResourceInfo.mExternalResource.mpBuffer = mapUniformBuffers[name];
                }
                else if(name == "Default Uniform Buffer")
                {
                    shaderResourceInfo.mExternalResource.mpBuffer = mpDefaultUniformBuffer;
                }
                else if(name == "Depth Output")
                {
                    shaderResourceInfo.mExternalResource.mpImage = mpDepthImage;
                }
                else
                {
                    if(name != "sampler" && name != "textureSampler")
                    {
                        WTFASSERT(0, "Can\'t find \"%s\" shader resource in attachments", name.c_str());
                    }
                }
                
                shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
                if(type.find("StructuredBuffer") == 0)
                {
                    shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
                }
                else if(type.find("RWStructuredBuffer") == 0)
                {
                    shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
                }
                else if(type.find("RWTexture") == 0)
                {
                    shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT;
                }
                else if(type.find("Texture") == 0)
                {
                    shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
                }
                else if(type.find("ConstantBuffer") == 0)
                {
                    shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
                }
                else
                {
                    if(type != "SamplerState")
                    {
                        WTFASSERT(0, "Not handled");
                    }
                }
                
                pDescriptorSetMetal->maShaderResources.push_back(shaderResourceInfo);
            }
            
            mpDescriptorSet->getDesc().mpaShaderResources = &pDescriptorSetMetal->maShaderResources;
            
            return pMetalDesc;
		}

		/*
		**
		*/
		RenderDriver::Common::ComputePipelineStateDescriptor* CRenderJob::platformFillOutComputePipelineDescriptor(
			RenderDriver::Common::ComputePipelineStateDescriptor* pDesc,
			std::vector<char>& acShaderBuffer,
			std::string const& shaderPath,
            std::string const& pipelinePath
		)
		{
			RenderDriver::Metal::CPipelineState::ComputePipelineStateDescriptor* pMetalDesc = (RenderDriver::Metal::CPipelineState::ComputePipelineStateDescriptor*)pDesc;
            //DEBUG_PRINTF("%s\n", shaderPath.c_str());
            
            std::string shaderOutputPath = std::string(getSaveDir()) + "/shader-output";
            
            auto fileNameEnd = shaderPath.find_last_of(".");
            auto directoryEnd = shaderPath.find_last_of("\\");
            std::string directory = shaderPath.substr(0, directoryEnd);
            std::string outputDirectory = directory + "-output";
            std::string fileName = shaderPath.substr(directoryEnd + 1, (fileNameEnd - (directoryEnd + 1)));
            std::string metalShaderFileName = fileName + ".metallib";
            
            // shader library data
            std::string filePath = shaderOutputPath + "/" + metalShaderFileName;
            FILE* fp = fopen(filePath.c_str(), "rb");
            fseek(fp, 0, SEEK_END);
            size_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            acShaderBuffer.resize(iFileSize);
            fread(acShaderBuffer.data(), sizeof(char), iFileSize, fp);
            fclose(fp);
            
            // set data
            pMetalDesc->mLibraryFilePath = std::string("shader-output/") + metalShaderFileName;
            pMetalDesc->mComputeEntryName = "CSMain";
			pDesc->miFlags = 0;
			pDesc->miComputeShaderSize = (uint32_t)iFileSize;
			pDesc->mpComputeShader = (uint8_t*)acShaderBuffer.data();
			pDesc->mpDescriptor = mpDescriptorSet;

            RenderDriver::Metal::CDescriptorSet* pDescriptorSetMetal = (RenderDriver::Metal::CDescriptorSet*)mpDescriptorSet;
            
            // shader resource bindings
            std::string bindingFileName = pipelinePath + "-bindings.json";
            std::string bindingFilePath = std::string(getSaveDir()) + "/shader-output/" + bindingFileName;
            rapidjson::Document doc;
            {
                std::vector<char> acBuffer;
                FILE* fp = fopen(bindingFilePath.c_str(), "rb");
                fseek(fp, 0, SEEK_END);
                size_t iFileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                acBuffer.resize(iFileSize + 1);
                memset(acBuffer.data(), 0, iFileSize + 1);
                fread(acBuffer.data(), sizeof(char), iFileSize, fp);
                doc.Parse(acBuffer.data());
            }
            auto aBindingInfo = doc.GetArray();
            for(auto const& bindingInfo : aBindingInfo)
            {
                std::string shaderResourceName = bindingInfo["shader-name"].GetString();
                uint32_t iBindingIndex = bindingInfo["index"].GetInt();
                uint32_t iBindingSet = bindingInfo["set"].GetInt();
                std::string name = bindingInfo["name"].GetString();
                
                SerializeUtils::Common::ShaderResourceInfo shaderResourceInfo;
                shaderResourceInfo.mName = name;
                shaderResourceInfo.mShaderResourceName = shaderResourceName;
                shaderResourceInfo.miResourceIndex = iBindingIndex;
                shaderResourceInfo.miResourceSet = iBindingSet;
                
                // set the image or buffer ptr
                if(mapImageAttachments.find(name) != mapImageAttachments.end())
                {
                    shaderResourceInfo.mExternalResource.mpImage = mapImageAttachments[name];
                }
                else if(mapBufferAttachments.find(name) != mapBufferAttachments.end())
                {
                    shaderResourceInfo.mExternalResource.mpBuffer = mapBufferAttachments[name];
                }
                else if(mapUniformImages.find(name) != mapUniformImages.end())
                {
                    shaderResourceInfo.mExternalResource.mpImage = mapUniformImages[name];
                }
                else if(mapUniformBuffers.find(name) != mapUniformBuffers.end())
                {
                    shaderResourceInfo.mExternalResource.mpBuffer = mapUniformBuffers[name];
                }
                else if(name == "Default Uniform Buffer")
                {
                    shaderResourceInfo.mExternalResource.mpBuffer = mpDefaultUniformBuffer;
                }
                else
                {
                    if(name != "sampler" && name != "textureSampler")
                    {
                        WTFASSERT(0, "Can\'t find \"%s\" shader resource in attachments", name.c_str());
                    }
                }
                
                pDescriptorSetMetal->maShaderResources.push_back(shaderResourceInfo);
            }
                        
            // fill out shader resources
            // set 0, attachments
            uint32_t iIndex = 0;
            for(auto const& mapping : maAttachmentMappings)
            {
                // look for the corresponding shader resource
                auto iter = std::find_if(
                    pDescriptorSetMetal->maShaderResources.begin(),
                    pDescriptorSetMetal->maShaderResources.end(),
                    [&](SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
                    {
                        return mapping.first == shaderResourceInfo.mName;
                        
                    });
                WTFASSERT(iter != pDescriptorSetMetal->maShaderResources.end(), "can\'t find \"%s\"", mapping.first.c_str());
                
                if(mapping.second == "texture-input")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
                }
                else if(mapping.second == "texture-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT;
                }
                else if(mapping.second == "texture-input-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT;
                }
                else if(mapping.second == "buffer-input")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
                }
                else if(mapping.second == "buffer-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
                }
                else if(mapping.second == "buffer-input-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
                }
                
                ++iIndex;
            }
           
            // set 1, shader resources
            for(auto const& mapping : maUniformMappings)
            {
                // look for the corresponding shader resource
                std::string name = mapping.first;
                auto iter = std::find_if(
                    pDescriptorSetMetal->maShaderResources.begin(),
                    pDescriptorSetMetal->maShaderResources.end(),
                    [name](SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
                    {
                        return name == shaderResourceInfo.mName;
                    });
                WTFASSERT(iter != pDescriptorSetMetal->maShaderResources.end(), "can\'t find \"%s\"", mapping.first.c_str());
                
                if(mapping.second == "texture-input")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
                }
                else if(mapping.second == "texture-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT;
                }
                else if(mapping.second == "texture-input-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT;
                }
                else if(mapping.second == "buffer-input")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
                }
                else if(mapping.second == "buffer-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
                }
                else if(mapping.second == "buffer-input-output")
                {
                    iter->mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
                }
            }
            
            mpDescriptorSet->getDesc().mpaShaderResources = &pDescriptorSetMetal->maShaderResources;
            
			return pMetalDesc;
		}

		/*
		**
		*/
		RenderDriver::Common::RayTracePipelineStateDescriptor* CRenderJob::platformFillOutRayTracePipelineDescriptor(
			RenderDriver::Common::RayTracePipelineStateDescriptor* pDesc,
			std::vector<char>& acRayGenShaderBuffer,
			std::vector<char>& acCloseHitShaderBuffer,
			std::vector<char>& acMissShaderBuffer,
			std::string const& shaderPath,
			void* pPlatformInstance
		)
		{
            WTFASSERT(0, "Implement me");
            
#if 0
			RenderDriver::Metal::CPipelineState::RayTracePipelineStateDescriptor* pMetalDesc = (RenderDriver::Metal::CPipelineState::RayTracePipelineStateDescriptor*)pDesc;
			//pMetalDesc->mpPlatformInstance = pPlatformInstance;

			// get shaders
			auto fileNameEnd = shaderPath.find_last_of(".");
			auto directoryEnd = shaderPath.find_last_of("\\");
			std::string directory = shaderPath.substr(0, directoryEnd);
			std::string outputDirectory = directory + "-output";
			std::string fileName = shaderPath.substr(directoryEnd + 1, (fileNameEnd - (directoryEnd + 1)));
			std::string filePath = outputDirectory + "\\" + fileName + "-raygen.spirv";

			// compile compute shader
			// ray gen
			std::string mainFunctionName = "rayGen";
			std::string compileCommand = "D:\\MetalSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				filePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			int iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}

			FILE* fp = fopen(filePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acRayGenShaderBuffer.resize(iFileSize);
			fread(acRayGenShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);
			//pMetalDesc->mpRayGenShader = (uint8_t*)acRayGenShaderBuffer.data();
			//pMetalDesc->miRayGenShaderSize = (uint32_t)acRayGenShaderBuffer.size();

			// close hit
			filePath = outputDirectory + "\\" + fileName + "-closehit.spirv";
			mainFunctionName = "hitTriangle";
			compileCommand = "D:\\MetalSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				filePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}
			fp = fopen(filePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acCloseHitShaderBuffer.resize(iFileSize);
			fread(acCloseHitShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);
			//pMetalDesc->mpCloseHitShader = (uint8_t*)acCloseHitShaderBuffer.data();
			//pMetalDesc->miHitShaderSize = (uint32_t)acCloseHitShaderBuffer.size();

			// miss
			filePath = outputDirectory + "\\" + fileName + "-miss.spirv";
			mainFunctionName = "missShader";
			compileCommand = "D:\\MetalSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				filePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}
			fp = fopen(filePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acMissShaderBuffer.resize(iFileSize);
			fread(acMissShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);
			//pMetalDesc->mpMissShader = (uint8_t*)acMissShaderBuffer.data();
			//pMetalDesc->miMissShaderSize = (uint32_t)acMissShaderBuffer.size();

			return pMetalDesc;
#endif // #if 0
            
            return nullptr;
		}

		/*
		**
		*/
		void CRenderJob::platformCreateDepthImage(
			uint32_t iWidth,
			uint32_t iHeight
		)
		{
			mDepthImage = std::make_unique<RenderDriver::Metal::CImage>();
			
			RenderDriver::Common::Format format = RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT;

			float afDefaultClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			RenderDriver::Common::ImageDescriptor desc = {};
			desc.miWidth = iWidth;
			desc.miHeight = iHeight;
			desc.miNumImages = 1;
			desc.mFormat = format;
			desc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess;
			desc.mafClearColor = afDefaultClearColor;
			mDepthImage->create(
				desc,
				*mpDevice);
			mDepthImage->setID(mName + "-" + "Depth Output");
			mpDepthImage = mDepthImage.get();
		}

		/*
		**
		*/
		void CRenderJob::platformCreateFrameBuffer(
			RenderDriver::Common::CSwapChain* pSwapChain
		)
		{
			//mFrameBuffer = std::make_unique<RenderDriver::Metal::CFrameBuffer>();
			
			if(mPassType == Render::Common::PassType::SwapChain)
			{
				RenderDriver::Metal::CSwapChain* pSwapChainMetal = (RenderDriver::Metal::CSwapChain*)pSwapChain;
				RenderDriver::Metal::CPipelineState* pPipelineStateMetal = (RenderDriver::Metal::CPipelineState*)mpPipelineState;
				// create frame buffer
			}
			else
			{
				RenderDriver::Common::FrameBufferDescriptor2 desc;
				desc.mDepthView = (mpDepthImageView != nullptr) ?
					std::make_pair(mpDepthImageView, (uint32_t)mapOutputImageAttachments.size() - 1) :
					std::make_pair(nullptr, UINT32_MAX);

				// attachment views for the output render targets used in the frame buffer
				std::vector<RenderDriver::Common::CImageView*> apOutputImageAttachmentViews;
				uint32_t iImageWidth = 0, iImageHeight = 0;
				for(auto const& keyValue : maAttachmentMappings)
				{
					if(keyValue.second == "texture-output")
					{
						auto& attachmentView = mapImageAttachmentViews[keyValue.first];
						apOutputImageAttachmentViews.push_back(attachmentView);
						if(iImageWidth == 0)
						{
							iImageWidth = mapOutputImageAttachments[keyValue.first]->getDescriptor().miWidth;
							iImageHeight = mapOutputImageAttachments[keyValue.first]->getDescriptor().miHeight;
						}
					}
					else if(keyValue.second == "texture-input-output")
					{
						if(keyValue.first == "Depth Output")
						{
							//apOutputImageAttachmentViews.push_back(mpDepthImageView);
						}
						else 
						{
							auto& attachmentView = mapImageAttachmentViews[keyValue.first];
							apOutputImageAttachmentViews.push_back(attachmentView);
						}
						if(iImageWidth == 0)
						{
							iImageWidth = mapImageAttachments[keyValue.first]->getDescriptor().miWidth;
							iImageHeight = mapImageAttachments[keyValue.first]->getDescriptor().miHeight;
						}
					}
				}
				desc.mpaImageView = &apOutputImageAttachmentViews;
				desc.mpPipelineState = mpPipelineState;
				desc.miWidth = iImageWidth;
				desc.miHeight = iImageHeight;

				//mFrameBuffer->create2(
				//	desc,
				//	*mpDevice
				//);
				//mpFrameBuffer = mFrameBuffer.get();
			}
		}

		/*
		**
		*/
		void CRenderJob::platformCreateSemaphore()
		{
			mSignalFence = std::make_unique<RenderDriver::Metal::CFence>();

			RenderDriver::Common::FenceDescriptor desc = {};
			mSignalFence->create(
				desc,
				*mpDevice);
			mpSignalFence = mSignalFence.get();
		}

		/*
		**
		*/
		void CRenderJob::platformCreateImageView(
			std::string const& name,
			RenderDriver::Common::ImageViewDescriptor const& imageViewDesc)
		{
            WTFASSERT(0, "Implement me");
		}

		/*
		**
		*/
		void CRenderJob::platformInitAttachmentBarriers(
			CreateInfo const& createInfo
		)
		{
#if 0
			RenderDriver::Common::CCommandQueue::Type jobType = createInfo.mpCommandQueue->getType();
			for(auto& outputAttachment : mapOutputImageAttachments)
			{
				if(outputAttachment.second == nullptr)
				{
					continue;
				}

				if(jobType == RenderDriver::Common::CCommandQueue::Type::Copy)
				{
					//DEBUG_PRINTF("\"%s\"\n\told layout: %d src access: %d srcStage: %d\n\tnew layout: %d dst access: %d dstStage: %d\n",
					//	outputAttachment.first.c_str(),
					//	oldLayout,
					//	imageMemoryBarrier.srcAccessMask,
					//	srcStageMask,
					//	newLayout,
					//	imageMemoryBarrier.dstAccessMask,
					//	dstStageMask
					//);
				}
			}
#endif // #if 0
		}

		/*
		**
		*/
		void CRenderJob::platformUploadDataToBuffer(
			RenderDriver::Common::CBuffer& buffer,
			char const* pacData,
			uint32_t iDataSize,
			RenderDriver::Common::CCommandQueue& commandQueue
		)
		{
            WTFASSERT(0, "Implement me");
            
			// upload command buffer allocator
			RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc = {};
			commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
			RenderDriver::Metal::CCommandAllocator commandAllocator;
			commandAllocator.create(commandAllocatorDesc, *mpDevice);

			// upload command buffer
			RenderDriver::Common::CommandBufferDescriptor commandBufferDesc = {};
			commandBufferDesc.mpCommandAllocator = &commandAllocator;
			commandBufferDesc.mpPipelineState = nullptr;
			commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
			RenderDriver::Metal::CCommandBuffer commandBuffer;
			commandBuffer.create(commandBufferDesc, *mpDevice);
			commandBuffer.reset();

			// upload buffer 
			RenderDriver::Metal::CBuffer uploadBuffer;
			RenderDriver::Common::BufferDescriptor bufferDesc;
			bufferDesc.miSize = iDataSize;
			bufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage(
				uint32_t(RenderDriver::Common::BufferUsage::TransferSrc) |
				uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer)
			);
			bufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
			uploadBuffer.create(bufferDesc, *mpDevice);

			// set image data
			uploadBuffer.setData(
				(void*)pacData,
				iDataSize);

			buffer.copy(
				uploadBuffer,
				commandBuffer,
				0,
				0,
				iDataSize
			);

			commandBuffer.close();

			// execute command 
			commandQueue.execCommandBuffer(
				commandBuffer,
				*mpDevice
			);

			// command queue wait
            
			uploadBuffer.releaseNativeBuffer();
		}

    }	// Metal

}	// Render 
