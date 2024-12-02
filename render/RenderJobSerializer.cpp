#include <render/RenderJobSerializer.h>
#include <render/Renderer.h>
#include <render-driver/PipelineInfo.h>

#include <LogPrint.h>
#include <wtfassert.h>
#include <functional>
#include <sstream>

namespace Render
{
	namespace Common
	{
		RenderDriver::Common::Format convertFormat(std::string const& format);

		/*
		**
		*/
		void Serializer::init(ShaderResourceInitializeDescriptor& desc)
		{
			mpRenderer = desc.mpRenderer;
		}

		/*
		**
		*/
		void Serializer::addInitDataFunction(
			std::string const& renderJobName,
			std::string const& shaderResourceName,
			ShaderResourceInitializeFunctions const& functions)
		{
			std::pair<std::string, std::string> renderJobResourcePair = std::make_pair(renderJobName, shaderResourceName);
			mInitializeFunctions[renderJobResourcePair] = functions;
			mInitializeFunctions[renderJobResourcePair].mpUserData = functions.mpUserData;
		}

		/*
		**
		*/
		void Serializer::setRenderJobDrawFunc(
			void(*pfnDrawOverride)(RenderDriver::Common::CCommandBuffer&, void*),
			void* pUserData,
			std::string const& renderJobName)
		{
			for(auto& renderJob : maRenderJobs)
			{
				if(renderJob.mName == renderJobName)
				{
					renderJob.mpfnDrawOverride = pfnDrawOverride;
					renderJob.mpDrawFuncUserData = pUserData;
					break;
				}
			}

			DEBUG_PRINTF("!!! didn\'t find job: \"%s\" to set draw override\n", renderJobName.c_str());
		}

		/*
		**
		*/
		void Serializer::initRenderJobs(
			std::string const& filePath,
			uint32_t iScreenWidth,
			uint32_t iScreenHeight,
			RenderDriver::Common::CDevice& device)
		{
			mpDevice = &device;

			Render::Common::RenderDriverType renderDriverType = mpRenderer->getRenderDriverType();

			miNumGraphicJobs = 0;
			miNumComputeJobs = 0;
			miNumCopyJobs = 0;

			std::string dirPath = "";
			auto end0 = filePath.find_last_of("/");
			auto end1 = filePath.find_last_of("\\");
			if(end0 == std::string::npos)
			{
				dirPath = filePath.substr(0, end1);
			}
			else if(end1 == std::string::npos)
			{
				dirPath = filePath.substr(0, end0);
			}
			else
			{
				if(end1 > end0)
				{
					dirPath = filePath.substr(0, end1);
				}
				else
				{
					dirPath = filePath.substr(0, end0);
				}
			}

			rapidjson::Document doc;
			{
				std::vector<char> acBuffer;
				FILE* fp = fopen(filePath.c_str(), "rb");
				fseek(fp, 0, SEEK_END);
				size_t iFileSize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				acBuffer.resize(iFileSize + 1);
				memset(acBuffer.data(), 0, iFileSize + 1);
				fread(acBuffer.data(), sizeof(char), iFileSize, fp);
				doc.Parse(acBuffer.data());
			}

			auto const& aJobs = doc["Jobs"].GetArray();
			auto iNumJobs = aJobs.Size();
			maRenderJobs.resize(iNumJobs);

			// load render jobs along with their shader resources
			uint32_t iJobIndex = 0;
			for(auto const& job : aJobs)
			{
				RenderJobInfo& renderJob = maRenderJobs[iJobIndex];
				char const* szPipelineFilePath = nullptr;
				if(job.HasMember("Pipeline"))
				{
					szPipelineFilePath = job["Pipeline"].GetString();
				}

				std::string typeStr = std::string(job["Type"].GetString());

				if(typeStr == "Compute")
				{
					++miNumComputeJobs;
				}
				else if(typeStr == "Graphics")
				{
					++miNumGraphicJobs;
				}
				else if(typeStr == "Copy")
				{
					++miNumCopyJobs;
				}

				renderJob.mName = job["Name"].GetString();

				DEBUG_PRINTF("Render Job: \"%s\"\n",
					renderJob.mName.c_str());

				szPipelineFilePath = nullptr;
				if(job.HasMember("Pipeline"))
				{
					szPipelineFilePath = job["Pipeline"].GetString();
				}

				// load pipeline json file
				if(typeStr != "Copy" && szPipelineFilePath != nullptr)
				{
					std::string fullPath = dirPath + "/" + szPipelineFilePath;

					std::vector<char> acBuffer;
					FILE* fp = fopen(fullPath.c_str(), "rb");
					WTFASSERT("Can\'t find file %s", fullPath.c_str());
					fseek(fp, 0, SEEK_END);
					size_t iFileSize = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					acBuffer.resize(iFileSize + 1);
					memset(acBuffer.data(), 0, iFileSize + 1);
					fread(acBuffer.data(), sizeof(char), iFileSize, fp);
					doc.Parse(acBuffer.data());

					if(doc.HasMember("Shaders"))
					{
						auto const& aShaders = doc["Shaders"].GetArray();
						std::string baseName;
						if(typeStr == "Compute")
						{
							for(auto const& shader : aShaders)
							{
								baseName = shader["Compute"].GetString();
								char const* szShaderType = "Compute";

								std::string shaderResourceFileName = baseName + "_shader_resources.json";
								std::string shaderResourceRegisterFilePath = dirPath + "/output/" + baseName + "_shader_variable_registers.json";
								std::string fullShaderResourcePath = dirPath + "/output/" + shaderResourceFileName;
								
SerializeUtils::Common::loadShaderResources3(
	renderJob.maShaderResourceInfo, 
	fullShaderResourcePath,
	shaderResourceRegisterFilePath,
	szShaderType,
	renderDriverType == Render::Common::RenderDriverType::Vulkan);

								//SerializeUtils::Common::loadShaderResources2(
								//	renderJob.maShaderResourceInfo, 
								//	fullShaderResourcePath,
								//	shaderResourceRegisterFilePath,
								//	szShaderType);
							}
						}
						else if(typeStr == "Graphics")
						{
							for(auto const& shader : aShaders)
							{
								rapidjson::Value::ConstMemberIterator iter = shader.MemberBegin();
								baseName = iter->value.GetString();
								char const* szShaderType = iter->name.GetString();

								std::string shaderResourceFileName = baseName + "_shader_resources.json";
								std::string shaderResourceRegisterFilePath = dirPath + "/output/" + baseName + "_shader_variable_registers.json";
								std::string fullShaderResourcePath = dirPath + "/output/" + shaderResourceFileName;
								

SerializeUtils::Common::loadShaderResources3(
	renderJob.maShaderResourceInfo,
	fullShaderResourcePath,
	shaderResourceRegisterFilePath,
	szShaderType,
	renderDriverType == Render::Common::RenderDriverType::Vulkan);

								//SerializeUtils::Common::loadShaderResources2(
								//	renderJob.maShaderResourceInfo, 
								//	fullShaderResourcePath,
								//	shaderResourceRegisterFilePath,
								//	szShaderType);
							}
						}

						// allocate triple buffer shader resource
						for(uint32_t iBuffer = 0; iBuffer < 3; iBuffer++)
						{
							renderJob.maaTripleBufferShaderResourceDataInfo[iBuffer].resize(renderJob.maShaderResourceInfo.size());
						}

					}	// has "Shaders"

				}	// load pipeline json

				// get pipeline name from the referenced file
				std::string pipelineName = "";
				if(szPipelineFilePath != nullptr)
				{
					std::string fullPath = dirPath + "/" + szPipelineFilePath;
					FILE* fp = fopen(fullPath.c_str(), "rb");
					fseek(fp, 0, SEEK_END);
					size_t iFileSize = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					std::vector<char> aBuffer(iFileSize + 1);
					memset(aBuffer.data(), 0, iFileSize + 1);
					fread(aBuffer.data(), sizeof(char), iFileSize, fp);
					fclose(fp);
					rapidjson::Document pipelineDoc;
					pipelineDoc.Parse(aBuffer.data());
					pipelineName = pipelineDoc["Name"].GetString();

					// connect attachment of shader resource to dependant parent
					auto const& aAttachments = pipelineDoc["Attachments"].GetArray();
					for(auto const& attachment : aAttachments)
					{
						Render::Common::RenderJobAttachmentInfo attachmentInfo;

						char const* szParentJobName = "This";
						if(attachment.HasMember("ParentJobName"))
						{
							szParentJobName = attachment["ParentJobName"].GetString();
						}

						char const* szParentName = "This";
						if(attachment.HasMember("ParentName"))
						{
							szParentName = attachment["ParentName"].GetString();
						}

						char const* szShaderResourceName = "";
						if(attachment.HasMember("ShaderResourceName"))
						{
							attachmentInfo.mShaderResourceName = attachment["ShaderResourceName"].GetString();
						}

						attachmentInfo.mName = attachment["Name"].GetString();
						attachmentInfo.mParentJobName = szParentJobName;
						attachmentInfo.mParentName = szParentName;

						if(attachment.HasMember("Clear"))
						{
							attachmentInfo.mbClear = (strcmp(attachment["Clear"].GetString(), "False") == 0) ? false : true;
						}

						// attachment type
						if(attachment.HasMember("Type"))
						{
							char const* szType = attachment["Type"].GetString();
							if(!strcmp(szType, "TextureInput"))
							{
								attachmentInfo.mType = AttachmentType::TextureIn;
							}
							else if(!strcmp(szType, "TextureOutput"))
							{
								attachmentInfo.mType = AttachmentType::TextureOut;
							}
							else if(!strcmp(szType, "TextureInputOutput"))
							{
								attachmentInfo.mType = AttachmentType::TextureInOut;
							}
							else if(!strcmp(szType, "BufferInput"))
							{
								attachmentInfo.mType = AttachmentType::BufferIn;
							}
							else if(!strcmp(szType, "BufferOutput"))
							{
								attachmentInfo.mType = AttachmentType::BufferOut;
							}
							else if(!strcmp(szType, "BufferInputOutput"))
							{
								attachmentInfo.mType = AttachmentType::BufferInOut;
							}
							else if(!strcmp(szType, "VertexOutput"))
							{
								attachmentInfo.mType = AttachmentType::VertexOutput;
							}
							else if(!strcmp(szType, "IndirectDrawListInput"))
							{
								attachmentInfo.mType = AttachmentType::IndirectDrawListInput;
							}
							else
							{
								WTFASSERT(0, "Invalid attachment type: %s", szType);
							}
						}

						// attachment view type
						if(attachment.HasMember("ViewType"))
						{
							char const* szViewType = attachment["ViewType"].GetString();
							WTFASSERT(attachmentInfo.mType == AttachmentType::TextureIn || attachmentInfo.mType == AttachmentType::BufferIn,
								"Invalid type (%d), creating view type %s for %s",
								attachmentInfo.mType,
								szViewType,
								attachmentInfo.mName.c_str());
							if(!strcmp(szViewType, "TextureInput"))
							{
								attachmentInfo.mViewType = AttachmentType::TextureIn;
							}
							else if(!strcmp(szViewType, "BufferInput"))
							{
								attachmentInfo.mViewType = AttachmentType::BufferIn;
							}
							else
							{
								WTFASSERT(0, "Unhandled view type: %s", szViewType);
							}
						}
						
						attachmentInfo.miDataSize = 0;
						if(attachment.HasMember("DataSize"))
						{
							attachmentInfo.miDataSize = attachment["DataSize"].GetInt();
						}

						attachmentInfo.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
						if(attachment.HasMember("Format"))
						{
							attachmentInfo.mFormat = convertFormat(attachment["Format"].GetString());
						}

						if(attachment.HasMember("ClearColor"))
						{
							auto aClearColor = attachment["ClearColor"].GetArray();
							for(uint32_t i = 0; i < aClearColor.Size(); i++)
							{
								attachmentInfo.mafClearColor[i] = aClearColor[i].GetFloat();
							}
							attachmentInfo.mbHasClearColor = true;
						}

						if(attachment.HasMember("External"))
						{
							attachmentInfo.mbExternalData = (std::string("True") == attachment["External"].GetString()) ? true : false;
						}

						renderJob.maAttachmentInfo.push_back(attachmentInfo);

					}	// for attachment in attachments

				}	// if has pipeline file path

				// viewport
				{
					if(job.HasMember("ViewportScaleWidth"))
					{
						renderJob.mViewportScale.x = job["ViewportScaleWidth"].GetFloat();
					}

					if(job.HasMember("ViewportScaleHeight"))
					{
						renderJob.mViewportScale.y = job["ViewportScaleHeight"].GetFloat();
					}
				}

				// extra constant buffer entries
				if(job.HasMember("ExtraConstantBufferEntries"))
				{
					ExtraConstantBufferEntry extraEntry;
					auto const& entries = job["ExtraConstantBufferEntries"].GetArray();
					for(auto const& entry : entries)
					{
						extraEntry.mName = entry["Name"].GetString();
						extraEntry.mType = entry["Type"].GetString();
						extraEntry.mDynamicValue = "";
						if(entry.HasMember("DynamicValue"))
						{
							extraEntry.mDynamicValue = entry["DynamicValue"].GetString();
						}
						else
						{
							extraEntry.mfValue = entry["Value"].GetFloat();
						}

						renderJob.maExtraConstantBufferEntries.push_back(extraEntry);
					}
				}

				// root constants
				if(job.HasMember("RootConstants"))
				{
					renderJob.miNumRootConstants = 0;
					auto const& entries = job["RootConstants"].GetArray();
					for(auto const& entry : entries)
					{
						renderJob.mafRootConstants[renderJob.miNumRootConstants] = entry["Value"].GetFloat();
						++renderJob.miNumRootConstants;
					}
				}

				// Wait On Parent Job
				if(job.HasMember("WaitOnJob"))
				{
					renderJob.maWaitOnJobs.push_back(job["WaitOnJob"].GetString());
				}

				if(job.HasMember("SkipWaitOnJob"))
				{
					auto skipArray = job["SkipWaitOnJob"].GetArray();
					for(auto const& skipJobEntry : skipArray)
					{
						renderJob.maSkipWaitOnJobs.push_back(skipJobEntry.GetString());
					}

					
				}

				if(renderJob.mType == JobType::Compute)
				{
					if(job.HasMember("Dispatch"))
					{
						if(job["Dispatch"].HasMember("x"))
						{
							renderJob.miNumDispatchX = job["Dispatch"]["x"].GetUint();
						}

						if(job["Dispatch"].HasMember("y"))
						{
							renderJob.miNumDispatchY = job["Dispatch"]["y"].GetUint();
						}

						if(job["Dispatch"].HasMember("z"))
						{
							renderJob.miNumDispatchZ = job["Dispatch"]["z"].GetUint();
						}
					}
				}

				//if(pipelineName.length() > 0)
				//{
				//	auto const& iter = std::find_if(
				//		maRenderJobs.begin(),
				//		maRenderJobs.end(),
				//		[pipelineName](RenderJobInfo const& check)
				//		{
				//			return check.mName == pipelineName;
				//		});
				//	assert(iter == maRenderJobs.end());
				//
				//	//renderJob.mName = pipelineName;
				//}

				//uto const& iter = std::find_if(
				//		maRenderJobs.begin(),
				//		maRenderJobs.end(),
				//		[renderJob](RenderJobInfo const& check)
				//		{
				//			return check.mName == renderJob.mName;
				//		});
				//TFASSERT(iter == maRenderJobs.end(), "Duplicate Render Job (%s)\n", renderJob.mName);

				++iJobIndex;

			}	// for renderJob in renderJobs

			iJobIndex = 0;
			for(auto const& job : aJobs)
			{
				RenderJobInfo& renderJob = maRenderJobs[iJobIndex];
				std::string pipelineName = renderJob.mName;

				DEBUG_PRINTF("\n***********\npipeline: %s\n", pipelineName.c_str());

				execPreDataInitFunctions(renderJob);

				// pipeline
				char const* szPipelineFilePath = nullptr;
				std::string fullPath = "";
				if(job.HasMember("Pipeline"))
				{
					szPipelineFilePath = job["Pipeline"].GetString();
					fullPath = dirPath + "/" + szPipelineFilePath;
				}

				Render::Common::PassType passType = Render::Common::PassType::Compute;
				if(job.HasMember("PassType"))
				{
					std::string passTypeStr = job["PassType"].GetString();
					if(passTypeStr == "Draw Meshes Pass")
					{
						passType = Render::Common::PassType::DrawMeshes;
					}
					else if(passTypeStr == "Full Triangle Pass")
					{
						passType = Render::Common::PassType::FullTriangle;
					}
					else if(passTypeStr == "Copy Pass")
					{
						passType = Render::Common::PassType::Copy;
					}
					else if(passTypeStr == "Swap Chain Pass")
					{
						passType = Render::Common::PassType::SwapChain;
					}
					else if(passTypeStr == "Imgui Pass")
					{
						passType = Render::Common::PassType::Imgui;
					}
					else if(passTypeStr == "Draw Mesh Cluster Pass")
					{
						passType = Render::Common::PassType::DrawMeshClusters;
					}
					else if(passTypeStr != "Compute Pass")
					{
						WTFASSERT(0, "Didn\'t have an entry for pass type: \"%s\"", passTypeStr.c_str());
					}
				}

				renderJob.mPassType = passType;
				renderJob.mType = JobType::Graphics;

				if(passType != Render::Common::PassType::Imgui)
				{
					// resources
					initShaderResources(
						renderJob,
						iScreenWidth,
						iScreenHeight,
						pipelineName.c_str(),
						device);

					//if(passType != Render::Common::PassType::Copy)
					//{
					//	assert(renderJob.maShaderResourceInfo.size() > 1);
					//}

					uint32_t iPipelineIndex = serializePipeline(
						maPipelines,
						maPipelineRootSignatures,
						maPipelineStates,
						renderJob.maShaderResourceInfo,
						fullPath,
						maRenderJobs,
						renderJob,
						device);

					//initRenderJob(
					//	renderJob,
					//	iPipelineIndex,
					//	device);

					renderJob.miPipelineIndex = iPipelineIndex;
					if(maPipelines[iPipelineIndex].mType == PipelineType::COMPUTE_PIPELINE_TYPE)
					{
						renderJob.mType = JobType::Compute;
					}
					else if(maPipelines[iPipelineIndex].mType == PipelineType::GRAPHICS_PIPELINE_TYPE)
					{
						renderJob.mType = JobType::Graphics;
					}
					else if(maPipelines[iPipelineIndex].mType == PipelineType::COPY_PIPELINE_TYPE)
					{
						renderJob.mType = JobType::Copy;
					}
				}
				else
				{
					maPipelines.resize(maPipelines.size() + 1);
					auto& pipeline = maPipelines[maPipelines.size() - 1];
					renderJob.miPipelineIndex = static_cast<uint32_t>(maPipelines.size()) - 1;
					pipeline.mName = renderJob.mName;
					pipeline.mType = PipelineType::GRAPHICS_PIPELINE_TYPE;

					AttachmentInfo attachmentInfo =
					{
						/* std::string							mName;								*/		renderJob.maAttachmentInfo[0].mName,
						/* std::string							mParentRenderJobName;				*/		renderJob.maAttachmentInfo[0].mParentJobName,
						/* AttachmentTypeBits					mType;								*/		AttachmentTypeBits::TextureOutput,
						/* RenderDriver::Common::Format			mFormat;							*/		mpRenderer->getDescriptor().mFormat,
						/* bool									mbBlend = true;						*/		true,
						/* float								mfScaleWidth = 1.0f;				*/		1.0f,
						/* float								mfScaleHeight = 1.0f;				*/		1.0f,
						/* uint32_t								miShaderResourceIndex = UINT32_MAX;	*/		UINT32_MAX,
						/* bool									mbComputeShaderWritable = false;	*/		false,
					};

					pipeline.maAttachments.push_back(attachmentInfo);

				}	// imgui pass

				++iJobIndex;
			}

			// create input and output mapping
			for(auto& renderJob : maRenderJobs)
			{
				for(uint32_t i = 0; i < renderJob.maAttachmentInfo.size(); i++)
				{
					if(renderJob.maAttachmentInfo[i].mType == AttachmentType::TextureOut || 
						renderJob.maAttachmentInfo[i].mType == AttachmentType::BufferOut)
					{
						renderJob.maiOutputAttachmentMapping.push_back(i);
					}
					else if(renderJob.maAttachmentInfo[i].mType == AttachmentType::TextureIn || 
						renderJob.maAttachmentInfo[i].mType == AttachmentType::BufferIn)
					{
						renderJob.maiInputAttachmentMapping.push_back(i);
					}
					else if(renderJob.maAttachmentInfo[i].mType == AttachmentType::BufferInOut)
					{
						renderJob.maiOutputAttachmentMapping.push_back(i);
						renderJob.maiInputAttachmentMapping.push_back(i);
					}
					else if(renderJob.maAttachmentInfo[i].mType == AttachmentType::TextureInOut)
					{
						renderJob.maiOutputAttachmentMapping.push_back(i);
						renderJob.maiInputAttachmentMapping.push_back(i);
					}
				}

				// attach copy input to output for later creation
				if(renderJob.mType == JobType::Copy)
				{
					if(renderJob.maiOutputAttachmentMapping.size() != renderJob.maiInputAttachmentMapping.size())
					{
						std::vector<uint32_t> aiNoLinks;
						if(renderJob.maiInputAttachmentMapping.size() > renderJob.maiOutputAttachmentMapping.size())
						{
							for(uint32_t iInput = 0; iInput < renderJob.maiInputAttachmentMapping.size(); iInput++)
							{
								auto const& inputAttachment = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInput]];
								bool bFound = false;
								for(uint32_t iOutput = 0; iOutput < renderJob.maiOutputAttachmentMapping.size(); iOutput++)
								{
									auto const& outputAttachment = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutput]];
									if(outputAttachment.mParentName == inputAttachment.mName &&
										outputAttachment.mParentJobName == inputAttachment.mParentJobName)
									{
										bFound = true;
										break;
									}
								}

								if(!bFound)
								{
									aiNoLinks.push_back(renderJob.maiInputAttachmentMapping[iInput]);
								}
							}

							for(uint32_t i = 0; i < static_cast<uint32_t>(aiNoLinks.size()); i++)
							{
								DEBUG_PRINTF("No output link name: \"%s\" parent job: \"%s\"\n",
									renderJob.maAttachmentInfo[aiNoLinks[i]].mName.c_str(),
									renderJob.maAttachmentInfo[aiNoLinks[i]].mParentJobName.c_str());
							}
						}
						else
						{
							for(uint32_t iOutput = 0; iOutput < renderJob.maiInputAttachmentMapping.size(); iOutput++)
							{
								auto const& outputAttachment = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutput]];
								bool bFound = false;
								for(uint32_t iInput = 0; iInput < renderJob.maiInputAttachmentMapping.size(); iInput++)
								{
									auto const& inputAttachment = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInput]];
									if(inputAttachment.mParentName == outputAttachment.mName &&
										inputAttachment.mParentJobName == outputAttachment.mParentJobName)
									{
										bFound = true;
										break;
									}
								}

								if(!bFound)
								{
									aiNoLinks.push_back(renderJob.maiOutputAttachmentMapping[iOutput]);
								}
							}

							for(uint32_t i = 0; i < static_cast<uint32_t>(aiNoLinks.size()); i++)
							{
								DEBUG_PRINTF("No input link name: \"%s\" parent job: \"%s\"\n",
									renderJob.maAttachmentInfo[aiNoLinks[i]].mName.c_str(),
									renderJob.maAttachmentInfo[aiNoLinks[i]].mParentJobName.c_str());
							}
						}
					}

					WTFASSERT(renderJob.maiOutputAttachmentMapping.size() == renderJob.maiInputAttachmentMapping.size(), "copy render job \"%s\" input and output attachment mapping size need to match, they're %d to %d",
						renderJob.mName.c_str(),
						renderJob.maiInputAttachmentMapping.size(),
						renderJob.maiOutputAttachmentMapping.size());

				}	// if render job type == Copy
			
			}	// for render job in render jobs

			// build dimension scale dictionary for output attachments that are not copy jobs
			std::map<std::string, std::pair<float, float>> aAttachmentScales;
			for(auto& renderJob : maRenderJobs)
			{
				if(renderJob.mType != JobType::Copy)
				{
					RenderDriver::Common::PipelineInfo& pipelineInfo = maPipelines[renderJob.miPipelineIndex];
					for(uint32_t i = 0; i < renderJob.maiOutputAttachmentMapping.size(); i++)
					{
						uint32_t iAttachmentIndex = renderJob.maiOutputAttachmentMapping[i];
						std::string entryName = renderJob.mName + "-" + renderJob.maAttachmentInfo[iAttachmentIndex].mName;

						WTFASSERT(
							renderJob.maAttachmentInfo[iAttachmentIndex].mName == pipelineInfo.maAttachments[iAttachmentIndex].mName,
							"Not the same attachment %s %s",
							renderJob.maAttachmentInfo[iAttachmentIndex].mName.c_str(), pipelineInfo.maAttachments[iAttachmentIndex].mName.c_str());

						aAttachmentScales[entryName] = std::make_pair(pipelineInfo.maAttachments[iAttachmentIndex].mfScaleWidth, pipelineInfo.maAttachments[iAttachmentIndex].mfScaleHeight);
					}
				}
			}

			// match up the out attachment scales of the copy jobs
			for(auto& renderJob : maRenderJobs)
			{
				if(renderJob.mType == JobType::Copy)
				{
					RenderDriver::Common::PipelineInfo& pipelineInfo = maPipelines[renderJob.miPipelineIndex];
					for(uint32_t i = 0; i < renderJob.maiOutputAttachmentMapping.size(); i++)
					{
						uint32_t iInputAttachmentIndex = renderJob.maiInputAttachmentMapping[i];
						uint32_t iOutputAttachmentIndex = renderJob.maiOutputAttachmentMapping[i];
						WTFASSERT(
							renderJob.maAttachmentInfo[iOutputAttachmentIndex].mName == pipelineInfo.maAttachments[iOutputAttachmentIndex].mName,
							"Not the same attachment %s %s",
							renderJob.maAttachmentInfo[iOutputAttachmentIndex].mName.c_str(), pipelineInfo.maAttachments[iOutputAttachmentIndex].mName.c_str());

						std::string entryName = pipelineInfo.maAttachments[iInputAttachmentIndex].mParentRenderJobName + "-" + pipelineInfo.maAttachments[iInputAttachmentIndex].mName;

						auto iter = aAttachmentScales.find(entryName);
						WTFASSERT(iter != aAttachmentScales.end(), "render job \"%s\" can\'t find entry \"%s\"", renderJob.mName.c_str(), entryName.c_str());
						pipelineInfo.maAttachments[iOutputAttachmentIndex].mfScaleWidth = iter->second.first;
						pipelineInfo.maAttachments[iOutputAttachmentIndex].mfScaleHeight = iter->second.second;

					}
				}
			}

			// create render targets and attach input and out for the render jobs
			for(auto& renderJob : maRenderJobs)
			{
				uint32_t iPipelineIndex = renderJob.miPipelineIndex;

				// render job output render targets 
				RenderDriver::Common::PipelineInfo& pipelineInfo = maPipelines[iPipelineIndex];
				for(uint32_t i = 0; i < pipelineInfo.maAttachments.size(); i++)
				{
					if(pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::TextureOutput)
					{
						// create output render target 

						renderJob.maOutputRenderTargetAttachments.resize(renderJob.maOutputRenderTargetAttachments.size() + 1);
						renderJob.maOutputRenderTargetColorHeaps.resize(renderJob.maOutputRenderTargetColorHeaps.size() + 1);
						renderJob.maOutputRenderTargetDepthHeaps.resize(renderJob.maOutputRenderTargetDepthHeaps.size() + 1);

						uint32_t iRenderTargetWidth = static_cast<uint32_t>(static_cast<float>(iScreenWidth) * pipelineInfo.maAttachments[i].mfScaleWidth);
						uint32_t iRenderTargetHeight = static_cast<uint32_t>(static_cast<float>(iScreenHeight) * pipelineInfo.maAttachments[i].mfScaleHeight);

						// writable texture for compute job
						if(renderJob.mType == JobType::Compute)
						{
							pipelineInfo.maAttachments[i].mbComputeShaderWritable = true;
						}

						// shader resource name for this output attachment
						uint32_t iShaderResource = 0;
						SerializeUtils::Common::ShaderResourceInfo const* pShaderResource = nullptr;
						for(iShaderResource = 0; iShaderResource < static_cast<uint32_t>(renderJob.maShaderResourceInfo.size()); iShaderResource++)
						{
							if(renderJob.maShaderResourceInfo[iShaderResource].mName == renderJob.maAttachmentInfo[i].mShaderResourceName)
							{
								pShaderResource = &renderJob.maShaderResourceInfo[iShaderResource];
								break;
							}
						}

						// in the shader descriptor
						if(pShaderResource)
						{
							renderJob.maOutputRenderTargetColorHeaps[renderJob.maOutputRenderTargetColorHeaps.size() - 1] = renderJob.maDescriptorHeapHandles[0];
							pipelineInfo.maAttachments[i].miShaderResourceIndex = iShaderResource;
						}
						else
						{
							if(renderJob.mType != JobType::Graphics)
							{
								DEBUG_PRINTF("!!! WARNING: no shader resource associated with \"%s\"\n",
									pipelineInfo.maAttachments[i].mName.c_str());
							}
						}

						if(renderJob.mType == JobType::Copy)
						{
							auto const& outputAttachmentInfo = renderJob.maAttachmentInfo[i];

							std::string parentName = outputAttachmentInfo.mParentName;
							std::string parentJobName = outputAttachmentInfo.mParentJobName;

							// get the correct attachment scale by getting the parent attachment info
							for(uint32_t iParentJob = 0; iParentJob < 4; iParentJob++)
							{
								// find parent job
								auto parentJobIter = std::find_if(
									maRenderJobs.begin(),
									maRenderJobs.end(),
									[parentJobName](auto const& renderJobInfo)
									{
										return renderJobInfo.mName == parentJobName;
									});
								WTFASSERT(parentJobIter != maRenderJobs.end(), "Can\'t find render job \"%s\"", parentJobName.c_str());

								// parent pipeline and input attachment 
								auto const& parentPipeline = maPipelines[parentJobIter->miPipelineIndex];
								auto parentAttachmentIter = std::find_if(
									parentPipeline.maAttachments.begin(),
									parentPipeline.maAttachments.end(),
									[parentName](AttachmentInfo const& attachmentInfo)
									{
										return attachmentInfo.mName == parentName;
									});
								WTFASSERT(parentAttachmentIter != parentPipeline.maAttachments.end(), "Can\'t find attachment \"%s\"", outputAttachmentInfo.mParentName.c_str());

								// "This" as parent render job = attachment originated from this pass, else go up a parent to get the right scale
								if(parentAttachmentIter->mParentRenderJobName == "This")
								{
									// originated from this render job, return the scale
									iRenderTargetWidth = static_cast<uint32_t>(static_cast<float>(iScreenWidth) * parentAttachmentIter->mfScaleWidth);
									iRenderTargetHeight = static_cast<uint32_t>(static_cast<float>(iScreenHeight) * parentAttachmentIter->mfScaleHeight);

									WTFASSERT(iRenderTargetWidth > 0, "Invalid width %d scale %.4f", iRenderTargetWidth, parentAttachmentIter->mfScaleWidth);
									WTFASSERT(iRenderTargetHeight > 0, "Invalid height %d scale %.4f", iRenderTargetWidth, parentAttachmentIter->mfScaleHeight);

									break;
								}
								else
								{
									// "up" to parent job
									parentJobName = parentAttachmentIter->mParentRenderJobName;
									parentName = parentAttachmentIter->mName;
								}

							}

							// create attachment image
							initAttachmentImage(
								renderJob.maOutputRenderTargetAttachments[renderJob.maOutputRenderTargetAttachments.size() - 1],
								pipelineInfo.maAttachments[i],
								iRenderTargetWidth,
								iRenderTargetHeight,
								device);
						}
						else
						{
							if(renderJob.mPassType == PassType::FullTriangle)
							{
								pipelineInfo.maAttachments[i].mbHasDepthStencil = false;
							}

							float afDefaultClearColor[4] = { 0.0f, 0.0f, 0.3f, 0.0f };
							initRenderTarget(
								renderJob.maOutputRenderTargetAttachments[renderJob.maOutputRenderTargetAttachments.size() - 1],
								renderJob.maOutputRenderTargetColorHeaps[renderJob.maOutputRenderTargetColorHeaps.size() - 1],
								renderJob.maOutputRenderTargetDepthHeaps[renderJob.maOutputRenderTargetDepthHeaps.size() - 1],
								pipelineInfo.maAttachments[i],
								iRenderTargetWidth,
								iRenderTargetHeight,
								(renderJob.maAttachmentInfo[i].mbHasClearColor == false) ? afDefaultClearColor : renderJob.maAttachmentInfo[i].mafClearColor,
								device);
						}

						// image views for the remaining descriptor heaps
						if(pShaderResource)
						{
							for(uint32_t iDescriptorHeap = 1; iDescriptorHeap < 3; iDescriptorHeap++)
							{
								PLATFORM_OBJECT_HANDLE renderTargetHandle = renderJob.maOutputRenderTargetAttachments[renderJob.maOutputRenderTargetAttachments.size() - 1];
								RenderDriver::Common::CDescriptorHeap* pDescriptorHeap = mapDescriptorHeaps[renderJob.maDescriptorHeapHandles[iDescriptorHeap]].get();

								RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
								imageViewDesc.mpDescriptorHeap = pDescriptorHeap;
								imageViewDesc.mFormat = pipelineInfo.maAttachments[i].mFormat;
								imageViewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
								imageViewDesc.mpImage = mapRenderTargets[renderTargetHandle]->getImage().get();
								imageViewDesc.mViewType = RenderDriver::Common::ResourceViewType::RenderTargetView;
								imageViewDesc.miShaderResourceIndex = iShaderResource;
								imageViewDesc.mViewType = pipelineInfo.maAttachments[i].mbComputeShaderWritable ? RenderDriver::Common::ResourceViewType::UnorderedAccessView : RenderDriver::Common::ResourceViewType::ShaderResourceView;
								mapRenderTargets[renderTargetHandle]->createImageView(imageViewDesc, device);
							}
						}

						WTFASSERT(renderJob.maAttachmentInfo[i].mName == pipelineInfo.maAttachments[i].mName, "Not the same attachment for render job info and pipeline");
						renderJob.maAttachmentInfo[i].mAttachmentHandle = renderJob.maOutputRenderTargetAttachments[renderJob.maOutputRenderTargetAttachments.size() - 1];

						// looks for the shader resource referenced by this attachment
						for(auto& shaderResource : renderJob.maShaderResourceInfo)
						{
							if(shaderResource.mName == renderJob.maAttachmentInfo[i].mShaderResourceName)
							{
								auto const& renderTarget = this->getRenderTarget(renderJob.maAttachmentInfo[i].mAttachmentHandle);
								shaderResource.mExternalResource.mbSkipCreation = true;
								shaderResource.mExternalResource.mpImage = renderTarget->getImage().get();
								shaderResource.mExternalResource.mRenderTargetHandle = renderJob.maAttachmentInfo[i].mAttachmentHandle;
								shaderResource.mExternalResource.mShaderResourceViewDimension = RenderDriver::Common::ShaderResourceViewDimension::Texture2D;
								shaderResource.mExternalResource.miNumImages = 1;
								shaderResource.maHandles[0] = shaderResource.mExternalResource.mpImage->getHandle();
							}
						}

						DEBUG_PRINTF("Created render target texture: \"%s\" for render job: \"%s\" (%d x %d) handle: %lld\n",
							renderJob.maAttachmentInfo[i].mName.c_str(),
							renderJob.mName.c_str(),
							iRenderTargetWidth,
							iRenderTargetHeight,
							renderJob.maAttachmentInfo[i].mAttachmentHandle);

					}	// if pipeline attachment type == texture output

					else if(pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::BufferOutput ||
						pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::BufferInputOutput)
					{
						// create output buffer for render target to copy into

						// check for the same input and output buffer
						bool bSameInputOutputBuffer = false;
						PLATFORM_OBJECT_HANDLE outputAttachmentHandle = UINT64_MAX;
						auto const& attachment = pipelineInfo.maAttachments[i];
						if(attachment.mParentRenderJobName == "This")
						{
							for(uint32_t iCheckAttachment = 0; iCheckAttachment < static_cast<uint32_t>(pipelineInfo.maAttachments.size()); iCheckAttachment++)
							{
								auto const& checkAttachment = pipelineInfo.maAttachments[iCheckAttachment];
								if(attachment.mName == checkAttachment.mName && checkAttachment.mType == AttachmentTypeBits::BufferInput)
								{

									for(uint32_t iRenderJob = 0; iRenderJob < static_cast<uint32_t>(maRenderJobs.size()); iRenderJob++)
									{
										if(maRenderJobs[iRenderJob].mName == renderJob.maAttachmentInfo[iCheckAttachment].mParentJobName)
										{
											for(uint32_t iParentAttachment = 0; iParentAttachment < static_cast<uint32_t>(maRenderJobs[iRenderJob].maAttachmentInfo.size()); iParentAttachment++)
											{
												if(maRenderJobs[iRenderJob].maAttachmentInfo[iParentAttachment].mName == attachment.mName)
												{
													outputAttachmentHandle = maRenderJobs[iRenderJob].maAttachmentInfo[iParentAttachment].mAttachmentHandle;
													break;
												}
											}

											break;
										}
									}

									WTFASSERT(outputAttachmentHandle > 0, "Didn\'t find parent for attachment %s", attachment.mName.c_str());
									break;
								}
							}
						}

						renderJob.maOutputRenderTargetAttachments.resize(renderJob.maOutputRenderTargetAttachments.size() + 1);
						renderJob.maOutputRenderTargetAttachmentBuffers.resize(renderJob.maOutputRenderTargetAttachments.size());

						// attachment size
						uint32_t iSize = static_cast<uint32_t>(static_cast<float>(iScreenWidth) * pipelineInfo.maAttachments[i].mfScaleWidth) *
							static_cast<uint32_t>(static_cast<float>(iScreenHeight) * pipelineInfo.maAttachments[i].mfScaleHeight) *
							4 *
							sizeof(float);
						if(renderJob.maAttachmentInfo[i].miDataSize > 0 && renderJob.maAttachmentInfo[i].miDataSize != UINT32_MAX)
						{
							iSize = renderJob.maAttachmentInfo[i].miDataSize;
						}

						// get the shader index of the attachment
						auto iter = std::find_if(
							renderJob.maShaderResourceInfo.begin(),
							renderJob.maShaderResourceInfo.end(),
							[&](auto const& shaderResource)
							{
								return shaderResource.mName == renderJob.maAttachmentInfo[i].mShaderResourceName;
							});
						uint32_t iShaderResourceIndex = (iter == renderJob.maShaderResourceInfo.end()) ? UINT32_MAX : static_cast<uint32_t>(std::distance(renderJob.maShaderResourceInfo.begin(), iter));

						// attachment info
						AttachmentInfo attachmentInfo;
						attachmentInfo.mbComputeShaderWritable = true;
						attachmentInfo.mFormat = renderJob.maAttachmentInfo[i].mFormat;
						attachmentInfo.mfScaleHeight = static_cast<float>(iScreenHeight) * pipelineInfo.maAttachments[i].mfScaleHeight;
						attachmentInfo.mfScaleWidth = static_cast<float>(iScreenWidth) * pipelineInfo.maAttachments[i].mfScaleWidth;
						attachmentInfo.miShaderResourceIndex = iShaderResourceIndex;
						attachmentInfo.mType = pipelineInfo.maAttachments[i].mType; //AttachmentTypeBits::BufferOutput;
						attachmentInfo.mName = renderJob.maAttachmentInfo[i].mName;
						attachmentInfo.miDataSize = renderJob.maAttachmentInfo[i].miDataSize;
						attachmentInfo.mbExternalData = renderJob.maAttachmentInfo[i].mbExternalData;

						renderJob.maAttachmentInfo[i].miDataSize = iSize;

						// create new render target buffer only if input and output are different, ie. input-output buffer
						PLATFORM_OBJECT_HANDLE handle = UINT64_MAX;
						if(outputAttachmentHandle != UINT64_MAX)
						{
							handle = outputAttachmentHandle;
						}
						else
						{
							if(attachmentInfo.mbExternalData == false)
							{
								// new render target buffer
								initRenderTargetBuffer(
									handle,
									attachmentInfo,
									static_cast<uint32_t>(attachmentInfo.mfScaleWidth),
									static_cast<uint32_t>(attachmentInfo.mfScaleHeight),
									iPipelineIndex,
									device);
							}

							if(handle != UINT64_MAX)
							{
								mapRenderTargets[handle] = nullptr;
							}
						}

						// look for the output buffer shader resource
						for(auto& shaderResource : renderJob.maShaderResourceInfo)
						{
							if(shaderResource.mName == renderJob.maAttachmentInfo[i].mShaderResourceName)
							{
								// skip default creation, will be created externally
								shaderResource.mType = 
									(renderJob.maAttachmentInfo[i].mType == AttachmentType::BufferInOut) ? 
									ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT : 
									ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
								shaderResource.mExternalResource.mbSkipCreation = true;
								shaderResource.mExternalResource.mShaderResourceViewDimension = RenderDriver::Common::ShaderResourceViewDimension::Buffer;

								// set buffer to attachment
								for(auto const& buffer : mapBuffers)
								{
									if(buffer.second->getID() == attachment.mName)
									{
										shaderResource.mExternalResource.mpBuffer = buffer.second.get();
										shaderResource.maHandles[0] = shaderResource.mExternalResource.mpBuffer->getHandle();
										handle = shaderResource.maHandles[0];
										break;
									}
								}

							}
						}

						renderJob.maOutputRenderTargetAttachments[renderJob.maOutputRenderTargetAttachments.size() - 1] = handle;
						renderJob.maOutputRenderTargetAttachmentBuffers[renderJob.maOutputRenderTargetAttachmentBuffers.size() - 1] = handle;

						WTFASSERT(renderJob.maAttachmentInfo[i].mName == pipelineInfo.maAttachments[i].mName, "Not the same attachment for render job info and pipeline");
						renderJob.maAttachmentInfo[i].mAttachmentHandle = handle;

						DEBUG_PRINTF("Created render target buffer: \"%s\" for render job: \"%s\" shader resource name: \"%s\" size: %d handle: %lld\n",
							renderJob.maAttachmentInfo[i].mName.c_str(),
							renderJob.mName.c_str(),
							renderJob.maAttachmentInfo[i].mShaderResourceName.c_str(),
							iSize,
							handle);
					}
					else if(pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::TextureInput)
					{
						float fScaleWidth = 1.0f;
						float fScaleHeight = 1.0f;

						// look for buffer out from parent render job attachments
						bool bNeedsSeparateImage = false;
						std::string const& parentRenderJobName = pipelineInfo.maAttachments[i].mParentRenderJobName;
						for(auto const& parentRenderJob : maRenderJobs)
						{
							if(parentRenderJob.mName == parentRenderJobName)
							{
								for(uint32_t iParentAttachment = 0; iParentAttachment < static_cast<uint32_t>(parentRenderJob.maAttachmentInfo.size()); iParentAttachment++)
								{
									auto const& attachmentInfo = parentRenderJob.maAttachmentInfo[iParentAttachment];
									if(attachmentInfo.mName == pipelineInfo.maAttachments[i].mName)
									{
										if(attachmentInfo.mType == AttachmentType::BufferOut)
										{
											uint32_t iParentPipelineIndex = parentRenderJob.miPipelineIndex;
											RenderDriver::Common::PipelineInfo const& parentPipelineInfo = maPipelines[iParentPipelineIndex];

											fScaleWidth = parentPipelineInfo.maAttachments[iParentAttachment].mfScaleHeight;
											fScaleHeight = parentPipelineInfo.maAttachments[iParentAttachment].mfScaleWidth;
											bNeedsSeparateImage = true;

										}

										break;
									}
								}

								break;
							}
						}

						// create texture for copying buffer attachment
						if(bNeedsSeparateImage)
						{
							// get the shader index of the attachment
							//auto iter = std::find_if(
							//	renderJob.maShaderResourceInfo.begin(),
							//	renderJob.maShaderResourceInfo.end(),
							//	[&](auto const& shaderResource)
							//	{
							//		return shaderResource.mName == attachment.mName;
							//	});
							//uint32_t iShaderResourceIndex = static_cast<uint32_t>(std::distance(renderJob.maShaderResourceInfo.begin(), iter));

							AttachmentInfo attachmentInfo;
							attachmentInfo.mbComputeShaderWritable = true;
							attachmentInfo.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
							attachmentInfo.mfScaleHeight = static_cast<float>(iScreenHeight) * fScaleHeight;
							attachmentInfo.mfScaleWidth = static_cast<float>(iScreenWidth) * fScaleWidth;
							attachmentInfo.miShaderResourceIndex = i;
							attachmentInfo.mType = AttachmentTypeBits::BufferOutput;
							attachmentInfo.mName = renderJob.maAttachmentInfo[i].mName;

							PLATFORM_OBJECT_HANDLE handle;
							initAttachmentImage(
								handle,
								attachmentInfo,
								static_cast<uint32_t>(attachmentInfo.mfScaleWidth),
								static_cast<uint32_t>(attachmentInfo.mfScaleHeight),
								device);

							renderJob.maInputRenderTargetAttachmentBufferAsImages.push_back(handle);
						}
					}

				}	// for i = 0 to num attachments

				platformCreateFrameBuffer(renderJob);

				++iJobIndex;
			}

			// link input render targets to output
			for(auto& renderJob : maRenderJobs)
			{
				DEBUG_PRINTF("\n\n *** RENDER JOB: %s ***\n", renderJob.mName.c_str());

				// render job input render targets
				uint32_t iCreatedImageAsBufferAttachments = 0;
				RenderDriver::Common::PipelineInfo& pipelineInfo = maPipelines[renderJob.miPipelineIndex];
				for(uint32_t iAttachmentIndex = 0; iAttachmentIndex < pipelineInfo.maAttachments.size(); iAttachmentIndex++)
				{
					if(pipelineInfo.maAttachments[iAttachmentIndex].mType == AttachmentTypeBits::TextureInput ||
						pipelineInfo.maAttachments[iAttachmentIndex].mType == AttachmentTypeBits::TextureInputOutput ||
						pipelineInfo.maAttachments[iAttachmentIndex].mType == AttachmentTypeBits::BufferInput || 
						pipelineInfo.maAttachments[iAttachmentIndex].mType == AttachmentTypeBits::BufferInputOutput)
					{
						// link up parent output render target to input render target

						std::string const& attachmentName = pipelineInfo.maAttachments[iAttachmentIndex].mName;
						std::string const& parentRenderJobName = pipelineInfo.maAttachments[iAttachmentIndex].mParentRenderJobName;

						uint32_t iNumInputRenderTargets = static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size());
						renderJob.maInputRenderTargetAttachments.resize(iNumInputRenderTargets + 1);

						// look for attachment info in render job
						uint32_t iAttachmentIndex = 0;
						for(auto& attachmentInfo : renderJob.maAttachmentInfo)
						{
							// check if it's not attached and is the right attachment
							if(attachmentInfo.mAttachmentHandle == 0 &&
								attachmentInfo.mName == attachmentName)
							{
								// reference to external buffer
								if(parentRenderJobName == "This" && 
									(attachmentInfo.mType == AttachmentType::BufferIn || attachmentInfo.mType == AttachmentType::BufferInOut || 
									 attachmentInfo.mType == AttachmentType::TextureIn || attachmentInfo.mType == AttachmentType::TextureInOut))
								{
									auto iter = std::find_if(
										renderJob.maShaderResourceInfo.begin(),
										renderJob.maShaderResourceInfo.end(),
										[attachmentInfo](SerializeUtils::Common::ShaderResourceInfo const& checkShaderResourceInfo)
										{
											return checkShaderResourceInfo.mName == attachmentInfo.mShaderResourceName;
										});
									if(iter != renderJob.maShaderResourceInfo.end())
									{
										SerializeUtils::Common::ShaderResourceInfo const& shaderResource = *iter;
										uint32_t iShaderResourceIndex = static_cast<uint32_t>(std::distance(renderJob.maShaderResourceInfo.begin(), iter));
										if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN ||
										   shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT)
										{
											setBufferView(
												renderJob,
												shaderResource.mExternalResource.mpBuffer,
												iShaderResourceIndex,
												static_cast<uint32_t>(shaderResource.mExternalResource.mpBuffer->getDescriptor().miSize),
												device);

											DEBUG_PRINTF("\tinput job: %s create attachment buffer view input index: %d size: %d\n",
												renderJob.mName.c_str(),
												iShaderResourceIndex,
												static_cast<uint32_t>(shaderResource.mExternalResource.mpBuffer->getDescriptor().miSize));
										}
										else
										{
											WTFASSERT(0, "Need to support image view here");
										}
									}

									continue;
								}
								

								// look for parent render job specified in the attachment info
								bool bFound = false;
								for(auto& parentRenderJob : maRenderJobs)
								{
									if(parentRenderJob.mName == parentRenderJobName)
									{
										// look for name of attachment from parent render job
										uint32_t iParentOutputAttachment = 0;
										auto const& parentPipelineInfo = maPipelines[parentRenderJob.miPipelineIndex];
										for(uint32_t iParentAttachment = 0; iParentAttachment < static_cast<uint32_t>(parentPipelineInfo.maAttachments.size()); iParentAttachment++)
										{
											// matches parent job name and is an output attachment
											if(parentPipelineInfo.maAttachments[iParentAttachment].mName == attachmentInfo.mName &&
												(parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::TextureOutput ||
													parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::TextureInputOutput ||
													parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::BufferOutput || 
													parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::BufferInputOutput))
											{
												if(parentRenderJob.maOutputRenderTargetAttachmentBuffers.size() > iParentOutputAttachment && parentRenderJob.maOutputRenderTargetAttachmentBuffers[iParentOutputAttachment] > 0)
												{
													renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets] = parentRenderJob.maOutputRenderTargetAttachmentBuffers[iParentOutputAttachment];
													attachmentInfo.mAttachmentHandle = renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets];

													// from buffer to image
													if(parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::BufferOutput)
													{
														renderJob.maInputRenderTargetAttachmentBuffers.push_back(parentRenderJob.maOutputRenderTargetAttachmentBuffers[iParentOutputAttachment]);

														if(pipelineInfo.maAttachments[iAttachmentIndex].mType == AttachmentTypeBits::TextureInput)
														{
															renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets] = renderJob.maInputRenderTargetAttachmentBufferAsImages[iCreatedImageAsBufferAttachments];
															attachmentInfo.mAttachmentHandle = renderJob.maInputRenderTargetAttachmentBufferAsImages[iNumInputRenderTargets];
															++iCreatedImageAsBufferAttachments;
														}
													}
												}
												else
												{
													renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets] = parentRenderJob.maOutputRenderTargetAttachments[iParentOutputAttachment];
													attachmentInfo.mAttachmentHandle = renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets];

													if(parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::BufferOutput)
													{
														renderJob.maInputRenderTargetAttachmentBuffers.push_back(parentRenderJob.maOutputRenderTargetAttachments[iParentOutputAttachment]);
													}
													
													// set the output render target to parent's output render target
													// also the color and depth heaps
													if(attachmentInfo.mType == AttachmentType::TextureInOut)
													{
														renderJob.maOutputRenderTargetAttachments.push_back(parentRenderJob.maOutputRenderTargetAttachments[iParentOutputAttachment]);
														renderJob.maOutputRenderTargetColorHeaps.push_back(parentRenderJob.maOutputRenderTargetColorHeaps[iParentOutputAttachment]);
														renderJob.maOutputRenderTargetDepthHeaps.push_back(parentRenderJob.maOutputRenderTargetDepthHeaps[iParentOutputAttachment]);
													}
												}

												// keep track of parent and children render jobs
												{
													Render::Common::RenderJobInfo const* pParentRenderJob = &parentRenderJob;
													Render::Common::RenderJobInfo const* pRenderJob = &renderJob;
													auto parentIter = std::find_if(
														renderJob.mapParents.begin(),
														renderJob.mapParents.end(),
														[pParentRenderJob](RenderJobInfo const* pEntry)
														{
															return pEntry == pParentRenderJob;
														});

													auto iter = std::find_if(
														parentRenderJob.mapChildren.begin(),
														parentRenderJob.mapChildren.end(),
														[pRenderJob](RenderJobInfo const* pEntry)
														{
															return pEntry == pRenderJob;
														});

													if(parentIter == renderJob.mapParents.end())
													{
														renderJob.mapParents.push_back(pParentRenderJob);
													}

													if(iter == parentRenderJob.mapChildren.end())
													{
														parentRenderJob.mapChildren.push_back(pRenderJob);
													}
												}	// parent / children render job info

												DEBUG_PRINTF("\tlink attachment input job: \"%s\" (%d) to output job: \"%s\" attachment: \"%s\" (%d) shader resource: \"%s\"\n",
													renderJob.mName.c_str(),
													iNumInputRenderTargets,
													parentRenderJob.mName.c_str(),
													parentPipelineInfo.maAttachments[iParentAttachment].mName.c_str(),
													iParentOutputAttachment,
													attachmentInfo.mShaderResourceName.c_str());

												if(attachmentInfo.mShaderResourceName == "readOnlyBuffer" || attachmentInfo.mShaderResourceName == "readWriteBuffer")
												{
													bFound = true;
													continue;
												}

												// set the shader resource to use parent attachment image
												if(attachmentInfo.mShaderResourceName != "")
												{
													uint32_t iShaderResourceIndex = 0;
													bool bFoundShaderResource = false;
													for(auto& shaderResource : renderJob.maShaderResourceInfo)
													{
														if(shaderResource.mName == attachmentInfo.mShaderResourceName)
														{
															WTFASSERT(
																attachmentInfo.mType == AttachmentType::TextureIn || 
																attachmentInfo.mType == AttachmentType::BufferIn || 
																attachmentInfo.mType == AttachmentType::TextureInOut || 
																attachmentInfo.mType == AttachmentType::BufferInOut, 
																"Invalid attachment type to create an image view");

															// set the input render target image to the parent's output render target image
															RenderDriver::Common::ImageDescriptor imageDescriptor = {};
															bool bExternalCreatedImage = false;
															if(parentRenderJob.maOutputRenderTargetAttachmentBuffers.size() > iParentOutputAttachment && parentRenderJob.maOutputRenderTargetAttachmentBuffers[iParentAttachment] > 0)
															{
																if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
																{
																	// external buffer, created from previous step
																	RenderDriver::Common::CImage* pImage = nullptr;
																	if(!isImage(renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets]))
																	{
																		pImage = getRenderTarget(renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets])->getImage().get();
																	}
																	else
																	{
																		pImage = getImage(renderJob.maInputRenderTargetAttachments[iNumInputRenderTargets]).get();
																	}
																	shaderResource.mExternalResource.mpImage = pImage;

																	// set to image view
																	RenderDriver::Common::ImageDescriptor const& imageDesc = pImage->getDescriptor();
																	imageDescriptor.mFormat = imageDesc.mFormat;
																	imageDescriptor.miWidth = imageDesc.miWidth;
																	imageDescriptor.miHeight = imageDesc.miHeight;

																	bExternalCreatedImage = true;
																}
																else
																{
																	RenderDriver::Common::CBuffer* pBuffer = shaderResource.mExternalResource.mpBuffer;
																	if(shaderResource.mExternalResource.mpBuffer == nullptr)
																	{
																		// external buffer, created from previous step
																		pBuffer = platformGetRenderTargetBuffer(parentRenderJob.maOutputRenderTargetAttachmentBuffers[iParentOutputAttachment]).get();
																		shaderResource.mExternalResource.mpBuffer = pBuffer;
																	}

																	// set to buffer view
																	RenderDriver::Common::BufferDescriptor const& bufferDesc = pBuffer->getDescriptor();
																	imageDescriptor.mFormat = bufferDesc.mImageViewFormat;
																	imageDescriptor.miWidth = bufferDesc.miImageViewWidth;
																	imageDescriptor.miHeight = bufferDesc.miImageViewHeight;
																}
															}
															else
															{
																WTFASSERT(parentRenderJob.maOutputRenderTargetAttachments[iParentOutputAttachment], "Not valid output render target for parent job: \"%s\"", parentRenderJob.mName.c_str());
																if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN)
																{
																	shaderResource.mExternalResource.mpBuffer = platformGetRenderTargetBuffer(parentRenderJob.maOutputRenderTargetAttachments[iParentOutputAttachment]).get();
																}
																else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN || shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT)
																{
																	if(parentRenderJob.mType == JobType::Copy)
																	{
																		shaderResource.mExternalResource.mpImage = mapImages[parentRenderJob.maOutputRenderTargetAttachments[iParentOutputAttachment]].get();
																		bExternalCreatedImage = true;
																	}
																	else
																	{
																		shaderResource.mExternalResource.mpImage = platformGetRenderTargetImage(parentRenderJob.maOutputRenderTargetAttachments[iParentOutputAttachment]).get();
																	}

																	imageDescriptor = shaderResource.mExternalResource.mpImage->getDescriptor();
																}
															}

															// image view for this attachment in the descriptor heap
															if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN || 
															   shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN)
															{
																uint32_t iNumComponents = 1;
																uint32_t iBaseComponentSize = 4;
																uint32_t iDataSize = 0;
																if(shaderResource.mExternalResource.mpBuffer)
																{
																	auto const& desc = shaderResource.mExternalResource.mpBuffer->getDescriptor();
																	iNumComponents = SerializeUtils::Common::getNumComponents(desc.mFormat);
																	iBaseComponentSize = SerializeUtils::Common::getBaseComponentSize(desc.mFormat);
																	iDataSize = static_cast<uint32_t>(desc.miSize);
																}
																else if(shaderResource.mExternalResource.mpImage)
																{
																	auto const& desc = shaderResource.mExternalResource.mpImage->getDescriptor();
																	iNumComponents = SerializeUtils::Common::getNumComponents(desc.mFormat);
																	iBaseComponentSize = SerializeUtils::Common::getBaseComponentSize(desc.mFormat);
																	iDataSize = imageDescriptor.miWidth * imageDescriptor.miHeight * iNumComponents * iBaseComponentSize;
																}
																else
																{
																	assert(0);
																}

																createAttachmentView(
																	renderJob,
																	iNumInputRenderTargets,
																	iShaderResourceIndex,
																	iDataSize,
																	shaderResource.mExternalResource.mpBuffer != nullptr,
																	bExternalCreatedImage,
																	attachmentInfo.mViewType,
																	device);

																DEBUG_PRINTF("\tinput job: %s create attachment image view input index: %d width: %d height: %d: num components: %d\n",
																	renderJob.mName.c_str(),
																	iNumInputRenderTargets,
																	imageDescriptor.miWidth,
																	imageDescriptor.miHeight,
																	iNumComponents);
															}
															else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT)
															{
																uint32_t iNumComponents = 1;
																uint32_t iBaseComponentSize = 4;
																auto const& desc = shaderResource.mExternalResource.mpImage->getDescriptor();
																iNumComponents = SerializeUtils::Common::getNumComponents(desc.mFormat);
																iBaseComponentSize = SerializeUtils::Common::getBaseComponentSize(desc.mFormat);

																DEBUG_PRINTF("\tcreate attachment image view: %s (%d)\n",
																	renderJob.maShaderResourceInfo[iShaderResourceIndex].mName.c_str(),
																	iShaderResourceIndex);

																createWritableAttachmentView(
																	renderJob,
																	iNumInputRenderTargets,
																	iShaderResourceIndex,
																	imageDescriptor.miWidth* imageDescriptor.miHeight* iNumComponents* iBaseComponentSize,
																	shaderResource.mExternalResource.mpBuffer != nullptr,
																	bExternalCreatedImage,
																	attachmentInfo.mViewType,
																	device);
																
																
															}
															

															bFoundShaderResource = true;

															break;
														}

														++iShaderResourceIndex;
													}

													if(!bFoundShaderResource)
													{
														DEBUG_PRINTF("\t!!! render job: %s no shader resource view (%s) for attachment (%s) !!!\n",
															renderJob.mName.c_str(),
															attachmentInfo.mShaderResourceName.c_str(),
															attachmentInfo.mName.c_str());
													}
												}

												bFound = true;
												break;
											}

											if(parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::TextureInputOutput ||
												parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::TextureOutput ||
												parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::BufferOutput ||
												parentPipelineInfo.maAttachments[iParentAttachment].mType == AttachmentTypeBits::BufferInputOutput)
											{
												++iParentOutputAttachment;
											}
										}

										break;
									}
								}

								WTFASSERT(bFound, "Can\'t find attachment:\n\"%s\"\nfor render job\n\"%s\"\nfrom parent render job\n\"%s\"",
									attachmentInfo.mName.c_str(),
									renderJob.mName.c_str(),
									parentRenderJobName.c_str());

								break;

							}	// if check render job attachment name == render job attachment name

							++iAttachmentIndex;

						}	// for attachment in potential parent render job attachments

					}	// if attachment type == input

				}	// for attachment in num attachments in pipeline

				platformUpdateOutputAttachments(renderJob);
				platformUpdateSamplers(renderJob);

				++iJobIndex;

			}	// for render job in render jobs

			// set the parent for copy jobs 
			for(uint32_t iRenderJob = 0; iRenderJob < static_cast<uint32_t>(maRenderJobs.size()); iRenderJob++)
			{
				if(maRenderJobs[iRenderJob].mType == Render::Common::JobType::Copy)
				{
					auto& renderJob = maRenderJobs[iRenderJob];
					for(uint32_t iAttachment = 0; iAttachment < static_cast<uint32_t>(renderJob.maAttachmentInfo.size()); iAttachment++)
					{
						auto const& attachment = maRenderJobs[iRenderJob].maAttachmentInfo[iAttachment];

						bool bInserted = false;
						for(uint32_t iParentJob = iRenderJob + 1; iParentJob < static_cast<uint32_t>(maRenderJobs.size()); iParentJob++)
						{
							auto& parentJob = maRenderJobs[iParentJob];
							auto iter = std::find(parentJob.mapParents.begin(), parentJob.mapParents.end(), &renderJob);
							if(iter == parentJob.mapParents.end() && attachment.mParentJobName == parentJob.mName)
							{
								parentJob.mapParents.push_back(&renderJob);
								bInserted = true;
								break;
							}
						}

					}
				}
			}

			// set additional parents
			for(uint32_t iJob = 0; iJob < static_cast<uint32_t>(maRenderJobs.size()); iJob++)
			{
				auto& renderJob = maRenderJobs[iJob];
				for(uint32_t iWait = 0; iWait < static_cast<uint32_t>(renderJob.maWaitOnJobs.size()); iWait++)
				{
					for(uint32_t iCheckJob = 0; iCheckJob < static_cast<uint32_t>(maRenderJobs.size()); iCheckJob++)
					{
						if(iJob == iCheckJob)
						{
							continue;
						}

						// check 
						auto const& checkJob = maRenderJobs[iCheckJob];
						if(checkJob.mName == renderJob.maWaitOnJobs[iWait])
						{
							renderJob.mapParents.push_back(&checkJob);
							break;
						}
					}
				}
			}

			// get rid of explicitly given jobs to skip waiting on
			for(uint32_t iJob = 0; iJob < static_cast<uint32_t>(maRenderJobs.size()); iJob++)
			{
				auto& renderJob = maRenderJobs[iJob];
				for(uint32_t iSkipJob = 0; iSkipJob < static_cast<uint32_t>(renderJob.maSkipWaitOnJobs.size()); iSkipJob++)
				{
					std::string const& skipJobName = renderJob.maSkipWaitOnJobs[iSkipJob];
					auto iter = std::find_if(
						renderJob.mapParents.begin(),
						renderJob.mapParents.end(),
						[skipJobName](RenderJobInfo const* pParentRenderJob)
						{
							return skipJobName == pParentRenderJob->mName;
						});
					if(iter != renderJob.mapParents.end())
					{
						renderJob.mapParents.erase(iter);
					}
				}
			}

			for(iJobIndex = 0; iJobIndex < aJobs.Size(); iJobIndex++)
			{
				RenderJobInfo& renderJob = maRenderJobs[iJobIndex];
				execPostDataInitFunctions(renderJob);
			}


			// get the pass level of the render jobs
			for(auto& renderJob : maRenderJobs)
			{
				renderJob.miPassLevel = findRenderJobLevel(renderJob, 0);
			}

			mpDevice = &device;
		}


		/*
		**
		*/
		void Serializer::initShaderResources(
			Render::Common::RenderJobInfo& renderJobInfo,
			uint32_t iScreenWidth,
			uint32_t iScreenHeight,
			std::string const& renderJobName,
			RenderDriver::Common::CDevice& device)
		{
			uint32_t const iMaxDescriptorSize = 1024;
			uint32_t iHeapSize = static_cast<uint32_t>(renderJobInfo.maShaderResourceInfo.size());
			for(uint32_t iShaderResource = 0; iShaderResource < static_cast<uint32_t>(renderJobInfo.maShaderResourceInfo.size()); iShaderResource++)
			{
				if(renderJobInfo.maShaderResourceInfo[iShaderResource].mDesc.miCount == UINT32_MAX)
				{
					iHeapSize = iMaxDescriptorSize;
					break;
				}
			}

			// create descriptor heap
			if(renderJobInfo.maShaderResourceInfo.size() > 0)
			{
				for(uint32_t iBuffer = 0; iBuffer < 3; iBuffer++)
				{
					renderJobInfo.maDescriptorHeapHandles[iBuffer] = createDescriptorHeap(
						iHeapSize,
						device);

					std::ostringstream oss;
					oss << renderJobInfo.mName << " Descriptor Heap " << iBuffer;
					mapDescriptorHeaps[renderJobInfo.maDescriptorHeapHandles[iBuffer]]->setID(oss.str());
				}
			}

			// create resource and create descriptor tables
			std::vector<SerializeUtils::Common::ShaderResourceInfo>& aShaderResourceInfo = renderJobInfo.maShaderResourceInfo;
			for(uint32_t iResource = 0; iResource < (uint32_t)aShaderResourceInfo.size(); iResource++)
			{
				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo = aShaderResourceInfo[iResource];

				std::string attachmentName = "";
				for(auto const& attachment : renderJobInfo.maAttachmentInfo)
				{
					if(attachment.mShaderResourceName == shaderResourceInfo.mName)
					{
						attachmentName = attachment.mName;
						break;
					}
				}

				DEBUG_PRINTF("resource %d: \"%s\" attachment: \"%s\"\n", 
					iResource, 
					shaderResourceInfo.mName.c_str(),
					attachmentName.c_str());

				bool bFound = false;
				for(auto const& entry : mapBuffers)
				{
					if(entry.second->getID() == attachmentName)
					{
						bFound = true;

						//shaderResourceInfo.mExternalResource.mbSkipCreation = true;
						shaderResourceInfo.mExternalResource.mDimension = RenderDriver::Common::Dimension::Buffer;
						shaderResourceInfo.mExternalResource.mpBuffer = entry.second.get();
						shaderResourceInfo.mExternalResource.mShaderResourceViewDimension = RenderDriver::Common::ShaderResourceViewDimension::Buffer;
						shaderResourceInfo.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
						shaderResourceInfo.mDesc.miWidth = static_cast<uint32_t>(entry.second->getDescriptor().miSize);
						shaderResourceInfo.maDataInfo[0].miDataSize = static_cast<uint32_t>(entry.second->getDescriptor().miSize);

						break;
					}
				}

				if(canSkipShaderResourceCreation(shaderResourceInfo, renderJobInfo))
				{
					continue;
				}

				shaderResourceInfo.miResourceIndex = iResource;
				for(uint32_t iBuffer = 0; iBuffer < 3; iBuffer++)
				{
					if(shaderResourceInfo.mExternalResource.mpBuffer != nullptr ||
						shaderResourceInfo.mExternalResource.mpImage != nullptr ||
						shaderResourceInfo.mExternalResource.mRenderTargetHandle != UINT64_MAX)
					{
						if(shaderResourceInfo.mExternalResource.mpBuffer != nullptr)
						{
							shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
						}

						DEBUG_PRINTF("\tcreate buffer resource view %d only\n", iBuffer);
						createResourceViewOnly(
							renderJobInfo.maDescriptorHeapHandles[iBuffer],
							shaderResourceInfo,
							iResource,
							device);
					}
					else if(shaderResourceInfo.mDesc.miCount == UINT32_MAX)
					{
						for(uint32_t iImage = 0; iImage < shaderResourceInfo.mExternalResource.miNumImages; iImage++)
						{
							DEBUG_PRINTF("\tcreate image resource view %d only\n", iImage);
							createResourceViewOnly(
								renderJobInfo.maDescriptorHeapHandles[iBuffer],
								shaderResourceInfo,
								iResource,
								device);
						}
					}
					else
					{
						WTFASSERT(0, "should not even be here for shader resource \"%s\"", shaderResourceInfo.mName.c_str());

						DEBUG_PRINTF("\tfully create resource\n");
						PLATFORM_OBJECT_HANDLE handle = createResource(
							renderJobInfo.maDescriptorHeapHandles[iBuffer],
							shaderResourceInfo,
							iResource,
							iScreenWidth,
							iScreenHeight,
							iBuffer,
							renderJobInfo.mType,
							device);

						renderJobInfo.maaTripleBufferShaderResourceDataInfo[iBuffer][iResource].mHandle = handle;
					}

				}
			}
		}

		/*
		**
		*/
		void Serializer::buildDependencyGraph()
		{
			// link pipelines
			// check for output attachment name from other pipelines to the pipeline's input attachment
			uint32_t iJobIndex = 0;
			for(auto& renderJob : maRenderJobs)
			{
				uint32_t iAttachmentIndex = 0;
				RenderDriver::Common::PipelineInfo& pipeline = maPipelines[renderJob.miPipelineIndex];
				bool bCreatedList = false;
				for(auto const& attachment : pipeline.maAttachments)
				{
					ShaderResourceType attachmentType = ShaderResourceType::RESOURCE_TYPE_NONE;
					if(attachment.miShaderResourceIndex != UINT32_MAX)
					{
						attachmentType = renderJob.maShaderResourceInfo[attachment.miShaderResourceIndex].mType;
					}

					// check for children input with the parent output attachments
					if(attachmentType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT ||
						attachmentType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT)
					{
						// check for the same attachment name for the color input
						PipelineInfo* pChildPipeline = nullptr;
						uint32_t iDependentAttachment = UINT32_MAX;
						uint32_t iCheckJobIndex = 0;
						uint32_t iCheckAttachmentIndex = 0;
						uint32_t iChildRenderJob = UINT32_MAX;
						for(auto& checkRenderJob : maRenderJobs)
						{
							RenderJobInfo* pChildRenderJob = nullptr;
							if(checkRenderJob.mName == renderJob.mName)
							{
								continue;
							}

							RenderDriver::Common::PipelineInfo& checkPipeline = maPipelines[checkRenderJob.miPipelineIndex];
							iCheckAttachmentIndex = 0;
							for(auto const& checkAttachment : checkPipeline.maAttachments)
							{
								if(checkAttachment.mName == attachment.mName)
								{
									if(!bCreatedList)
									{
										bCreatedList = false;
									}

									pChildRenderJob = &checkRenderJob;
									iChildRenderJob = iCheckJobIndex;
								}

								++iCheckAttachmentIndex;
							}

							++iCheckJobIndex;

							// link child and parent job indices
							if(pChildRenderJob)
							{
								assert(iChildRenderJob != UINT32_MAX);
								checkPipeline.maParentPipelines.push_back(pipeline.mName);
							}
						}


					}

					++iAttachmentIndex;
				}

				++iJobIndex;
			}
		}

		/*
		**
		*/
		void Serializer::initRenderJob(
			Render::Common::RenderJobInfo& renderJobInfo,
			uint32_t iPipelineIndex,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Common::PipelineInfo const& pipelineInfo = maPipelines[iPipelineIndex];

			// initialize command list and fences for pipeline
			initCommandBuffer(
				renderJobInfo.mCommandBufferHandle,
				renderJobInfo.maCommandAllocatorHandles,
				renderJobInfo.maFenceHandles,
				pipelineInfo.mPipelineStateHandle,
				pipelineInfo.mType,
				pipelineInfo.mName,
				device);
		}

		/*
		**
		*/
		uint32_t Serializer::serializePipeline(
			std::vector<RenderDriver::Common::PipelineInfo>& aPipelineInfos,
			std::vector<RenderDriver::Common::RootSignature>& aRootSignatures,
			std::vector<RenderDriver::Common::PipelineState>& aPipelineStates,
			std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResourceInfo,
			std::string const& filePath,
			std::vector<RenderJobInfo> const& aRenderJobs,
			RenderJobInfo const& renderJob,
			RenderDriver::Common::CDevice& device)
		{
			uint32_t iRet = UINT32_MAX;
			for(uint32_t i = 0; i < maPipelines.size(); i++)
			{
				if(maPipelines[i].mFilePath == filePath)
				{
					iRet = i;
					break;
				}
			}

			if(iRet == UINT32_MAX)
			{
				FILE* fp = fopen(filePath.c_str(), "rb");
				if(fp)
				{
					aPipelineInfos.resize(aPipelineInfos.size() + 1);
					aRootSignatures.resize(maPipelineRootSignatures.size() + 1);
					aPipelineStates.resize(aPipelineStates.size() + 1);

					iRet = (uint32_t)aPipelineInfos.size() - 1;
					RenderDriver::Common::PipelineInfo& pipelineInfo = aPipelineInfos[iRet];

					fseek(fp, 0, SEEK_END);
					size_t iFileSize = ftell(fp);
					std::vector<char> acBuffer(iFileSize + 1);
					memset(acBuffer.data(), 0, iFileSize + 1);
					fseek(fp, 0, SEEK_SET);
					fread(acBuffer.data(), sizeof(char), iFileSize, fp);
					fclose(fp);

					rapidjson::Document doc;
					doc.Parse(acBuffer.data());

					PipelineType pipelineType = PipelineType::GRAPHICS_PIPELINE_TYPE;
					if(std::string(doc["Type"].GetString()) == "Compute")
					{
						pipelineType = PipelineType::COMPUTE_PIPELINE_TYPE;
					}
					else if(std::string(doc["Type"].GetString()) == "Copy")
					{
						pipelineType = PipelineType::COPY_PIPELINE_TYPE;
					}

					pipelineInfo.mType = pipelineType;

					char const* szName = doc["Name"].GetString();
					pipelineInfo.mName = szName;

					std::vector<ShaderInfo> aPipelineShaders;
					if(doc.HasMember("ShaderInfo"))
					{
						char const* szShaderFilePath = doc["ShaderInfo"].GetString();
						SerializeUtils::Common::getPipelineShaders(aPipelineShaders, szShaderFilePath);
					}
					else if(doc.HasMember("Shaders"))
					{
						std::string dirPath = "";
						size_t iEnd0 = filePath.find_last_of("\\");
						size_t iEnd1 = filePath.find_last_of("/");
						if(iEnd0 == std::string::npos)
						{
							dirPath = filePath.substr(0, iEnd1);
						}
						else if(iEnd1 == std::string::npos)
						{
							dirPath = filePath.substr(0, iEnd0);
						}
						else
						{
							if(iEnd0 > iEnd1)
							{
								dirPath = filePath.substr(0, iEnd0);
							}
							else
							{
								dirPath = filePath.substr(0, iEnd1);
							}
						}

						auto const& aShaders = doc["Shaders"].GetArray();
						for(auto const& shader : aShaders)
						{
							ShaderInfo shaderInfo;
							shaderInfo.mName = std::string(szName);
							if(shader.HasMember("Compute"))
							{
								shaderInfo.mType = ShaderType::COMPUTE_SHADER_TYPE;
								shaderInfo.mName += "Compute";
								shaderInfo.mFilePath = shader["Compute"].GetString();

								size_t iEnd = shaderInfo.mFilePath.find_last_of(".");
								shaderInfo.mFileOutputPath = dirPath + "/output/" + shaderInfo.mFilePath.substr(0, iEnd) + ".cso";
							}
							else if(shader.HasMember("Vertex"))
							{
								shaderInfo.mType = ShaderType::VERTEX_SHADER_TYPE;
								shaderInfo.mName += "Vertex";
								shaderInfo.mFilePath = shader["Vertex"].GetString();

								size_t iEnd = shaderInfo.mFilePath.find_last_of(".");
								shaderInfo.mFileOutputPath = dirPath + "/output/" + shaderInfo.mFilePath.substr(0, iEnd) + ".vso";
							}
							else if(shader.HasMember("Fragment"))
							{
								shaderInfo.mType = ShaderType::PIXEL_SHADER_TYPE;
								shaderInfo.mName += "Fragment";
								shaderInfo.mFilePath = shader["Fragment"].GetString();

								size_t iEnd = shaderInfo.mFilePath.find_last_of(".");
								shaderInfo.mFileOutputPath = dirPath + "/output/" + shaderInfo.mFilePath.substr(0, iEnd) + ".fso";
							}

							aPipelineShaders.push_back(shaderInfo);
						}
					}

					SerializeUtils::Common::getAttachments(pipelineInfo.maAttachments, doc);

					// root signature
					pipelineInfo.mDescriptorHandle = serializeShaderDescriptors(
						aShaderResourceInfo,
						pipelineType,
						renderJob.miNumRootConstants,
						device);
					RenderDriver::Common::CDescriptorSet& descriptorSet = *(mpRenderer->getSerializer()->getDescriptorSet(pipelineInfo.mDescriptorHandle).get());
					std::string descriptorSetName = pipelineInfo.mName + " Descriptor Set";
					descriptorSet.setID(descriptorSetName);

					if(pipelineType == PipelineType::COMPUTE_PIPELINE_TYPE)
					{
						// serialize compute pipeline
						pipelineInfo.mPipelineStateHandle = serializeComputePipeline(
							pipelineInfo.mDescriptorHandle,
							aPipelineShaders[0],
							renderJob,
							device);

						mapComputePipelineStates[pipelineInfo.mPipelineStateHandle]->setID(pipelineInfo.mName + " Pipeline State");

					}	// if compute pipeline
					else if(pipelineType == PipelineType::GRAPHICS_PIPELINE_TYPE)
					{
						std::vector<RenderDriver::Common::BlendState> aBlendStates;
						SerializeUtils::Common::getBlendState(aBlendStates, doc);
						RenderDriver::Common::DepthStencilState depthStencilState = SerializeUtils::Common::getDepthStencilState(doc);
						RenderDriver::Common::RasterState rasterState = SerializeUtils::Common::getRasterState(doc);

						std::vector<RenderDriver::Common::Format> aRenderTargetFormats;

						uint32_t iNumRenderTargetAttachments = 0;
						uint32_t iNumAttachments = static_cast<uint32_t>(pipelineInfo.maAttachments.size());
						for(uint32_t i = 0; i < iNumAttachments; i++)
						{
							if(pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::TextureOutput)
							{
								// disable blend based format
								if(pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32G32B32A32_UINT ||
									pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32G32B32_UINT ||
									pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32G32_UINT ||
									pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32_UINT ||
									pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32G32B32A32_SINT ||
									pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32G32B32_SINT ||
									pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32G32_SINT ||
									pipelineInfo.maAttachments[i].mFormat == RenderDriver::Common::Format::R32_SINT)
								{
									pipelineInfo.maAttachments[i].mbBlend = false;
								}

								aRenderTargetFormats.push_back(pipelineInfo.maAttachments[i].mFormat);
								++iNumRenderTargetAttachments;
							}
							else if(pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::TextureInputOutput)
							{
								// input/output texture uses parents output render target, need to get the parent's attachment info to set the correct format
								bool bFound = false;
								for(auto const& parentRenderJob : aRenderJobs)
								{
									if(parentRenderJob.mName == pipelineInfo.maAttachments[i].mParentRenderJobName)
									{
										for(auto const& attachmentInfo : parentRenderJob.maAttachmentInfo)
										{
											if(attachmentInfo.mName == pipelineInfo.maAttachments[i].mName)
											{
												aRenderTargetFormats.push_back(attachmentInfo.mFormat);
												++iNumRenderTargetAttachments;
												bFound = true;
												break;
											}
										}
										
										break;
									}
								}

								WTFASSERT(bFound, "Didn\'t find parent pass \"%s\" for \"%s\"", 
									pipelineInfo.maAttachments[i].mParentRenderJobName.c_str(), 
									pipelineInfo.maAttachments[i].mName.c_str());
							}
						}

						if(iNumRenderTargetAttachments <= 0)
						{
							aRenderTargetFormats.push_back(mpRenderer->getDescriptor().mFormat);
							iNumRenderTargetAttachments = 1;
						}

						std::vector<RenderDriver::Common::BlendState> aAttachmentBlendStates(iNumRenderTargetAttachments);
						if(aBlendStates.size() < iNumAttachments)
						{
							uint32_t iTextureAttachment = 0;
							for(uint32_t i = 0; i < iNumAttachments; i++)
							{
								if(pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::TextureOutput || pipelineInfo.maAttachments[i].mType == AttachmentTypeBits::TextureInputOutput)
								{
									if(pipelineInfo.maAttachments[i].mbBlend)
									{
										WTFASSERT(iTextureAttachment < aAttachmentBlendStates.size(), "texture attachment %d out of bounds (%d)", iTextureAttachment, aAttachmentBlendStates.size());
										aAttachmentBlendStates[iTextureAttachment] = aBlendStates[0];
									}
									else
									{
										WTFASSERT(iTextureAttachment < aAttachmentBlendStates.size(), "texture attachment %d out of bounds (%d)", iTextureAttachment, aAttachmentBlendStates.size());
										aAttachmentBlendStates[iTextureAttachment].mbEnabled = false;
									}

									++iTextureAttachment;
								}
							}
						}

						// serialize graphis pipeline
						pipelineInfo.mPipelineStateHandle = serializeGraphicsPipeline(
							pipelineInfo.mDescriptorHandle,
							aAttachmentBlendStates,
							depthStencilState,
							rasterState,
							aRenderTargetFormats,
							doc,
							aPipelineShaders[0],
							aPipelineShaders[1],
							renderJob,
							device);

						mapGraphicsPipelineStates[pipelineInfo.mPipelineStateHandle]->setID(pipelineInfo.mName + " Pipeline State");

					}	// if graphics pipeline

				}	// if fp
			}

			return iRet;
		}

		/*
		**
		*/
		uint32_t Serializer::findRenderJobLevel(RenderJobInfo& renderJob, uint32_t iLevel)
		{
			uint32_t iRet = iLevel;
			for(uint32_t i = 0; i < renderJob.maShaderResourceInfo.size(); i++)
			{
				uint32_t iParentRenderJobIndex = renderJob.maShaderResourceInfo[i].miParentRenderJobIndex;
				if(iParentRenderJobIndex != UINT32_MAX)
				{
					RenderJobInfo& parentRenderJob = maRenderJobs[iParentRenderJobIndex];
					uint32_t iParentLevel = findRenderJobLevel(parentRenderJob, iLevel + 1);
					if(iParentLevel > iRet)
					{
						iRet = iParentLevel;
					}
				}
			}

			return iRet;
		}

		/*
		**
		*/
		RenderDriver::Common::PipelineInfo const& Serializer::getPipelineInfo(uint32_t iRenderJob)
		{
			return maPipelines[iRenderJob];
		}

		/*
		**
		*/
		RenderDriver::Common::PipelineInfo const* Serializer::getPipelineInfo(std::string const& name)
		{
			auto const& iter = std::find_if(
				maRenderJobs.begin(),
				maRenderJobs.end(),
				[name](RenderJobInfo const& renderJobInfo)
				{
					return renderJobInfo.mName == name;
				}
			);
			//WTFASSERT(iter != maRenderJobs.end(), "Can\'t find render job \"%s\"", name.c_str());

			return (iter == maRenderJobs.end()) ? nullptr : &maPipelines[iter->miPipelineIndex];
		}

		/*
		**
		*/
		RenderDriver::Common::PipelineState& Serializer::getPipelineState(uint32_t iRenderJob)
		{
			return maPipelineStates[iRenderJob];
		}

		/*
		**
		*/
		std::vector<SerializeUtils::Common::ShaderResourceInfo> const& Serializer::getShaderResourceInfo(uint32_t iRenderJob)
		{
			return maRenderJobs[iRenderJob].maShaderResourceInfo;
		}

		/*
		**
		*/
		std::vector<RenderJobInfo>& Serializer::getRenderJobs()
		{
			return maRenderJobs;
		}

		/*
		**
		*/
		RenderJobInfo& Serializer::getRenderJob(uint32_t iRenderJob)
		{
			return maRenderJobs[iRenderJob];
		}

		/*
		**
		*/
		RenderJobInfo& Serializer::getRenderJob(std::string const& jobName)
		{
			uint32_t iRenderJob = 0;
			for(iRenderJob = 0; iRenderJob < static_cast<uint32_t>(maRenderJobs.size()); iRenderJob++)
			{
				if(maRenderJobs[iRenderJob].mName == jobName)
				{
					break;
				}
			}

			assert(iRenderJob < static_cast<uint32_t>(maRenderJobs.size()));

			return maRenderJobs[iRenderJob];
		}

		/*
		**
		*/
		bool Serializer::hasRenderJob(std::string const& jobName)
		{
			bool bRet = false;
			for(uint32_t iRenderJob = 0; iRenderJob < static_cast<uint32_t>(maRenderJobs.size()); iRenderJob++)
			{
				if(maRenderJobs[iRenderJob].mName == jobName)
				{
					bRet = true;
					break;
				}
			}

			return bRet;
		}

		/*
		**
		*/
		void Serializer::execPreDataInitFunctions(RenderJobInfo& renderJob)
		{
			for(auto const& [key, value] : mInitializeFunctions)
			{
				if(key.first == renderJob.mName)
				{
					auto& funcs = mInitializeFunctions[key];

					uint32_t iResourceIndex = 0;
					for(auto& shaderResourceInfo : renderJob.maShaderResourceInfo)
					{
						if(shaderResourceInfo.mName == key.second)
						{
							if(funcs.mPreCreationDataInitFunction)
							{
								ShaderResourceInitializeDescriptor desc =
								{
									/* .mpShaderResource	*/	&shaderResourceInfo,
									/* .mpRenderJobInfo		*/	&renderJob,
									/* .mpSerializer		*/	this,
									/* .mpRenderer			*/  mpRenderer,
									/* .miResourceIndex		*/	iResourceIndex,
									/* .mpUserData			*/	funcs.mpUserData,
								};
								funcs.mPreCreationDataInitFunction(desc);
							}
							else
							{
								DEBUG_PRINTF("No pre data initialization functions for %s : %s\n", renderJob.mName.c_str(), shaderResourceInfo.mName.c_str());
							}

							break;
						}

						++iResourceIndex;
					}
				}
			}
		}

		/*
		**
		*/
		void Serializer::execPostDataInitFunctions(RenderJobInfo& renderJob)
		{
			for(auto const& [key, value] : mInitializeFunctions)
			{
				if(key.first == renderJob.mName)
				{
					auto& funcs = mInitializeFunctions[key];

					uint32_t iResourceIndex = 0;
					for(auto& shaderResourceInfo : renderJob.maShaderResourceInfo)
					{
						if(key.second == shaderResourceInfo.mName)
						{
							if(funcs.mPostCreationDataInitFunction)
							{
								ShaderResourceInitializeDescriptor desc =
								{
									/*.mpShaderResource =	*/	&shaderResourceInfo,
									/*.mpRenderJobInfo =	*/	&renderJob,
									/*.mpSerializer =		*/	this,
									/* .mpRenderer			*/  mpRenderer,
									/* .miResourceIndex =	*/	iResourceIndex,
									/* .mpUserData          */	funcs.mpUserData,
								};
								funcs.mPostCreationDataInitFunction(desc);
							}
							else
							{
								DEBUG_PRINTF("No post data initialization functions for %s : %s\n", renderJob.mName.c_str(), shaderResourceInfo.mName.c_str());
							}

							break;
						}

						++iResourceIndex;
					}
				}
			}
		}

		/*
		**
		*/
		SerializeUtils::Common::ShaderResourceInfo* Serializer::getShaderResource(
			std::string const& renderJobName,
			std::string const& shaderResourceName)
		{
			auto const& iter = std::find_if(
				maRenderJobs.begin(),
				maRenderJobs.end(),
				[renderJobName](Render::Common::RenderJobInfo const& renderJobInfo)
				{
					return (renderJobInfo.mName == renderJobName);
				});

			if(iter != maRenderJobs.end())
			{
				auto const& shaderResourceIter = std::find_if(
					iter->maShaderResourceInfo.begin(),
					iter->maShaderResourceInfo.end(),
					[shaderResourceName](SerializeUtils::Common::ShaderResourceInfo const& shaderResourceInfo)
					{
						return (shaderResourceInfo.mName == shaderResourceName);
					});

				if(shaderResourceIter != iter->maShaderResourceInfo.end())
				{
					return &(*shaderResourceIter);
				}
			}

			return nullptr;
		}

		/*
		**
		*/
		void Serializer::initRenderTarget(
			PLATFORM_OBJECT_HANDLE& renderTarget,
			PLATFORM_OBJECT_HANDLE& colorRenderTargetHeap,
			PLATFORM_OBJECT_HANDLE& depthStencilRenderTargetHeap,
			AttachmentInfo const& attachmentInfo,
			uint32_t iWidth,
			uint32_t iHeight,
			float const* afClearColor,
			RenderDriver::Common::CDevice& device)
		{
		}

		/*
		**
		*/
		void Serializer::initRenderTargetBuffer(
			PLATFORM_OBJECT_HANDLE& renderTargetBuffer,
			AttachmentInfo const& attachmentInfo,
			uint32_t iWidth,
			uint32_t iHeight,
			uint32_t iPipelineIndex,
			RenderDriver::Common::CDevice& device)
		{
		}

		/*
		**
		*/
		void Serializer::initAttachmentImage(
			PLATFORM_OBJECT_HANDLE& attachmentImageHandle,
			AttachmentInfo const& attachmentInfo,
			uint32_t iWidth,
			uint32_t iHeight,
			RenderDriver::Common::CDevice& device)
		{
		}

		/*
		**
		*/
		void Serializer::updateShaderResourceData(
			SerializeUtils::Common::ShaderResourceInfo& shaderResource,
			void const* pNewData,
			uint32_t iBufferIndex,
			RenderDriver::Common::CDevice& device)
		{

		}

		void Serializer::createResourceViewOnly(
			PLATFORM_OBJECT_HANDLE descriptorHeapHandle,
			SerializeUtils::Common::ShaderResourceInfo& shaderResource,
			uint32_t iResourceIndex,
			RenderDriver::Common::CDevice& device)
		{
		}

		/*
		**
		*/
		bool Serializer::canSkipShaderResourceCreation(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo,
			Render::Common::RenderJobInfo& renderJobInfo)
		{
			bool bSkip = shaderResourceInfo.mExternalResource.mbSkipCreation;
			if(!bSkip)
			{
				// check if it's an input attachment
				for(auto const& attachmentInfo : renderJobInfo.maAttachmentInfo)
				{
					if(attachmentInfo.mShaderResourceName != "" &&
						attachmentInfo.mShaderResourceName == shaderResourceInfo.mName &&
						attachmentInfo.mParentJobName != "This")
					{
						if(attachmentInfo.mType == AttachmentType::BufferIn)
						{
							shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
						}
						else if(attachmentInfo.mType == AttachmentType::TextureIn)
						{
							shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
						}
						else if(attachmentInfo.mType == AttachmentType::TextureInOut)
						{
							shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT;
						}

						bSkip = true;
						DEBUG_PRINTF("*** is input attachment, skip ***\n");
						break;
					}

					if(attachmentInfo.mShaderResourceName != "" &&
						attachmentInfo.mShaderResourceName == shaderResourceInfo.mName &&
						(attachmentInfo.mType == AttachmentType::TextureOut || attachmentInfo.mType == AttachmentType::BufferOut))
					{
						shaderResourceInfo.mType = (attachmentInfo.mType == AttachmentType::TextureOut) ? ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT : ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
						bSkip = true;
						DEBUG_PRINTF("*** is output attachment, skip ***\n");
						break;
					}
				}

				if(shaderResourceInfo.mName == "readOnlyBuffer")
				{
					shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					shaderResourceInfo.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
					shaderResourceInfo.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
					shaderResourceInfo.miStructByteStride = 4;
					associateReadOnlyBufferToShaderResource(shaderResourceInfo);

					DEBUG_PRINTF("*** read only buffer, can skip ***\n");

					bSkip = true;
				}
				else if(shaderResourceInfo.mName == "readWriteBuffer")
				{
					shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
					shaderResourceInfo.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
					shaderResourceInfo.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
					shaderResourceInfo.miStructByteStride = 4;
					associateReadWriteBufferToShaderResource(shaderResourceInfo);

					DEBUG_PRINTF("*** read/write buffer, can skip ***\n");

					bSkip = true;
				}
				else if(shaderResourceInfo.mName == "rayTraceSceneBuffer")
				{
					shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
					shaderResourceInfo.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
					shaderResourceInfo.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
					shaderResourceInfo.miStructByteStride = 4;
					associateRayTraceSceneBufferToShaderResource(shaderResourceInfo);

					DEBUG_PRINTF("*** ray trace scene buffer, can skip ***\n");

					bSkip = true;
				}
				else if(shaderResourceInfo.mName.find("$Globals-") == 0)
				{
					shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					shaderResourceInfo.mViewType = RenderDriver::Common::ResourceViewType::ConstantBufferView;
					shaderResourceInfo.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
					shaderResourceInfo.miStructByteStride = 256;
					associateConstantBufferToShaderResource(shaderResourceInfo, renderJobInfo.mName);

					DEBUG_PRINTF("*** pass constant buffer from descriptor manager, can skip ***\n");

					bSkip = true;
				}
				else if(shaderResourceInfo.mName == "totalTextureArray")
				{
					shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
					shaderResourceInfo.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
					shaderResourceInfo.mDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
					shaderResourceInfo.miStructByteStride = 4;

					DEBUG_PRINTF("*** total texture array, can skip ***\n");

					bSkip = true;
				}
				else
				{
					if(shaderResourceInfo.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN ||
						shaderResourceInfo.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT ||
						shaderResourceInfo.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT)
					{
						WTFASSERT(0, "Implement me");
					}
				}
			}
			else
			{
				if(shaderResourceInfo.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT && shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::ConstantBufferView)
				{
					DEBUG_PRINTF("*** is root constant, can skip ***\n");
				}
				else
				{
					DEBUG_PRINTF("*** marked as skip, skip ***\n");
				}
			}

			return bSkip;
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CRenderTarget>& Serializer::getRenderTarget(PLATFORM_OBJECT_HANDLE handle)
		{
			WTFASSERT(mapRenderTargets.find(handle) != mapRenderTargets.end(), "not a valid render target handle (%d)", handle);
			return mapRenderTargets[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CFence>& Serializer::getFence(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapFences.find(handle) != mapFences.end());
			return mapFences[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CCommandAllocator>& Serializer::getCommandAllocator(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapCommandAllocators.find(handle) != mapCommandAllocators.end());
			return mapCommandAllocators[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CCommandBuffer>& Serializer::getCommandBuffer(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapCommandBuffers.find(handle) != mapCommandBuffers.end());
			return mapCommandBuffers[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CPipelineState>& Serializer::getPipelineStateFromHandle(PLATFORM_OBJECT_HANDLE handle)
		{
			if(mapComputePipelineStates.find(handle) != mapComputePipelineStates.end())
			{
				return mapComputePipelineStates[handle];
			}

			assert(mapGraphicsPipelineStates.find(handle) != mapGraphicsPipelineStates.end());
			return mapGraphicsPipelineStates[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CDescriptorHeap>& Serializer::getDescriptorHeap(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapDescriptorHeaps.find(handle) != mapDescriptorHeaps.end());
			return mapDescriptorHeaps[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CDescriptorSet>& Serializer::getDescriptorSet(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapDescriptorSets.find(handle) != mapDescriptorSets.end());
			return mapDescriptorSets[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CBuffer>& Serializer::getBuffer(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapBuffers.find(handle) != mapBuffers.end());
			return mapBuffers[handle];
		}

		/*
		**
		*/
		RenderDriver::Common::CBuffer* Serializer::getBuffer(std::string const& bufferName)
		{
			RenderDriver::Common::CBuffer* pRet = nullptr;
			for(auto const& keyValue : mapBuffers)
			{
				if(keyValue.second->getID() == bufferName)
				{
					pRet = keyValue.second.get();
					break;
				}
			}

			return pRet;
		}

		/*
		**
		*/
		void Serializer::setBuffer(std::unique_ptr<RenderDriver::Common::CBuffer>& buffer)
		{
			PLATFORM_OBJECT_HANDLE handle = buffer->getHandle();
			std::swap(mapBuffers[handle], buffer);

			for(auto& renderJob : maRenderJobs)
			{
				for(auto& attachmentInfo : renderJob.maAttachmentInfo)
				{
					if(attachmentInfo.mName == mapBuffers[handle]->getID())
					{
						attachmentInfo.mAttachmentHandle = handle;
						DEBUG_PRINTF("render job: %s set attachment \"%s\" handle = %lld\n",
							renderJob.mName.c_str(),
							attachmentInfo.mName.c_str(),
							handle);

						for(auto& shaderResource : renderJob.maShaderResourceInfo)
						{
							if(shaderResource.mName == attachmentInfo.mShaderResourceName)
							{
								shaderResource.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
								shaderResource.mDesc.miWidth = static_cast<uint32_t>(mapBuffers[handle]->getDescriptor().miSize);
								shaderResource.mDesc.mViewFormat = mapBuffers[handle]->getDescriptor().mFormat;
								shaderResource.maHandles[0] = handle;
								shaderResource.mExternalResource.mpBuffer = mapBuffers[handle].get();
								shaderResource.mExternalResource.mDimension = RenderDriver::Common::Dimension::Buffer;

								DEBUG_PRINTF("\tshader resource \"%s\" handle = %lld\n",
									shaderResource.mName.c_str(),
									handle);

								break;
							}
						}

					}
				}

			}
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CImage>& Serializer::getImage(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapImages.find(handle) != mapImages.end());
			return mapImages[handle];
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CFrameBuffer>& Serializer::getFrameBuffer(PLATFORM_OBJECT_HANDLE handle)
		{
			assert(mapFrameBuffers.find(handle) != mapFrameBuffers.end());
			return mapFrameBuffers[handle];
		}

		/*
		**
		*/
		bool Serializer::isImage(PLATFORM_OBJECT_HANDLE handle)
		{
			return (mapImages.find(handle) != mapImages.end());
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CImage>& Serializer::platformGetRenderTargetImage(PLATFORM_OBJECT_HANDLE handle)
		{
			std::unique_ptr<RenderDriver::Common::CRenderTarget>& pRenderTarget = mapRenderTargets[handle];
			return pRenderTarget->getImage();
		}

		/*
		**
		*/
		std::unique_ptr<RenderDriver::Common::CBuffer>& Serializer::platformGetRenderTargetBuffer(PLATFORM_OBJECT_HANDLE handle)
		{
			return mapBuffers[handle];
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::createDescriptorHeap(
			uint32_t iNumShaderResources,
			RenderDriver::Common::CDevice& device)
		{
			return 0;
		}

		/*
		**
		*/
		void Serializer::initCommandBuffer(
			PLATFORM_OBJECT_HANDLE& commandList,
			PLATFORM_OBJECT_HANDLE* aCommandAllocator,
			PLATFORM_OBJECT_HANDLE* aFenceHandles,
			PLATFORM_OBJECT_HANDLE pipelineState,
			PipelineType const& type,
			std::string const& renderJobName,
			RenderDriver::Common::CDevice& device)
		{
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::serializeShaderDescriptors(
			std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResourceInfo,
			PipelineType pipelineType,
			uint32_t iNumRootConstants,
			RenderDriver::Common::CDevice& device)
		{
			return 0;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::serializeComputePipeline(
			PLATFORM_OBJECT_HANDLE rootSignature,
			ShaderInfo const& shaderInfo,
			Render::Common::RenderJobInfo const& renderJob,
			RenderDriver::Common::CDevice& device)
		{
			return 0;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::serializeGraphicsPipeline(
			PLATFORM_OBJECT_HANDLE descriptorHandle,
			std::vector<RenderDriver::Common::BlendState> const& aBlendStates,
			RenderDriver::Common::DepthStencilState const& depthStencilState,
			RenderDriver::Common::RasterState const& rasterState,
			std::vector<RenderDriver::Common::Format> const& aRenderTargetFormats,
			rapidjson::Document const& doc,
			ShaderInfo const& vertexShaderInfo,
			ShaderInfo const& fragmentShaderInfo,
			Render::Common::RenderJobInfo const& renderJob,
			RenderDriver::Common::CDevice& device)
		{
			return 0;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::createResourceView(
			SerializeUtils::Common::ShaderResourceInfo& shaderResource,
			SerializeUtils::Common::ShaderResourceDataInfo& shaderData,
			uint32_t iDataSize,
			RenderDriver::Common::CDevice& device)
		{
			return 0;
		}


		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::createResource(
			PLATFORM_OBJECT_HANDLE descriptorHeapHandle,
			SerializeUtils::Common::ShaderResourceInfo& shaderResource,
			uint32_t iResourceIndex,
			uint32_t iImageWidth,
			uint32_t iImageHeight,
			uint32_t iTripleBufferIndex,
			Render::Common::JobType jobType,
			RenderDriver::Common::CDevice& device)
		{
			return 0;
		}

		/*
		**
		*/
		void Serializer::associateReadOnlyBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
		}

		/*
		**
		*/
		void Serializer::associateReadWriteBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
		}

		/*
		**
		*/
		void Serializer::associateConstantBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo,
			std::string const& renderJobName)
		{
		}

		/*
		**
		*/
		void Serializer::associateRayTraceSceneBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
		}

		/*
		**
		*/
		void Serializer::associateExternalBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
		}

		/*
		**
		*/
		bool Serializer::hasRenderTarget(PLATFORM_OBJECT_HANDLE handle)
		{
			auto iter = mapRenderTargets.find(handle);
			return (iter != mapRenderTargets.end());
		}


		/*
		**
		*/
		RenderDriver::Common::Format convertFormat(std::string const& format)
		{
#define _IF(FORMAT_STRING)	if(format == #FORMAT_STRING) ret = RenderDriver::Common::Format::FORMAT_STRING;		



#define _ELSE_IF(FORMAT_STRING)	\
	else if(format == #FORMAT_STRING)	ret = RenderDriver::Common::Format::FORMAT_STRING; 


#define _ELSE \
	else WTFASSERT(0, "Invalid format string %s", format.c_str());

			RenderDriver::Common::Format ret = RenderDriver::Common::Format::UNKNOWN;

			_IF(UNKNOWN)
				_ELSE_IF(R32G32B32A32_TYPELESS)
				_ELSE_IF(R32G32B32A32_FLOAT)
				_ELSE_IF(R32G32B32A32_UINT)
				_ELSE_IF(R32G32B32A32_SINT)
				_ELSE_IF(R32G32B32_TYPELESS)
				_ELSE_IF(R32G32B32_FLOAT)
				_ELSE_IF(R32G32B32_UINT)
				_ELSE_IF(R32G32B32_SINT)
				_ELSE_IF(R16G16B16A16_TYPELESS)
				_ELSE_IF(R16G16B16A16_FLOAT)
				_ELSE_IF(R16G16B16A16_UNORM)
				_ELSE_IF(R16G16B16A16_UINT)
				_ELSE_IF(R16G16B16A16_SNORM)
				_ELSE_IF(R16G16B16A16_SINT)
				_ELSE_IF(R32G32_TYPELESS)
				_ELSE_IF(R32G32_FLOAT)
				_ELSE_IF(R32G32_UINT)
				_ELSE_IF(R32G32_SINT)
				_ELSE_IF(R32G8X24_TYPELESS)
				_ELSE_IF(D32_FLOAT_S8X24_UINT)
				_ELSE_IF(R32_FLOAT_X8X24_TYPELESS)
				_ELSE_IF(X32_TYPELESS_G8X24_UINT)
				_ELSE_IF(R10G10B10A2_TYPELESS)
				_ELSE_IF(R10G10B10A2_UNORM)
				_ELSE_IF(R10G10B10A2_UINT)
				_ELSE_IF(R11G11B10_FLOAT)
				_ELSE_IF(R8G8B8A8_TYPELESS)
				_ELSE_IF(R8G8B8A8_UNORM)
				_ELSE_IF(R8G8B8A8_UNORM_SRGB)
				_ELSE_IF(R8G8B8A8_UINT)
				_ELSE_IF(R8G8B8A8_SNORM)
				_ELSE_IF(R8G8B8A8_SINT)
				_ELSE_IF(R16G16_TYPELESS)
				_ELSE_IF(R16G16_FLOAT)
				_ELSE_IF(R16G16_UNORM)
				_ELSE_IF(R16G16_UINT)
				_ELSE_IF(R16G16_SNORM)
				_ELSE_IF(R16G16_SINT)
				_ELSE_IF(R32_TYPELESS)
				_ELSE_IF(D32_FLOAT)
				_ELSE_IF(R32_FLOAT)
				_ELSE_IF(R32_UINT)
				_ELSE_IF(R32_SINT)
				_ELSE_IF(R24G8_TYPELESS)
				_ELSE_IF(D24_UNORM_S8_UINT)
				_ELSE_IF(R24_UNORM_X8_TYPELESS)
				_ELSE_IF(X24_TYPELESS_G8_UINT)
				_ELSE_IF(R8G8_TYPELESS)
				_ELSE_IF(R8G8_UNORM)
				_ELSE_IF(R8G8_UINT)
				_ELSE_IF(R8G8_SNORM)
				_ELSE_IF(R8G8_SINT)
				_ELSE_IF(R16_TYPELESS)
				_ELSE_IF(R16_FLOAT)
				_ELSE_IF(D16_UNORM)
				_ELSE_IF(R16_UNORM)
				_ELSE_IF(R16_UINT)
				_ELSE_IF(R16_SNORM)
				_ELSE_IF(R16_SINT)
				_ELSE_IF(R8_TYPELESS)
				_ELSE_IF(R8_UNORM)
				_ELSE_IF(R8_UINT)
				_ELSE_IF(R8_SNORM)
				_ELSE_IF(R8_SINT)
				_ELSE_IF(A8_UNORM)
				_ELSE_IF(R1_UNORM)
				_ELSE

#undef _IF
#undef _ELSE_IF
#undef _ELSE

				return ret;

		}

		

	}	// Common

}	// Render