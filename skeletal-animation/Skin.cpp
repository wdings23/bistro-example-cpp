#include <skeletal-animation/Skin.h>

#include <LogPrint.h>
#include <wtfassert.h>

#include <sstream>

namespace Render
{
    namespace Animation
    {
        namespace Skin
        {
            /*
            **
            */
            void applyTransformationToVertices(ApplyTransformationDescriptor& desc)
            {
                for(uint32_t iV = 0; iV < desc.miNumVertices; iV++)
                {
                    uint32_t iNumInfluences = static_cast<uint32_t>(desc.maaiInfluences[iV].size());
                    for(uint32_t iInfluence = 0; iInfluence < iNumInfluences; iInfluence++)
                    {
                        uint32_t iInfluenceJointIndex = desc.maaiInfluences[iV][iInfluence];
                        float fWeight = desc.maafWeightPct[iV][iInfluence];
                    
                        mat4 const& transformMatrix = desc.maMatrices[iInfluenceJointIndex];
                        desc.mpaXFormVertices[iV].mPosition += ((transformMatrix * desc.mpaVertices[iV].mPosition) * fWeight);
                        
                        mat4 const& inverseTransposeTransformMatrix = transpose(invert(transformMatrix));
                        desc.mpaXFormVertices[iV].mNormal += ((inverseTransposeTransformMatrix * desc.mpaVertices[iV].mNormal * fWeight));
                    }

                    desc.mpaXFormVertices[iV].mTexCoord = desc.mpaVertices[iV].mTexCoord;
                }
            }

            /*
            **
            */
            void updateSkinningDataToGPU(UpdateGPUSkinningDataDescriptor& desc)
            {
                WTFASSERT(0, "Implement me");
            }

        }   // Skin


    }   // Animation

}   // Render