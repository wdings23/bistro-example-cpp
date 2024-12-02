#include <skeletal-animation/SkeletalHierarchy.h>
#include <quaternion.h>
#include <LogPrint.h>

#include <sstream>
#include <wtfassert.h>

namespace Render
{
    namespace Animation
    {
        /*
        **
        */
        void CSkeletalHierarchy::setup(SkeletalHiearchySetupDescriptor const& desc)
        {
            uint32_t iNumJoints = static_cast<uint32_t>(desc.mpaJointRotations->size());
            WTFASSERT(iNumJoints > 0, "no joints");
            maJoints.resize(iNumJoints);
            std::vector<vec4> const& aJointRotations = *(desc.mpaJointRotations);
            std::vector<vec4> const& aJointScalings = *(desc.mpaJointScalings);
            std::vector<vec4> const& aJointTranslations = *(desc.mpaJointTranslations);
            std::vector<mat4> const& aJointInverseBindMatrices = *(desc.mpaJointInverseBindMatrices);
            std::vector<std::vector<uint32_t>> const& aaiJointChildren = *(desc.mpaaiJointChildren);
            std::vector<std::vector<uint32_t>> const& aaiJointParents = *(desc.mpaaiJointParents);
            std::vector<mat4> const& aJointParentNodeMatrices = *(desc.mpaJointParentNodeMatrices);

            for(uint32_t i = 0; i < iNumJoints; i++)
            {
                Joint& joint = maJoints[i];
                joint.miIndex = i;

                if(desc.mpaJointNames->size() > 0)
                {
                    joint.mName = (*desc.mpaJointNames)[i];
                }
                else
                {
                    std::ostringstream oss;
                    oss << "joint" << i;
                    joint.mName = oss.str();
                }
                
                joint.mRotation = aJointRotations[i];
                joint.mScale = aJointScalings[i];
                joint.mTranslation = aJointTranslations[i];
                joint.mInverseBindMatrix = aJointInverseBindMatrices[i];
                joint.maiChildrenIndices = aaiJointChildren[i];
                joint.maiParentIndices = aaiJointParents[i];
                //joint.mParentNodeMatrix = aJointParentNodeMatrices[i];
                //joint.mInverseParentNodeMatrix = invert(joint.mParentNodeMatrix);

                for(uint32_t j = 0; j < static_cast<uint32_t>(joint.maiChildrenIndices.size()); j++)
                {
                    uint32_t iChildIndex = joint.maiChildrenIndices[j];
                    joint.mapChildren.push_back(&maJoints[iChildIndex]);
                }

                for(uint32_t j = 0; j < static_cast<uint32_t>(joint.maiParentIndices.size()); j++)
                {
                    uint32_t iParentIndex = joint.maiParentIndices[j];
                    joint.mapParents.push_back(&maJoints[iParentIndex]);
                }
            }

            maInverseBindMatrices = *(desc.mpaJointInverseBindMatrices);

            computeJointMatrices();
        }

        /*
        **
        */
        void CSkeletalHierarchy::computeJointMatrices()
        {
            uint32_t iCurrJointIndex = 0;
            Joint* pJoint = &maJoints[iCurrJointIndex];
            std::vector<uint32_t> aiJointStack;
            aiJointStack.push_back(pJoint->miIndex);

            for(;;)
            {
                aiJointStack.pop_back();
                if(pJoint->maiChildrenIndices.size() > 0)
                {
                    for(auto const& iChild : pJoint->maiChildrenIndices)
                    {
                        Joint const& pChildJoint = maJoints[iChild];
                        aiJointStack.push_back(iChild);
                    }
                }

                mat4 localTranslationMatrix = translate(pJoint->mTranslation);
                mat4 localScaleMatrix = scale(pJoint->mScale);
                quaternion localRotationQuat(pJoint->mRotation.x, pJoint->mRotation.y, pJoint->mRotation.z, pJoint->mRotation.w);
                mat4 localRotationMatrix = localRotationQuat.matrix();
                pJoint->mLocalBindMatrix = localTranslationMatrix * localRotationMatrix * localScaleMatrix;

                Joint* pParentJoint = nullptr; 
                if(pJoint->mapParents.size() > 0)
                {
                    pParentJoint = pJoint->mapParents[0];
                }

                mat4 parentTotalBindMatrix;
                if(pParentJoint != nullptr)
                {
                    parentTotalBindMatrix = pParentJoint->mTotalBindMatrix;
                }

                pJoint->mTotalBindMatrix = parentTotalBindMatrix * pJoint->mLocalBindMatrix;

#if 0
                DEBUG_PRINTF("%s\nlocal bind matrix\n%.4f\t%.4f\t%.4f\t%.4f\n%.4f\t%.4f\t%.4f\t%.4f\n%.4f\t%.4f\t%.4f\t%.4f\n%.4f\t%.4f\t%.4f\t%.4f\n\n",
                    pJoint->mName.c_str(),
                    pJoint->mLocalBindMatrix.mafEntries[0],     pJoint->mLocalBindMatrix.mafEntries[1],     pJoint->mLocalBindMatrix.mafEntries[2],     pJoint->mLocalBindMatrix.mafEntries[3],
                    pJoint->mLocalBindMatrix.mafEntries[4],     pJoint->mLocalBindMatrix.mafEntries[5],     pJoint->mLocalBindMatrix.mafEntries[6],     pJoint->mLocalBindMatrix.mafEntries[7],
                    pJoint->mLocalBindMatrix.mafEntries[8],     pJoint->mLocalBindMatrix.mafEntries[9],     pJoint->mLocalBindMatrix.mafEntries[10],    pJoint->mLocalBindMatrix.mafEntries[11],
                    pJoint->mLocalBindMatrix.mafEntries[12],    pJoint->mLocalBindMatrix.mafEntries[13],    pJoint->mLocalBindMatrix.mafEntries[14],    pJoint->mLocalBindMatrix.mafEntries[15]);


                DEBUG_PRINTF("%s\ntotal bind matrix\n%.4f\t%.4f\t%.4f\t%.4f\n%.4f\t%.4f\t%.4f\t%.4f\n%.4f\t%.4f\t%.4f\t%.4f\n%.4f\t%.4f\t%.4f\t%.4f\n\n",
                    pJoint->mName.c_str(),
                    pJoint->mTotalBindMatrix.mafEntries[0],  pJoint->mTotalBindMatrix.mafEntries[1],  pJoint->mTotalBindMatrix.mafEntries[2],  pJoint->mTotalBindMatrix.mafEntries[3],
                    pJoint->mTotalBindMatrix.mafEntries[4],  pJoint->mTotalBindMatrix.mafEntries[5],  pJoint->mTotalBindMatrix.mafEntries[6],  pJoint->mTotalBindMatrix.mafEntries[7],
                    pJoint->mTotalBindMatrix.mafEntries[8],  pJoint->mTotalBindMatrix.mafEntries[9],  pJoint->mTotalBindMatrix.mafEntries[10], pJoint->mTotalBindMatrix.mafEntries[11],
                    pJoint->mTotalBindMatrix.mafEntries[12], pJoint->mTotalBindMatrix.mafEntries[13], pJoint->mTotalBindMatrix.mafEntries[14], pJoint->mTotalBindMatrix.mafEntries[15]);
#endif // #if 0

                if(aiJointStack.size() <= 0)
                {
                    break;
                }

                iCurrJointIndex = aiJointStack.back();
                pJoint = &maJoints[iCurrJointIndex];
            }
        }

        /*
        **
        */
        void CSkeletalHierarchy::updateMatrices(std::vector<mat4> const& aMatrices)
        {
            maTransformedMatrices = aMatrices;
        }

        /*
        **
        */
        void CSkeletalHierarchy::getJointParentChildIndices(JointParentChildIndexDescriptor& desc) const
        {
            for(auto const& joint : maJoints)
            {
                std::vector<uint32_t> const& aiParents = joint.maiParentIndices;
                std::vector<uint32_t> const& aiChildren = joint.maiChildrenIndices;

                desc.mpaaChildrenJointIndices->push_back(aiChildren);
                desc.mpaaParentIndices->push_back(aiParents);
            }
        }

        /*
        **
        */
        void CSkeletalHierarchy::getRootJoints(std::vector<Joint const*>& aJoints) const
        {
            for(auto const& joint : maJoints)
            {
                if(joint.mapParents.size() <= 0)
                {
                    aJoints.push_back(&joint);
                }
            }
        }

    }   // Animation

}   // Render