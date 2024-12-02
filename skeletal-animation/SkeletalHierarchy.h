#pragma once

#include <vec.h>
#include <mat4.h>

#include <skeletal-animation/Joint.h>

#include <vector>

namespace Render
{
    namespace Animation
    {

        struct JointParentChildIndexDescriptor
        {
            std::vector<std::vector<uint32_t>>*         mpaaParentIndices;
            std::vector<std::vector<uint32_t>>*         mpaaChildrenJointIndices;
        };

        struct SkeletalHiearchySetupDescriptor
        {

            std::vector<vec4>*                    mpaJointRotations;
            std::vector<vec4>*                    mpaJointTranslations;
            std::vector<vec4>*                    mpaJointScalings;
            std::vector<mat4>*                    mpaJointInverseBindMatrices;
            std::vector<std::vector<uint32_t>>*   mpaaiJointChildren;
            std::vector<std::vector<uint32_t>>*   mpaaiJointParents;
            std::vector<mat4>*                    mpaJointParentNodeMatrices;
            std::vector<std::string>*             mpaJointNames;
        };

        class CSkeletalHierarchy
        {
        public:
            CSkeletalHierarchy() = default;
            virtual ~CSkeletalHierarchy() = default;

            void setup(SkeletalHiearchySetupDescriptor const& desc);

            void updateMatrices(std::vector<mat4> const& aMatrices);

            void getJointParentChildIndices(JointParentChildIndexDescriptor& desc) const;

            inline std::vector<Joint> const& getJoints() const
            {
                return maJoints;
            }

            void getRootJoints(std::vector<Joint const*>& aJoints) const;

            inline std::vector<mat4> const& getInverseBindMatrices() const
            {
                return maInverseBindMatrices;
            }

            void computeJointMatrices();

        protected:
            std::vector<Joint>          maJoints;
            std::vector<mat4>           maTransformedMatrices;
            std::vector<mat4>           maInverseBindMatrices;
        };

    }   // Animation

}   // Render