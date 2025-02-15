#pragma once

#include <string>
#include <vector>

#include <render-driver/Object.h>
//#include <render_enums.h>
#include <utils/serialize_utils.h>

struct AttachmentInfo;

namespace RenderDriver
{
    namespace Common
    {
		struct PipelineInfo
		{
			std::string															mName;
			std::string															mFilePath;

			PipelineType														mType;
			PipelineDataInfo													mDataInfo;

			std::vector<std::string>											maParentPipelines;
			std::vector<std::string>											maChildrenPipelines;
			std::vector<AttachmentInfo>											maAttachments;

			std::vector<uint32_t>												maiChildAttachmentIndices;
			std::vector<uint32_t>												maiParentAttachmentIndices;

			PLATFORM_OBJECT_HANDLE												mPipelineStateHandle;
			PLATFORM_OBJECT_HANDLE												mDescriptorHandle;
		};

    }	// Common

}	// RenderDriver
