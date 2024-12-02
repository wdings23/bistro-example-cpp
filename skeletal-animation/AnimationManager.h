#pragma once

#include <skeletal-animation/SkeletalHierarchy.h>
#include <skeletal-animation/SkeletalAnimation.h>
#include <skeletal-animation/Skin.h>

#include <vec.h>

#include <map>
#include <string>
#include <memory>

namespace Render
{
    namespace Animation
    {
        struct SkeletalAnimationInfo
        {
            CSkeletalHierarchy          mSkeletalHierarchy;
            CSkeletalAnimation          mSkeletalAnimation;
        };

        struct AnimationManagerSetupDescriptor
        {
            uint32_t                    miTempStorageSize = 1 << 20;
        };

        struct AddAnimationDescriptor
        {
            std::vector<std::string>*               mpaFilePaths;
            std::vector<SkeletalAnimationInfo>*     mpaSkeletalAnimations;
            std::vector<std::string>*               mpaMeshNames;
            uint32_t                                miMeshIndex;
        };

        struct AnimationUpdateDescriptor
        {
            std::string                                                         mAnimationName;
            std::vector<mat4>*                                                  mpaTransformationMatrices;

            std::vector<std::pair<std::string, vec3>>*                          mpaAdditionalJointTranslations;

            float                                                               mfTime;
        };

        struct SkeletalSkinQueryDescriptor
        {
            std::string                                                         mName;
            Render::Animation::SkeletalSkinningInfo*                            mpSkinInfo;
        };

        struct SkeletalAnimationQueryDescriptor
        {
            std::string                                                         mName;
            Render::Animation::SkeletalAnimationInfo*                           mpAnimationInfo;
        };

        class CAnimationManager
        {
        public:
            CAnimationManager() = default;
            virtual ~CAnimationManager() = default;

            void setup(AnimationManagerSetupDescriptor const& desc);
            void addAnimations(AddAnimationDescriptor const& desc);

            void getMatricesAtTime(AnimationUpdateDescriptor const& desc);
            void getSkeletalSkinInfo(SkeletalSkinQueryDescriptor& desc);
            void getSkeletalAnimations(SkeletalAnimationQueryDescriptor& desc);

            void uploadHiearchyDataToGPU();
            void getAnimationNames(std::vector<std::string>& aAnimationNames);

        public:
            static std::unique_ptr<CAnimationManager> spInstance;
            static std::unique_ptr<CAnimationManager>& instance();

        protected:
            std::map<std::string, SkeletalAnimationInfo>                        mSkeletalAnimations;
            std::map<std::string, Render::Animation::SkeletalSkinningInfo>      mSkeletalSkins;

            std::vector<char> macTempStorage;

        };

    }   // Animation

}   // Render
