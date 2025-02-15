#pragma once

#include <math/vec.h>

namespace Render
{
    namespace Common
    {
        struct VertexFormat
        {
            vec4        mPosition;
            vec4        mNormal;
            vec4        mTexCoord;
        };

    }   // Common

}   // render
