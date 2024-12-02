#pragma once

#include <stdint.h>
#include <string>

#define PLATFORM_OBJECT_HANDLE	uint64_t

namespace RenderDriver
{
    namespace Common
    {
        class CObject
        {
        public:
            CObject() = default;
            virtual ~CObject() = default;

            inline PLATFORM_OBJECT_HANDLE getHandle() { return mHandle; }

            virtual inline void setID(std::string const& id) { mID = id; }
            inline std::string const& getID() { return mID; }

        protected:
            std::string                     mID;
            
            PLATFORM_OBJECT_HANDLE          mHandle;
            
        };

    }   // Common

}   // RenderDriver