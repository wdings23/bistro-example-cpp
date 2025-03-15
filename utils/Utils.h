#pragma once

#include <string>
#include <vec.h>
#include <vector>
#include <math/vec.h>
#include <math/mat4.h>

#include <functional>
#include <map>


#define MAX_RASTERIZER_RENDER_TARGETS   8
         
#ifdef __APPLE__
#define FLT_MAX __FLT_MAX__
#endif // __APPLE__

namespace Render
{
    namespace Common
    {
        struct Tri
        {
            float3      maPositions[3];
            float3      mCentroid;
            uint32_t    miObject;

            Tri(float3 const& pos0, float3 const& pos1, float3 const& pos2)
            {
                maPositions[0] = pos0;
                maPositions[1] = pos1;
                maPositions[2] = pos2;

                mCentroid = (pos0 + pos1 + pos2) / 3.0f;
            }

            Tri(float4 const& pos0, float4 const& pos1, float4 const& pos2)
            {
                maPositions[0] = float3(pos0.x, pos0.y, pos0.z);
                maPositions[1] = float3(pos1.x, pos1.y, pos1.z);
                maPositions[2] = float3(pos2.x, pos2.y, pos2.z);

                mCentroid = (maPositions[0] + maPositions[1] + maPositions[2]) / 3.0f;
            }
        };

        struct uint2
        {
            uint32_t x;
            uint32_t y;

            uint2(uint32_t iX, uint32_t iY)
            {
                x = iX; y = iY;
            }

            uint2 operator >> (uint32_t iBits)
            {
                uint32_t iShiftedX = x >> iBits;
                uint32_t iShiftedY = y >> iBits;

                return uint2(iShiftedX, iShiftedY);
            }

            void operator ^= (uint2 input)
            {
                x = x ^ input.x;
                y = y ^ input.y;
            }

            void operator *= (uint32_t iVal)
            {
                x = x * iVal;
                y = y * iVal;
            }

        };

        struct HashMapEntry
        {
            uint32_t    miHashKey;
            uint2       mMeshCluster;
            uint32_t    miValue;
        };

        namespace Utils
        {
            void getFilePathBaseName(
                std::string& baseName,
                std::string const& filePath);

            vec3 barycentric(vec3 const& p, vec3 const& a, vec3 const& b, vec3 const& c);
            bool rayBoxIntersect(
                float3 const& rayPosition,
                float3 const& rayDir,
                float3 const& bboxMin,
                float3 const& bboxMax);
            float2 rayBoxIntersect2(
                float3 const& rayOrigin,
                float3 const& rayEndPt,
                float3 const& bboxMin,
                float3 const& bboxMax);
            float rayPlaneIntersection(vec3 const& pt0,
                vec3 const& pt1,
                vec3 const& planeNormal,
                float fPlaneDistance);
            vec3 rayTriangleIntersection(
                float& fRet,
                vec3 const& rayPt0, 
                vec3 const& rayPt1, 
                vec3 const& triPt0, 
                vec3 const& triPt1, 
                vec3 const& triPt2);

            void intersectTri(
                float& fRetT,
                float3 const& pos,
                float3 const& dir,
                Tri const& tri);

            void addToHashMap(
                uint2 const& key,
                uint32_t iValue,
                std::vector<char>& buffer,
                uint32_t iBufferByteSize);

            uint32_t getFromHashMap(
                uint2 const& key,
                std::vector<char>& buffer,
                uint32_t iBufferByteSize);

            bool eraseFromHashMap(
                uint2 const& key,
                std::vector<char>& buffer,
                uint32_t iBufferByteSize);

            bool intersectTriangleAABB(
                float3 const& triPt0,
                float3 const& triPt1,
                float3 const& triPt2,
                float3 const& aabbCenter,
                float3 const& aabb);

            float3 closestPointToTriangle(
                float3 const& pt,
                float3 const& v0,
                float3 const& v1,
                float3 const& v2);

            enum VertexShaderOutputMapping
            {
                WORLD_POSITION = 0,
                NORMAL = 1,
                COLOR = 2,
                CLIP_SPACE_POSITON = 3,
                TEXCOORD0 = 4,
                TEXCOORD1 = 5,
                TEXCOORD2 = 6,
                TEXCOORD3 = 7,
                TEXCOORD4 = 8,
                TEXCOORD5 = 9,
                TEXCOORD6 = 10,
                TEXCOORD7 = 11,

                NUM_VERTEX_SHADER_OUTPUT_MAPPINGS,
            };

            enum MatrixInputMapping
            {
                VIEW = 0,
                PROJECTION,
                PREVIOUS_VIEW,
                PREVIOUS_PROJECTION,
                
                MODEL0,
                MODEL1,
                MODEL2,
                MODEL3,

                NUM_MATRIX_INPUTS
            };
        
            struct VertexShaderInput
            {
                float4      maInput[NUM_VERTEX_SHADER_OUTPUT_MAPPINGS] =
                {
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                };
                float4x4        maInputMatrix[NUM_MATRIX_INPUTS];
                uint32_t        miMeshID;

                std::map<std::string, float4> maUserData;
            };
        
            struct VertexShaderOutput
            {
                float4      maOutput[NUM_VERTEX_SHADER_OUTPUT_MAPPINGS] =
                {
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                    {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX},
                };
            };

            struct FragmentShaderOutput
            {
                float4 maRenderTargets[MAX_RASTERIZER_RENDER_TARGETS];
            };

            struct FragmentShaderInput
            {
                float4 maFragmentShaderInput[NUM_VERTEX_SHADER_OUTPUT_MAPPINGS];
            };

            enum ShaderDataType
            {
                INT32 = 0,
                UINT32,
                FLOAT32,
                VEC_FLOAT_1,
                VEC_FLOAT_2,
                VEC_FLOAT_3,
                VEC_FLOAT_4,

                MAT_FLOAT_3x3,
                MAT_FLOAT_3x4,
                MAT_FLOAT_4x3,
                MAT_FLOAT_4x4,

                TEXTURE_ID,

                NUM_SHADER_DATA_TYPES,
            };

            struct ShaderResourceDataType
            {
                std::vector<ShaderDataType>                  maDataTypes;
            };

            struct ShaderResources
            {
                std::map<std::string, std::pair<uint32_t, uint32_t>>        maShaderResources;
                std::vector<char>                      maShaderResourceBuffer;
                std::vector<ShaderResourceDataType>    maShaderResourceDataTypes;
                std::vector<void*>                     mapTextures;
            };

            struct RasterTriangleDescriptor
            {
                float3          maClipSpacePositions[3];
                float3          maWorldPositions[3];
                float3          maNormals[3];
                float2          maUVs[3];
                float4          maColors[3];

                uint32_t        miMode;
                float4*         maaImageBuffers[MAX_RASTERIZER_RENDER_TARGETS] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
                float*          mafDepthBuffer;
                uint32_t        miImageWidth;
                uint32_t        miImageHeight;


                VertexShaderOutput  maVertexShaderOutput[3];
                std::map<std::string, float4>       maUserDataInputMap;

                ShaderResources* mpShaderResources;

                std::function<void(FragmentShaderOutput&, FragmentShaderInput const&, ::uint2 const&, Render::Common::Utils::ShaderResources&)> mFragmentShaderFunc;
            };

            void rasterTriangle(RasterTriangleDescriptor const& rasterTriangleDesc);

            struct RasterMeshDescriptor
            {
                uint32_t            miMeshID;

                float3 const*       maPositions;
                float3 const*       maNormals;
                float3 const*       maUVs;
                float4 const*       maColors;
                
                uint32_t const*     maiTrianglePositionIndices;
                uint32_t const*     maiTriangleNormalIndices;
                uint32_t const*     maiTriangleUVIndices;
                uint32_t const*     maiTriangleColorIndices;
                
                uint32_t            miNumTriangles;

                //mat4                mViewProjectionMatrix;
                //mat4                mPrevViewProjectionMatrix;

                mat4                mViewMatrix;
                mat4                mProjectionMatrix;

                mat4                mPrevViewMatrix;
                mat4                mPrevProjectionMatrix;

                uint32_t            miMode;
                float4*             maaImageBuffers[MAX_RASTERIZER_RENDER_TARGETS] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
                float*              mafDepthBuffer;
                uint32_t            miImageWidth;
                uint32_t            miImageHeight;

                std::map<std::string, uint32_t>     maVertexInputMap;
                std::map<std::string, uint32_t>     maVertexOutputMap;

                std::function<void(VertexShaderOutput&, VertexShaderInput const&, Render::Common::Utils::ShaderResources&)>                           mVertexShaderFunc;
                std::function<void(FragmentShaderOutput&, FragmentShaderInput const&, ::uint2 const&, Render::Common::Utils::ShaderResources&)>       mFragmentShaderFunc;

                std::map<std::string, float4>       maUserDataInputMap;

                ShaderResources*                    mpShaderResources;
            };
            void rasterMesh(RasterMeshDescriptor& meshDesc);
            void rasterMeshMT(RasterMeshDescriptor& meshDesc);

        }   // Utils

    }   // Common


}   // Render

#ifdef __APPLE__
#undef FLT_MAX
#endif // __APPLE__
