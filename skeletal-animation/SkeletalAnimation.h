#pragma once

#include <vec.h>
#include <mat4.h>

#include <map>
#include <string>
#include <vector>

namespace Render
{
    namespace Animation
    {
        enum class StepType
        {
            LINEAR = 0
        };

        struct Joint;
        struct AnimationAtTimeDescriptor
        {
            std::string             mName;
            float                   mfTime;
            vec4*                   mpaRotation;
            vec4*                   mpaTranslation;
            vec4*                   mpaScaling;
            Joint const*            mpaJoints;

        };

        struct AnimationInfo
        {
            std::vector<std::vector<float>>                         maafRotationTimes;
            std::vector<std::vector<float>>                         maafTranslationTimes;
            std::vector<std::vector<float>>                         maafScaleTimes;

            std::vector<std::vector<vec4>>                          maaRotations;
            std::vector<std::vector<vec4>>                          maaTranslations;
            std::vector<std::vector<vec4>>                          maaScalings;

            StepType                                                mStepType;

            float                                                   mfLastTime;
        };

        struct Joint;
        class CSkeletalHierarchy;
        struct ApplyAnimationDescriptor
        {
            std::string                                             mName;
            CSkeletalHierarchy const*                               mpSkeletalHiearachy;
            std::vector<mat4>*                                      mpaTransformedMatrices;
            std::vector<std::pair<uint32_t, vec3>>*                 mpaAdditionalJointTranslations;

            float                                                   mfTime;
        };

        class CSkeletalAnimation
        {
        public:
            CSkeletalAnimation() = default;
            virtual ~CSkeletalAnimation() = default;

            void addAnimation(
                std::string const& animationName,
                std::vector<std::vector<vec4>> const& aaRotations,
                std::vector<std::vector<vec4>> const& aaTranslations,
                std::vector<std::vector<vec4>> const& aaScalings,
                std::vector<std::vector<float>> const& aafRotationTimes,
                std::vector<std::vector<float>> const& aafTranslationTimes,
                std::vector<std::vector<float>> const& aafScaleTimes,
                StepType const& stepType);

            void getAnimationKeyFrameAtTime(AnimationAtTimeDescriptor& desc);
            void applyAnimation(ApplyAnimationDescriptor& desc);
            float getLastAnimationTime(std::string const& name);

            void getAnimationNames(std::vector<std::string>& aAnimationNames);

        protected:
            std::map<std::string, AnimationInfo>        mAnimationInfo;

        protected:
            static void applyAnimation(
                uint32_t iJointIndex,
                mat4* aJointMatrices,
                mat4 const* aInverseBindMatrices,
                vec4 const* paRotations,
                vec4 const* paTranslations,
                vec4 const* paScalings,
                Joint const* paJoints,
                uint32_t iNumJoints);

        };

    }   // Animation

}   // Render