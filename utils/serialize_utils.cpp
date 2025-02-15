#include <utils/serialize_utils.h>
#include <utils/LogPrint.h>
#include <utils/wtfassert.h>
#include <sstream>
namespace SerializeUtils
{
	namespace Common
	{
        /*
        **
        */
        RenderDriver::Common::Format _getTextureFormat(std::string const& format)
        {

#define IF_STATEMENT(FORMAT)    \
        if(format == #FORMAT)   \
            ret = RenderDriver::Common::Format::FORMAT;
            
#define ELSE_IF_STATEMENT(FORMAT)                                  \
        else if(format == #FORMAT)                                 \
            ret = RenderDriver::Common::Format::FORMAT;

            RenderDriver::Common::Format ret = RenderDriver::Common::Format::UNKNOWN;

            IF_STATEMENT(UNKNOWN)
            ELSE_IF_STATEMENT(R32G32B32A32_TYPELESS)
            ELSE_IF_STATEMENT(R32G32B32A32_FLOAT)
            ELSE_IF_STATEMENT(R32G32B32A32_UINT)
            ELSE_IF_STATEMENT(R32G32B32A32_SINT)
            ELSE_IF_STATEMENT(R32G32B32_TYPELESS)
            ELSE_IF_STATEMENT(R32G32B32_FLOAT)
            ELSE_IF_STATEMENT(R32G32B32_UINT)
            ELSE_IF_STATEMENT(R32G32B32_SINT)
            ELSE_IF_STATEMENT(R16G16B16A16_TYPELESS)
            ELSE_IF_STATEMENT(R16G16B16A16_FLOAT)
            ELSE_IF_STATEMENT(R16G16B16A16_UNORM)
            ELSE_IF_STATEMENT(R16G16B16A16_UINT)
            ELSE_IF_STATEMENT(R16G16B16A16_SNORM)
            ELSE_IF_STATEMENT(R16G16B16A16_SINT)
            ELSE_IF_STATEMENT(R32G32_TYPELESS)
            ELSE_IF_STATEMENT(R32G32_FLOAT)
            ELSE_IF_STATEMENT(R32G32_UINT)
            ELSE_IF_STATEMENT(R32G32_SINT)
            ELSE_IF_STATEMENT(R32G8X24_TYPELESS)
            ELSE_IF_STATEMENT(D32_FLOAT_S8X24_UINT)
            ELSE_IF_STATEMENT(R32_FLOAT_X8X24_TYPELESS)
            ELSE_IF_STATEMENT(X32_TYPELESS_G8X24_UINT)
            ELSE_IF_STATEMENT(R10G10B10A2_TYPELESS)
            ELSE_IF_STATEMENT(R10G10B10A2_UNORM)
            ELSE_IF_STATEMENT(R10G10B10A2_UINT)
            ELSE_IF_STATEMENT(R11G11B10_FLOAT)
            ELSE_IF_STATEMENT(R8G8B8A8_TYPELESS)
            ELSE_IF_STATEMENT(R8G8B8A8_UNORM)
            ELSE_IF_STATEMENT(R8G8B8A8_UNORM_SRGB)
            ELSE_IF_STATEMENT(R8G8B8A8_UINT)
            ELSE_IF_STATEMENT(R8G8B8A8_SNORM)
            ELSE_IF_STATEMENT(R8G8B8A8_SINT)
            ELSE_IF_STATEMENT(R16G16_TYPELESS)
            ELSE_IF_STATEMENT(R16G16_FLOAT)
            ELSE_IF_STATEMENT(R16G16_UNORM)
            ELSE_IF_STATEMENT(R16G16_UINT)
            ELSE_IF_STATEMENT(R16G16_SNORM)
            ELSE_IF_STATEMENT(R16G16_SINT)
            ELSE_IF_STATEMENT(R32_TYPELESS)
            ELSE_IF_STATEMENT(D32_FLOAT)
            ELSE_IF_STATEMENT(R32_FLOAT)
            ELSE_IF_STATEMENT(R32_UINT)
            ELSE_IF_STATEMENT(R32_SINT)
            ELSE_IF_STATEMENT(R24G8_TYPELESS)
            ELSE_IF_STATEMENT(D24_UNORM_S8_UINT)
            ELSE_IF_STATEMENT(R24_UNORM_X8_TYPELESS)
            ELSE_IF_STATEMENT(X24_TYPELESS_G8_UINT)
            ELSE_IF_STATEMENT(R8G8_TYPELESS)
            ELSE_IF_STATEMENT(R8G8_UNORM)
            ELSE_IF_STATEMENT(R8G8_UINT)
            ELSE_IF_STATEMENT(R8G8_SNORM)
            ELSE_IF_STATEMENT(R8G8_SINT)
            ELSE_IF_STATEMENT(R16_TYPELESS)
            ELSE_IF_STATEMENT(R16_FLOAT)
            ELSE_IF_STATEMENT(D16_UNORM)
            ELSE_IF_STATEMENT(R16_UNORM)
            ELSE_IF_STATEMENT(R16_UINT)
            ELSE_IF_STATEMENT(R16_SNORM)
            ELSE_IF_STATEMENT(R16_SINT)
            ELSE_IF_STATEMENT(R8_TYPELESS)
            ELSE_IF_STATEMENT(R8_UNORM)
            ELSE_IF_STATEMENT(R8_UINT)
            ELSE_IF_STATEMENT(R8_SNORM)
            ELSE_IF_STATEMENT(R8_SINT)
            ELSE_IF_STATEMENT(A8_UNORM)
            ELSE_IF_STATEMENT(R1_UNORM)
            ELSE_IF_STATEMENT(R9G9B9E5_SHAREDEXP)
            ELSE_IF_STATEMENT(R8G8_B8G8_UNORM)
            ELSE_IF_STATEMENT(G8R8_G8B8_UNORM)

                ELSE_IF_STATEMENT(BC1_TYPELESS)
                ELSE_IF_STATEMENT(BC1_UNORM)
                ELSE_IF_STATEMENT(BC1_UNORM_SRGB)
                ELSE_IF_STATEMENT(BC2_TYPELESS)
                ELSE_IF_STATEMENT(BC2_UNORM)
                ELSE_IF_STATEMENT(BC2_UNORM_SRGB)
                ELSE_IF_STATEMENT(BC3_TYPELESS)
                ELSE_IF_STATEMENT(BC3_UNORM)
                ELSE_IF_STATEMENT(BC3_UNORM_SRGB)
                ELSE_IF_STATEMENT(BC4_TYPELESS)
                ELSE_IF_STATEMENT(BC4_UNORM)
                ELSE_IF_STATEMENT(BC4_SNORM)
                ELSE_IF_STATEMENT(BC5_TYPELESS)
                ELSE_IF_STATEMENT(BC5_UNORM)
                ELSE_IF_STATEMENT(BC5_SNORM)
                ELSE_IF_STATEMENT(B5G6R5_UNORM)
                ELSE_IF_STATEMENT(B5G5R5A1_UNORM)
                ELSE_IF_STATEMENT(B8G8R8A8_UNORM)
                ELSE_IF_STATEMENT(B8G8R8X8_UNORM)
                ELSE_IF_STATEMENT(R10G10B10_XR_BIAS_A2_UNORM)
                ELSE_IF_STATEMENT(B8G8R8A8_TYPELESS)
                ELSE_IF_STATEMENT(B8G8R8A8_UNORM_SRGB)
                ELSE_IF_STATEMENT(B8G8R8X8_TYPELESS)
                ELSE_IF_STATEMENT(B8G8R8X8_UNORM_SRGB)
                ELSE_IF_STATEMENT(BC6H_TYPELESS)
                ELSE_IF_STATEMENT(BC6H_UF16)
                ELSE_IF_STATEMENT(BC6H_SF16)
                ELSE_IF_STATEMENT(BC7_TYPELESS)
                ELSE_IF_STATEMENT(BC7_UNORM)
                ELSE_IF_STATEMENT(BC7_UNORM_SRGB)
                ELSE_IF_STATEMENT(AYUV)
                ELSE_IF_STATEMENT(Y410)
                ELSE_IF_STATEMENT(Y416)
                ELSE_IF_STATEMENT(NV12)
                ELSE_IF_STATEMENT(P010)
                ELSE_IF_STATEMENT(P016)
                ELSE_IF_STATEMENT(YUY2)
                ELSE_IF_STATEMENT(Y210)
                ELSE_IF_STATEMENT(Y216)
                ELSE_IF_STATEMENT(NV11)
                ELSE_IF_STATEMENT(AI44)
                ELSE_IF_STATEMENT(IA44)
                ELSE_IF_STATEMENT(P8)
                ELSE_IF_STATEMENT(A8P8)
                ELSE_IF_STATEMENT(B4G4R4A4_UNORM)


            return ret;

    #undef IF_STATEMENT
    #undef ELSE_IF_STATEMENT
        }
    
        /*
        **
        */
        uint32_t _getFormatDataSize(std::string const& formatStr)
        {
            uint32_t ret = 0;

            RenderDriver::Common::Format format = _getTextureFormat(formatStr);
            switch(format)
            {
                case RenderDriver::Common::Format::R32G32B32A32_FLOAT:
                ret = 4 * sizeof(float);
                break;

            case RenderDriver::Common::Format::R32G32B32_FLOAT:
                ret = 3 * sizeof(float);
                break;

            case RenderDriver::Common::Format::R32G32_FLOAT:
                ret = 2 * sizeof(float);
                break;

            case RenderDriver::Common::Format::R32_FLOAT:
                ret = 1 * sizeof(float);
                break;

            default:
                assert(0);
            }

            return ret;
        }
    
		/*
		**
		*/
		void getPipelineShaders(
			std::vector<ShaderInfo>& aPipelineShaders,
			char const* szShaderFilePath)
		{
			FILE* fp = fopen(szShaderFilePath, "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			std::vector<char> acShadersFileBuffer(iFileSize + 1);
			memset(acShadersFileBuffer.data(), 0, iFileSize + 1);
			fseek(fp, 0, SEEK_SET);
			fread(acShadersFileBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			rapidjson::Document pipelineShadersDoc;
			pipelineShadersDoc.Parse(acShadersFileBuffer.data());

			// shaders
			auto const& aShaders = pipelineShadersDoc["Shaders"].GetArray();
			for(auto const& shader : aShaders)
			{
				char const* szShaderName = shader["Name"].GetString();
				char const* szType = shader["Type"].GetString();

				char const* szShaderFilePath = shader["FilePath"].GetString();
				char const* szShaderOutputFilePath = shader["OutputFilePath"].GetString();

				ShaderInfo shaderInfo;
				shaderInfo.mName = szShaderName;
				shaderInfo.mType = ShaderType::COMPUTE_SHADER_TYPE;
				if(std::string(szType) == "Vertex")
				{
					shaderInfo.mType = ShaderType::VERTEX_SHADER_TYPE;
				}
				else if(std::string(szType) == "Fragment")
				{
					shaderInfo.mType = ShaderType::PIXEL_SHADER_TYPE;
				}

				shaderInfo.mFilePath = szShaderFilePath;
				shaderInfo.mFileOutputPath = szShaderOutputFilePath;

				aPipelineShaders.push_back(shaderInfo);
			}
		}

		/*
		**
		*/
		void getVertexFormat(
			std::vector<RenderDriver::Common::VertexFormat>& aInputElementDescs,
			rapidjson::Document const& doc)
		{
			uint32_t iCurrOffset = 0;
			if(doc.HasMember("VertexFormat"))
			{
				auto const& aVertexEntries = doc["VertexFormat"].GetArray();
				for(auto const& entry : aVertexEntries)
				{
					RenderDriver::Common::VertexFormat desc = {};
					desc.mSemanticName = entry["Name"].GetString();
					desc.miSemanticIndex = 0;
					desc.mFormat = _getTextureFormat(entry["Format"].GetString());
					desc.miInputSlot = 0;
					desc.miAlignedByteOffset = iCurrOffset;
					desc.miInputSlot = 0;
					desc.miInstanceDataStepRate = 0;

					iCurrOffset += _getFormatDataSize(entry["Format"].GetString());

					aInputElementDescs.push_back(desc);
				}
			}
			else
			{
				RenderDriver::Common::VertexFormat desc = {};
				desc.mSemanticName = "POSITION";
				desc.miSemanticIndex = 0;
				desc.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
				desc.miInputSlot = 0;
				desc.miAlignedByteOffset = iCurrOffset;
				desc.miInstanceDataStepRate = 0;

				aInputElementDescs.push_back(desc);
				iCurrOffset += (4 * sizeof(float));

				desc.mSemanticName = "NORMAL";
				desc.miSemanticIndex = 0;
				desc.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
				desc.miInputSlot = 1;
				desc.miAlignedByteOffset = iCurrOffset;
				desc.miInstanceDataStepRate = 0;

				aInputElementDescs.push_back(desc);
				iCurrOffset += (4 * sizeof(float));

				desc.mSemanticName = "TEXCOORD";
				desc.miSemanticIndex = 0;
				desc.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
				desc.miInputSlot = 2;
				desc.miAlignedByteOffset = iCurrOffset;
				desc.miInstanceDataStepRate = 0;

				aInputElementDescs.push_back(desc);
				iCurrOffset += (4 * sizeof(float));
			}
		}

		/*
		**
		*/
		uint32_t getNumComponents(RenderDriver::Common::Format format)
		{
			uint32_t iRet = 1;
			switch(format)
			{
			case RenderDriver::Common::Format::R32G32B32A32_TYPELESS:
			case RenderDriver::Common::Format::R32G32B32A32_FLOAT:
			case RenderDriver::Common::Format::R32G32B32A32_UINT:
			case RenderDriver::Common::Format::R32G32B32A32_SINT:
			case RenderDriver::Common::Format::R16G16B16A16_TYPELESS:
			case RenderDriver::Common::Format::R16G16B16A16_FLOAT:
			case RenderDriver::Common::Format::R16G16B16A16_UNORM:
			case RenderDriver::Common::Format::R16G16B16A16_UINT:
			case RenderDriver::Common::Format::R16G16B16A16_SNORM:
			case RenderDriver::Common::Format::R16G16B16A16_SINT:
			case RenderDriver::Common::Format::R10G10B10A2_TYPELESS:
			case RenderDriver::Common::Format::R10G10B10A2_UNORM:
			case RenderDriver::Common::Format::R10G10B10A2_UINT:
			case RenderDriver::Common::Format::R11G11B10_FLOAT:
			case RenderDriver::Common::Format::R8G8B8A8_TYPELESS:
			case RenderDriver::Common::Format::R8G8B8A8_UNORM:
			case RenderDriver::Common::Format::R8G8B8A8_UNORM_SRGB:
			case RenderDriver::Common::Format::R8G8B8A8_UINT:
			case RenderDriver::Common::Format::R8G8B8A8_SNORM:
			case RenderDriver::Common::Format::R8G8B8A8_SINT:
				iRet = 4;
				break;

			case RenderDriver::Common::Format::R32G32B32_TYPELESS:
			case RenderDriver::Common::Format::R32G32B32_FLOAT:
			case RenderDriver::Common::Format::R32G32B32_UINT:
			case RenderDriver::Common::Format::R32G32B32_SINT:
				iRet = 3;
				break;

			case RenderDriver::Common::Format::R32G32_TYPELESS:
			case RenderDriver::Common::Format::R32G32_FLOAT:
			case RenderDriver::Common::Format::R32G32_UINT:
			case RenderDriver::Common::Format::R32G32_SINT:
			case RenderDriver::Common::Format::R16G16_TYPELESS:
			case RenderDriver::Common::Format::R16G16_FLOAT:
			case RenderDriver::Common::Format::R16G16_UNORM:
			case RenderDriver::Common::Format::R16G16_UINT:
			case RenderDriver::Common::Format::R16G16_SNORM:
			case RenderDriver::Common::Format::R16G16_SINT:
			case RenderDriver::Common::Format::R8G8_TYPELESS:
			case RenderDriver::Common::Format::R8G8_UNORM:
			case RenderDriver::Common::Format::R8G8_UINT:
			case RenderDriver::Common::Format::R8G8_SNORM:
			case RenderDriver::Common::Format::R8G8_SINT:
				iRet = 2;
				break;
			}

			return iRet;
		}

		/*
	**
	*/
		void getAttachments(
			std::vector<AttachmentInfo>& aAttachments,
			rapidjson::Document const& doc)
		{
			auto const& aAttachmentsJSON = doc["Attachments"].GetArray();
			for(auto const& attachmentJSON : aAttachmentsJSON)
			{
				char const* szName = attachmentJSON["Name"].GetString();

				char const* szParentJobName = "This";
				if(attachmentJSON.HasMember("ParentJobName"))
				{
					szParentJobName = attachmentJSON["ParentJobName"].GetString();
				}

				std::string typeStr = std::string(attachmentJSON["Type"].GetString());

				AttachmentTypeBits attachmentTypeBits = AttachmentTypeBits::None;
				if(typeStr == "TextureInput")
				{
					attachmentTypeBits = AttachmentTypeBits::TextureInput;
				}
				else if(typeStr == "TextureOutput")
				{
					attachmentTypeBits = AttachmentTypeBits::TextureOutput;
				}
				else if(typeStr == "BufferInput")
				{
					attachmentTypeBits = AttachmentTypeBits::BufferInput;
				}
				else if(typeStr == "BufferOutput")
				{
					attachmentTypeBits = AttachmentTypeBits::BufferOutput;
				}
				else if(typeStr == "BufferInputOutput")
				{
					attachmentTypeBits = AttachmentTypeBits::BufferInputOutput;
				}
				else if(typeStr == "TextureInputOutput")
				{
					attachmentTypeBits = AttachmentTypeBits::TextureInputOutput;
				}
				else if(typeStr == "IndirectDrawListInput")
				{
					attachmentTypeBits = AttachmentTypeBits::IndirectDrawListInput;
				}
				else if(typeStr == "IndirectDrawListOutput")
				{
					attachmentTypeBits = AttachmentTypeBits::IndirectDrawListOutput;
				}
				else if(typeStr == "VertexOutput")
				{
					attachmentTypeBits = AttachmentTypeBits::VertexOutput;
				}
				else
				{
					DEBUG_PRINTF("Invalid attachment type: %s\n", typeStr.c_str());
					assert(0);
				}

				AttachmentInfo attachment;
				attachment.mName = szName;
				attachment.mType = attachmentTypeBits;
				attachment.mParentRenderJobName = szParentJobName;

				attachment.mFormat = RenderDriver::Common::Format::R8G8B8A8_UNORM;
				if(attachmentJSON.HasMember("Format"))
				{
					char const* szFormat = attachmentJSON["Format"].GetString();
					attachment.mFormat = SerializeUtils::Common::getTextureFormat(szFormat);
				}

				if(attachmentJSON.HasMember("BlendState"))
				{
					if(std::string(attachmentJSON["BlendState"].GetString()) == "False")
					{
						attachment.mbBlend = false;
					}
				}

				if(attachmentJSON.HasMember("ScaleWidth"))
				{
					attachment.mfScaleWidth = static_cast<float>(attachmentJSON["ScaleWidth"].GetFloat());
				}

				if(attachmentJSON.HasMember("ScaleHeight"))
				{
					attachment.mfScaleHeight = static_cast<float>(attachmentJSON["ScaleHeight"].GetFloat());
				}

				//attachment.mType = _getAttachmentType(szType);
				//attachment.mFormat = _getTextureFormat(szFormat);
				//attachment.mBufferType = _getBufferType(szType);

				aAttachments.push_back(attachment);
			}
		}

		/*
		**
		*/
		RenderDriver::Common::Format getTextureFormat(std::string const& format)
		{
#define IF_STATEMENT(FORMAT)							\
			if(format == #FORMAT)						\
			{											\
				ret = RenderDriver::Common::Format::FORMAT;		\
			}

#define ELSE_IF_STATEMENT(FORMAT)						\
			else if(format == #FORMAT)					\
			{											\
				ret = RenderDriver::Common::Format::FORMAT;		\
			}

			RenderDriver::Common::Format ret = RenderDriver::Common::Format::UNKNOWN;

			IF_STATEMENT(UNKNOWN)
			ELSE_IF_STATEMENT(R32G32B32A32_TYPELESS)
			ELSE_IF_STATEMENT(R32G32B32A32_FLOAT)
			ELSE_IF_STATEMENT(R32G32B32A32_UINT)
			ELSE_IF_STATEMENT(R32G32B32A32_SINT)
			ELSE_IF_STATEMENT(R32G32B32_TYPELESS)
			ELSE_IF_STATEMENT(R32G32B32_FLOAT)
			ELSE_IF_STATEMENT(R32G32B32_UINT)
			ELSE_IF_STATEMENT(R32G32B32_SINT)
			ELSE_IF_STATEMENT(R16G16B16A16_TYPELESS)
			ELSE_IF_STATEMENT(R16G16B16A16_FLOAT)
			ELSE_IF_STATEMENT(R16G16B16A16_UNORM)
			ELSE_IF_STATEMENT(R16G16B16A16_UINT)
			ELSE_IF_STATEMENT(R16G16B16A16_SNORM)
			ELSE_IF_STATEMENT(R16G16B16A16_SINT)
			ELSE_IF_STATEMENT(R32G32_TYPELESS)
			ELSE_IF_STATEMENT(R32G32_FLOAT)
			ELSE_IF_STATEMENT(R32G32_UINT)
			ELSE_IF_STATEMENT(R32G32_SINT)
			ELSE_IF_STATEMENT(R32G8X24_TYPELESS)
			ELSE_IF_STATEMENT(D32_FLOAT_S8X24_UINT)
			ELSE_IF_STATEMENT(R32_FLOAT_X8X24_TYPELESS)
			ELSE_IF_STATEMENT(X32_TYPELESS_G8X24_UINT)
			ELSE_IF_STATEMENT(R10G10B10A2_TYPELESS)
			ELSE_IF_STATEMENT(R10G10B10A2_UNORM)
			ELSE_IF_STATEMENT(R10G10B10A2_UINT)
			ELSE_IF_STATEMENT(R11G11B10_FLOAT)
			ELSE_IF_STATEMENT(R8G8B8A8_TYPELESS)
			ELSE_IF_STATEMENT(R8G8B8A8_UNORM)
			ELSE_IF_STATEMENT(R8G8B8A8_UNORM_SRGB)
			ELSE_IF_STATEMENT(R8G8B8A8_UINT)
			ELSE_IF_STATEMENT(R8G8B8A8_SNORM)
			ELSE_IF_STATEMENT(R8G8B8A8_SINT)
			ELSE_IF_STATEMENT(R16G16_TYPELESS)
			ELSE_IF_STATEMENT(R16G16_FLOAT)
			ELSE_IF_STATEMENT(R16G16_UNORM)
			ELSE_IF_STATEMENT(R16G16_UINT)
			ELSE_IF_STATEMENT(R16G16_SNORM)
			ELSE_IF_STATEMENT(R16G16_SINT)
			ELSE_IF_STATEMENT(R32_TYPELESS)
			ELSE_IF_STATEMENT(D32_FLOAT)
			ELSE_IF_STATEMENT(R32_FLOAT)
			ELSE_IF_STATEMENT(R32_UINT)
			ELSE_IF_STATEMENT(R32_SINT)
			ELSE_IF_STATEMENT(R24G8_TYPELESS)
			ELSE_IF_STATEMENT(D24_UNORM_S8_UINT)
			ELSE_IF_STATEMENT(R24_UNORM_X8_TYPELESS)
			ELSE_IF_STATEMENT(X24_TYPELESS_G8_UINT)
			ELSE_IF_STATEMENT(R8G8_TYPELESS)
			ELSE_IF_STATEMENT(R8G8_UNORM)
			ELSE_IF_STATEMENT(R8G8_UINT)
			ELSE_IF_STATEMENT(R8G8_SNORM)
			ELSE_IF_STATEMENT(R8G8_SINT)
			ELSE_IF_STATEMENT(R16_TYPELESS)
			ELSE_IF_STATEMENT(R16_FLOAT)
			ELSE_IF_STATEMENT(D16_UNORM)
			ELSE_IF_STATEMENT(R16_UNORM)
			ELSE_IF_STATEMENT(R16_UINT)
			ELSE_IF_STATEMENT(R16_SNORM)
			ELSE_IF_STATEMENT(R16_SINT)
			ELSE_IF_STATEMENT(R8_TYPELESS)
			ELSE_IF_STATEMENT(R8_UNORM)
			ELSE_IF_STATEMENT(R8_UINT)
			ELSE_IF_STATEMENT(R8_SNORM)
			ELSE_IF_STATEMENT(R8_SINT)
			ELSE_IF_STATEMENT(A8_UNORM)
			ELSE_IF_STATEMENT(R1_UNORM)
			ELSE_IF_STATEMENT(R9G9B9E5_SHAREDEXP)
			ELSE_IF_STATEMENT(R8G8_B8G8_UNORM)
			ELSE_IF_STATEMENT(G8R8_G8B8_UNORM)


			return ret;

#undef IF_STATEMENT
#undef ELSE_IF_STATEMENT
		}

		/*
		**
		*/
		RenderDriver::Common::TextureLayout _getTextureLayout(std::string const& layout)
		{
			RenderDriver::Common::TextureLayout ret = RenderDriver::Common::TextureLayout::Unknown;
			if(layout == "Unknown")
			{
				ret = RenderDriver::Common::TextureLayout::Unknown;
			}
			else if(layout == "RowMajor")
			{
				ret = RenderDriver::Common::TextureLayout::RowMajor;
			}
			else
			{
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::Dimension _getResourceDimension(std::string const& dimension)
		{
			RenderDriver::Common::Dimension ret = RenderDriver::Common::Dimension::Unknown;
			if(dimension == "Unknown")
			{
				ret = RenderDriver::Common::Dimension::Unknown;
			}
			else if(dimension == "Buffer")
			{
				ret = RenderDriver::Common::Dimension::Buffer;
			}
			else if(dimension == "Texture1D")
			{
				ret = RenderDriver::Common::Dimension::Texture1D;
			}
			else if(dimension == "Texture2D")
			{
				ret = RenderDriver::Common::Dimension::Texture2D;
			}
			else if(dimension == "Texture3D")
			{
				ret = RenderDriver::Common::Dimension::Texture3D;
			}
			else
			{
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		uint32_t getBaseComponentSize(RenderDriver::Common::Format format)
		{
			uint32_t ret = 1;
			switch(format)
			{
			case RenderDriver::Common::Format::R32G32B32A32_FLOAT:
			case RenderDriver::Common::Format::R32G32B32_FLOAT:
			case RenderDriver::Common::Format::R32G32_FLOAT:
			case RenderDriver::Common::Format::R32_FLOAT:
				ret = 4;
				break;

			case RenderDriver::Common::Format::R16G16B16A16_FLOAT:
			case RenderDriver::Common::Format::R16G16_FLOAT:
			case RenderDriver::Common::Format::R16_FLOAT:
			case RenderDriver::Common::Format::R10G10B10A2_UNORM:
				ret = 2;
				break;

			case RenderDriver::Common::Format::R8G8B8A8_UINT:
			case RenderDriver::Common::Format::R8G8_UINT:
			case RenderDriver::Common::Format::R8_UINT:
			case RenderDriver::Common::Format::R8G8B8A8_SINT:
			case RenderDriver::Common::Format::R8G8_SINT:
			case RenderDriver::Common::Format::R8_SINT:
			case RenderDriver::Common::Format::R8G8B8A8_UNORM:
			case RenderDriver::Common::Format::R8G8_UNORM:
			case RenderDriver::Common::Format::R8_UNORM:
			case RenderDriver::Common::Format::B8G8R8A8_UNORM:
				ret = 1;
				break;

			default:
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::ResourceFlagBits _getResourceFlag(std::string const& flag)
		{
			RenderDriver::Common::ResourceFlagBits ret = RenderDriver::Common::ResourceFlagBits::None;
			if(flag == "None")
			{
				ret = RenderDriver::Common::ResourceFlagBits::None;
			}
			else if(flag == "AllowRenderTarget")
			{
				ret = RenderDriver::Common::ResourceFlagBits::AllowRenderTarget;
			}
			else if(flag == "AllowDepthStencil")
			{
				ret = RenderDriver::Common::ResourceFlagBits::AllowDepthStencil;
			}
			else if(flag == "AllowUnorderedAccess")
			{
				ret = RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess;
			}
			else if(flag == "DenyShaderResource")
			{
				ret = RenderDriver::Common::ResourceFlagBits::DenyShaderResource;
			}
			else if(flag == "AllowCrossAdapter")
			{
				ret = RenderDriver::Common::ResourceFlagBits::AllowCrossAdaptor;
			}
			else if(flag == "AllowSimultaneousAccess")
			{
				ret = RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess;
			}
			else if(flag == "VideoDecodeReferenceOnly")
			{
				ret = RenderDriver::Common::ResourceFlagBits::VideoDecodeReferenceOnly;
			}
			else
			{
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		void loadShaderResources3(
			std::vector<SerializeUtils::Common::ShaderResourceInfo>& aResourceDesc,
			std::string const& shaderResourceFilePath,
			std::string const& shaderRegisterFilePath,
			char const* szShaderType,
			bool bFilloutConstantBufferEntries)
		{
			FILE* fp = fopen(shaderResourceFilePath.c_str(), "rb");
			if(fp == nullptr)
			{
				DEBUG_PRINTF("No resource file %s\n", shaderResourceFilePath.c_str());
				WTFASSERT(0, "Can\'t find JSON shader resource file file: %s", shaderResourceFilePath.c_str());
				return;
			}

			// shader resources that are valid and being used
			rapidjson::Document doc;
			{
				std::vector<char> acBuffer;
				FILE* fp = fopen(shaderResourceFilePath.c_str(), "rb");
				fseek(fp, 0, SEEK_END);
				size_t iFileSize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				acBuffer.resize(iFileSize + 1);
				memset(acBuffer.data(), 0, iFileSize + 1);
				fread(acBuffer.data(), sizeof(char), iFileSize, fp);
				doc.Parse(acBuffer.data());
			}

			// all the shader resources, even ones not used
			rapidjson::Document registerDoc;
			{
				std::vector<char> acBuffer;
				FILE* fp = fopen(shaderRegisterFilePath.c_str(), "rb");
				fseek(fp, 0, SEEK_END);
				size_t iFileSize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				acBuffer.resize(iFileSize + 1);
				memset(acBuffer.data(), 0, iFileSize + 1);
				fread(acBuffer.data(), sizeof(char), iFileSize, fp);
				registerDoc.Parse(acBuffer.data());
			}

DEBUG_PRINTF("Registers\n");
for(rapidjson::GenericMemberIterator totalIter = registerDoc.MemberBegin();
	totalIter != registerDoc.MemberEnd();
	++totalIter)
{
	std::string checkRegisterName = std::string(totalIter->name.GetString());
	std::string shaderResourceName = totalIter->value.GetArray()[0].GetString();
	std::string shaderResourceDataType = totalIter->value.GetArray()[1].GetString();
				
	DEBUG_PRINTF("\t%s : [%s, %s]\n",
		checkRegisterName.c_str(),
		shaderResourceName.c_str(),
		shaderResourceDataType.c_str());
}

DEBUG_PRINTF("\n");

DEBUG_PRINTF("Register Info\n");

			auto findResourceNameFromRegister = [&](std::string const& registerName)
			{
				// find the resource name from the total dictionary
				std::string origName = "";
				for(rapidjson::GenericMemberIterator totalIter = registerDoc.MemberBegin();
					totalIter != registerDoc.MemberEnd();
					++totalIter)
				{
					std::string checkRegisterName = std::string(totalIter->name.GetString());
					if(checkRegisterName == registerName)
					{
						origName = totalIter->value.GetArray()[0].GetString();
						break;
					}
				}

				//WTFASSERT(origName.length() > 0, "Can\'t find resource for register: %s\n", registerName.c_str());
				if(origName.length() <= 0)
				{
					DEBUG_PRINTF("Can\'t find resource for register: %s\n", registerName.c_str());
				}

				return origName;
			};

			auto findResourceDataTypeFromRegister = [&](std::string const& registerName)
			{
				// find the resource name from the total dictionary
				std::string origName = "";
				for(rapidjson::GenericMemberIterator totalIter = registerDoc.MemberBegin();
					totalIter != registerDoc.MemberEnd();
					++totalIter)
				{
					std::string checkRegisterName = std::string(totalIter->name.GetString());
					if(checkRegisterName == registerName)
					{
						origName = totalIter->value.GetArray()[1].GetString();
						break;
					}
				}

				WTFASSERT(origName.length() > 0, "Can\'t find resource for register: %s\n", registerName.c_str());

				return origName;
			};

			auto addShaderResourceFiller = [&](
				std::string const& hlslType, 
				uint32_t iStartShaderResource, 
				uint32_t iAddition,
				uint32_t iShaderResource)
			{
				SerializeUtils::Common::ShaderResourceInfo filler = {};

				std::ostringstream oss;
				oss << hlslType;
				oss << iStartShaderResource + iAddition;
				std::string registerName = oss.str();

				char const* szType = nullptr;
				std::string shaderResourceName = findResourceNameFromRegister(registerName);
				if(shaderResourceName.length() <= 0)
				{
					shaderResourceName = "None";
					szType = "ByteAddressBuffer";
				}
				else
				{
					szType = registerDoc[registerName.c_str()][1].GetString();
				}

				RenderDriver::Common::ResourceViewType viewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
				ShaderResourceType resourceType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
				uint32_t iWidth = 1;
				if(std::string(szType).find("ByteAddressBuffer") != std::string::npos)
				{
					viewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
					iWidth = 65536 + 1;
				}
				else if(std::string(szType).find("Texture") != std::string::npos)
				{
					resourceType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
				}

				filler.mName = "fillerShaderResource (" + shaderResourceName + ")";
				filler.miResourceIndex = (uint32_t)aResourceDesc.size();
				filler.mDesc.miAlignment = 1;
				filler.mDesc.miDepthOrArraySize = 1;
				filler.mDesc.miWidth = iWidth;
				filler.mDesc.miHeight = 1;
				filler.mDesc.miMipLevels = 1;
				filler.mDesc.mSampleDesc.miCount = 1;
				filler.mDesc.mSampleDesc.miQuality = 0;
				filler.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
				filler.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
				filler.mType = resourceType;
				filler.mExternalResource.mbSkipCreation = true;
				if(!strcmp(szShaderType, "Compute"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Compute;
				}
				else if(!strcmp(szShaderType, "Fragment"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Fragment;
				}
				else if(!strcmp(szShaderType, "Vertex"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Vertex;
				}

DEBUG_PRINTF("\t\tfill t%d %s\n", iShaderResource, shaderResourceName.c_str());
				aResourceDesc.push_back(filler);
			};

			auto addUnorderedShaderResourceFiller = [&](
				std::string const& hlslType,
				uint32_t iStartUnorderedAccessResource,
				uint32_t iAddition,
				uint32_t iUnorderAccessResource)
			{
				SerializeUtils::Common::ShaderResourceInfo filler = {};

				std::ostringstream oss;
				oss << hlslType;
				oss << iStartUnorderedAccessResource + iAddition;
				std::string registerName = oss.str();

				std::string shaderResourceName = findResourceNameFromRegister(registerName);

				char const* szType = nullptr;
				if(shaderResourceName.length() <= 0)
				{
					shaderResourceName = "None";
					szType = "ByteAddressBuffer";
				}
				else
				{
					szType = registerDoc[registerName.c_str()][1].GetString();
				}

				RenderDriver::Common::ResourceViewType viewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
				ShaderResourceType resourceType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
				uint32_t iWidth = 1;
				if(std::string(szType).find("ByteAddressBuffer") != std::string::npos)
				{
					viewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
					iWidth = 65536 + 1;
				}
				else if(std::string(szType).find("Texture") != std::string::npos)
				{
					resourceType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
				}

				filler.mName = "fillerUnorderedAccessResource (" + shaderResourceName + ")";
				filler.miResourceIndex = (uint32_t)aResourceDesc.size();
				filler.mDesc.miAlignment = 1;
				filler.mDesc.miDepthOrArraySize = 1;
				filler.mDesc.miWidth = iWidth;
				filler.mDesc.miHeight = 1;
				filler.mDesc.miMipLevels = 1;
				filler.mDesc.mSampleDesc.miCount = 1;
				filler.mDesc.mSampleDesc.miQuality = 0;
				filler.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
				filler.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
				filler.mType = resourceType;
				filler.mExternalResource.mbSkipCreation = true;
				if(!strcmp(szShaderType, "Compute"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Compute;
				}
				else if(!strcmp(szShaderType, "Fragment"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Fragment;
				}
				else if(!strcmp(szShaderType, "Vertex"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Vertex;
				}

DEBUG_PRINTF("\t\tfill u%d %s\n", iUnorderAccessResource, shaderResourceName.c_str());

				aResourceDesc.push_back(filler);
			};

			auto addConstantBufferFiller = [&](
				std::string const& hlslType,
				uint32_t iStartConstantBuffer,
				uint32_t iAddition,
				uint32_t iUnorderAccessResource)
			{
				SerializeUtils::Common::ShaderResourceInfo filler = {};

				std::ostringstream oss;
				oss << hlslType;
				oss << iStartConstantBuffer + iAddition;
				std::string registerName = oss.str();

				std::string constantBufferName = findResourceNameFromRegister(registerName);
				uint32_t iWidth = 1 << 8;

				filler.mName = "fillerConstantBuffer (" + constantBufferName + ")";
				filler.miResourceIndex = (uint32_t)aResourceDesc.size();
				filler.mDesc.miAlignment = 1;
				filler.mDesc.miDepthOrArraySize = 1;
				filler.mDesc.miWidth = iWidth;
				filler.mDesc.miHeight = 1;
				filler.mDesc.miMipLevels = 1;
				filler.mDesc.mSampleDesc.miCount = 1;
				filler.mDesc.mSampleDesc.miQuality = 0;
				filler.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
				filler.mViewType = RenderDriver::Common::ResourceViewType::ConstantBufferView;
				filler.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
				filler.mExternalResource.mbSkipCreation = true;
				if(!strcmp(szShaderType, "Compute"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Compute;
				}
				else if(!strcmp(szShaderType, "Fragment"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Fragment;
				}
				else if(!strcmp(szShaderType, "Vertex"))
				{
					filler.mShaderType = RenderDriver::Common::ShaderType::Vertex;
				}

				DEBUG_PRINTF("\t\tfill b%d %s\n", iUnorderAccessResource, constantBufferName.c_str());

				aResourceDesc.push_back(filler);
			};

			// get the largest register indices
			uint32_t iLargestUAV = UINT32_MAX, iLargestSRV = UINT32_MAX, iLargestCBV = UINT32_MAX;
			for(rapidjson::GenericMemberIterator totalIter = registerDoc.MemberBegin();
				totalIter != registerDoc.MemberEnd();
				++totalIter)
			{
				char const* szRegisterName = totalIter->name.GetString();
				uint32_t iRegisterIndex = atoi(&szRegisterName[1]);
				if(szRegisterName[0] == 'u')
				{
					iLargestUAV = (iLargestUAV == UINT32_MAX || iRegisterIndex > iLargestUAV) ? iRegisterIndex : iLargestUAV;
				}
				else if(szRegisterName[0] == 't')
				{
					iLargestSRV = (iLargestSRV == UINT32_MAX || iRegisterIndex > iLargestSRV) ? iRegisterIndex : iLargestSRV;
				}
				else if(szRegisterName[0] == 'b')
				{
					iLargestCBV = (iLargestCBV == UINT32_MAX || iRegisterIndex > iLargestCBV) ? iRegisterIndex : iLargestCBV;
				}
			}

			// shader resources in use
			uint32_t iShaderResource = 0, iUnorderAccessResource = 0, iConstantBuffer = 0;
			for(rapidjson::Value::ConstMemberIterator iter = doc["Variables"].MemberBegin();
				iter != doc["Variables"].MemberEnd();
				++iter)
			{
				SerializeUtils::Common::ShaderResourceInfo shaderResource = {};

				shaderResource.mName = iter->name.GetString();
				shaderResource.miResourceIndex = (uint32_t)aResourceDesc.size();

				shaderResource.mDesc.miAlignment = 1;
				shaderResource.mDesc.miDepthOrArraySize = 1;
				shaderResource.mDesc.miWidth = 1;
				shaderResource.mDesc.miHeight = 1;
				shaderResource.mDesc.miMipLevels = 1;
				shaderResource.mDesc.mSampleDesc.miCount = 1;
				shaderResource.mDesc.mSampleDesc.miQuality = 0;
				shaderResource.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;

				uint32_t iResoureceFlagBits = 0;

				auto const& variableInfo = doc["Variables"][shaderResource.mName.c_str()];
				std::string structName = variableInfo["Struct"].GetString();

				std::string count = variableInfo["Count"].GetString();
				if(count == "unbounded")
				{
					shaderResource.mDesc.miCount = UINT32_MAX;
				}
				else
				{
					shaderResource.mDesc.miCount = atoi(count.c_str());
				}

				// add filler shader resources (null descriptor) 
				{
					// get binding index and type
					std::string hlslIndex = "";
					std::string hlslType = "";
					std::string hlslBinding = variableInfo["HLSLBind"].GetString();

DEBUG_PRINTF("\t%s %s\n", hlslBinding.c_str(), shaderResource.mName.c_str());

					uint32_t iStart = UINT32_MAX;
					for(uint32_t iChar = 0; iChar < static_cast<uint32_t>(hlslBinding.size()); iChar++)
					{
						if(hlslBinding[iChar] >= '0' && hlslBinding[iChar] <= '9')
						{
							hlslType = hlslBinding.substr(0, iChar);
							hlslIndex = hlslBinding.substr(iChar);
							break;
						}
					}
					WTFASSERT(hlslIndex.length() > 0, "invalid hlsl binding");

					// get the ID register index
					std::string ID = variableInfo["ID"].GetString();
					std::string idIndexStr = "";
					for(uint32_t iChar = 0; iChar < static_cast<uint32_t>(ID.size()); iChar++)
					{
						if(ID[iChar] >= '0' && ID[iChar] <= '9')
						{
							idIndexStr = ID.substr(iChar);
							break;
						}
					}
					WTFASSERT(idIndexStr.length() > 0, "invalid id register index");
					uint32_t iIDRegisterIndex = atoi(idIndexStr.c_str());

					// add fillers between the binding index
					uint32_t iBindingIndex = atoi(hlslIndex.c_str());
					if(hlslType == "cb" && bFilloutConstantBufferEntries)
					{
						if(iBindingIndex > iConstantBuffer)
						{
							uint32_t iStartConstantBuffer = iConstantBuffer;
							uint32_t iDiff = iBindingIndex - iConstantBuffer;
							for(uint32_t iAddition = 0; iAddition < iDiff; iAddition++)
							{
								addConstantBufferFiller(
									hlslType,
									iStartConstantBuffer,
									iAddition,
									iConstantBuffer);
							}
						}
					}
					else if(hlslType == "t")
					{
						if(iBindingIndex > iShaderResource)
						{
							uint32_t iStartShaderResource = iShaderResource;
							uint32_t iDiff = iBindingIndex - iShaderResource;
							for(uint32_t iAddition = 0; iAddition < iDiff; iAddition++)
							{
								addShaderResourceFiller(
									hlslType,
									iStartShaderResource,
									iAddition,
									iShaderResource);

								++iShaderResource;
							}
						}
					}
					else if(hlslType == "u")
					{
						// start of next same type of shader resource
						if(iUnorderAccessResource == 0)
						{
							if(iShaderResource < iLargestSRV + 1)
							{
								uint32_t iStartShaderResource = iShaderResource;
								uint32_t iDiff = (iLargestSRV + 1) - iShaderResource;
								for(uint32_t i = 0; i < iDiff; i++)
								{
									addShaderResourceFiller(
										"t",
										iStartShaderResource,
										i,
										iShaderResource);
									++iShaderResource;
								}
							}
						}

						if(iBindingIndex > iUnorderAccessResource)
						{
							uint32_t iStartUnorderdAccessResource = iUnorderAccessResource;
							uint32_t iDiff = iBindingIndex - iUnorderAccessResource;
							for(uint32_t iAddition = 0; iAddition < iDiff; iAddition++)
							{
								addUnorderedShaderResourceFiller(
									hlslType,
									iStartUnorderdAccessResource,
									iAddition,
									iUnorderAccessResource);

								++iUnorderAccessResource;
							}
						}
					}

					// keeps track of resource type
					if(hlslType == "t")
					{
						++iShaderResource;
					}
					else if(hlslType == "u")
					{
						++iUnorderAccessResource;
					}
					else if(hlslType == "cb")
					{
						++iConstantBuffer;
					}

				}	// add filler shader resources


				std::string dataType = "buffer";
				if(variableInfo.HasMember("DataType"))
				{
					dataType = std::string(variableInfo["DataType"].GetString());
				}

				std::string viewType = std::string(variableInfo["Type"].GetString());
				if(viewType == "cbuffer")
				{
					shaderResource.mViewType = RenderDriver::Common::ResourceViewType::ConstantBufferView;
				}
				else if(viewType == "SRV")
				{
					shaderResource.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
				}
				else if(viewType == "UAV")
				{
					shaderResource.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
				}
				else
				{
					assert(!"unsupported view type");
				}

				bool bHasDataStructure = doc["Structs"].HasMember(structName.c_str());

				// type, dimension, flag bits, struct byte stride
				if(structName == "struct" || bHasDataStructure)
				{
					shaderResource.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
					shaderResource.mDesc.mLayout = RenderDriver::Common::TextureLayout::RowMajor;

					if(std::string(variableInfo["Dimension"].GetString()).find("r/o") != std::string::npos)
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					}
					else if(std::string(variableInfo["Dimension"].GetString()).find("r/w") != std::string::npos)
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
						iResoureceFlagBits |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess);
					}
					else
					{
						if(dataType == "buffer")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
						}
						else if(dataType == "texture")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
						}
					}

					std::string viewType = variableInfo["Type"].GetString();
					if(viewType == "UAV")
					{
						shaderResource.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
					}

std::string structName = variableInfo["Struct"].GetString();
char const* szSize = doc["Structs"][structName.c_str()]["Size"].GetString();
shaderResource.miStructByteStride = (uint32_t)atoi(szSize);
				}
				else
				{
				shaderResource.mDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
				std::string dimension = variableInfo["Dimension"].GetString();
				if(dimension.find("r/o") != std::string::npos)
				{
					if(dataType == "buffer")
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					}
					else
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
					}
				}
				else if(dimension.find("r/w") != std::string::npos)
				{
					if(dataType == "buffer")
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
					}
					else
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT;
						iResoureceFlagBits |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowRenderTarget);
					}
				}
				else if(dimension.find("buf") != std::string::npos)
				{
					shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					if(std::string(variableInfo["Type"].GetString()) == "UAV")
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
						iResoureceFlagBits |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess);
						shaderResource.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
					}

					shaderResource.miStructByteStride = 4;
					std::string format = std::string(variableInfo["Format"].GetString());
					if(format == "u32")
					{
						shaderResource.miStructByteStride = 4;
					}
				}
				else if(dimension.find("2d") != std::string::npos)
				{
					// 2d texture
					std::string format = std::string(variableInfo["Format"].GetString());
					if(format == "f32")
					{
						shaderResource.mDesc.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
					}
				}
				else if(dimension.find("NA") != std::string::npos)
				{
					// buffer in/out to specify as root constant
					if(viewType == "cbuffer")
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
						shaderResource.mViewType = RenderDriver::Common::ResourceViewType::ConstantBufferView;
						shaderResource.mExternalResource.mbSkipCreation = true;
					}
				}
				}

				shaderResource.mDesc.mFlags = static_cast<RenderDriver::Common::ResourceFlagBits>(iResoureceFlagBits);

				if(!strcmp(szShaderType, "Compute"))
				{
					shaderResource.mShaderType = RenderDriver::Common::ShaderType::Compute;
				}
				else if(!strcmp(szShaderType, "Fragment"))
				{
					shaderResource.mShaderType = RenderDriver::Common::ShaderType::Fragment;
				}
				else if(!strcmp(szShaderType, "Vertex"))
				{
					shaderResource.mShaderType = RenderDriver::Common::ShaderType::Vertex;
				}

				// check for un-initialized texture
				if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT ||
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT)
				{
					assert(shaderResource.mDesc.mFormat != RenderDriver::Common::Format::UNKNOWN);
				}

				aResourceDesc.push_back(shaderResource);

			}	// for variable in variables

			// check if there are missing resources at the end

			// fill out the rest
			// shader resource
			if(iLargestSRV != UINT32_MAX && iShaderResource < iLargestSRV + 1)
			{
				uint32_t iStartShaderResource = iShaderResource;
				iLargestSRV += 1;
				uint32_t iDiff = iLargestSRV - iShaderResource;
				for(uint32_t i = 0; i < iDiff; i++)
				{
					std::ostringstream oss;
					oss << "t" << iStartShaderResource + i;
					std::string registerName = oss.str();

					// look for the info
					std::string shaderResourceName = findResourceNameFromRegister(registerName);
					std::string shaderResourceDataType = findResourceDataTypeFromRegister(registerName);
					
					RenderDriver::Common::ResourceViewType viewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
					ShaderResourceType resourceType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					uint32_t iWidth = 1;
					if(shaderResourceDataType.find("ByteAddressBuffer") != std::string::npos)
					{
						viewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
						iWidth = 65536 + 1;
					}
					else if(shaderResourceDataType.find("Texture") != std::string::npos)
					{
						resourceType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
					}

					SerializeUtils::Common::ShaderResourceInfo filler = {};

					filler.mName = "fillerShaderResource (" + shaderResourceName + ")";
					filler.miResourceIndex = (uint32_t)aResourceDesc.size();
					filler.mDesc.miAlignment = 1;
					filler.mDesc.miDepthOrArraySize = 1;
					filler.mDesc.miWidth = iWidth;
					filler.mDesc.miHeight = 1;
					filler.mDesc.miMipLevels = 1;
					filler.mDesc.mSampleDesc.miCount = 1;
					filler.mDesc.mSampleDesc.miQuality = 0;
					filler.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
					filler.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
					filler.mType = resourceType;
					filler.mExternalResource.mbSkipCreation = true;
					if(!strcmp(szShaderType, "Compute"))
					{
						filler.mShaderType = RenderDriver::Common::ShaderType::Compute;
					}
					else if(!strcmp(szShaderType, "Fragment"))
					{
						filler.mShaderType = RenderDriver::Common::ShaderType::Fragment;
					}
					else if(!strcmp(szShaderType, "Vertex"))
					{
						filler.mShaderType = RenderDriver::Common::ShaderType::Vertex;
					}

DEBUG_PRINTF("\t\tfill end t%d name: %s\n", iShaderResource, filler.mName.c_str());

					aResourceDesc.push_back(filler);
					++iShaderResource;
				}
			}

			// unordered access resource
			if(iLargestUAV != UINT32_MAX && iUnorderAccessResource < iLargestUAV + 1)
			{
				uint32_t iStartUnorderAccessResource = iUnorderAccessResource;
				iLargestUAV += 1;
				uint32_t iDiff = iLargestUAV - iUnorderAccessResource;
				for(uint32_t i = 0; i < iDiff; i++)
				{
					std::ostringstream oss;
					oss << "u" << iStartUnorderAccessResource + i;
					std::string registerName = oss.str();

					// look for the info
					std::string shaderResourceName = findResourceNameFromRegister(registerName);
					std::string shaderResourceDataType = findResourceDataTypeFromRegister(registerName);
					
					SerializeUtils::Common::ShaderResourceInfo filler = {};

					RenderDriver::Common::ResourceViewType viewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
					ShaderResourceType resourceType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					uint32_t iWidth = 1;
					if(shaderResourceDataType.find("ByteAddressBuffer") != std::string::npos)
					{
						viewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
						iWidth = 65536 + 1;
					}
					else if(shaderResourceDataType.find("Texture") != std::string::npos)
					{
						resourceType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
					}

					filler.mName = "fillerUnorderedAccessResource (" + shaderResourceName + ")";
					filler.miResourceIndex = (uint32_t)aResourceDesc.size();
					filler.mDesc.miAlignment = 1;
					filler.mDesc.miDepthOrArraySize = 1;
					filler.mDesc.miWidth = iWidth;
					filler.mDesc.miHeight = 1;
					filler.mDesc.miMipLevels = 1;
					filler.mDesc.mSampleDesc.miCount = 1;
					filler.mDesc.mSampleDesc.miQuality = 0;
					filler.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
					filler.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
					filler.mType = resourceType;
					filler.mExternalResource.mbSkipCreation = true;
					if(!strcmp(szShaderType, "Compute"))
					{
						filler.mShaderType = RenderDriver::Common::ShaderType::Compute;
					}
					else if(!strcmp(szShaderType, "Fragment"))
					{
						filler.mShaderType = RenderDriver::Common::ShaderType::Fragment;
					}
					else if(!strcmp(szShaderType, "Vertex"))
					{
						filler.mShaderType = RenderDriver::Common::ShaderType::Vertex;
					}

DEBUG_PRINTF("\t\tfill end u%d name: %s\n", iUnorderAccessResource, filler.mName.c_str());

					aResourceDesc.push_back(filler);
					++iUnorderAccessResource;
				}
			}
		}

		/*
		**
		*/
		void loadShaderResources2(
			std::vector<SerializeUtils::Common::ShaderResourceInfo>& aResourceDesc,
			std::string const& shaderResourceFilePath,
			std::string const& shaderRegisterFilePath,
			char const* szShaderType)
		{
			FILE* fp = fopen(shaderResourceFilePath.c_str(), "rb");
			if(fp == nullptr)
			{
				DEBUG_PRINTF("No resource file %s\n", shaderResourceFilePath.c_str());
				WTFASSERT(0, "Can\'t find JSON shader resource file file: %s", shaderResourceFilePath.c_str());
				return;
			}

			rapidjson::Document doc;
			{
				std::vector<char> acBuffer;
				FILE* fp = fopen(shaderResourceFilePath.c_str(), "rb");
				fseek(fp, 0, SEEK_END);
				size_t iFileSize = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				acBuffer.resize(iFileSize + 1);
				memset(acBuffer.data(), 0, iFileSize + 1);
				fread(acBuffer.data(), sizeof(char), iFileSize, fp);
				doc.Parse(acBuffer.data());
			}

			std::vector<SerializeUtils::Common::ShaderResourceInfo> aShaderResources;

			uint32_t iShaderResource = 0, iUnorderAccessResource = 0, iConstantBuffer = 0;
			for(rapidjson::Value::ConstMemberIterator iter = doc["Variables"].MemberBegin();
				iter != doc["Variables"].MemberEnd();
				++iter)
			{
				SerializeUtils::Common::ShaderResourceInfo shaderResource = {};

				shaderResource.mName = iter->name.GetString();
				shaderResource.miResourceIndex = (uint32_t)aResourceDesc.size();

				shaderResource.mDesc.miAlignment = 1;
				shaderResource.mDesc.miDepthOrArraySize = 1;
				shaderResource.mDesc.miWidth = 1;
				shaderResource.mDesc.miHeight = 1;
				shaderResource.mDesc.miMipLevels = 1;
				shaderResource.mDesc.mSampleDesc.miCount = 1;
				shaderResource.mDesc.mSampleDesc.miQuality = 0;
				shaderResource.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;

				uint32_t iResoureceFlagBits = 0;

				auto const& variableInfo = doc["Variables"][shaderResource.mName.c_str()];
				std::string structName = variableInfo["Struct"].GetString();

				std::string count = variableInfo["Count"].GetString();
				if(count == "unbounded")
				{
					shaderResource.mDesc.miCount = UINT32_MAX;
				}
				else
				{
					shaderResource.mDesc.miCount = atoi(count.c_str());
				}

				// add filler shader resources (null descriptor) 
				{
					// get binding index and type
					std::string hlslIndex = "";
					std::string hlslType = "";
					std::string hlslBinding = variableInfo["HLSLBind"].GetString();
					uint32_t iStart = UINT32_MAX;
					for(uint32_t iChar = 0; iChar < static_cast<uint32_t>(hlslBinding.size()); iChar++)
					{
						if(hlslBinding[iChar] >= '0' && hlslBinding[iChar] <= '9')
						{
							hlslType = hlslBinding.substr(0, iChar);
							hlslIndex = hlslBinding.substr(iChar);
							break;
						}
					}
					WTFASSERT(hlslIndex.length() > 0, "invalid hlsl binding");

					// add fillers between the binding index
					uint32_t iBindingIndex = atoi(hlslIndex.c_str());
					if(hlslType == "t")
					{
						if(iBindingIndex > iShaderResource)
						{
							uint32_t iDiff = iBindingIndex - iShaderResource;
							for(uint32_t iAddition = 0; iAddition < iDiff; iAddition++)
							{
								SerializeUtils::Common::ShaderResourceInfo filler = {};

								filler.mName = "fillerShaderResource";
								filler.miResourceIndex = (uint32_t)aResourceDesc.size();
								filler.mDesc.miAlignment = 1;
								filler.mDesc.miDepthOrArraySize = 1;
								filler.mDesc.miWidth = 1;
								filler.mDesc.miHeight = 1;
								filler.mDesc.miMipLevels = 1;
								filler.mDesc.mSampleDesc.miCount = 1;
								filler.mDesc.mSampleDesc.miQuality = 0;
								filler.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
								filler.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
								filler.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
								filler.mExternalResource.mbSkipCreation = true;
								aResourceDesc.push_back(filler);

								++iShaderResource;
							}
						}
					}
					else if(hlslType == "u")
					{
						if(iBindingIndex > iUnorderAccessResource)
						{
							uint32_t iDiff = iBindingIndex - iUnorderAccessResource;
							for(uint32_t iAddition = 0; iAddition < iDiff; iAddition++)
							{
								SerializeUtils::Common::ShaderResourceInfo filler = {};

								filler.mName = "fillerUnorderedAccessResource";
								filler.miResourceIndex = (uint32_t)aResourceDesc.size();
								filler.mDesc.miAlignment = 1;
								filler.mDesc.miDepthOrArraySize = 1;
								filler.mDesc.miWidth = 1;
								filler.mDesc.miHeight = 1;
								filler.mDesc.miMipLevels = 1;
								filler.mDesc.mSampleDesc.miCount = 1;
								filler.mDesc.mSampleDesc.miQuality = 0;
								filler.mDesc.mViewFormat = RenderDriver::Common::Format::UNKNOWN;
								filler.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
								filler.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
								filler.mExternalResource.mbSkipCreation = true;
								aResourceDesc.push_back(filler);

								++iUnorderAccessResource;
							}
						}
					}

					// keeps track of resource type
					if(hlslType == "t")
					{
						++iShaderResource;
					}
					else if(hlslType == "u")
					{
						++iUnorderAccessResource;
					}
					else if(hlslType == "cb")
					{
						++iConstantBuffer;
					}

				}	// add filler shader resources


				std::string dataType = "buffer";
				if(variableInfo.HasMember("DataType"))
				{
					dataType = std::string(variableInfo["DataType"].GetString());
				}

				std::string viewType = std::string(variableInfo["Type"].GetString());
				if(viewType == "cbuffer")
				{
					shaderResource.mViewType = RenderDriver::Common::ResourceViewType::ConstantBufferView;
				}
				else if(viewType == "SRV")
				{
					shaderResource.mViewType = RenderDriver::Common::ResourceViewType::ShaderResourceView;
				}
				else if(viewType == "UAV")
				{
					shaderResource.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
				}
				else
				{
					assert(!"unsupported view type");
				}

				bool bHasDataStructure = doc["Structs"].HasMember(structName.c_str());

				// type, dimension, flag bits, struct byte stride
				if(structName == "struct" || bHasDataStructure)
				{
					shaderResource.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
					shaderResource.mDesc.mLayout = RenderDriver::Common::TextureLayout::RowMajor;

					if(std::string(variableInfo["Dimension"].GetString()).find("r/o") != std::string::npos)
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
					}
					else if(std::string(variableInfo["Dimension"].GetString()).find("r/w") != std::string::npos)
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
						iResoureceFlagBits |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess);
					}
					else
					{
						if(dataType == "buffer")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
						}
						else if(dataType == "texture")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
						}
					}

					std::string viewType = variableInfo["Type"].GetString();
					if(viewType == "UAV")
					{
						shaderResource.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
					}

					std::string structName = variableInfo["Struct"].GetString();
					char const* szSize = doc["Structs"][structName.c_str()]["Size"].GetString();
					shaderResource.miStructByteStride = (uint32_t)atoi(szSize);
				}
				else
				{
					shaderResource.mDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
					std::string dimension = variableInfo["Dimension"].GetString();
					if(dimension.find("r/o") != std::string::npos)
					{
						if(dataType == "buffer")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
						}
						else
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
						}
					}
					else if(dimension.find("r/w") != std::string::npos)
					{
						if(dataType == "buffer")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT;
						}
						else
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT;
							iResoureceFlagBits |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowRenderTarget);
						}
					}
					else if(dimension.find("buf") != std::string::npos)
					{
						shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
						if(std::string(variableInfo["Type"].GetString()) == "UAV")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
							iResoureceFlagBits |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess);
							shaderResource.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
						}

						shaderResource.miStructByteStride = 4;
						std::string format = std::string(variableInfo["Format"].GetString());
						if(format == "u32")
						{
							shaderResource.miStructByteStride = 4;
						}
					}
					else if(dimension.find("2d") != std::string::npos)
					{
						// 2d texture
						std::string format = std::string(variableInfo["Format"].GetString());
						if(format == "f32")
						{
							shaderResource.mDesc.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
						}
					}
					else if(dimension.find("NA") != std::string::npos)
					{
						// buffer in/out to specify as root constant
						if(viewType == "cbuffer")
						{
							shaderResource.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT;
							shaderResource.mViewType = RenderDriver::Common::ResourceViewType::ConstantBufferView;
							shaderResource.mExternalResource.mbSkipCreation = true;
						}
					}
				}

				shaderResource.mDesc.mFlags = static_cast<RenderDriver::Common::ResourceFlagBits>(iResoureceFlagBits);

				if(!strcmp(szShaderType, "Compute"))
				{
					shaderResource.mShaderType = RenderDriver::Common::ShaderType::Compute;
				}
				else if(!strcmp(szShaderType, "Fragment"))
				{
					shaderResource.mShaderType = RenderDriver::Common::ShaderType::Fragment;
				}
				else if(!strcmp(szShaderType, "Vertex"))
				{
					shaderResource.mShaderType = RenderDriver::Common::ShaderType::Vertex;
				}

				// check for un-initialized texture
				if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT ||
					shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT)
				{
					assert(shaderResource.mDesc.mFormat != RenderDriver::Common::Format::UNKNOWN);
				}

				aResourceDesc.push_back(shaderResource);

			}	// for variable in variables
		}

		/*
		**
		*/
		RenderDriver::Common::BlendType setBlendType(std::string const& type)
		{
			RenderDriver::Common::BlendType ret = RenderDriver::Common::BlendType::Zero;
			if(type == "Zero")
			{
				ret = RenderDriver::Common::BlendType::Zero;
			}
			else if(type == "One")
			{
				ret = RenderDriver::Common::BlendType::One;
			}
			else if(type == "SrcColor")
			{
				ret = RenderDriver::Common::BlendType::SrcColor;
			}
			else if(type == "InvSrcColor")
			{
				ret = RenderDriver::Common::BlendType::InvSrcColor;
			}
			else if(type == "SrcAlpha")
			{
				ret = RenderDriver::Common::BlendType::SrcAlpha;
			}
			else if(type == "InvSrcAlpha")
			{
				ret = RenderDriver::Common::BlendType::InvSrcAlpha;
			}
			else if(type == "DestAlpha")
			{
				ret = RenderDriver::Common::BlendType::DestAlpha;
			}
			else if(type == "InvDestAlpha")
			{
				ret = RenderDriver::Common::BlendType::InvDestAlpha;
			}
			else if(type == "DestColor")
			{
				ret = RenderDriver::Common::BlendType::DestColor;
			}
			else if(type == "InvDestColor")
			{
				ret = RenderDriver::Common::BlendType::InvDestColor;
			}
			else if(type == "SrcAlphaSaturate")
			{
				ret = RenderDriver::Common::BlendType::SrcAlphaSaturate;
			}
			else if(type == "BlendFactor")
			{
				ret = RenderDriver::Common::BlendType::BlendFactor;
			}
			else if(type == "InvBlendFactor")
			{
				ret = RenderDriver::Common::BlendType::InvBlendFactor;
			}
			else if(type == "Src1Color")
			{
				ret = RenderDriver::Common::BlendType::Src1Color;
			}
			else if(type == "InvSrc1Color")
			{
				ret = RenderDriver::Common::BlendType::InvSrc1Color;
			}
			else if(type == "Src1Alpha")
			{
				ret = RenderDriver::Common::BlendType::Src1Alpha;
			}
			else if(type == "InvSrc1Alpha")
			{
				ret = RenderDriver::Common::BlendType::InvSrc1Alpha;
			}
			else
			{
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::BlendOperator setBlendOpType(std::string const& type)
		{
			RenderDriver::Common::BlendOperator ret;
			if(type == std::string("Add"))
			{
				ret = RenderDriver::Common::BlendOperator::Add;
			}
			else if(type == std::string("Subtract"))
			{
				ret = RenderDriver::Common::BlendOperator::Subtract;
			}
			else if(type == std::string("ReverseSubtract"))
			{
				ret = RenderDriver::Common::BlendOperator::ReverseSubtract;
			}
			else if(type == std::string("Min"))
			{
				ret = RenderDriver::Common::BlendOperator::Min;
			}
			else if(type == std::string("Max"))
			{
				ret = RenderDriver::Common::BlendOperator::Max;
			}
			else
			{
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		void getBlendState(
			std::vector<RenderDriver::Common::BlendState>& aBlendStates,
			rapidjson::Document const& doc)
		{
			// blend state
			auto const& aBlendStatesJSON = doc["BlendStates"].GetArray();
			for(auto const& blendState : aBlendStatesJSON)
			{
				// default value
				RenderDriver::Common::BlendState state = {};
				state.mbEnabled = false;
				state.mSrcColor = RenderDriver::Common::BlendType::SrcAlpha;
				state.mDestColor = RenderDriver::Common::BlendType::InvSrcAlpha;
				state.mSrcAlpha = RenderDriver::Common::BlendType::One;
				state.mDestAlpha = RenderDriver::Common::BlendType::Zero;
				state.mColorOp = RenderDriver::Common::BlendOperator::Add;
				state.mAlphaOp = RenderDriver::Common::BlendOperator::Add;
				
				if(blendState.HasMember("Enabled"))
				{
					char const* szEnableBlend = blendState["Enabled"].GetString();
					state.mbEnabled = (std::string("True") == szEnableBlend);
				}

				if(blendState.HasMember("SrcColor"))
				{
					char const* szSrcColor = blendState["SrcColor"].GetString();
					state.mSrcColor = Common::setBlendType(szSrcColor);
				}
				
				if(blendState.HasMember("DestColor"))
				{
					char const* szDestColor = blendState["DestColor"].GetString();
					state.mDestColor = Common::setBlendType(szDestColor);
				}

				if(blendState.HasMember("SrcAlpha"))
				{
					char const* szSrcAlpha = blendState["SrcAlpha"].GetString();
					state.mSrcAlpha = Common::setBlendType(szSrcAlpha);
				}
				
				if(blendState.HasMember("DestAlpha"))
				{
					char const* szDestAlpha = blendState["DestAlpha"].GetString();
					state.mDestAlpha = Common::setBlendType(szDestAlpha);
				}
				
				if(blendState.HasMember("ColorOp"))
				{
					char const* szColorOp = blendState["ColorOp"].GetString();
					state.mColorOp = Common::setBlendOpType(szColorOp);
				}
				
				if(blendState.HasMember("AlphaOp"))
				{
					char const* szAlphaOp = blendState["AlphaOp"].GetString();
					state.mAlphaOp = Common::setBlendOpType(szAlphaOp);
				}

				aBlendStates.push_back(state);
			}
		}

		/*
		**
		*/
		void getRenderTargetsFormat(
			std::vector<RenderDriver::Common::Format>& aFormats,
			rapidjson::Document const& doc)
		{
			if(doc.HasMember("Format"))
			{ 
				auto const& aFormatStr = doc["Formats"].GetArray();
				for(uint32_t i = 0; i < aFormatStr.Size(); i++)
				{
					char const* szFormat = aFormatStr[i].GetString();
					RenderDriver::Common::Format format = _getTextureFormat(std::string(szFormat));
					aFormats.push_back(format);
				}
			}
			else
			{
				aFormats.push_back(RenderDriver::Common::Format::R8G8B8A8_UNORM);
			}
		}

		/*
		**
		*/
		RenderDriver::Common::StencilOperator getDepthStencilOp(std::string const& op)
		{
			RenderDriver::Common::StencilOperator ret = RenderDriver::Common::StencilOperator::Keep;
			if(op == std::string("Keep"))
			{
				ret = RenderDriver::Common::StencilOperator::Keep;
			}
			else if(op == std::string("Zero"))
			{
				ret = RenderDriver::Common::StencilOperator::Zero;
			}
			else if(op == std::string("Replace"))
			{
				ret = RenderDriver::Common::StencilOperator::Replace;
			}
			else if(op == std::string("IncreaseSaturate"))
			{
				ret = RenderDriver::Common::StencilOperator::IncrementSaturate;
			}
			else if(op == std::string("DecreaseSaturate"))
			{
				ret = RenderDriver::Common::StencilOperator::DecrementSaturate;
			}
			else if(op == std::string("Invert"))
			{
				ret = RenderDriver::Common::StencilOperator::Invert;
			}
			else if(op == std::string("Increment"))
			{
				ret = RenderDriver::Common::StencilOperator::Increment;
			}
			else if(op == std::string("Decrement"))
			{
				ret = RenderDriver::Common::StencilOperator::Decrement;
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::ComparisonFunc getDepthStencilFunc(std::string const& depthFunc)
		{
			RenderDriver::Common::ComparisonFunc ret = RenderDriver::Common::ComparisonFunc::Never;
			if(depthFunc == std::string("Never"))
			{
				ret = RenderDriver::Common::ComparisonFunc::Never;
			}
			else if(depthFunc == std::string("Less"))
			{
				ret = RenderDriver::Common::ComparisonFunc::Less;
			}
			else if(depthFunc == std::string("Equal"))
			{
				ret = RenderDriver::Common::ComparisonFunc::Equal;
			}
			else if(depthFunc == std::string("LessEqual"))
			{
				ret = RenderDriver::Common::ComparisonFunc::LessEqual;
			}
			else if(depthFunc == std::string("Greater"))
			{
				ret = RenderDriver::Common::ComparisonFunc::Greater;
			}
			else if(depthFunc == std::string("NotEqual"))
			{
				ret = RenderDriver::Common::ComparisonFunc::NotEqual;
			}
			else if(depthFunc == std::string("GreaterEqual"))
			{
				ret = RenderDriver::Common::ComparisonFunc::GreaterEqual;
			}
			else if(depthFunc == std::string("Always"))
			{
				ret = RenderDriver::Common::ComparisonFunc::Always;
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::DepthWriteMask getDepthWriteMask(std::string const& mask)
		{
			RenderDriver::Common::DepthWriteMask ret = RenderDriver::Common::DepthWriteMask::Zero;
			if(mask == "All" || mask == "One")
			{
				ret = RenderDriver::Common::DepthWriteMask::All;
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::DepthStencilState getDepthStencilState(rapidjson::Document const& doc)
		{
			auto const& depthStencilState = doc["DepthStencilState"];

			RenderDriver::Common::DepthStencilState ret;
			ret.mbDepthEnabled = false;
			ret.mbStencilEnabled = false;
			
			ret.mDepthWriteMask = RenderDriver::Common::DepthWriteMask::All;
			ret.mDepthFunc = RenderDriver::Common::ComparisonFunc::LessEqual;
			
			ret.mFrontFace.mStencilFailOp = RenderDriver::Common::StencilOperator::Keep;
			ret.mFrontFace.mStencilDepthFailOp = RenderDriver::Common::StencilOperator::Keep;
			ret.mFrontFace.mStencilFunc = RenderDriver::Common::ComparisonFunc::Less;

			ret.mBackFace.mStencilFailOp = RenderDriver::Common::StencilOperator::Keep;
			ret.mBackFace.mStencilDepthFailOp = RenderDriver::Common::StencilOperator::Keep;
			ret.mBackFace.mStencilPassOp = RenderDriver::Common::StencilOperator::Keep;
			ret.mBackFace.mStencilFunc = RenderDriver::Common::ComparisonFunc::Less;

			ret.miStencilReadMask = 255;
			ret.miStencilWriteMask = 255;

			if(depthStencilState.HasMember("FrontFaceStencilFail"))
			{
				char const* szFrontFaceStencilFail = depthStencilState["FrontFaceStencilFail"].GetString();
				ret.mFrontFace.mStencilFailOp = Common::getDepthStencilOp(szFrontFaceStencilFail);
			}

			if(depthStencilState.HasMember("FrontFaceDepthFail"))
			{
				char const* szFrontFaceDepthFail = depthStencilState["FrontFaceDepthFail"].GetString();
				ret.mFrontFace.mStencilDepthFailOp = Common::getDepthStencilOp(szFrontFaceDepthFail);
			}

			if(depthStencilState.HasMember("FrontFaceStencilPass"))
			{
				char const* szFrontFaceStencilPass = depthStencilState["FrontFaceStencilPass"].GetString();
				ret.mFrontFace.mStencilPassOp = Common::getDepthStencilOp(szFrontFaceStencilPass);
			}

			if(depthStencilState.HasMember("BackFaceStencilFail"))
			{
				char const* szBackFaceStencilFail = depthStencilState["BackFaceStencilFail"].GetString();
				ret.mBackFace.mStencilFailOp = Common::getDepthStencilOp(szBackFaceStencilFail);
			}
			
			if(depthStencilState.HasMember("BackFaceDepthFail"))
			{
				char const* szBackFaceDepthFail = depthStencilState["BackFaceDepthFail"].GetString();
				ret.mBackFace.mStencilDepthFailOp = Common::getDepthStencilOp(szBackFaceDepthFail);
			}
			
			if(depthStencilState.HasMember("BackFaceStencilPass"))
			{
				char const* szBackFaceStencilPass = depthStencilState["BackFaceStencilPass"].GetString();
				ret.mBackFace.mStencilPassOp = Common::getDepthStencilOp(szBackFaceStencilPass);
			}

			if(depthStencilState.HasMember("FrontFaceStencilFunc"))
			{
				char const* szFrontFaceStencilFunc = depthStencilState["FrontFaceStencilFunc"].GetString();
				ret.mFrontFace.mStencilFunc = Common::getDepthStencilFunc(szFrontFaceStencilFunc);
			}
			
			if(depthStencilState.HasMember("BackFaceStencilFunc"))
			{
				char const* szBackFaceStencilFunc = depthStencilState["BackFaceStencilFunc"].GetString();
				ret.mBackFace.mStencilFunc = Common::getDepthStencilFunc(szBackFaceStencilFunc);
			}
			
			if(depthStencilState.HasMember("DepthEnable"))
			{
				char const* szDepthEnable = depthStencilState["DepthEnable"].GetString();
				ret.mbDepthEnabled = (std::string(szDepthEnable) == "True");
			}
			
			if(depthStencilState.HasMember("DepthWriteMask"))
			{
				char const* szDepthWriteMask = depthStencilState["DepthWriteMask"].GetString();
				ret.mDepthWriteMask = Common::getDepthWriteMask(szDepthWriteMask);
			}

			if(depthStencilState.HasMember("DepthFunc"))
			{
				char const* szDepthFunc = depthStencilState["DepthFunc"].GetString();
				ret.mDepthFunc = Common::getDepthStencilFunc(szDepthFunc);
			}

			if(depthStencilState.HasMember("StencilEnable"))
			{
				char const* szStencilEnable = depthStencilState["StencilEnable"].GetString();
				ret.mbStencilEnabled = (std::string(szStencilEnable) == "True");
			}
			
			
			if(depthStencilState.HasMember("StencilReadMask"))
			{
				uint8_t iStencilReadMask = depthStencilState["StencilReadMask"].GetInt();
				ret.miStencilReadMask = iStencilReadMask;
			}
			
			if(depthStencilState.HasMember("StencilWriteMask"))
			{
				uint8_t iStencilWriteMask = depthStencilState["StencilWriteMask"].GetInt();
				ret.miStencilWriteMask = iStencilWriteMask;
			}
			
			return ret;
		}

		RenderDriver::Common::FillMode getFillMode(std::string const& mode)
		{
			RenderDriver::Common::FillMode ret = RenderDriver::Common::FillMode::Solid;
			if(mode == "WireFrame")
			{
				ret = RenderDriver::Common::FillMode::Wireframe;
			}
			else if(mode == "Solid")
			{
				ret = RenderDriver::Common::FillMode::Solid;
			}

			return ret;
		}

		RenderDriver::Common::CullMode getCullMode(std::string const& mode)
		{
			RenderDriver::Common::CullMode ret = RenderDriver::Common::CullMode::None;
			if(mode == "None")
			{
				ret = RenderDriver::Common::CullMode::None;
			}
			else if(mode == "Front")
			{
				ret = RenderDriver::Common::CullMode::Front;
			}
			else if(mode == "Back")
			{
				ret = RenderDriver::Common::CullMode::Back;
			}

			return ret;
		}

		/*
		**
		*/
		bool getBool(std::string const& flag)
		{
			return (flag == "True");
		}

		/*
		**
		*/
		RenderDriver::Common::ConservativeRasterizationMode getConservateRaster(std::string const& mode)
		{
			RenderDriver::Common::ConservativeRasterizationMode ret = RenderDriver::Common::ConservativeRasterizationMode::Off;
			if(mode == "Off")
			{
				ret = RenderDriver::Common::ConservativeRasterizationMode::Off;
			}
			else if(mode == "On")
			{
				ret = RenderDriver::Common::ConservativeRasterizationMode::On;
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::RasterState getRasterState(rapidjson::Document const& doc)
		{
			auto const& rasterState = doc["RasterState"];

			RenderDriver::Common::RasterState ret;
			ret.mFillMode = RenderDriver::Common::FillMode::Solid;
			ret.mCullMode = RenderDriver::Common::CullMode::None;
			ret.mbFrontCounterClockwise = true;
			ret.miDepthBias = 0;
			ret.mfSlopeScaledDepthBias = 0.0f;
			ret.mfDepthBiasClamp = 0.0f;
			ret.mbDepthClipEnable = true;
			ret.mbMultisampleEnable = false;
			ret.mbAntialiasedLineEnable = false;
			ret.miForcedSampleCount = 0;
			ret.mConservativeRaster = RenderDriver::Common::ConservativeRasterizationMode::Off;

			if(rasterState.HasMember("FillMode"))
			{
				char const* szFillMode = rasterState["FillMode"].GetString();
				ret.mFillMode = Common::getFillMode(szFillMode);
			}

			if(rasterState.HasMember("CullMode"))
			{
				char const* szCullMode = rasterState["CullMode"].GetString();
				ret.mCullMode = Common::getCullMode(szCullMode);
			}
			
			if(rasterState.HasMember("FrontFace"))
			{
				char const* szFrontFace = rasterState["FrontFace"].GetString();
				ret.mbFrontCounterClockwise = Common::getBool(szFrontFace);

				if(std::string(szFrontFace) == "CounterClockwise")
				{
					ret.mbFrontCounterClockwise = true;
				}
			}

			if(rasterState.HasMember("DepthBias"))
			{
				uint32_t iDepthBias = rasterState["DepthBias"].GetInt();
				ret.miDepthBias = iDepthBias;
			}
			
			if(rasterState.HasMember("SlopeScaleDepthBias"))
			{
				float fSlopeScaleDepthBias = rasterState["SlopeScaleDepthBias"].GetFloat();
				ret.mfSlopeScaledDepthBias = fSlopeScaleDepthBias;
			}
			
			if(rasterState.HasMember("DepthBiasClamp"))
			{
				float fDepthBiasClamp = rasterState["DepthBiasClamp"].GetFloat();
				ret.mfDepthBiasClamp = fDepthBiasClamp;
			}
			
			if(rasterState.HasMember("DepthClipEnable"))
			{
				char const* szDepthClipEnable = rasterState["DepthClipEnable"].GetString();
				ret.mbDepthClipEnable = Common::getBool(szDepthClipEnable);
			}

			if(rasterState.HasMember("MultiSampleEnable"))
			{
				char const* szMultiSampleEnable = rasterState["MultiSampleEnable"].GetString();
				ret.mbMultisampleEnable = Common::getBool(szMultiSampleEnable);
			}

			if(rasterState.HasMember("AntialiasedLineEnable"))
			{
				char const* szAntialiasedLineEnable = rasterState["AntialiasedLineEnable"].GetString();
				ret.mbAntialiasedLineEnable = Common::getBool(szAntialiasedLineEnable);
			}
			
			if(rasterState.HasMember("ForcedSampleCount"))
			{
				uint32_t iForcedSampleCount = rasterState["ForcedSampleCount"].GetInt();
				ret.miForcedSampleCount = iForcedSampleCount;
			}
			
			if(rasterState.HasMember("ConservativeRaster"))
			{
				char const* szConservativeRaster = rasterState["ConservativeRaster"].GetString();
				ret.mConservativeRaster = Common::getConservateRaster(szConservativeRaster);
			}
			
			return ret;
		}

	}	// Common

#if defined(_MSC_VER)
	namespace DX12
	{
#define CASE(X, Y)	\
	case X:			\
	{				\
		ret = Y;	\
		break;		\
	}	

#define CASE2(X, Y)	\
	case Y:			\
	{				\
		ret = X;	\
		break;		\
	}	

		D3D12_FILL_MODE convert(RenderDriver::Common::FillMode mode)
		{
			D3D12_FILL_MODE ret = D3D12_FILL_MODE_WIREFRAME;
			switch(mode)
			{
				CASE(RenderDriver::Common::FillMode::Wireframe, D3D12_FILL_MODE_WIREFRAME)
				CASE(RenderDriver::Common::FillMode::Solid, D3D12_FILL_MODE_SOLID)
			};

			return ret;
		}

		D3D12_CULL_MODE convert(RenderDriver::Common::CullMode mode)
		{
			D3D12_CULL_MODE ret = D3D12_CULL_MODE_NONE;
			switch(mode)
			{
				CASE(RenderDriver::Common::CullMode::None, D3D12_CULL_MODE_NONE)
				CASE(RenderDriver::Common::CullMode::Front, D3D12_CULL_MODE_FRONT)
				CASE(RenderDriver::Common::CullMode::Back, D3D12_CULL_MODE_BACK)
			}

			return ret;
		}

		D3D12_CONSERVATIVE_RASTERIZATION_MODE convert(RenderDriver::Common::ConservativeRasterizationMode mode)
		{
			D3D12_CONSERVATIVE_RASTERIZATION_MODE ret = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			switch(mode)
			{
				CASE(RenderDriver::Common::ConservativeRasterizationMode::Off, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF)
				CASE(RenderDriver::Common::ConservativeRasterizationMode::On, D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_BLEND convert(RenderDriver::Common::BlendType type)
		{
			D3D12_BLEND ret = D3D12_BLEND_ZERO;
			switch(type)
			{
				CASE(RenderDriver::Common::BlendType::Zero, D3D12_BLEND_ZERO)
				CASE(RenderDriver::Common::BlendType::One, D3D12_BLEND_ONE)
				CASE(RenderDriver::Common::BlendType::SrcColor, D3D12_BLEND_SRC_COLOR)
				CASE(RenderDriver::Common::BlendType::InvSrcColor, D3D12_BLEND_INV_SRC_COLOR)
				CASE(RenderDriver::Common::BlendType::SrcAlpha, D3D12_BLEND_SRC_ALPHA)
				CASE(RenderDriver::Common::BlendType::InvSrcAlpha, D3D12_BLEND_INV_SRC_ALPHA)
				CASE(RenderDriver::Common::BlendType::DestColor, D3D12_BLEND_DEST_COLOR)
				CASE(RenderDriver::Common::BlendType::InvDestColor, D3D12_BLEND_INV_DEST_COLOR)
				CASE(RenderDriver::Common::BlendType::DestAlpha, D3D12_BLEND_DEST_ALPHA)
				CASE(RenderDriver::Common::BlendType::InvDestAlpha, D3D12_BLEND_INV_DEST_ALPHA)
				CASE(RenderDriver::Common::BlendType::SrcAlphaSaturate, D3D12_BLEND_SRC_ALPHA_SAT)
				CASE(RenderDriver::Common::BlendType::BlendFactor, D3D12_BLEND_BLEND_FACTOR)
				CASE(RenderDriver::Common::BlendType::InvBlendFactor, D3D12_BLEND_INV_BLEND_FACTOR)
				CASE(RenderDriver::Common::BlendType::Src1Color, D3D12_BLEND_SRC_COLOR)
				CASE(RenderDriver::Common::BlendType::InvSrc1Color, D3D12_BLEND_SRC1_COLOR)
				CASE(RenderDriver::Common::BlendType::Src1Alpha, D3D12_BLEND_SRC1_ALPHA)
				CASE(RenderDriver::Common::BlendType::InvSrc1Alpha, D3D12_BLEND_INV_SRC1_ALPHA)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_BLEND_OP convert(RenderDriver::Common::BlendOperator type)
		{
			D3D12_BLEND_OP ret = D3D12_BLEND_OP_ADD;
			switch(type)
			{
				CASE(RenderDriver::Common::BlendOperator::Add, D3D12_BLEND_OP_ADD)
				CASE(RenderDriver::Common::BlendOperator::Subtract, D3D12_BLEND_OP_SUBTRACT)
				CASE(RenderDriver::Common::BlendOperator::ReverseSubtract, D3D12_BLEND_OP_REV_SUBTRACT)
				CASE(RenderDriver::Common::BlendOperator::Min, D3D12_BLEND_OP_MIN)
				CASE(RenderDriver::Common::BlendOperator::Max, D3D12_BLEND_OP_MAX)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_LOGIC_OP convert(RenderDriver::Common::BlendLogicOperator type)
		{
			D3D12_LOGIC_OP ret = D3D12_LOGIC_OP_NOOP;
			switch(type)
			{
				CASE(RenderDriver::Common::BlendLogicOperator::Clear, D3D12_LOGIC_OP_CLEAR)
				CASE(RenderDriver::Common::BlendLogicOperator::Set, D3D12_LOGIC_OP_SET)
				CASE(RenderDriver::Common::BlendLogicOperator::Copy, D3D12_LOGIC_OP_COPY)
				CASE(RenderDriver::Common::BlendLogicOperator::CopyInverted, D3D12_LOGIC_OP_COPY)
				CASE(RenderDriver::Common::BlendLogicOperator::NoOp, D3D12_LOGIC_OP_NOOP)
				CASE(RenderDriver::Common::BlendLogicOperator::Invert, D3D12_LOGIC_OP_INVERT)
				CASE(RenderDriver::Common::BlendLogicOperator::And, D3D12_LOGIC_OP_AND)
				CASE(RenderDriver::Common::BlendLogicOperator::Nand, D3D12_LOGIC_OP_NAND)
				CASE(RenderDriver::Common::BlendLogicOperator::Or, D3D12_LOGIC_OP_OR)
				CASE(RenderDriver::Common::BlendLogicOperator::Nor, D3D12_LOGIC_OP_NOR)
				CASE(RenderDriver::Common::BlendLogicOperator::Xor, D3D12_LOGIC_OP_XOR)
				CASE(RenderDriver::Common::BlendLogicOperator::Equivalent, D3D12_LOGIC_OP_EQUIV)
				CASE(RenderDriver::Common::BlendLogicOperator::AndReverse, D3D12_LOGIC_OP_AND_REVERSE)
				CASE(RenderDriver::Common::BlendLogicOperator::AndInverted, D3D12_LOGIC_OP_AND_INVERTED)
				CASE(RenderDriver::Common::BlendLogicOperator::OrReverse, D3D12_LOGIC_OP_OR_REVERSE)
				CASE(RenderDriver::Common::BlendLogicOperator::OrInverted, D3D12_LOGIC_OP_OR_INVERTED)
			}

			return ret;
		}

		/*
		**
		*/
		UINT8 convert(RenderDriver::Common::ColorWriteMask mask)
		{
			INT8 ret = static_cast<INT8>(RenderDriver::Common::ColorWriteMask::WriteAll);
			return ret;
		}

		/*
		**
		*/
		D3D12_DEPTH_WRITE_MASK convert(RenderDriver::Common::DepthWriteMask mask)
		{
			D3D12_DEPTH_WRITE_MASK ret = D3D12_DEPTH_WRITE_MASK_ZERO;
			switch(mask)
			{
				CASE(RenderDriver::Common::DepthWriteMask::Zero, D3D12_DEPTH_WRITE_MASK_ZERO)
				CASE(RenderDriver::Common::DepthWriteMask::All, D3D12_DEPTH_WRITE_MASK_ALL)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_COMPARISON_FUNC convert(RenderDriver::Common::ComparisonFunc func)
		{
			D3D12_COMPARISON_FUNC ret = D3D12_COMPARISON_FUNC_NEVER;
			switch(func)
			{
				CASE(RenderDriver::Common::ComparisonFunc::Never, D3D12_COMPARISON_FUNC_NEVER)
				CASE(RenderDriver::Common::ComparisonFunc::Less, D3D12_COMPARISON_FUNC_LESS)
				CASE(RenderDriver::Common::ComparisonFunc::Equal, D3D12_COMPARISON_FUNC_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::LessEqual, D3D12_COMPARISON_FUNC_LESS_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::Greater, D3D12_COMPARISON_FUNC_GREATER)
				CASE(RenderDriver::Common::ComparisonFunc::NotEqual, D3D12_COMPARISON_FUNC_NOT_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::GreaterEqual, D3D12_COMPARISON_FUNC_GREATER_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::Always, D3D12_COMPARISON_FUNC_ALWAYS)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_STENCIL_OP convert(RenderDriver::Common::StencilOperator op)
		{
			D3D12_STENCIL_OP ret = D3D12_STENCIL_OP_KEEP;
			switch(op)
			{
				CASE(RenderDriver::Common::StencilOperator::Keep, D3D12_STENCIL_OP_KEEP)
				CASE(RenderDriver::Common::StencilOperator::Zero, D3D12_STENCIL_OP_ZERO)
				CASE(RenderDriver::Common::StencilOperator::Replace, D3D12_STENCIL_OP_REPLACE)
				CASE(RenderDriver::Common::StencilOperator::IncrementSaturate, D3D12_STENCIL_OP_INCR_SAT)
				CASE(RenderDriver::Common::StencilOperator::DecrementSaturate, D3D12_STENCIL_OP_DECR_SAT)
				CASE(RenderDriver::Common::StencilOperator::Invert, D3D12_STENCIL_OP_INVERT)
				CASE(RenderDriver::Common::StencilOperator::Increment, D3D12_STENCIL_OP_INCR)
				CASE(RenderDriver::Common::StencilOperator::Decrement, D3D12_STENCIL_OP_DECR)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_DEPTH_STENCILOP_DESC convert(RenderDriver::Common::DepthStencilOpDesc const& desc)
		{
			D3D12_DEPTH_STENCILOP_DESC ret;
			ret.StencilDepthFailOp = convert(desc.mStencilDepthFailOp);
			ret.StencilFailOp = convert(desc.mStencilFailOp);
			ret.StencilFunc = convert(desc.mStencilFunc);
			ret.StencilPassOp = convert(desc.mStencilPassOp);

			return ret;
		}

		/*
		**
		*/
		DXGI_FORMAT convert(RenderDriver::Common::Format format)
		{
			DXGI_FORMAT ret = DXGI_FORMAT_UNKNOWN;

			switch(format)
			{
				CASE(RenderDriver::Common::Format::UNKNOWN, DXGI_FORMAT_UNKNOWN)
				CASE(RenderDriver::Common::Format::R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32A32_TYPELESS)
				CASE(RenderDriver::Common::Format::R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT)
				CASE(RenderDriver::Common::Format::R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_UINT)
				CASE(RenderDriver::Common::Format::R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_SINT)
				CASE(RenderDriver::Common::Format::R32G32B32_TYPELESS, DXGI_FORMAT_R32G32B32_TYPELESS)
				CASE(RenderDriver::Common::Format::R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT)
				CASE(RenderDriver::Common::Format::R32G32B32_UINT, DXGI_FORMAT_R32G32B32_UINT)
				CASE(RenderDriver::Common::Format::R32G32B32_SINT, DXGI_FORMAT_R32G32B32_SINT)
				CASE(RenderDriver::Common::Format::R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_TYPELESS)
				CASE(RenderDriver::Common::Format::R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT)
				CASE(RenderDriver::Common::Format::R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM)
				CASE(RenderDriver::Common::Format::R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_UINT)
				CASE(RenderDriver::Common::Format::R16G16B16A16_SNORM, DXGI_FORMAT_R16G16B16A16_SNORM)
				CASE(RenderDriver::Common::Format::R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SINT)
				CASE(RenderDriver::Common::Format::R32G32_TYPELESS, DXGI_FORMAT_R32G32_TYPELESS)
				CASE(RenderDriver::Common::Format::R32G32_FLOAT, DXGI_FORMAT_R32G32_FLOAT)
				CASE(RenderDriver::Common::Format::R32G32_UINT, DXGI_FORMAT_R32G32_UINT)
				CASE(RenderDriver::Common::Format::R32G32_SINT, DXGI_FORMAT_R32G32_SINT)
				CASE(RenderDriver::Common::Format::R32G8X24_TYPELESS, DXGI_FORMAT_R32G8X24_TYPELESS)
				CASE(RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
				CASE(RenderDriver::Common::Format::R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS)
				CASE(RenderDriver::Common::Format::X32_TYPELESS_G8X24_UINT, DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
				CASE(RenderDriver::Common::Format::R10G10B10A2_TYPELESS, DXGI_FORMAT_R10G10B10A2_TYPELESS)
				CASE(RenderDriver::Common::Format::R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM)
				CASE(RenderDriver::Common::Format::R10G10B10A2_UINT, DXGI_FORMAT_R10G10B10A2_UINT)
				CASE(RenderDriver::Common::Format::R11G11B10_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT)
				CASE(RenderDriver::Common::Format::R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_TYPELESS)
				CASE(RenderDriver::Common::Format::R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM)
				CASE(RenderDriver::Common::Format::R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
				CASE(RenderDriver::Common::Format::R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UINT)
				CASE(RenderDriver::Common::Format::R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SNORM)
				CASE(RenderDriver::Common::Format::R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SINT)
				CASE(RenderDriver::Common::Format::R16G16_TYPELESS, DXGI_FORMAT_R16G16_TYPELESS)
				CASE(RenderDriver::Common::Format::R16G16_FLOAT, DXGI_FORMAT_R16G16_FLOAT)
				CASE(RenderDriver::Common::Format::R16G16_UNORM, DXGI_FORMAT_R16G16_UNORM)
				CASE(RenderDriver::Common::Format::R16G16_UINT, DXGI_FORMAT_R16G16_UINT)
				CASE(RenderDriver::Common::Format::R16G16_SNORM, DXGI_FORMAT_R16G16_SNORM)
				CASE(RenderDriver::Common::Format::R16G16_SINT, DXGI_FORMAT_R16G16_SINT)
				CASE(RenderDriver::Common::Format::R32_TYPELESS, DXGI_FORMAT_R32_TYPELESS)
				CASE(RenderDriver::Common::Format::D32_FLOAT, DXGI_FORMAT_D32_FLOAT)
				CASE(RenderDriver::Common::Format::R32_FLOAT, DXGI_FORMAT_R32_FLOAT)
				CASE(RenderDriver::Common::Format::R32_UINT, DXGI_FORMAT_R32_UINT)
				CASE(RenderDriver::Common::Format::R32_SINT, DXGI_FORMAT_R32_SINT)
				CASE(RenderDriver::Common::Format::R24G8_TYPELESS, DXGI_FORMAT_R24G8_TYPELESS)
				CASE(RenderDriver::Common::Format::D24_UNORM_S8_UINT, DXGI_FORMAT_D24_UNORM_S8_UINT)
				CASE(RenderDriver::Common::Format::R24_UNORM_X8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS)
				CASE(RenderDriver::Common::Format::X24_TYPELESS_G8_UINT, DXGI_FORMAT_X24_TYPELESS_G8_UINT)
				CASE(RenderDriver::Common::Format::R8G8_TYPELESS, DXGI_FORMAT_R8G8_TYPELESS)
				CASE(RenderDriver::Common::Format::R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM)
				CASE(RenderDriver::Common::Format::R8G8_UINT, DXGI_FORMAT_R8G8_UINT)
				CASE(RenderDriver::Common::Format::R8G8_SNORM, DXGI_FORMAT_R8G8_SNORM)
				CASE(RenderDriver::Common::Format::R8G8_SINT, DXGI_FORMAT_R8G8_SINT)
				CASE(RenderDriver::Common::Format::R16_TYPELESS, DXGI_FORMAT_R16_TYPELESS)
				CASE(RenderDriver::Common::Format::R16_FLOAT, DXGI_FORMAT_R16_FLOAT)
				CASE(RenderDriver::Common::Format::D16_UNORM, DXGI_FORMAT_D16_UNORM)
				CASE(RenderDriver::Common::Format::R16_UNORM, DXGI_FORMAT_R16_UNORM)
				CASE(RenderDriver::Common::Format::R16_UINT, DXGI_FORMAT_R16_UINT)
				CASE(RenderDriver::Common::Format::R16_SNORM, DXGI_FORMAT_R16_SNORM)
				CASE(RenderDriver::Common::Format::R16_SINT, DXGI_FORMAT_R16_SINT)
				CASE(RenderDriver::Common::Format::R8_TYPELESS, DXGI_FORMAT_R8_TYPELESS)
				CASE(RenderDriver::Common::Format::R8_UNORM, DXGI_FORMAT_R8_UNORM)
				CASE(RenderDriver::Common::Format::R8_UINT, DXGI_FORMAT_R8_UINT)
				CASE(RenderDriver::Common::Format::R8_SNORM, DXGI_FORMAT_R8_SNORM)
				CASE(RenderDriver::Common::Format::R8_SINT, DXGI_FORMAT_R8_SINT)
				CASE(RenderDriver::Common::Format::A8_UNORM, DXGI_FORMAT_A8_UNORM)
				CASE(RenderDriver::Common::Format::R1_UNORM, DXGI_FORMAT_R1_UNORM)
			default:
				assert(0);
				DEBUG_PRINTF("no such format: %d\n", format);
			}

			return ret;
		}

		D3D12_RESOURCE_STATES convert(RenderDriver::Common::ResourceStateFlagBits flagBits)
		{
			uint32_t iFlagBits = static_cast<uint32_t>(flagBits);
			uint32_t iRet = 0;
			uint32_t iCurrBit = 1;
			for(uint32_t i = 0; iCurrBit <= static_cast<uint32_t>(RenderDriver::Common::ResourceStateFlagBits::RayTracingAccelerationStructure); i++)
			{
				iCurrBit = (1 << i);
				if((iFlagBits & iCurrBit) > 0)
				{
					RenderDriver::Common::ResourceStateFlagBits checkBit = static_cast<RenderDriver::Common::ResourceStateFlagBits>(iCurrBit);
					switch(checkBit)
					{
					case RenderDriver::Common::ResourceStateFlagBits::VertexAndConstantBuffer:
						iRet |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::IndexBuffer:
						iRet |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::RenderTarget:
						iRet |= D3D12_RESOURCE_STATE_RENDER_TARGET;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess:
						iRet |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::DepthWrite:
						iRet |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::DepthRead:
						iRet |= D3D12_RESOURCE_STATE_DEPTH_READ;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::NonPixelShaderResource:
						iRet |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::PixelShaderResource:
						iRet |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::StreamOut:
						iRet |= D3D12_RESOURCE_STATE_STREAM_OUT;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::IndirectArgument:
						iRet |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::CopyDestination:
						iRet |= D3D12_RESOURCE_STATE_COPY_DEST;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::CopySource:
						iRet |= D3D12_RESOURCE_STATE_COPY_SOURCE;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::ResolveDestination:
						iRet |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::ResolveSource:
						iRet |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::RayTracingAccelerationStructure:
						iRet |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
						break;
					case RenderDriver::Common::ResourceStateFlagBits::Present:
						iRet |= D3D12_RESOURCE_STATE_PRESENT;
						break;

					}
				}
			}

			return static_cast<D3D12_RESOURCE_STATES>(iRet);
		}

		/*
		**
		*/
		D3D12_DESCRIPTOR_HEAP_TYPE convert(RenderDriver::Common::DescriptorHeapType type)
		{
			D3D12_DESCRIPTOR_HEAP_TYPE ret = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			switch(type)
			{
			case RenderDriver::Common::DescriptorHeapType::General:
				ret = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				break;

			case RenderDriver::Common::DescriptorHeapType::Sampler:
				ret = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
				break;

			case RenderDriver::Common::DescriptorHeapType::RenderTarget:
				ret = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				break;

			case RenderDriver::Common::DescriptorHeapType::DepthStencil:
				ret = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				break;
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_DESCRIPTOR_HEAP_FLAGS convert(RenderDriver::Common::DescriptorHeapFlag flag)
		{
			D3D12_DESCRIPTOR_HEAP_FLAGS ret = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			switch(flag)
			{
			case RenderDriver::Common::DescriptorHeapFlag::None:
				ret = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				break;

			case RenderDriver::Common::DescriptorHeapFlag::ShaderVisible:
				ret = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				break;
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_UAV_DIMENSION convert(RenderDriver::Common::UAVDimension dimension)
		{
			D3D12_UAV_DIMENSION ret = D3D12_UAV_DIMENSION_UNKNOWN;
			switch(dimension)
			{
				CASE(RenderDriver::Common::UAVDimension::Unknown, D3D12_UAV_DIMENSION_UNKNOWN)
				CASE(RenderDriver::Common::UAVDimension::Buffer, D3D12_UAV_DIMENSION_BUFFER)
				CASE(RenderDriver::Common::UAVDimension::Texture1D, D3D12_UAV_DIMENSION_TEXTURE1D)
				CASE(RenderDriver::Common::UAVDimension::Texture1DArray, D3D12_UAV_DIMENSION_TEXTURE1DARRAY)
				CASE(RenderDriver::Common::UAVDimension::Texture2D, D3D12_UAV_DIMENSION_TEXTURE2D)
				CASE(RenderDriver::Common::UAVDimension::Texture2Drray, D3D12_UAV_DIMENSION_TEXTURE2DARRAY)
				CASE(RenderDriver::Common::UAVDimension::Texture3D, D3D12_UAV_DIMENSION_TEXTURE3D)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_COMMAND_LIST_TYPE convert(RenderDriver::Common::CommandBufferType type)
		{
			D3D12_COMMAND_LIST_TYPE ret = D3D12_COMMAND_LIST_TYPE_DIRECT;
			switch(type)
			{
				CASE(RenderDriver::Common::CommandBufferType::Graphics, D3D12_COMMAND_LIST_TYPE_DIRECT)
				CASE(RenderDriver::Common::CommandBufferType::Compute, D3D12_COMMAND_LIST_TYPE_COMPUTE)
				CASE(RenderDriver::Common::CommandBufferType::Copy, D3D12_COMMAND_LIST_TYPE_COPY)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_INPUT_ELEMENT_DESC convert(RenderDriver::Common::VertexFormat const& format, uint32_t iOffset)
		{
			D3D12_INPUT_ELEMENT_DESC desc = {};
			desc.SemanticName = format.mSemanticName.c_str();
			desc.SemanticIndex = 0;
			desc.Format = convert(format.mFormat);
			desc.InputSlot = 0;
			desc.AlignedByteOffset = iOffset;
			desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			desc.InstanceDataStepRate = 0;

			return desc;
		}

		/*
		**
		*/
		D3D12_BUFFER_UAV_FLAGS convert(RenderDriver::Common::UnorderedAccessViewFlags flags)
		{
			D3D12_BUFFER_UAV_FLAGS ret = D3D12_BUFFER_UAV_FLAG_NONE;
			switch(flags)
			{
				CASE(RenderDriver::Common::UnorderedAccessViewFlags::None, D3D12_BUFFER_UAV_FLAG_NONE)
				CASE(RenderDriver::Common::UnorderedAccessViewFlags::Raw, D3D12_BUFFER_UAV_FLAG_RAW)
			}

			return ret;
		}
		
		/*
		**
		*/
		D3D12_MEMORY_POOL convert(RenderDriver::Common::MemoryPool const& pool)
		{
			D3D12_MEMORY_POOL ret = D3D12_MEMORY_POOL_UNKNOWN;
			switch(pool)
			{
				CASE(RenderDriver::Common::MemoryPool::Unknown, D3D12_MEMORY_POOL_UNKNOWN)
				CASE(RenderDriver::Common::MemoryPool::L0, D3D12_MEMORY_POOL_L0)
				CASE(RenderDriver::Common::MemoryPool::L1, D3D12_MEMORY_POOL_L1)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_HEAP_TYPE convert(RenderDriver::Common::HeapType const& type)
		{
			D3D12_HEAP_TYPE ret = D3D12_HEAP_TYPE_DEFAULT;
			switch(type)
			{
				CASE(RenderDriver::Common::HeapType::Default, D3D12_HEAP_TYPE_DEFAULT)
				CASE(RenderDriver::Common::HeapType::Upload, D3D12_HEAP_TYPE_UPLOAD)
				CASE(RenderDriver::Common::HeapType::ReadBack, D3D12_HEAP_TYPE_READBACK)
				CASE(RenderDriver::Common::HeapType::Custom, D3D12_HEAP_TYPE_CUSTOM)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_INPUT_CLASSIFICATION convert(RenderDriver::Common::InputClassification const& classification)
		{
			D3D12_INPUT_CLASSIFICATION ret = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			switch(classification)
			{
				CASE(RenderDriver::Common::InputClassification::PerVertexData, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA)
				CASE(RenderDriver::Common::InputClassification::PerInstanceData, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_RESOURCE_FLAGS convert(RenderDriver::Common::ResourceFlagBits const& flagBits)
		{
			D3D12_RESOURCE_FLAGS ret = D3D12_RESOURCE_FLAG_NONE;
			switch(flagBits)
			{
				case RenderDriver::Common::ResourceFlagBits::None:
					ret = D3D12_RESOURCE_FLAG_NONE;
					break;
				case RenderDriver::Common::ResourceFlagBits::AllowRenderTarget:
					ret = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
					break;
				case RenderDriver::Common::ResourceFlagBits::AllowDepthStencil:
					ret = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
					break;
				case RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess:
					ret = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
					break;
				case RenderDriver::Common::ResourceFlagBits::DenyShaderResource:
					ret = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
					break;
				case RenderDriver::Common::ResourceFlagBits::AllowCrossAdaptor:
					ret = D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
					break;
				case RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess:
					ret = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
					break;
				case RenderDriver::Common::ResourceFlagBits::VideoDecodeReferenceOnly:
					ret = D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY;
					break;
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::Format convert(DXGI_FORMAT format)
		{
			RenderDriver::Common::Format ret = RenderDriver::Common::Format::UNKNOWN;

			switch(format)
			{
					CASE2(RenderDriver::Common::Format::UNKNOWN, DXGI_FORMAT_UNKNOWN)
					CASE2(RenderDriver::Common::Format::R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32A32_TYPELESS)
					CASE2(RenderDriver::Common::Format::R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT)
					CASE2(RenderDriver::Common::Format::R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_UINT)
					CASE2(RenderDriver::Common::Format::R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_SINT)
					CASE2(RenderDriver::Common::Format::R32G32B32_TYPELESS, DXGI_FORMAT_R32G32B32_TYPELESS)
					CASE2(RenderDriver::Common::Format::R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT)
					CASE2(RenderDriver::Common::Format::R32G32B32_UINT, DXGI_FORMAT_R32G32B32_UINT)
					CASE2(RenderDriver::Common::Format::R32G32B32_SINT, DXGI_FORMAT_R32G32B32_SINT)
					CASE2(RenderDriver::Common::Format::R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_TYPELESS)
					CASE2(RenderDriver::Common::Format::R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT)
					CASE2(RenderDriver::Common::Format::R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM)
					CASE2(RenderDriver::Common::Format::R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_UINT)
					CASE2(RenderDriver::Common::Format::R16G16B16A16_SNORM, DXGI_FORMAT_R16G16B16A16_SNORM)
					CASE2(RenderDriver::Common::Format::R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SINT)
					CASE2(RenderDriver::Common::Format::R32G32_TYPELESS, DXGI_FORMAT_R32G32_TYPELESS)
					CASE2(RenderDriver::Common::Format::R32G32_FLOAT, DXGI_FORMAT_R32G32_FLOAT)
					CASE2(RenderDriver::Common::Format::R32G32_UINT, DXGI_FORMAT_R32G32_UINT)
					CASE2(RenderDriver::Common::Format::R32G32_SINT, DXGI_FORMAT_R32G32_SINT)
					CASE2(RenderDriver::Common::Format::R32G8X24_TYPELESS, DXGI_FORMAT_R32G8X24_TYPELESS)
					CASE2(RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT, DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
					CASE2(RenderDriver::Common::Format::R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS)
					CASE2(RenderDriver::Common::Format::X32_TYPELESS_G8X24_UINT, DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
					CASE2(RenderDriver::Common::Format::R10G10B10A2_TYPELESS, DXGI_FORMAT_R10G10B10A2_TYPELESS)
					CASE2(RenderDriver::Common::Format::R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM)
					CASE2(RenderDriver::Common::Format::R10G10B10A2_UINT, DXGI_FORMAT_R10G10B10A2_UINT)
					CASE2(RenderDriver::Common::Format::R11G11B10_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT)
					CASE2(RenderDriver::Common::Format::R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_TYPELESS)
					CASE2(RenderDriver::Common::Format::R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM)
					CASE2(RenderDriver::Common::Format::R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
					CASE2(RenderDriver::Common::Format::R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UINT)
					CASE2(RenderDriver::Common::Format::R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SNORM)
					CASE2(RenderDriver::Common::Format::R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SINT)
					CASE2(RenderDriver::Common::Format::R16G16_TYPELESS, DXGI_FORMAT_R16G16_TYPELESS)
					CASE2(RenderDriver::Common::Format::R16G16_FLOAT, DXGI_FORMAT_R16G16_FLOAT)
					CASE2(RenderDriver::Common::Format::R16G16_UNORM, DXGI_FORMAT_R16G16_UNORM)
					CASE2(RenderDriver::Common::Format::R16G16_UINT, DXGI_FORMAT_R16G16_UINT)
					CASE2(RenderDriver::Common::Format::R16G16_SNORM, DXGI_FORMAT_R16G16_SNORM)
					CASE2(RenderDriver::Common::Format::R16G16_SINT, DXGI_FORMAT_R16G16_SINT)
					CASE2(RenderDriver::Common::Format::R32_TYPELESS, DXGI_FORMAT_R32_TYPELESS)
					CASE2(RenderDriver::Common::Format::D32_FLOAT, DXGI_FORMAT_D32_FLOAT)
					CASE2(RenderDriver::Common::Format::R32_FLOAT, DXGI_FORMAT_R32_FLOAT)
					CASE2(RenderDriver::Common::Format::R32_UINT, DXGI_FORMAT_R32_UINT)
					CASE2(RenderDriver::Common::Format::R32_SINT, DXGI_FORMAT_R32_SINT)
					CASE2(RenderDriver::Common::Format::R24G8_TYPELESS, DXGI_FORMAT_R24G8_TYPELESS)
					CASE2(RenderDriver::Common::Format::D24_UNORM_S8_UINT, DXGI_FORMAT_D24_UNORM_S8_UINT)
					CASE2(RenderDriver::Common::Format::R24_UNORM_X8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS)
					CASE2(RenderDriver::Common::Format::X24_TYPELESS_G8_UINT, DXGI_FORMAT_X24_TYPELESS_G8_UINT)
					CASE2(RenderDriver::Common::Format::R8G8_TYPELESS, DXGI_FORMAT_R8G8_TYPELESS)
					CASE2(RenderDriver::Common::Format::R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM)
					CASE2(RenderDriver::Common::Format::R8G8_UINT, DXGI_FORMAT_R8G8_UINT)
					CASE2(RenderDriver::Common::Format::R8G8_SNORM, DXGI_FORMAT_R8G8_SNORM)
					CASE2(RenderDriver::Common::Format::R8G8_SINT, DXGI_FORMAT_R8G8_SINT)
					CASE2(RenderDriver::Common::Format::R16_TYPELESS, DXGI_FORMAT_R16_TYPELESS)
					CASE2(RenderDriver::Common::Format::R16_FLOAT, DXGI_FORMAT_R16_FLOAT)
					CASE2(RenderDriver::Common::Format::D16_UNORM, DXGI_FORMAT_D16_UNORM)
					CASE2(RenderDriver::Common::Format::R16_UNORM, DXGI_FORMAT_R16_UNORM)
					CASE2(RenderDriver::Common::Format::R16_UINT, DXGI_FORMAT_R16_UINT)
					CASE2(RenderDriver::Common::Format::R16_SNORM, DXGI_FORMAT_R16_SNORM)
					CASE2(RenderDriver::Common::Format::R16_SINT, DXGI_FORMAT_R16_SINT)
					CASE2(RenderDriver::Common::Format::R8_TYPELESS, DXGI_FORMAT_R8_TYPELESS)
					CASE2(RenderDriver::Common::Format::R8_UNORM, DXGI_FORMAT_R8_UNORM)
					CASE2(RenderDriver::Common::Format::R8_UINT, DXGI_FORMAT_R8_UINT)
					CASE2(RenderDriver::Common::Format::R8_SNORM, DXGI_FORMAT_R8_SNORM)
					CASE2(RenderDriver::Common::Format::R8_SINT, DXGI_FORMAT_R8_SINT)
					CASE2(RenderDriver::Common::Format::A8_UNORM, DXGI_FORMAT_A8_UNORM)
					CASE2(RenderDriver::Common::Format::R1_UNORM, DXGI_FORMAT_R1_UNORM)
			}

			return ret;
		}

		/*
		**
		*/
		D3D12_CPU_PAGE_PROPERTY convert(RenderDriver::Common::CPUPageProperty const& pageProperty)
		{
			D3D12_CPU_PAGE_PROPERTY ret = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			switch(pageProperty)
			{
			case RenderDriver::Common::CPUPageProperty::Unknown:
				ret = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				break;
			case RenderDriver::Common::CPUPageProperty::NotAvailable:
				ret = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
				break;
			case RenderDriver::Common::CPUPageProperty::WriteCombine:
				ret = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
				break;
			case RenderDriver::Common::CPUPageProperty::WriteBack:
				ret = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
				break;
			}

			return ret;
		}

		D3D12_TEXTURE_LAYOUT convert(RenderDriver::Common::TextureLayout layout)
		{
			D3D12_TEXTURE_LAYOUT ret = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			switch(layout)
			{
				case RenderDriver::Common::TextureLayout::Unknown:
				{
					ret = D3D12_TEXTURE_LAYOUT_UNKNOWN;
					break;
				}
				case RenderDriver::Common::TextureLayout::RowMajor:
				{
					ret = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					break;
				}
				case RenderDriver::Common::TextureLayout::UndefinedSwizzle:
				{
					ret = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
					break;
				}
				case RenderDriver::Common::TextureLayout::StandardSwizzle:
				{
					ret = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE;
					break;
				}
			}

			return ret;
		}

		D3D12_SRV_DIMENSION convert(RenderDriver::Common::ShaderResourceViewDimension dimension)
		{
			D3D12_SRV_DIMENSION ret = D3D12_SRV_DIMENSION_UNKNOWN;
			switch(dimension)
			{
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Unknown, D3D12_SRV_DIMENSION_UNKNOWN)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Buffer, D3D12_SRV_DIMENSION_BUFFER)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Texture1D, D3D12_SRV_DIMENSION_TEXTURE1D)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Texture1DArray, D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Texture2D, D3D12_SRV_DIMENSION_TEXTURE2D)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Texture2DArray, D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Texture2DMS, D3D12_SRV_DIMENSION_TEXTURE2DMS)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Texture2DMSArray, D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::Texture3D, D3D12_SRV_DIMENSION_TEXTURE3D)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::TextureCube, D3D12_SRV_DIMENSION_TEXTURECUBE)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::TextureCubeArray, D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)
				CASE(RenderDriver::Common::ShaderResourceViewDimension::RayTracingAccelerationStructure, D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
			}

			return ret;
		}

		D3D12_RTV_DIMENSION convert(RenderDriver::Common::Dimension dimension)
		{
			D3D12_RTV_DIMENSION ret = D3D12_RTV_DIMENSION_UNKNOWN;
			switch(dimension)
			{
				CASE(RenderDriver::Common::Dimension::Buffer, D3D12_RTV_DIMENSION_BUFFER)
				CASE(RenderDriver::Common::Dimension::Texture1D, D3D12_RTV_DIMENSION_TEXTURE1D)
				CASE(RenderDriver::Common::Dimension::Texture2D, D3D12_RTV_DIMENSION_TEXTURE2D)
				CASE(RenderDriver::Common::Dimension::Texture3D, D3D12_RTV_DIMENSION_TEXTURE3D)
				CASE(RenderDriver::Common::Dimension::Unknown, D3D12_RTV_DIMENSION_UNKNOWN)
			}

			return ret;
		}

		D3D12_DSV_DIMENSION convert(RenderDriver::Common::DepthStencilDimension dimension)
		{
			D3D12_DSV_DIMENSION ret = D3D12_DSV_DIMENSION_UNKNOWN;
			switch(dimension)
			{
				CASE(RenderDriver::Common::DepthStencilDimension::Texture1D,	D3D12_DSV_DIMENSION_TEXTURE1D)
				CASE(RenderDriver::Common::DepthStencilDimension::Texture2D,	D3D12_DSV_DIMENSION_TEXTURE2D)
				CASE(RenderDriver::Common::DepthStencilDimension::Unknown,		D3D12_DSV_DIMENSION_UNKNOWN)
			}

			return ret;
		}

		/*
		**
		*/
		uint32_t getBaseComponentSize(DXGI_FORMAT format)
		{
			uint32_t ret = 1;
			switch(format)
			{
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
				ret = 4;
				break;

			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT_R8G8_SINT:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8_UNORM:
				ret = 1;
				break;

			default:
				assert(0);
			}

			return ret;
		}

	}	// DX12						   

	namespace Vulkan
	{
		/*
		**
		*/
		VkPolygonMode convert(RenderDriver::Common::FillMode const& mode)
		{
			VkPolygonMode ret = VK_POLYGON_MODE_FILL;
			switch(mode)
			{
				CASE(RenderDriver::Common::FillMode::Wireframe, VK_POLYGON_MODE_LINE)
				CASE(RenderDriver::Common::FillMode::Solid, VK_POLYGON_MODE_FILL)
			};

			return ret;
		}

		/*
		**
		*/
		VkCullModeFlags convert(RenderDriver::Common::CullMode const& mode)
		{
			VkCullModeFlags ret = VK_CULL_MODE_NONE;
			switch(mode)
			{
				CASE(RenderDriver::Common::CullMode::None, VK_CULL_MODE_NONE)
				CASE(RenderDriver::Common::CullMode::Front, VK_CULL_MODE_FRONT_BIT)
				CASE(RenderDriver::Common::CullMode::Back, VK_CULL_MODE_BACK_BIT)
			}

			return ret;
		}

		/*
		**
		*/
		VkColorComponentFlags convert(RenderDriver::Common::ColorWriteMask const& mask)
		{
			VkColorComponentFlags ret = 
				VK_COLOR_COMPONENT_R_BIT |
				VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT |
				VK_COLOR_COMPONENT_A_BIT;

			return ret;
		}

		/*
		**
		*/
		VkBlendFactor convert(RenderDriver::Common::BlendType const& type)
		{
			VkBlendFactor ret = VK_BLEND_FACTOR_ZERO;
			switch(type)
			{
				CASE(RenderDriver::Common::BlendType::Zero, VK_BLEND_FACTOR_ZERO)
				CASE(RenderDriver::Common::BlendType::One, VK_BLEND_FACTOR_ONE)
				CASE(RenderDriver::Common::BlendType::SrcColor, VK_BLEND_FACTOR_SRC_COLOR)
				CASE(RenderDriver::Common::BlendType::InvSrcColor, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR)
				CASE(RenderDriver::Common::BlendType::SrcAlpha, VK_BLEND_FACTOR_SRC_ALPHA)
				CASE(RenderDriver::Common::BlendType::InvSrcAlpha, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
				CASE(RenderDriver::Common::BlendType::DestColor, VK_BLEND_FACTOR_DST_COLOR)
				CASE(RenderDriver::Common::BlendType::InvDestColor, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR)
				CASE(RenderDriver::Common::BlendType::DestAlpha, VK_BLEND_FACTOR_DST_ALPHA)
				CASE(RenderDriver::Common::BlendType::InvDestAlpha, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA)
				CASE(RenderDriver::Common::BlendType::SrcAlphaSaturate, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE)
				CASE(RenderDriver::Common::BlendType::BlendFactor, VK_BLEND_FACTOR_CONSTANT_COLOR)
				CASE(RenderDriver::Common::BlendType::InvBlendFactor, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR)
				CASE(RenderDriver::Common::BlendType::Src1Color, VK_BLEND_FACTOR_SRC1_COLOR)
				CASE(RenderDriver::Common::BlendType::InvSrc1Color, VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR)
				CASE(RenderDriver::Common::BlendType::Src1Alpha, VK_BLEND_FACTOR_SRC1_ALPHA)
				CASE(RenderDriver::Common::BlendType::InvSrc1Alpha, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA)
			}

			return ret;
		}

		/*
		**
		*/
		VkBlendOp convert(RenderDriver::Common::BlendOperator const& type)
		{
			VkBlendOp ret = VK_BLEND_OP_ADD;
			switch(type)
			{
				CASE(RenderDriver::Common::BlendOperator::Add, VK_BLEND_OP_ADD)
				CASE(RenderDriver::Common::BlendOperator::Subtract, VK_BLEND_OP_SUBTRACT)
				CASE(RenderDriver::Common::BlendOperator::ReverseSubtract, VK_BLEND_OP_REVERSE_SUBTRACT)
				CASE(RenderDriver::Common::BlendOperator::Min, VK_BLEND_OP_MIN)
				CASE(RenderDriver::Common::BlendOperator::Max, VK_BLEND_OP_MAX)
			}

			return ret;
		}

		/*
		**
		*/
		VkLogicOp convert(RenderDriver::Common::BlendLogicOperator const& type)
		{
			VkLogicOp ret = VK_LOGIC_OP_NO_OP;
			switch(type)
			{
				CASE(RenderDriver::Common::BlendLogicOperator::Clear, VK_LOGIC_OP_CLEAR)
				CASE(RenderDriver::Common::BlendLogicOperator::Set, VK_LOGIC_OP_SET)
				CASE(RenderDriver::Common::BlendLogicOperator::Copy, VK_LOGIC_OP_COPY)
				CASE(RenderDriver::Common::BlendLogicOperator::CopyInverted, VK_LOGIC_OP_COPY_INVERTED)
				CASE(RenderDriver::Common::BlendLogicOperator::NoOp, VK_LOGIC_OP_NO_OP)
				CASE(RenderDriver::Common::BlendLogicOperator::Invert, VK_LOGIC_OP_INVERT)
				CASE(RenderDriver::Common::BlendLogicOperator::And, VK_LOGIC_OP_AND)
				CASE(RenderDriver::Common::BlendLogicOperator::Nand, VK_LOGIC_OP_NAND)
				CASE(RenderDriver::Common::BlendLogicOperator::Or, VK_LOGIC_OP_OR)
				CASE(RenderDriver::Common::BlendLogicOperator::Nor, VK_LOGIC_OP_NOR)
				CASE(RenderDriver::Common::BlendLogicOperator::Xor, VK_LOGIC_OP_XOR)
				CASE(RenderDriver::Common::BlendLogicOperator::Equivalent, VK_LOGIC_OP_EQUIVALENT)
				CASE(RenderDriver::Common::BlendLogicOperator::AndReverse, VK_LOGIC_OP_AND_REVERSE)
				CASE(RenderDriver::Common::BlendLogicOperator::AndInverted, VK_LOGIC_OP_AND_INVERTED)
				CASE(RenderDriver::Common::BlendLogicOperator::OrReverse, VK_LOGIC_OP_OR_REVERSE)
				CASE(RenderDriver::Common::BlendLogicOperator::OrInverted, VK_LOGIC_OP_OR_INVERTED)
			}

			return ret;
		}

		/*
		**
		*/
		VkFormat convert(RenderDriver::Common::Format const& format)
		{
			VkFormat ret = VK_FORMAT_UNDEFINED;
			switch(format)
			{
				CASE(RenderDriver::Common::Format::UNKNOWN, VK_FORMAT_UNDEFINED)
				CASE(RenderDriver::Common::Format::R32G32B32A32_TYPELESS, VK_FORMAT_R32G32B32A32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32G32B32A32_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT)
				CASE(RenderDriver::Common::Format::R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SINT)
				CASE(RenderDriver::Common::Format::R32G32B32_TYPELESS, VK_FORMAT_R32G32B32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32G32B32_FLOAT, VK_FORMAT_R32G32B32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32G32B32_UINT, VK_FORMAT_R32G32B32_UINT)
				CASE(RenderDriver::Common::Format::R32G32B32_SINT, VK_FORMAT_R32G32B32_SINT)
				CASE(RenderDriver::Common::Format::R16G16B16A16_TYPELESS, VK_FORMAT_R16G16B16A16_SFLOAT)
				CASE(RenderDriver::Common::Format::R16G16B16A16_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT)
				CASE(RenderDriver::Common::Format::R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_UNORM)
				CASE(RenderDriver::Common::Format::R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT)
				CASE(RenderDriver::Common::Format::R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_SNORM)
				CASE(RenderDriver::Common::Format::R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT)
				CASE(RenderDriver::Common::Format::R32G32_TYPELESS, VK_FORMAT_R32G32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32G32_FLOAT, VK_FORMAT_R32G32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32G32_UINT, VK_FORMAT_R32G32_UINT)
				CASE(RenderDriver::Common::Format::R32G32_SINT, VK_FORMAT_R32G32_SINT)
				CASE(RenderDriver::Common::Format::R32G8X24_TYPELESS, VK_FORMAT_UNDEFINED)
				CASE(RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT)
				CASE(RenderDriver::Common::Format::R32_FLOAT_X8X24_TYPELESS, VK_FORMAT_D32_SFLOAT_S8_UINT)
				CASE(RenderDriver::Common::Format::X32_TYPELESS_G8X24_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT)
				CASE(RenderDriver::Common::Format::R10G10B10A2_TYPELESS, VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
				CASE(RenderDriver::Common::Format::R10G10B10A2_UNORM, VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
				CASE(RenderDriver::Common::Format::R10G10B10A2_UINT, VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
				CASE(RenderDriver::Common::Format::R11G11B10_FLOAT, VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
				CASE(RenderDriver::Common::Format::R8G8B8A8_TYPELESS, VK_FORMAT_R8G8B8A8_UNORM)
				CASE(RenderDriver::Common::Format::R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM)
				CASE(RenderDriver::Common::Format::R8G8B8A8_UNORM_SRGB, VK_FORMAT_R8G8B8A8_UNORM)
				CASE(RenderDriver::Common::Format::R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT)
				CASE(RenderDriver::Common::Format::R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM)
				CASE(RenderDriver::Common::Format::R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT)
				CASE(RenderDriver::Common::Format::R16G16_TYPELESS, VK_FORMAT_R16G16_SFLOAT)
				CASE(RenderDriver::Common::Format::R16G16_FLOAT, VK_FORMAT_R16G16_SFLOAT)
				CASE(RenderDriver::Common::Format::R16G16_UNORM, VK_FORMAT_R16G16_UNORM)
				CASE(RenderDriver::Common::Format::R16G16_UINT, VK_FORMAT_R16G16_UINT)
				CASE(RenderDriver::Common::Format::R16G16_SNORM, VK_FORMAT_R16G16_SNORM)
				CASE(RenderDriver::Common::Format::R16G16_SINT, VK_FORMAT_R16G16_SINT)
				CASE(RenderDriver::Common::Format::R32_TYPELESS, VK_FORMAT_R32_SFLOAT)
				CASE(RenderDriver::Common::Format::D32_FLOAT, VK_FORMAT_D32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32_FLOAT, VK_FORMAT_R32_SFLOAT)
				CASE(RenderDriver::Common::Format::R32_UINT, VK_FORMAT_R32_UINT)
				CASE(RenderDriver::Common::Format::R32_SINT, VK_FORMAT_R32_SINT)
				CASE(RenderDriver::Common::Format::R24G8_TYPELESS, VK_FORMAT_R32G32_SFLOAT)
				CASE(RenderDriver::Common::Format::D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT)
				CASE(RenderDriver::Common::Format::R24_UNORM_X8_TYPELESS, VK_FORMAT_D24_UNORM_S8_UINT)
				CASE(RenderDriver::Common::Format::X24_TYPELESS_G8_UINT, VK_FORMAT_D24_UNORM_S8_UINT)
				CASE(RenderDriver::Common::Format::R8G8_TYPELESS, VK_FORMAT_R8G8_UNORM)
				CASE(RenderDriver::Common::Format::R8G8_UNORM, VK_FORMAT_R8G8_UNORM)
				CASE(RenderDriver::Common::Format::R8G8_UINT, VK_FORMAT_R8G8_UINT)
				CASE(RenderDriver::Common::Format::R8G8_SNORM, VK_FORMAT_R8G8_SNORM)
				CASE(RenderDriver::Common::Format::R8G8_SINT, VK_FORMAT_R8G8_SINT)
				CASE(RenderDriver::Common::Format::R16_TYPELESS, VK_FORMAT_R16_SFLOAT)
				CASE(RenderDriver::Common::Format::R16_FLOAT, VK_FORMAT_R16_SFLOAT)
				CASE(RenderDriver::Common::Format::D16_UNORM, VK_FORMAT_R16_UNORM)
				CASE(RenderDriver::Common::Format::R16_UNORM, VK_FORMAT_R16_UNORM)
				CASE(RenderDriver::Common::Format::R16_UINT, VK_FORMAT_R16_UINT)
				CASE(RenderDriver::Common::Format::R16_SNORM, VK_FORMAT_R16G16_SNORM)
				CASE(RenderDriver::Common::Format::R16_SINT, VK_FORMAT_R16_SINT)
				CASE(RenderDriver::Common::Format::R8_TYPELESS, VK_FORMAT_R8_SRGB)
				CASE(RenderDriver::Common::Format::R8_UNORM, VK_FORMAT_R8_UNORM)
				CASE(RenderDriver::Common::Format::R8_UINT, VK_FORMAT_R8_UINT)
				CASE(RenderDriver::Common::Format::R8_SNORM, VK_FORMAT_R8_SNORM)
				CASE(RenderDriver::Common::Format::R8_SINT, VK_FORMAT_R8_SINT)
				CASE(RenderDriver::Common::Format::A8_UNORM, VK_FORMAT_A8_UNORM_KHR)
				CASE(RenderDriver::Common::Format::R1_UNORM, VK_FORMAT_A8_UNORM_KHR)
				//CASE(RenderDriver::Common::Format::BC1_TYPELESS, VK_FORMAT_BC1_TYPELESS)
				//CASE(RenderDriver::Common::Format::BC1_UNORM, VK_FORMAT_BC1_UNORM)
				//CASE(RenderDriver::Common::Format::BC1_UNORM_SRGB, VK_FORMAT_BC1_UNORM_SRGB)
				//CASE(RenderDriver::Common::Format::BC2_TYPELESS, VK_FORMAT_BC2_TYPELESS)
				//CASE(RenderDriver::Common::Format::BC2_UNORM, VK_FORMAT_BC2_UNORM)
				//CASE(RenderDriver::Common::Format::BC2_UNORM_SRGB, VK_FORMAT_BC2_UNORM_SRGB)
				//CASE(RenderDriver::Common::Format::BC3_TYPELESS, VK_FORMAT_BC3_TYPELESS)
				//CASE(RenderDriver::Common::Format::BC3_UNORM, VK_FORMAT_BC3_UNORM)
				//CASE(RenderDriver::Common::Format::BC3_UNORM_SRGB, VK_FORMAT_BC3_UNORM_SRGB)
				//CASE(RenderDriver::Common::Format::BC4_TYPELESS, VK_FORMAT_BC4_TYPELESS)
				//CASE(RenderDriver::Common::Format::BC4_UNORM, VK_FORMAT_BC4_UNORM)
				//CASE(RenderDriver::Common::Format::BC4_SNORM, VK_FORMAT_BC4_SNORM)
				//CASE(RenderDriver::Common::Format::BC5_TYPELESS, VK_FORMAT_BC5_TYPELESS)
				//CASE(RenderDriver::Common::Format::BC5_UNORM, VK_FORMAT_BC5_UNORM)
				//CASE(RenderDriver::Common::Format::BC5_SNORM, VK_FORMAT_BC5_SNORM)
				//CASE(RenderDriver::Common::Format::B5G6R5_UNORM, VK_FORMAT_B5G6R5_UNORM)
				//CASE(RenderDriver::Common::Format::B5G5R5A1_UNORM, VK_FORMAT_B5G5R5A1_UNORM)
				CASE(RenderDriver::Common::Format::B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)
				//CASE(RenderDriver::Common::Format::B8G8R8X8_UNORM, VK_FORMAT_B8G8R8X8_UNORM)
				CASE(RenderDriver::Common::Format::R10G10B10_XR_BIAS_A2_UNORM, VK_FORMAT_A2R10G10B10_UNORM_PACK32)
				//CASE(RenderDriver::Common::Format::B8G8R8A8_TYPELESS, VK_FORMAT_B8G8R8A8_TYPELESS)
				CASE(RenderDriver::Common::Format::B8G8R8A8_UNORM_SRGB, VK_FORMAT_B8G8R8A8_SNORM)
				//CASE(RenderDriver::Common::Format::B8G8R8X8_TYPELESS, VK_FORMAT_B8G8R8X8_TYPELESS)
				//CASE(RenderDriver::Common::Format::B8G8R8X8_UNORM_SRGB, VK_FORMAT_B8G8R8X8_UNORM_SRGB)
				//CASE(RenderDriver::Common::Format::BC6H_TYPELESS, VK_FORMAT_BC6H_TYPELESS)
				//CASE(RenderDriver::Common::Format::BC6H_UF16, VK_FORMAT_BC6H_UF16)
				//CASE(RenderDriver::Common::Format::BC6H_SF16, VK_FORMAT_BC6H_SF16)
				//CASE(RenderDriver::Common::Format::BC7_TYPELESS, VK_FORMAT_BC7_TYPELESS)
				//CASE(RenderDriver::Common::Format::BC7_UNORM, VK_FORMAT_BC7_UNORM)
				//CASE(RenderDriver::Common::Format::BC7_UNORM_SRGB, VK_FORMAT_BC7_UNORM_SRGB)
				//CASE(RenderDriver::Common::Format::AYUV, VK_FORMAT_AYUV)
				//CASE(RenderDriver::Common::Format::Y410, VK_FORMAT_Y410)
				//CASE(RenderDriver::Common::Format::Y416, VK_FORMAT_Y416)
				//CASE(RenderDriver::Common::Format::NV12, VK_FORMAT_NV12)
				//CASE(RenderDriver::Common::Format::P010, VK_FORMAT_P010)
				//CASE(RenderDriver::Common::Format::P016, VK_FORMAT_P016)
				//CASE(RenderDriver::Common::Format::YUY2, VK_FORMAT_YUY2)
				//CASE(RenderDriver::Common::Format::Y210, VK_FORMAT_Y210)
				//CASE(RenderDriver::Common::Format::Y216, VK_FORMAT_Y216)
				//CASE(RenderDriver::Common::Format::NV11, VK_FORMAT_NV11)
				//CASE(RenderDriver::Common::Format::AI44, VK_FORMAT_AI44)
				//CASE(RenderDriver::Common::Format::IA44, VK_FORMAT_IA44)
				//CASE(RenderDriver::Common::Format::P8, VK_FORMAT_P8)
				//CASE(RenderDriver::Common::Format::A8P8, VK_FORMAT_A8P8)
				//CASE(RenderDriver::Common::Format::B4G4R4A4_UNORM, VK_FORMAT_B4G4R4A4_UNORM)


			}

			return ret;
		}

		/*
		**
		*/
		VkCompareOp convert(RenderDriver::Common::ComparisonFunc const& op)
		{
			VkCompareOp ret = VK_COMPARE_OP_NEVER;
			switch(op)
			{
				CASE(RenderDriver::Common::ComparisonFunc::Never, VK_COMPARE_OP_NEVER)
				CASE(RenderDriver::Common::ComparisonFunc::Less, VK_COMPARE_OP_LESS)
				CASE(RenderDriver::Common::ComparisonFunc::Equal, VK_COMPARE_OP_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::LessEqual, VK_COMPARE_OP_LESS_OR_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::Greater, VK_COMPARE_OP_GREATER)
				CASE(RenderDriver::Common::ComparisonFunc::NotEqual, VK_COMPARE_OP_NOT_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::GreaterEqual, VK_COMPARE_OP_GREATER_OR_EQUAL)
				CASE(RenderDriver::Common::ComparisonFunc::Always, VK_COMPARE_OP_ALWAYS)
			}

			return ret;
		}

		/*
		**
		*/
		VkStencilOp convert(RenderDriver::Common::StencilOperator op)
		{
			VkStencilOp ret = VK_STENCIL_OP_KEEP;
			switch(op)
			{
				CASE(RenderDriver::Common::StencilOperator::Keep, VK_STENCIL_OP_KEEP)
				CASE(RenderDriver::Common::StencilOperator::Zero, VK_STENCIL_OP_ZERO)
				CASE(RenderDriver::Common::StencilOperator::Replace, VK_STENCIL_OP_REPLACE)
				CASE(RenderDriver::Common::StencilOperator::IncrementSaturate, VK_STENCIL_OP_INCREMENT_AND_CLAMP)
				CASE(RenderDriver::Common::StencilOperator::DecrementSaturate, VK_STENCIL_OP_DECREMENT_AND_CLAMP)
				CASE(RenderDriver::Common::StencilOperator::Invert, VK_STENCIL_OP_INVERT)
				CASE(RenderDriver::Common::StencilOperator::Increment, VK_STENCIL_OP_INCREMENT_AND_WRAP)
				CASE(RenderDriver::Common::StencilOperator::Decrement, VK_STENCIL_OP_DECREMENT_AND_WRAP)
			}

			return ret;
		}

		/*
		**
		*/
		VkStencilOpState convert(RenderDriver::Common::DepthStencilOpDesc const& desc)
		{
			VkStencilOpState ret;
			ret.depthFailOp = SerializeUtils::Vulkan::convert(desc.mStencilDepthFailOp);
			ret.failOp = SerializeUtils::Vulkan::convert(desc.mStencilFailOp);
			ret.compareOp = SerializeUtils::Vulkan::convert(desc.mStencilFunc);
			ret.passOp = SerializeUtils::Vulkan::convert(desc.mStencilPassOp);

			ret.compareMask = 0;
			ret.writeMask = 0;
			ret.reference = 0;

			return ret;
		}

		/*
		**
		*/
		VkVertexInputAttributeDescription convert(
			RenderDriver::Common::VertexFormat const& format, 
			uint32_t iLocation,
			uint32_t iOffset)
		{
			VkVertexInputAttributeDescription desc = {};
			desc.binding = 0;
			desc.location = iLocation;
			desc.format = SerializeUtils::Vulkan::convert(format.mFormat);
			desc.offset = iOffset;

			return desc;
		}

		/*
		**
		*/
		uint32_t getBaseComponentSize(VkFormat const& format)
		{
			uint32_t ret = 1;
			switch(format)
			{
			case VK_FORMAT_R32G32B32A32_SFLOAT:
			case VK_FORMAT_R32G32B32_SFLOAT:
			case VK_FORMAT_R32G32_SFLOAT:
			case VK_FORMAT_R32_SFLOAT:
				ret = 4;
				break;

			case VK_FORMAT_R8G8B8A8_UINT:
			case VK_FORMAT_R8G8B8_UINT:
			case VK_FORMAT_R8_UINT:
			case VK_FORMAT_R8G8B8A8_SINT:
			case VK_FORMAT_R8G8_SINT:
			case VK_FORMAT_R8_SINT:
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_R8G8_UNORM:
			case VK_FORMAT_R8_UNORM:
				ret = 1;
				break;

			default:
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		VkImageLayout convert(RenderDriver::Common::TextureLayout const& layout)
		{
			VkImageLayout ret = VK_IMAGE_LAYOUT_UNDEFINED;
			switch(layout)
			{
			case RenderDriver::Common::TextureLayout::Unknown:
			{
				ret = VK_IMAGE_LAYOUT_UNDEFINED;
				break;
			}
			
			}

			return ret;
		}

		/*
		**
		*/
		VkImageLayout convert(RenderDriver::Common::ResourceStateFlagBits const& resourceState)
		{
			VkImageLayout ret = VK_IMAGE_LAYOUT_UNDEFINED;
			switch(resourceState)
			{
			case RenderDriver::Common::ResourceStateFlagBits::Common:
			case RenderDriver::Common::ResourceStateFlagBits::RenderTarget:
			case RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess:
			{
				ret = VK_IMAGE_LAYOUT_GENERAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::ColorAttachment:
			{
				ret = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::DepthStencilAttachment:
			{
				ret = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::DepthWrite:
			case RenderDriver::Common::ResourceStateFlagBits::DepthRead:
			{
				ret = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::PixelShaderResource:
			case RenderDriver::Common::ResourceStateFlagBits::IndirectArgument:
			{
				ret = VK_IMAGE_LAYOUT_GENERAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::CopyDestination:
			{
				ret = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::CopySource:
			{
				ret = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::GenericRead:
			{
				ret = VK_IMAGE_LAYOUT_GENERAL;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::Present2:
			{
				ret = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				break;
			}
			case RenderDriver::Common::ResourceStateFlagBits::None:
			{
				ret = VK_IMAGE_LAYOUT_UNDEFINED;
				break;
			}
			default:
				assert(0);
			}

			return ret;
		}

		/*
		**
		*/
		VkBufferUsageFlagBits convert(RenderDriver::Common::BufferUsage const& usage)
		{
			VkBufferUsageFlagBits ret = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			switch(usage)
			{
				CASE(RenderDriver::Common::BufferUsage::TransferSrc, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
				CASE(RenderDriver::Common::BufferUsage::TransferDest, VK_BUFFER_USAGE_TRANSFER_DST_BIT)
				CASE(RenderDriver::Common::BufferUsage::UniformTexel, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
				CASE(RenderDriver::Common::BufferUsage::StorageTexel, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
				CASE(RenderDriver::Common::BufferUsage::UniformBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
				CASE(RenderDriver::Common::BufferUsage::StorageBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
				CASE(RenderDriver::Common::BufferUsage::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
				CASE(RenderDriver::Common::BufferUsage::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
				CASE(RenderDriver::Common::BufferUsage::IndirectBuffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
				CASE(RenderDriver::Common::BufferUsage::ShaderDeviceAddress, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
				CASE(RenderDriver::Common::BufferUsage::ShaderBindingTable, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR)
				CASE(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)
				CASE(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)
			}

			return ret;
		}

		/*
		**
		*/
		VkImageCreateFlags convert(RenderDriver::Common::ResourceFlagBits const& flag)
		{
			return 0;
		}

		/*
		**
		*/
		VkImageLayout convert(RenderDriver::Common::ImageLayout const& imageLayout)
		{
			VkImageLayout ret = VK_IMAGE_LAYOUT_UNDEFINED;
			switch(imageLayout)
			{
				CASE(RenderDriver::Common::ImageLayout::UNDEFINED,										VK_IMAGE_LAYOUT_UNDEFINED)
				CASE(RenderDriver::Common::ImageLayout::GENERAL,										VK_IMAGE_LAYOUT_GENERAL)
				CASE(RenderDriver::Common::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL,				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::SHADER_READ_ONLY_OPTIMAL,						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::TRANSFER_SRC_OPTIMAL,							VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::TRANSFER_DST_OPTIMAL,							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::PREINITIALIZED,									VK_IMAGE_LAYOUT_PREINITIALIZED)
				CASE(RenderDriver::Common::ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,		VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::DEPTH_ATTACHMENT_OPTIMAL,						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::DEPTH_READ_ONLY_OPTIMAL,						VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::STENCIL_ATTACHMENT_OPTIMAL,						VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::STENCIL_READ_ONLY_OPTIMAL,						VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::READ_ONLY_OPTIMAL,								VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::ATTACHMENT_OPTIMAL,								VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
				CASE(RenderDriver::Common::ImageLayout::PRESENT_SRC,									VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			}

			return ret;
		}

		/*
		**
		*/
		RenderDriver::Common::ImageLayout convert(VkImageLayout const& imageLayout)
		{
			RenderDriver::Common::ImageLayout ret = RenderDriver::Common::ImageLayout::UNDEFINED;
			switch(imageLayout)
			{
				CASE(VK_IMAGE_LAYOUT_UNDEFINED, RenderDriver::Common::ImageLayout::UNDEFINED)
				CASE(VK_IMAGE_LAYOUT_GENERAL, RenderDriver::Common::ImageLayout::GENERAL)
				CASE(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderDriver::Common::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, RenderDriver::Common::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, RenderDriver::Common::ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, RenderDriver::Common::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, RenderDriver::Common::ImageLayout::TRANSFER_SRC_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, RenderDriver::Common::ImageLayout::TRANSFER_DST_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_PREINITIALIZED, RenderDriver::Common::ImageLayout::PREINITIALIZED)
				CASE(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, RenderDriver::Common::ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, RenderDriver::Common::ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, RenderDriver::Common::ImageLayout::DEPTH_ATTACHMENT_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, RenderDriver::Common::ImageLayout::DEPTH_READ_ONLY_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, RenderDriver::Common::ImageLayout::STENCIL_ATTACHMENT_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL, RenderDriver::Common::ImageLayout::STENCIL_READ_ONLY_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, RenderDriver::Common::ImageLayout::READ_ONLY_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, RenderDriver::Common::ImageLayout::ATTACHMENT_OPTIMAL)
				CASE(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, RenderDriver::Common::ImageLayout::PRESENT_SRC)

			}

			return ret;
		}

		/*
		**
		*/
		VkAttachmentLoadOp convert(RenderDriver::Common::LoadOp const& loadOp)
		{
			VkAttachmentLoadOp ret = VK_ATTACHMENT_LOAD_OP_CLEAR;
			switch(loadOp)
			{
				CASE(RenderDriver::Common::LoadOp::Clear, VK_ATTACHMENT_LOAD_OP_CLEAR)
				CASE(RenderDriver::Common::LoadOp::Load, VK_ATTACHMENT_LOAD_OP_LOAD)
			}

			return ret;
		}

	}	// Vulkan
#endif // _MSC_VER

}	// SerializeUtils








