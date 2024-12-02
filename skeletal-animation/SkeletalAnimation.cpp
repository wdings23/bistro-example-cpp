#include <skeletal-animation/SkeletalAnimation.h>
#include <skeletal-animation/SkeletalHierarchy.h>
#include <skeletal-animation/Joint.h>
#include <quaternion.h>
#include <LogPrint.h>

#include <assert.h>
#include "wtfassert.h"

namespace Render
{

    namespace Animation
    {
        /*
        **
        */
        void CSkeletalAnimation::addAnimation(
            std::string const& animationName,
            std::vector<std::vector<vec4>> const& aaRotations,
            std::vector<std::vector<vec4>> const& aaTranslations,
            std::vector<std::vector<vec4>> const& aaScalings,
            std::vector<std::vector<float>> const& aafRotationTimes,
            std::vector<std::vector<float>> const& aafTranslationTimes,
            std::vector<std::vector<float>> const& aafScaleTimes,
            StepType const& stepType)
        {
            AnimationInfo animation = {};

            animation.maafRotationTimes = aafRotationTimes;
            animation.maafTranslationTimes = aafTranslationTimes;
            animation.maafScaleTimes = aafScaleTimes;

            animation.maaRotations = aaRotations;
            animation.maaTranslations = aaTranslations;
            animation.maaScalings = aaScalings;
            animation.mStepType = stepType;

            mAnimationInfo[animationName] = animation;
            
            float fLastTime = 0.0f;
            uint32_t iNumJoints = static_cast<uint32_t>(animation.maafRotationTimes.size());
            for(uint32_t i = 0; i < iNumJoints; i++)
            {
                uint32_t iNumKeyFrames = static_cast<uint32_t>(animation.maafRotationTimes[i].size());
                for(uint32_t j = 0; j < iNumKeyFrames; j++)
                {
                    if(animation.maafRotationTimes[i][j] > fLastTime)
                    {
                        fLastTime = animation.maafRotationTimes[i][j];
                    }
                }
            }

            iNumJoints = static_cast<uint32_t>(animation.maafTranslationTimes.size());
            for(uint32_t i = 0; i < iNumJoints; i++)
            {
                uint32_t iNumKeyFrames = static_cast<uint32_t>(animation.maafTranslationTimes[i].size());
                for(uint32_t j = 0; j < iNumKeyFrames; j++)
                {
                    if(animation.maafTranslationTimes[i][j] > fLastTime)
                    {
                        fLastTime = animation.maafTranslationTimes[i][j];
                    }
                }
            }

            iNumJoints = static_cast<uint32_t>(animation.maafScaleTimes.size());
            for(uint32_t i = 0; i < iNumJoints; i++)
            {
                uint32_t iNumKeyFrames = static_cast<uint32_t>(animation.maafScaleTimes[i].size());
                for(uint32_t j = 0; j < iNumKeyFrames; j++)
                {
                    if(animation.maafScaleTimes[i][j] > fLastTime)
                    {
                        fLastTime = animation.maafScaleTimes[i][j];
                    }
                }
            }

            mAnimationInfo[animationName].mfLastTime = fLastTime;

DEBUG_PRINTF("register animation: %s\n", animationName.c_str());

        }

        /*
        **
        */
        void CSkeletalAnimation::getAnimationKeyFrameAtTime(AnimationAtTimeDescriptor& desc)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CSkeletalAnimation::applyAnimation(ApplyAnimationDescriptor& desc)
        {
            AnimationAtTimeDescriptor animationAtTimeDesc = {};
            animationAtTimeDesc.mName = desc.mName;
            animationAtTimeDesc.mfTime = desc.mfTime;

            uint32_t iNumJoints = static_cast<uint32_t>(desc.mpSkeletalHiearachy->getJoints().size());
            std::vector<vec4> aRotations(iNumJoints);
            std::vector<vec4> aTranslations(iNumJoints);
            std::vector<vec4> aScalings(iNumJoints);
            animationAtTimeDesc.mpaRotation = aRotations.data();
            animationAtTimeDesc.mpaTranslation = aTranslations.data();
            animationAtTimeDesc.mpaScaling = aScalings.data();
            animationAtTimeDesc.mpaJoints = desc.mpSkeletalHiearachy->getJoints().data();
            for(uint32_t i = 0; i < iNumJoints; i++)
            {
                aScalings[i] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
            }

            // compute the transforms
            getAnimationKeyFrameAtTime(animationAtTimeDesc);

            // additional translations to the specified joint
            if(desc.mpaAdditionalJointTranslations)
            {
                std::vector<std::pair<uint32_t,vec3>> const& aAdditionalJointTranslations = *desc.mpaAdditionalJointTranslations;
                for(auto const& additionalTranslation : aAdditionalJointTranslations)
                {
                    aTranslations[additionalTranslation.first].x += additionalTranslation.second.x;
                    aTranslations[additionalTranslation.first].y += additionalTranslation.second.y;
                    aTranslations[additionalTranslation.first].z += additionalTranslation.second.z;
                }
            }

            std::vector<std::vector<uint32_t>>      aaParentIndices;
            std::vector<std::vector<uint32_t>>      aaChildrenIndices;
            JointParentChildIndexDescriptor        jointParentChildIndexDesc =
            {
                /*.mpaaParentIndices            */  &aaParentIndices,
                /*.mpaaChildrenJointIndices     */  &aaChildrenIndices,
            };

            desc.mpSkeletalHiearachy->getJointParentChildIndices(jointParentChildIndexDesc);
            std::vector<mat4> const& aInverseBindMatrices = desc.mpSkeletalHiearachy->getInverseBindMatrices();

            
            std::vector<Joint const*> apRootJoints;
            desc.mpSkeletalHiearachy->getRootJoints(apRootJoints);

            // apply animation to the joint matrices
            auto const& aJoints = desc.mpSkeletalHiearachy->getJoints();
            std::vector<mat4>& aJointMatrices = *desc.mpaTransformedMatrices;
            for(auto const pRootJoint : apRootJoints)
            {
                CSkeletalAnimation::applyAnimation(
                    pRootJoint->miIndex,
                    aJointMatrices.data(),
                    aInverseBindMatrices.data(),
                    aRotations.data(),
                    aTranslations.data(),
                    aScalings.data(),
                    aJoints.data(),
                    static_cast<uint32_t>(aJoints.size()));
            }
        }

        /*
        **
        */
        void CSkeletalAnimation::applyAnimation(
            uint32_t iJointIndex,
            mat4* aJointMatrices,
            mat4 const* aInverseBindJointMatrices,
            vec4 const* paRotations,
            vec4 const* paTranslations,
            vec4 const* paScalings,
            Joint const* paJoints,
            uint32_t iNumJoints)
        {
            std::vector<mat4> aTotalMatrices(iNumJoints);

            uint32_t iCurrJointIndex = iJointIndex;
            Joint const* pJoint = &paJoints[iCurrJointIndex];
            std::vector<uint32_t> aiJointStack;
            aiJointStack.push_back(pJoint->miIndex);
            
            uint32_t iCount = 0;
            for(;;)
            {
                uint32_t iParentIndex = UINT32_MAX;

                if(pJoint->mapParents.size() > 0)
                {
                    iParentIndex = pJoint->mapParents[0]->miIndex;
                }

                mat4 identityMatrix;
                mat4* pParentMatrix = &identityMatrix;
                if(iParentIndex != UINT32_MAX)
                {
                    pParentMatrix = &aTotalMatrices[iParentIndex];
                }

                mat4 const& inverseBindJointMatrix = aInverseBindJointMatrices[iCurrJointIndex];

                // animation matrices (has inital bind values pre-concatenated)
                quaternion animRotationQuat(paRotations[iCurrJointIndex].x, paRotations[iCurrJointIndex].y, paRotations[iCurrJointIndex].z, paRotations[iCurrJointIndex].w);
                    
                mat4 animRotationMatrix = animRotationQuat.matrix();
                mat4 animTranslationMatrix = translate(paTranslations[iCurrJointIndex]);
                mat4 animScaleMatrix = scale(paScalings[iCurrJointIndex]);
                
                mat4 animationMatrix = animTranslationMatrix * animRotationMatrix * animScaleMatrix;
                aTotalMatrices[iCurrJointIndex] = *pParentMatrix * animationMatrix;

                aJointMatrices[iCurrJointIndex] = /*pJoint->mInverseParentNodeMatrix * */aTotalMatrices[iCurrJointIndex] * inverseBindJointMatrix;

                aiJointStack.pop_back();
                if(pJoint->maiChildrenIndices.size() > 0)
                {
                    for(auto const& iChild : pJoint->maiChildrenIndices)
                    {
                        Joint const& pChildJoint = paJoints[iChild];
                        aiJointStack.push_back(iChild);
                    }
                }
                
                if(aiJointStack.size() <= 0)
                {
                    break;
                }

                iCurrJointIndex = aiJointStack.back();
                pJoint = &paJoints[iCurrJointIndex];
            }
        }

        /*
        **
        */
        float CSkeletalAnimation::getLastAnimationTime(std::string const& name)
        {
            assert(mAnimationInfo.find(name) != mAnimationInfo.end());
            return mAnimationInfo[name].mfLastTime;
        }

    }  // Animation

}   // Render