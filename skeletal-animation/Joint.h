#pragma once

#include <vec.h>
#include <mat4.h>

#include <string>
#include <vector>

namespace Render
{
    namespace Animation
    {
        struct Joint
        {
            uint32_t                miIndex;
            std::string             mName;

            vec4                    mRotation;
            vec4                    mTranslation;
            vec4                    mScale;

            mat4                    mInverseBindMatrix;

            mat4                    mLocalBindMatrix;
            mat4                    mTotalBindMatrix;

            //mat4                    mParentNodeMatrix;
            //mat4                    mInverseParentNodeMatrix;

            std::vector<Joint*>     mapParents;
            std::vector<Joint*>     mapChildren;
            std::vector<uint32_t>   maiParentIndices;
            std::vector<uint32_t>   maiChildrenIndices;
        };

    
    }   // Animation
}   // Render