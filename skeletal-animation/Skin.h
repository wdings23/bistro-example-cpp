#pragma once

#include <render/VertexFormat.h>


#include <mat4.h>
#include <string>
#include <vector>

namespace Render
{
    namespace Animation
    {
        struct SkeletalSkinningInfo
        {
            std::vector<std::vector<uint32_t>>       maaiJointInfluences;
            std::vector<std::vector<float>>          maafJointWeightPct;
        };
        
        namespace Skin
        {
            struct ApplyTransformationDescriptor
            {
                Render::Common::VertexFormat*                  mpaXFormVertices;
                Render::Common::VertexFormat const*            mpaVertices;
                uint32_t                                       miNumVertices;
                std::vector<std::vector<float>>                maafWeightPct;
                std::vector<std::vector<uint32_t>>             maaiInfluences;
                std::vector<mat4>                              maMatrices;
            };

            struct UpdateGPUSkinningDataDescriptor
            {
                std::string                                     mSkinMeshName;
                Render::Common::VertexFormat const*             mpaVertices;
                uint32_t                                        miNumVertices;
                std::vector<std::vector<float>> const*          mpaafWeightPct;
                std::vector<std::vector<uint32_t>> const*       mpaaiInfluences;
                std::vector<mat4> const*                        mpaMatrices;
                uint32_t                                        miInstanceIndex;
            };

            void applyTransformationToVertices(ApplyTransformationDescriptor& desc);
            void updateSkinningDataToGPU(UpdateGPUSkinningDataDescriptor& desc);

        }   // Skin

    }   // Animation

}   // Render