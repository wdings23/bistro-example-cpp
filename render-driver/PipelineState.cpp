#include <render-driver/PipelineState.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPipelineState::create(
            GraphicsPipelineStateDescriptor const& desc,
            CDevice& device)
        {
            mGraphicsDesc = desc;
            mHandle = device.assignHandleAndUpdate();
            mType = PipelineType::GRAPHICS_PIPELINE_TYPE;

            return mHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPipelineState::create(
            ComputePipelineStateDescriptor const& desc,
            CDevice& device)
        {
            mComputeDesc = desc;
            mHandle = device.assignHandleAndUpdate();
            mType = PipelineType::COMPUTE_PIPELINE_TYPE;

            return mHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPipelineState::create(
            RayTracePipelineStateDescriptor const& desc,
            CDevice& device)
        {
            mRayTraceDesc = desc;
            mHandle = device.assignHandleAndUpdate();
            mType = PipelineType::RAY_TRACE_PIPELINE_TYPE;

            return mHandle;
        }

        /*
        **
        */
        void* CPipelineState::getNativePipelineState()
        {
            return nullptr;
        }

    }   // Common

}   // RenderDriver