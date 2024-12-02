#include <skeletal-animation/AnimationManager.h>
#include <skeletal-animation/SkeletalHierarchy.h>
#include <Utils/Utils.h>
#include <quaternion.h>

#include <assert.h>
#include <sstream>

#include "wtfassert.h"

namespace Render
{
    namespace Animation
    {
        std::unique_ptr<CAnimationManager> CAnimationManager::spInstance = nullptr;
        std::unique_ptr<CAnimationManager>& CAnimationManager::instance()
        {
            if(CAnimationManager::spInstance == nullptr)
            {
                CAnimationManager::spInstance = std::make_unique<CAnimationManager>();
            }

            return CAnimationManager::spInstance;
        }

        /*
        **
        */
        void CAnimationManager::setup(AnimationManagerSetupDescriptor const& desc)
        {
            macTempStorage.resize(desc.miTempStorageSize);
        }

        /*
        **
        */
        void CAnimationManager::addAnimations(AddAnimationDescriptor const& desc)
        {   
            WTFASSERT(0, "Implement me");

            
        }
        
        /*
        **
        */
        void CAnimationManager::getMatricesAtTime(AnimationUpdateDescriptor const& desc)
        {
            auto baseEnd = desc.mAnimationName.find(".animation.");
            std::string baseName = desc.mAnimationName.substr(0, baseEnd);

            assert(mSkeletalAnimations.find(baseName) != mSkeletalAnimations.end());
            SkeletalAnimationInfo& skeletalAnimationInfo = mSkeletalAnimations[baseName];

            auto const& aJoints = skeletalAnimationInfo.mSkeletalHierarchy.getJoints();
            uint32_t iNumJoints = static_cast<uint32_t>(aJoints.size());

            std::vector<vec4> aJointRotations(iNumJoints);
            std::vector<vec4> aJointTranslations(iNumJoints);
            std::vector<vec4> aJointScalings(iNumJoints);

            std::vector<std::pair<uint32_t, vec3>> aAdditionalJointTranslations;
            if(desc.mpaAdditionalJointTranslations != nullptr)
            {
                for(auto const& info : *desc.mpaAdditionalJointTranslations)
                {
                    for(uint32_t iJoint = 0; iJoint < iNumJoints; iJoint++)
                    {
                        if(aJoints[iJoint].mName == info.first)
                        {
                            aAdditionalJointTranslations.push_back(std::make_pair(iJoint, info.second));
                            break;
                        }
                    }
                }
            }

            std::vector<mat4>& aTransformedMatrices = *desc.mpaTransformationMatrices;
            aTransformedMatrices.resize(iNumJoints);
            ApplyAnimationDescriptor applyAnimationDesc =
            {
                /* .mName                  */      desc.mAnimationName,
                /* .mpSkeletalHiearachy    */      &skeletalAnimationInfo.mSkeletalHierarchy,
                /* .mpaTransformedMatrices */      &aTransformedMatrices,
                /* .mpaAdditionalJointTranslations */ &aAdditionalJointTranslations,
                /* .mfTime                 */      desc.mfTime,               
            };
            skeletalAnimationInfo.mSkeletalAnimation.applyAnimation(applyAnimationDesc);
        }

        /*
        **
        */
        void CAnimationManager::getSkeletalSkinInfo(SkeletalSkinQueryDescriptor& desc)
        {
            assert(mSkeletalSkins.find(desc.mName) != mSkeletalSkins.end());
            desc.mpSkinInfo = &mSkeletalSkins[desc.mName];
        }

        /*
        **
        */
        void CAnimationManager::getSkeletalAnimations(SkeletalAnimationQueryDescriptor& desc)
        {
            assert(mSkeletalAnimations.find(desc.mName) != mSkeletalAnimations.end());
            desc.mpAnimationInfo = &mSkeletalAnimations[desc.mName];
        }

        /*
        **
        */
        void CAnimationManager::uploadHiearchyDataToGPU()
        {
            
            uint32_t const iMaxNumParents = 16;
            uint32_t const iMaxNumChildren = 16;
            uint32_t iStartTempAddress = 0;
            for(auto const& keyValue : mSkeletalAnimations)
            {
                std::vector<Joint> const& aJoints = keyValue.second.mSkeletalHierarchy.getJoints();
                uint32_t iNumJoints = static_cast<uint32_t>(aJoints.size());

                uint32_t iDataSize = 0;
                iDataSize += static_cast<uint32_t>(sizeof(uint32_t));                                    // miNumJoints
                iDataSize += static_cast<uint32_t>(iNumJoints * sizeof(mat4));                           // maJointAnimationMatrices
                iDataSize += static_cast<uint32_t>(iNumJoints * sizeof(mat4));                           // maJointInverseBindMatrices
                iDataSize += static_cast<uint32_t>(iNumJoints * iMaxNumParents * sizeof(uint32_t));      // maaiJointParents
                iDataSize += static_cast<uint32_t>(iNumJoints * iMaxNumChildren * sizeof(uint32_t));     // maaiJointChildren
                iDataSize += static_cast<uint32_t>(iNumJoints * sizeof(uint32_t));                       // maiNumParents
                iDataSize += static_cast<uint32_t>(iNumJoints * sizeof(uint32_t));                       // maiNumChildren

                char* pStart = macTempStorage.data() + iStartTempAddress;
                char* pData = macTempStorage.data() + iStartTempAddress;
                char* pLast = pData + iDataSize;
                
                uint32_t iAddress = UINT32_MAX;
                

                // number of joints
                uint32_t* pDataInt = reinterpret_cast<uint32_t*>(pData);
                *pDataInt++ = static_cast<uint32_t>(aJoints.size());
                pData = reinterpret_cast<char*>(pDataInt);

                // total bind matrix
                mat4* pDataMatrix = (mat4*)pData;
                for(auto const& joint : aJoints)
                {
                    *pDataMatrix = joint.mTotalBindMatrix;
                    ++pDataMatrix;
                }
                
                // inverse bind matrix
                for(auto const& joint : aJoints)
                {
                    *pDataMatrix = joint.mInverseBindMatrix;
                    ++pDataMatrix;
                }
                pData = reinterpret_cast<char*>(pDataMatrix);

                // joint parent indices
                pDataInt = reinterpret_cast<uint32_t*>(pData);
                for(auto const& joint : aJoints)
                {
                    for(uint32_t i = 0; i < iMaxNumParents; i++)
                    {
                        if(joint.maiParentIndices.size() > i)
                        {
                            *pDataInt = static_cast<uint32_t>(joint.maiParentIndices[i]);
                        }
                        else
                        {
                            *pDataInt = UINT32_MAX;
                        }
                           
                        ++pDataInt;
                    }
                }

                // joint children indices
                for(auto const& joint : aJoints)
                {
                    for(uint32_t i = 0; i < iMaxNumChildren; i++)
                    {
                        if(joint.maiChildrenIndices.size() > i)
                        {
                            *pDataInt = static_cast<uint32_t>(joint.maiChildrenIndices[i]);
                        }
                        else
                        {
                            *pDataInt = UINT32_MAX;
                        }

                        ++pDataInt;
                    }
                }

                // num parents
                for(auto const& joint : aJoints)
                {
                    *pDataInt++ = static_cast<uint32_t>(joint.maiParentIndices.size());
                }

                // num children
                for(auto const& joint : aJoints)
                {
                    *pDataInt++ = static_cast<uint32_t>(joint.maiChildrenIndices.size());
                }
                
                assert(reinterpret_cast<uint64_t>(pDataInt) <= reinterpret_cast<uint64_t>(pLast) + sizeof(uint32_t));
                
                // upload to GPU here

                iStartTempAddress += iDataSize;

            }   // for animation in total animations

        }

        /*
        **
        */
        void CAnimationManager::getAnimationNames(std::vector<std::string>& aAnimationNames)
        {
            for(auto const& keyValue : mSkeletalAnimations)
            {
                aAnimationNames.push_back(keyValue.first);
            }
        }

    }   // Animation

}   // Render