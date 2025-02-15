#include <utils/Utils.h>

#include <mutex>
#include <atomic>
#include <thread>

#include "LogPrint.h"
#include "wtfassert.h"

#ifdef __APPLE__
#define FLT_MAX __FLT_MAX__
#endif // __APPLE__

namespace Render
{
    namespace Common
    {
        namespace Utils
        {
            /*
            **
            */
            void getFilePathBaseName(
                std::string& baseName,
                std::string const& filePath)
            {
                // get the file path base name
                auto fileExtensionStart = filePath.find_last_of(".");
                auto baseNameStart0 = filePath.find_last_of("/");
                auto baseNameStart1 = filePath.find_last_of("\\");

                if(baseNameStart0 == std::string::npos)
                {
                    baseNameStart1 += 1;
                    baseName = filePath.substr(baseNameStart1, fileExtensionStart - baseNameStart1);
                }
                else if(baseNameStart1 == std::string::npos)
                {
                    baseNameStart0 += 1;
                    baseName = filePath.substr(baseNameStart0, fileExtensionStart - baseNameStart0);
                }
                else
                {
                    if(baseNameStart0 > baseNameStart1)
                    {
                        baseNameStart0 += 1;
                        baseName = filePath.substr(baseNameStart0, fileExtensionStart - baseNameStart0);
                    }
                    else
                    {
                        baseNameStart1 += 1;
                        baseName = filePath.substr(baseNameStart1, fileExtensionStart - baseNameStart0);
                    }
                }
            }
            
            /*
            **
            */
            vec3 barycentric(vec3 const& p, vec3 const& a, vec3 const& b, vec3 const& c)
            {
                vec3 v0 = b - a, v1 = c - a, v2 = p - a;
                float fD00 = dot(v0, v0);
                float fD01 = dot(v0, v1);
                float fD11 = dot(v1, v1);
                float fD20 = dot(v2, v0);
                float fD21 = dot(v2, v1);
                float fDenom = fD00 * fD11 - fD01 * fD01;
                float fV = (fD11 * fD20 - fD01 * fD21) / fDenom;
                float fW = (fD00 * fD21 - fD01 * fD20) / fDenom;
                float fU = 1.0f - fV - fW;

                return vec3(fU, fV, fW);
            }

            /*
            **
            */
            bool rayBoxIntersect(
                float3 const& rayOrigin,
                float3 const& rayEndPt,
                float3 const& bboxMin,
                float3 const& bboxMax)
            {
                float fOneOverRayX = 1.0f / rayEndPt.x;
                float fOneOverRayY = 1.0f / rayEndPt.y;
                float fOneOverRayZ = 1.0f / rayEndPt.z;

                float fTX1 = (bboxMin.x - rayOrigin.x) * fOneOverRayX;
                float fTX2 = (bboxMax.x - rayOrigin.x) * fOneOverRayX;

                float fTMin = minf(fTX1, fTX2);
                float fTMax = maxf(fTX1, fTX2);

                float fTY1 = (bboxMin.y - rayOrigin.y) * fOneOverRayY;
                float fTY2 = (bboxMax.y - rayOrigin.y) * fOneOverRayY;

                fTMin = maxf(fTMin, minf(fTY1, fTY2));
                fTMax = minf(fTMax, maxf(fTY1, fTY2));

                float fTZ1 = (bboxMin.z - rayOrigin.z) * fOneOverRayZ;
                float fTZ2 = (bboxMax.z - rayOrigin.z) * fOneOverRayZ;

                fTMin = maxf(fTMin, minf(fTZ1, fTZ2));
                fTMax = minf(fTMax, maxf(fTZ1, fTZ2));

                return fTMax >= fTMin;
            }

            /*
            **
            */
            float2 rayBoxIntersect2(
                float3 const& rayOrigin,
                float3 const& rayEndPt,
                float3 const& bboxMin,
                float3 const& bboxMax)
            {
                float fOneOverRayX = 1.0f / rayEndPt.x;
                float fOneOverRayY = 1.0f / rayEndPt.y;
                float fOneOverRayZ = 1.0f / rayEndPt.z;

                float fTX1 = (bboxMin.x - rayOrigin.x) * fOneOverRayX;
                float fTX2 = (bboxMax.x - rayOrigin.x) * fOneOverRayX;

                float fTMin = minf(fTX1, fTX2);
                float fTMax = maxf(fTX1, fTX2);

                float fTY1 = (bboxMin.y - rayOrigin.y) * fOneOverRayY;
                float fTY2 = (bboxMax.y - rayOrigin.y) * fOneOverRayY;

                fTMin = maxf(fTMin, minf(fTY1, fTY2));
                fTMax = minf(fTMax, maxf(fTY1, fTY2));

                float fTZ1 = (bboxMin.z - rayOrigin.z) * fOneOverRayZ;
                float fTZ2 = (bboxMax.z - rayOrigin.z) * fOneOverRayZ;

                fTMin = maxf(fTMin, minf(fTZ1, fTZ2));
                fTMax = minf(fTMax, maxf(fTZ1, fTZ2));

                return float2(fTMin, fTMax);
            }

            /*
            **
            */
            float rayPlaneIntersection(vec3 const& pt0,
                vec3 const& pt1,
                vec3 const& planeNormal,
                float fPlaneDistance)
            {
                float fRet = FLT_MAX;
                vec3 v = pt1 - pt0;

                float fDenom = dot(v, planeNormal);
                if(fabs(fDenom) > 0.00001f)
                {
                    fRet = -(dot(pt0, planeNormal) + fPlaneDistance) / fDenom;
                }

                return fRet;
            }

            /*
            **
            */
            void intersectTri(
                float& fRetT,
                float3 const& pos,
                float3 const& dir,
                Tri const& tri)
            {
                const float3 edge1 = tri.maPositions[1] - tri.maPositions[0];
                const float3 edge2 = tri.maPositions[2] - tri.maPositions[0];
                const float3 h = cross(dir, edge2);
                const float a = dot(edge1, h);
                if(a > -0.0001f && a < 0.0001f)
                    return; // ray parallel to triangle
                const float f = 1 / a;
                const float3 s = pos - tri.maPositions[0];
                const float u = f * dot(s, h);
                if(u < 0 || u > 1) return;
                const float3 q = cross(s, edge1);
                const float v = f * dot(dir, q);
                if(v < 0 || u + v > 1) return;
                const float t = f * dot(edge2, q);
                if(t > 0.0001f)
                    fRetT = std::min(fRetT, t);
            }

            /*
            **
            */
            vec3 rayTriangleIntersection(
                float& fRetT,
                vec3 const& rayPt0, 
                vec3 const& rayPt1, 
                vec3 const& triPt0, 
                vec3 const& triPt1, 
                vec3 const& triPt2)
            {
                vec3 v0 = normalize(triPt1 - triPt0);
                vec3 v1 = normalize(triPt2 - triPt0);
                vec3 cp = cross(v0, v1);
                float fLength = length(cp);
                if(fLength <= 0.0001f)
                {
                    return vec3(FLT_MAX, FLT_MAX, FLT_MAX);
                }

                vec3 triNormal = normalize(cp);
                float fPlaneDistance = -dot(triPt0, triNormal);

                vec3 collisionPtOnTriangle(FLT_MAX, FLT_MAX, FLT_MAX);

                bool bRet = false;
                float fT = rayPlaneIntersection(rayPt0, rayPt1, triNormal, fPlaneDistance);
                if(fT >= 0.0f && fT <= 1.0f)
                {
                    vec3 collision = rayPt0 + (rayPt1 - rayPt0) * fT;

                    vec3 baryCentricCoord = barycentric(collision, triPt0, triPt1, triPt2);
                    bRet = (baryCentricCoord.x >= -0.01f && baryCentricCoord.x <= 1.01f &&
                        baryCentricCoord.y >= -0.01f && baryCentricCoord.y <= 1.01f &&
                        baryCentricCoord.z >= -0.01f && baryCentricCoord.z <= 1.01f);

                    if(bRet)
                    {
                        collisionPtOnTriangle = triPt0 * baryCentricCoord.x + triPt1 * baryCentricCoord.y + triPt2 * baryCentricCoord.z;
                    }

                    fRetT = fT;
                }

                return collisionPtOnTriangle;
            }

            /*
            **
            */
            uint32_t murmurHash12(
                uint2 const& src)
            {
                uint2 srcCopy = src;
                const uint32_t M = 0x5bd1e995u;
                uint32_t h = 1190494759u;
                srcCopy *= M; srcCopy ^= srcCopy >> 24u; srcCopy *= M;
                h *= M; h ^= srcCopy.x; h *= M; h ^= srcCopy.y;
                h ^= h >> 13u; h *= M; h ^= h >> 15u;
                return h;
            }

            

            /*
            **
            */
            uint32_t hash12(
                uint2 src,
                uint32_t iNumSlots)
            {
                return murmurHash12(src) & (iNumSlots - 1);
            }

            static std::mutex sHashMapMutex;

            /*
            **
            */
            void addToHashMap(
                uint2 const& key,
                uint32_t iValue,
                std::vector<char>& buffer,
                uint32_t iBufferByteSize)
            {
                uint32_t iNumSlots = iBufferByteSize / sizeof(HashMapEntry);
                uint32_t iHashKey = hash12(key, iNumSlots);
                uint32_t iSlot = iHashKey;
                bool bFound = false;
                for(uint32_t i = 0; i < iNumSlots; i++)
                {
                    if(iSlot >= iNumSlots)
                    {
                        break;
                    }

                    HashMapEntry* aDataHashMap = reinterpret_cast<HashMapEntry*>(buffer.data());
                    HashMapEntry& dataHashMap = aDataHashMap[iSlot];

                    if(dataHashMap.miHashKey == 0 || (dataHashMap.miHashKey == iHashKey && (dataHashMap.mMeshCluster.x == key.x && dataHashMap.mMeshCluster.y == key.y)))
                    {
#if 0
                        if(dataHashMap.miHashKey == 0)
                        {
                            DEBUG_PRINTF("Add value %d to slot %d at address %d with hash key %d orig key (%d, %d)\n",
                                iValue,
                                iSlot,
                                iAddress,
                                iHashKey,
                                key.x,
                                key.y);
                        }
                        else
                        {
                            DEBUG_PRINTF("Update value %d to slot %d at address %d with hash key %d orig key (%d, %d)\n",
                                iValue,
                                iSlot,
                                iAddress,
                                iHashKey,
                                key.x,
                                key.y);
                        }
#endif // #if 0

                        {
                            std::lock_guard<std::mutex> lock(sHashMapMutex);
                            dataHashMap.miHashKey = iSlot;
                            dataHashMap.mMeshCluster.x = key.x;
                            dataHashMap.mMeshCluster.y = key.y;
                            dataHashMap.miValue = iValue;
                        }

                        bFound = true;
                        break;
                    }
                    

                    iSlot = (iSlot + 1) & (iNumSlots - 1);
                }

                if(!bFound)
                {
                    DEBUG_PRINTF("didn\'t add (%d, %d)\n", key.x, key.y);
                }
            }

            /*
            **
            */
            uint32_t getFromHashMap(
                uint2 const& key,
                std::vector<char>& buffer,
                uint32_t iBufferByteSize)
            {
                HashMapEntry* aDataHashMap = reinterpret_cast<HashMapEntry*>(buffer.data());
                uint32_t iNumSlots = iBufferByteSize / sizeof(HashMapEntry);
                uint32_t iHashKey = hash12(key, iNumSlots);
                uint32_t iSlot = iHashKey;
                for(uint32_t i = 0; i < iNumSlots; i++)
                {
                    if(iSlot >= iNumSlots)
                    {
                        break;
                    }

                    if(aDataHashMap[iSlot].mMeshCluster.x == key.x &&
                         aDataHashMap[iSlot].mMeshCluster.y == key.y)
                    {
                        uint32_t iValue = aDataHashMap[iSlot].miValue;
                        return iValue;
                    }
                    if(aDataHashMap[iSlot].miValue == 0x0)
                    {
                        return UINT32_MAX;
                    }

                    iSlot = (iSlot + 1) & (iNumSlots - 1);
                }
                
                return UINT32_MAX;
            }

            /*
            **
            */
            bool eraseFromHashMap(
                uint2 const& key,
                std::vector<char>& buffer,
                uint32_t iBufferByteSize)
            {
                HashMapEntry* aDataHashMap = reinterpret_cast<HashMapEntry*>(buffer.data());
                uint32_t iNumSlots = iBufferByteSize / sizeof(HashMapEntry);
                uint32_t iHashKey = hash12(key, iNumSlots);
                uint32_t iSlot = iHashKey;
                for(uint32_t i = 0; i < iNumSlots; i++)
                {
                    if(iSlot >= iNumSlots)
                    {
                        break;
                    }

                    if(aDataHashMap[iSlot].mMeshCluster.x == key.x &&
                        aDataHashMap[iSlot].mMeshCluster.y == key.y)
                    {
                        {
                            std::lock_guard<std::mutex> lock(sHashMapMutex);

                            aDataHashMap[iSlot].mMeshCluster.x = 0;
                            aDataHashMap[iSlot].mMeshCluster.y = 0;
                            aDataHashMap[iSlot].miHashKey = 0;
                            aDataHashMap[iSlot].miValue = 0;
                        }

                        return true;
                    }
                    
                    iSlot = (iSlot + 1) & (iNumSlots - 1);
                }

                return false;
            }

            /*
            **
            */
            bool intersectTriangleAABBAxis(
                float3 const& v0,
                float3 const& v1,
                float3 const& v2,
                float3 const& aabbExtent,
                float3 const& axis)
            {
                float fP0 = dot(v0, axis);
                float fP1 = dot(v1, axis);
                float fP2 = dot(v2, axis);

                float fRadius =
                    aabbExtent.x * fabsf(axis.x) +
                    aabbExtent.y * fabsf(axis.y) +
                    aabbExtent.z * fabsf(axis.z);

                float fMaxP = fmaxf(fP0, fmaxf(fP1, fP2));
                float fMinP = fminf(fP0, fminf(fP1, fP2));

                return !(fmaxf(-fMaxP, fMinP) > fRadius);
            }

            /*
            **
            */
            bool intersectTriangleAABB(
                float3 const& triPt0,
                float3 const& triPt1,
                float3 const& triPt2,
                float3 const& aabbCenter,
                float3 const& aabb)
            {
                float3 pt0 = triPt0 - aabbCenter;
                float3 pt1 = triPt1 - aabbCenter;
                float3 pt2 = triPt2 - aabbCenter;

                float3 ab = normalize(pt1 - pt0);
                float3 bc = normalize(pt2 - pt1);
                float3 ca = normalize(pt0 - pt2);

                float3 a00 = float3(0.0f, -ab.z, ab.y);
                float3 a01 = float3(0.0f, -bc.z, bc.y);
                float3 a02 = float3(0.0f, -ca.z, ca.y);

                float3 a10 = float3(ab.z, 0.0f, -ab.x);
                float3 a11 = float3(bc.z, 0.0f, -bc.x);
                float3 a12 = float3(ca.z, 0.0f, -ca.x);

                float3 a20 = float3(-ab.y, ab.x, 0.0f);
                float3 a21 = float3(-bc.y, bc.x, 0.0f);
                float3 a22 = float3(-ca.y, ca.x, 0.0f);

                bool bRet =
                    (intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a00) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a01) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a02) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a10) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a11) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a12) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a20) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a21) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, a22) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, float3(1.0f, 0.0f, 0.0f)) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, float3(0.0f, 1.0f, 0.0f)) &&
                     intersectTriangleAABBAxis(pt0, pt1, pt2, aabb, float3(0.0f, 0.0f, 1.0f)));

                return bRet;
            }

            /*
            ** https://stackoverflow.com/questions/2924795/fastest-way-to-compute-point-to-triangle-distance-in-3d
            */
            float3 closestPointToTriangle(
                float3 const& pt,
                float3 const& v0,
                float3 const& v1,
                float3 const& v2)
            {
                const float3 v10 = v1 - v0;
                const float3 v20 = v2 - v0;
                const float3 vp0 = pt - v0;

                const float d1 = dot(v10, vp0);
                const float d2 = dot(v20, vp0);
                if(d1 <= 0.f && d2 <= 0.f) return v0; //#1

                const float3 bp = pt - v1;
                const float d3 = dot(v10, bp);
                const float d4 = dot(v20, bp);
                if(d3 >= 0.f && d4 <= d3) return v1; //#2

                const float3 cp = pt - v2;
                const float d5 = dot(v10, cp);
                const float d6 = dot(v20, cp);
                if(d6 >= 0.f && d5 <= d6) return v2; //#3

                const float vc = d1 * d4 - d3 * d2;
                if(vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
                {
                    const float v = d1 / (d1 - d3);
                    return v0 + v10 * v; //#4
                }

                const float vb = d5 * d2 - d1 * d6;
                if(vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
                {
                    const float v = d2 / (d2 - d6);
                    return v0 + v20 * v; //#5
                }

                const float va = d3 * d6 - d5 * d4;
                if(va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
                {
                    const float v = (d4 - d3) / ((d4 - d3) + (d5 - d6));
                    return v1 + (v2 - v1) * v; //#6
                }

                const float denom = 1.f / (va + vb + vc);
                const float v = vb * denom;
                const float w = vc * denom;
                return v0 + v10 * v + v20 * w; //#0
            }

            /*
            **
            */
            static std::mutex sRasterTriangleMutex;
            void rasterTriangle(RasterTriangleDescriptor const& rasterTriangleDesc)
            {
                float const fBarycentricEpsilon = 1.0e-7f;

                // clipspace position, world position, normal, uv, and colors for the triangle

                float4 const& pos0 = rasterTriangleDesc.maVertexShaderOutput[0].maOutput[Render::Common::Utils::VertexShaderOutputMapping::CLIP_SPACE_POSITON];
                float4 const& pos1 = rasterTriangleDesc.maVertexShaderOutput[1].maOutput[Render::Common::Utils::VertexShaderOutputMapping::CLIP_SPACE_POSITON];
                float4 const& pos2 = rasterTriangleDesc.maVertexShaderOutput[2].maOutput[Render::Common::Utils::VertexShaderOutputMapping::CLIP_SPACE_POSITON];

                float4 const& worldPos0 = rasterTriangleDesc.maVertexShaderOutput[0].maOutput[Render::Common::Utils::VertexShaderOutputMapping::WORLD_POSITION];
                float4 const& worldPos1 = rasterTriangleDesc.maVertexShaderOutput[1].maOutput[Render::Common::Utils::VertexShaderOutputMapping::WORLD_POSITION];
                float4 const& worldPos2 = rasterTriangleDesc.maVertexShaderOutput[2].maOutput[Render::Common::Utils::VertexShaderOutputMapping::WORLD_POSITION];

                float4 const& normal0 = rasterTriangleDesc.maVertexShaderOutput[0].maOutput[Render::Common::Utils::VertexShaderOutputMapping::NORMAL];
                float4 const& normal1 = rasterTriangleDesc.maVertexShaderOutput[1].maOutput[Render::Common::Utils::VertexShaderOutputMapping::NORMAL];
                float4 const& normal2 = rasterTriangleDesc.maVertexShaderOutput[2].maOutput[Render::Common::Utils::VertexShaderOutputMapping::NORMAL];

                float4 const& uv0 = rasterTriangleDesc.maVertexShaderOutput[0].maOutput[Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD0];
                float4 const& uv1 = rasterTriangleDesc.maVertexShaderOutput[1].maOutput[Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD0];
                float4 const& uv2 = rasterTriangleDesc.maVertexShaderOutput[2].maOutput[Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD0];

                float4 const& color0 = rasterTriangleDesc.maVertexShaderOutput[0].maOutput[Render::Common::Utils::VertexShaderOutputMapping::COLOR];
                float4 const& color1 = rasterTriangleDesc.maVertexShaderOutput[1].maOutput[Render::Common::Utils::VertexShaderOutputMapping::COLOR];
                float4 const& color2 = rasterTriangleDesc.maVertexShaderOutput[2].maOutput[Render::Common::Utils::VertexShaderOutputMapping::COLOR];

                float const fImageWidth = static_cast<float>(rasterTriangleDesc.miImageWidth);
                float const fImageHeight = static_cast<float>(rasterTriangleDesc.miImageHeight);

                // screen space triangle coordinate
                float3 pt0 = float3(
                    (pos0.x * 0.5f + 0.5f) * fImageWidth,
                    (1.0f - (pos0.y * 0.5f + 0.5f)) * fImageHeight,
                    0.0f);

                float3 pt1 = float3(
                    (pos1.x * 0.5f + 0.5f) * fImageWidth,
                    (1.0f - (pos1.y * 0.5f + 0.5f)) * fImageHeight,
                    0.0f);

                float3 pt2 = float3(
                    (pos2.x * 0.5f + 0.5f) * fImageWidth,
                    (1.0f - (pos2.y * 0.5f + 0.5f)) * fImageHeight,
                    0.0f);

                float3 minV = fminf(pt0, fminf(pt1, pt2));
                float3 maxV = fmaxf(pt0, fmaxf(pt1, pt2));

                // get min max normalized coordinates (-1, 1)
                uint2 minI = uint2(
                    clamp(uint32_t(minV.x), 0, rasterTriangleDesc.miImageWidth - 1),
                    clamp(uint32_t(minV.y), 0, rasterTriangleDesc.miImageHeight - 1));
                uint2 maxI = uint2(
                    clamp(uint32_t(maxV.x), 0, rasterTriangleDesc.miImageWidth - 1),
                    clamp(uint32_t(maxV.y), 0, rasterTriangleDesc.miImageHeight - 1));

                // scanline
                for(uint32_t iY = minI.y; iY <= maxI.y; iY++)
                {
                    for(uint32_t iX = minI.x; iX <= maxI.x; iX++)
                    {
                        vec3 pt((float)iX, (float)iY, 0.0f);
                        vec3 barycentricCoord = barycentric(pt, pt0, pt1, pt2);
                        
                        // check for inside triangle
                        if(barycentricCoord.x >= -fBarycentricEpsilon && barycentricCoord.x <= 1.0f + fBarycentricEpsilon &&
                           barycentricCoord.y >= -fBarycentricEpsilon && barycentricCoord.y <= 1.0f + fBarycentricEpsilon &&
                           barycentricCoord.z >= -fBarycentricEpsilon && barycentricCoord.z <= 1.0f + fBarycentricEpsilon)
                        {
                            // default fragment shader inputs
                            float4 currClipSpacePosition = pos0 * barycentricCoord.x + pos1 * barycentricCoord.y + pos2 * barycentricCoord.z;
                            currClipSpacePosition.z = currClipSpacePosition.z * 0.5f + 0.5f;

                            float4 currWorldPosition = worldPos0 * barycentricCoord.x + worldPos1 * barycentricCoord.y + worldPos2 * barycentricCoord.z;
                            float4 currNormal = normal0 * barycentricCoord.x + normal1 * barycentricCoord.y + normal2 * barycentricCoord.z;

                            float4 currUV = uv0 * barycentricCoord.x + uv1 * barycentricCoord.y + uv2 * barycentricCoord.z;
                            float4 currColor = color0 * barycentricCoord.x + color1 * barycentricCoord.y + color2 * barycentricCoord.z;

                            // depth 
                            uint32_t iIndex = (iY * rasterTriangleDesc.miImageWidth + iX);
                            {
                                std::lock_guard<std::mutex> lock(sRasterTriangleMutex);

                                float fExistingDepth = rasterTriangleDesc.mafDepthBuffer[iIndex];
                                if(fExistingDepth > currClipSpacePosition.z)
                                {
                                    switch(rasterTriangleDesc.miMode)
                                    {
                                        case 0:
                                        {
                                            //float4 color = rasterTriangleDesc.maColors[0] * barycentricCoord.x +
                                            //    rasterTriangleDesc.maColors[1] * barycentricCoord.y +
                                            //    rasterTriangleDesc.maColors[2] * barycentricCoord.z;

                                            FragmentShaderInput fragmentShaderInput;
                                            FragmentShaderOutput fragmentShaderOutput;

                                            {
                                                // fill out extra inputs from TEXCOORD1 and on
                                                float4 aExtraInputs[7];
                                                for(uint32_t iInput = Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD1; iInput <= Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD7; iInput++)
                                                {
                                                    uint32_t iIndex = iInput - Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD1;
                                                    aExtraInputs[iIndex] =
                                                        rasterTriangleDesc.maVertexShaderOutput[0].maOutput[iInput] * barycentricCoord.x +
                                                        rasterTriangleDesc.maVertexShaderOutput[1].maOutput[iInput] * barycentricCoord.y +
                                                        rasterTriangleDesc.maVertexShaderOutput[2].maOutput[iInput] * barycentricCoord.z;
                                                }

                                                // default fragment shader inputs, world position, clip space position, etc.
                                                fragmentShaderInput.maFragmentShaderInput[Render::Common::Utils::VertexShaderOutputMapping::WORLD_POSITION] = float4(currWorldPosition, 1.0f);
                                                fragmentShaderInput.maFragmentShaderInput[Render::Common::Utils::VertexShaderOutputMapping::NORMAL] = float4(currNormal, 1.0f);
                                                fragmentShaderInput.maFragmentShaderInput[Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD0] = float4(currUV.x, currUV.y, 0.0f, 1.0f);
                                                fragmentShaderInput.maFragmentShaderInput[Render::Common::Utils::VertexShaderOutputMapping::COLOR] = currColor;
                                                fragmentShaderInput.maFragmentShaderInput[Render::Common::Utils::VertexShaderOutputMapping::CLIP_SPACE_POSITON] = float4(currClipSpacePosition, 1.0f);

                                                // extra inputs residing in TEXCOORD1 and on
                                                for(uint32_t iInput = Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD1; iInput <= Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD7; iInput++)
                                                {
                                                    uint32_t iExtraIndex = iInput - Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD1;
                                                    WTFASSERT(iExtraIndex < sizeof(aExtraInputs) / sizeof(*aExtraInputs), "out of bounds %d of %d", iExtraIndex, sizeof(aExtraInputs) / sizeof(*aExtraInputs));
                                                    fragmentShaderInput.maFragmentShaderInput[iInput] = aExtraInputs[iExtraIndex];
                                                }

                                                // fragment shader function
                                                rasterTriangleDesc.mFragmentShaderFunc(
                                                    fragmentShaderOutput,
                                                    fragmentShaderInput,
                                                    ::uint2(iX, iY),
                                                    *rasterTriangleDesc.mpShaderResources);

                                                // save output
                                                for(uint32_t iRenderTarget = 0; iRenderTarget < MAX_RASTERIZER_RENDER_TARGETS; iRenderTarget++)
                                                {
                                                    if(rasterTriangleDesc.maaImageBuffers[iRenderTarget] != nullptr)
                                                    {
                                                        rasterTriangleDesc.maaImageBuffers[iRenderTarget][iIndex] = fragmentShaderOutput.maRenderTargets[iRenderTarget];
                                                    }
                                                }
                                            }

                                            break;
                                        }
                                    }

                                    rasterTriangleDesc.mafDepthBuffer[iIndex] = currClipSpacePosition.z;
                                }
                            
                            }   // lock guard
                        }
                    }
                }

            }

            void rasterMesh(RasterMeshDescriptor& meshDesc)
            {
                RasterTriangleDescriptor rasterDesc;
                rasterDesc.miMode = 0;
                rasterDesc.miImageWidth = meshDesc.miImageWidth;
                rasterDesc.miImageHeight = meshDesc.miImageHeight;
                rasterDesc.mafDepthBuffer = meshDesc.mafDepthBuffer;
                rasterDesc.mFragmentShaderFunc = meshDesc.mFragmentShaderFunc;

                uint32_t iWorldPositionInputMapping = meshDesc.maVertexInputMap.find("world") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["world"] : UINT32_MAX;
                uint32_t iNormalInputMapping = meshDesc.maVertexInputMap.find("normal") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["normal"] : UINT32_MAX;
                uint32_t iUVInputMapping = meshDesc.maVertexInputMap.find("texcoord0") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["texcoord0"] : UINT32_MAX;
                uint32_t iColorInputMapping = meshDesc.maVertexInputMap.find("color") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["color"] : UINT32_MAX;

                WTFASSERT(iWorldPositionInputMapping != UINT32_MAX, "Need to have a default \"world\" input mapping");

                for(uint32_t i = 0; i < MAX_RASTERIZER_RENDER_TARGETS; i++)
                {
                    rasterDesc.maaImageBuffers[i] = meshDesc.maaImageBuffers[i];
                }

                // triangles
                for(uint32_t iTri = 0; iTri < meshDesc.miNumTriangles * 3; iTri += 3)
                {
                    uint32_t iPos0 = meshDesc.maiTrianglePositionIndices[iTri];
                    uint32_t iPos1 = meshDesc.maiTrianglePositionIndices[iTri + 1];
                    uint32_t iPos2 = meshDesc.maiTrianglePositionIndices[iTri + 2];

                    float4 pos0 = float4(meshDesc.maPositions[iPos0], 1.0f);
                    float4 pos1 = float4(meshDesc.maPositions[iPos1], 1.0f);
                    float4 pos2 = float4(meshDesc.maPositions[iPos2], 1.0f);

                    uint32_t iColor0 = meshDesc.maiTriangleColorIndices[iTri];
                    uint32_t iColor1 = meshDesc.maiTriangleColorIndices[iTri + 1];
                    uint32_t iColor2 = meshDesc.maiTriangleColorIndices[iTri + 2];

                    uint32_t iNorm0 = meshDesc.maiTriangleNormalIndices[iTri];
                    uint32_t iNorm1 = meshDesc.maiTriangleNormalIndices[iTri + 1];
                    uint32_t iNorm2 = meshDesc.maiTriangleNormalIndices[iTri + 2];

                    float3 const& norm0 = meshDesc.maNormals[iNorm0];
                    float3 const& norm1 = meshDesc.maNormals[iNorm1];
                    float3 const& norm2 = meshDesc.maNormals[iNorm2];

                    uint32_t iUV0 = meshDesc.maiTriangleUVIndices[iTri];
                    uint32_t iUV1 = meshDesc.maiTriangleUVIndices[iTri + 1];
                    uint32_t iUV2 = meshDesc.maiTriangleUVIndices[iTri + 2];
                    float2 const& uv0 = meshDesc.maUVs[iUV0];
                    float2 const& uv1 = meshDesc.maUVs[iUV1];
                    float2 const& uv2 = meshDesc.maUVs[iUV2];

                    // fill out the vertex shader inputs and call the vertex shader function for the 3 vertices

                    // vertex 0
                    VertexShaderOutput vertexOut0;
                    VertexShaderInput vertexIn0;
                    vertexIn0.maUserData = meshDesc.maUserDataInputMap;
                    vertexIn0.maInput[iWorldPositionInputMapping] = pos0;
                    float4* pNormal = (iNormalInputMapping != UINT32_MAX) ? &vertexIn0.maInput[iNormalInputMapping] : &vertexIn0.maInput[1];
                    float4* pUV = (iUVInputMapping != UINT32_MAX) ? &vertexIn0.maInput[iUVInputMapping] : &vertexIn0.maInput[2];
                    float4* pColor = (iColorInputMapping != UINT32_MAX) ? &vertexIn0.maInput[iColorInputMapping] : &vertexIn0.maInput[3];
                    memcpy(pNormal, &norm0, sizeof(norm0));
                    memcpy(pUV, &uv0, sizeof(uv0));
                    memcpy(pColor, &meshDesc.maColors[iColor0], sizeof(meshDesc.maColors[iColor0]));
                    meshDesc.mVertexShaderFunc(vertexOut0, vertexIn0, *meshDesc.mpShaderResources);
                    
                    // vertex 1
                    VertexShaderOutput vertexOut1;
                    VertexShaderInput vertexIn1;
                    vertexIn1.maUserData = meshDesc.maUserDataInputMap;
                    vertexIn1.maInput[iWorldPositionInputMapping] = pos1;
                    pNormal = (iNormalInputMapping != UINT32_MAX) ? &vertexIn1.maInput[iNormalInputMapping] : &vertexIn1.maInput[1];
                    pUV = (iUVInputMapping != UINT32_MAX) ? &vertexIn1.maInput[iUVInputMapping] : &vertexIn1.maInput[2];
                    pColor = (iColorInputMapping != UINT32_MAX) ? &vertexIn1.maInput[iColorInputMapping] : &vertexIn1.maInput[3];
                    memcpy(pNormal, &norm1, sizeof(norm1));
                    memcpy(pUV, &uv1, sizeof(uv1));
                    memcpy(pColor, &meshDesc.maColors[iColor1], sizeof(meshDesc.maColors[iColor1]));
                    meshDesc.mVertexShaderFunc(vertexOut1, vertexIn1, *meshDesc.mpShaderResources);

                    // vertex 2
                    VertexShaderOutput vertexOut2;
                    VertexShaderInput vertexIn2;
                    vertexIn2.maUserData = meshDesc.maUserDataInputMap;
                    vertexIn2.maInput[iWorldPositionInputMapping] = pos2;
                    pNormal = (iNormalInputMapping != UINT32_MAX) ? &vertexIn2.maInput[iNormalInputMapping] : &vertexIn2.maInput[1];
                    pUV = (iUVInputMapping != UINT32_MAX) ? &vertexIn2.maInput[iUVInputMapping] : &vertexIn2.maInput[2];
                    pColor = (iColorInputMapping != UINT32_MAX) ? &vertexIn2.maInput[iColorInputMapping] : &vertexIn2.maInput[3];
                    memcpy(pNormal, &norm2, sizeof(norm2));
                    memcpy(pUV, &uv2, sizeof(uv2));
                    memcpy(pColor, &meshDesc.maColors[iColor2], sizeof(meshDesc.maColors[iColor2]));
                    meshDesc.mVertexShaderFunc(vertexOut2, vertexIn2, *meshDesc.mpShaderResources);
                    
                    // fill out the rasterize triangle descriptor
                    rasterDesc.maClipSpacePositions[0] = vertexOut0.maOutput[WORLD_POSITION];
                    rasterDesc.maClipSpacePositions[1] = vertexOut1.maOutput[WORLD_POSITION];
                    rasterDesc.maClipSpacePositions[2] = vertexOut2.maOutput[WORLD_POSITION];

                    rasterDesc.maNormals[0] = vertexOut0.maOutput[NORMAL];
                    rasterDesc.maNormals[1] = vertexOut1.maOutput[NORMAL];
                    rasterDesc.maNormals[2] = vertexOut2.maOutput[NORMAL];

                    rasterDesc.maUVs[0] = float2(vertexOut0.maOutput[TEXCOORD0].x, vertexOut0.maOutput[TEXCOORD0].y);
                    rasterDesc.maUVs[1] = float2(vertexOut1.maOutput[TEXCOORD0].x, vertexOut1.maOutput[TEXCOORD0].y);
                    rasterDesc.maUVs[2] = float2(vertexOut2.maOutput[TEXCOORD0].x, vertexOut2.maOutput[TEXCOORD0].y);

                    rasterDesc.maColors[0] = vertexOut0.maOutput[COLOR];
                    rasterDesc.maColors[1] = vertexOut1.maOutput[COLOR];
                    rasterDesc.maColors[2] = vertexOut2.maOutput[COLOR];

                    rasterDesc.maVertexShaderOutput[0] = vertexOut0;
                    rasterDesc.maVertexShaderOutput[1] = vertexOut1;
                    rasterDesc.maVertexShaderOutput[2] = vertexOut2;

                    rasterTriangle(rasterDesc);
                }
            }

            /*
            **
            */
            void rasterMeshMT(RasterMeshDescriptor& meshDesc)
            {
                RasterTriangleDescriptor rasterDesc;
                rasterDesc.miMode = 0;
                rasterDesc.miImageWidth = meshDesc.miImageWidth;
                rasterDesc.miImageHeight = meshDesc.miImageHeight;
                rasterDesc.mafDepthBuffer = meshDesc.mafDepthBuffer;
                rasterDesc.mFragmentShaderFunc = meshDesc.mFragmentShaderFunc;

                for(uint32_t i = 0; i < MAX_RASTERIZER_RENDER_TARGETS; i++)
                {
                    rasterDesc.maaImageBuffers[i] = meshDesc.maaImageBuffers[i];
                }

                uint32_t iWorldPositionInputMapping = meshDesc.maVertexInputMap.find("world") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["world"] : UINT32_MAX;
                uint32_t iNormalInputMapping = meshDesc.maVertexInputMap.find("normal") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["normal"] : UINT32_MAX;
                uint32_t iUVInputMapping = meshDesc.maVertexInputMap.find("texcoord0") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["texcoord0"] : UINT32_MAX;
                uint32_t iColorInputMapping = meshDesc.maVertexInputMap.find("color") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["color"] : UINT32_MAX;
                uint32_t iViewMatrixMapping = meshDesc.maVertexInputMap.find("viewMatrix") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["viewMatrix"] : UINT32_MAX;
                uint32_t iProjectionMatrixMapping = meshDesc.maVertexInputMap.find("projectionMatrix") != meshDesc.maVertexInputMap.end() ? meshDesc.maVertexInputMap["projectionMatrix"] : UINT32_MAX;

                static std::atomic<int32_t> siCurrTri = 0;

                std::vector<float3> aClipSpacePositions(meshDesc.miNumTriangles * 3);
                std::vector<float3> aNormals(meshDesc.miNumTriangles * 3);
                std::vector<float4> aColors(meshDesc.miNumTriangles * 3);
                std::vector<float3> aWorldPositions(meshDesc.miNumTriangles * 3);

                std::vector<VertexShaderOutput> aVertexShaderOutputs(meshDesc.miNumTriangles * 3);

                auto xformTriangles = [&](uint32_t iNumTriangles, uint32_t iMeshID)
                {
                    for(;;)
                    {
                        int32_t iTri = siCurrTri.fetch_add(1);
                        if(iTri >= static_cast<int32_t>(iNumTriangles))
                        {
                            break;
                        }

                        iTri *= 3;

                        uint32_t iPos0 = meshDesc.maiTrianglePositionIndices[iTri];
                        uint32_t iPos1 = meshDesc.maiTrianglePositionIndices[iTri + 1];
                        uint32_t iPos2 = meshDesc.maiTrianglePositionIndices[iTri + 2];

                        float4 pos0 = float4(meshDesc.maPositions[iPos0], 1.0f);
                        float4 pos1 = float4(meshDesc.maPositions[iPos1], 1.0f);
                        float4 pos2 = float4(meshDesc.maPositions[iPos2], 1.0f);

                        uint32_t iColor0 = meshDesc.maiTriangleColorIndices[iTri];
                        uint32_t iColor1 = meshDesc.maiTriangleColorIndices[iTri + 1];
                        uint32_t iColor2 = meshDesc.maiTriangleColorIndices[iTri + 2];

                        uint32_t iNorm0 = meshDesc.maiTriangleNormalIndices[iTri];
                        uint32_t iNorm1 = meshDesc.maiTriangleNormalIndices[iTri + 1];
                        uint32_t iNorm2 = meshDesc.maiTriangleNormalIndices[iTri + 2];

                        uint32_t iUV0 = meshDesc.maiTriangleUVIndices[iTri];
                        uint32_t iUV1 = meshDesc.maiTriangleUVIndices[iTri + 1];
                        uint32_t iUV2 = meshDesc.maiTriangleUVIndices[iTri + 2];

                        float3 const& norm0 = meshDesc.maNormals[iNorm0];
                        float3 const& norm1 = meshDesc.maNormals[iNorm1];
                        float3 const& norm2 = meshDesc.maNormals[iNorm2];

                        float3 const& uv0 = meshDesc.maUVs[iUV0];
                        float3 const& uv1 = meshDesc.maUVs[iUV1];
                        float3 const& uv2 = meshDesc.maUVs[iUV2];

                        aColors[iTri] = meshDesc.maColors[iColor0];
                        aColors[iTri + 1] = meshDesc.maColors[iColor1];
                        aColors[iTri + 2] = meshDesc.maColors[iColor2];

                        aNormals[iTri] = norm0;
                        aNormals[iTri + 1] = norm1;
                        aNormals[iTri + 2] = norm2;

                        aWorldPositions[iTri] = pos0;
                        aWorldPositions[iTri + 1] = pos1;
                        aWorldPositions[iTri + 2] = pos2;

                        VertexShaderOutput& vertexOut0 = aVertexShaderOutputs[iTri];
                        VertexShaderInput vertexIn0;
                        vertexIn0.maUserData = meshDesc.maUserDataInputMap;
                        vertexIn0.miMeshID = iMeshID;
                        float4* pPosition = (iWorldPositionInputMapping != UINT32_MAX) ? &vertexIn0.maInput[iWorldPositionInputMapping] : &vertexIn0.maInput[Render::Common::Utils::VertexShaderOutputMapping::WORLD_POSITION];
                        float4* pNormal = (iNormalInputMapping != UINT32_MAX) ? &vertexIn0.maInput[iNormalInputMapping] : &vertexIn0.maInput[Render::Common::Utils::VertexShaderOutputMapping::NORMAL];
                        float4* pUV = (iUVInputMapping != UINT32_MAX) ? &vertexIn0.maInput[iUVInputMapping] : &vertexIn0.maInput[Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD0];
                        float4* pColor = (iColorInputMapping != UINT32_MAX) ? &vertexIn0.maInput[iColorInputMapping] : &vertexIn0.maInput[Render::Common::Utils::VertexShaderOutputMapping::COLOR];
                        memcpy(pPosition, &pos0, sizeof(pos0));
                        memcpy(pNormal, &norm0, sizeof(norm0));
                        memcpy(pUV, &uv0, sizeof(uv0));
                        memcpy(pColor, &meshDesc.maColors[iColor0], sizeof(meshDesc.maColors[iColor0]));
                        
                        float4x4* pViewMatrix = (iViewMatrixMapping != UINT32_MAX) ? &vertexIn0.maInputMatrix[iViewMatrixMapping] : &vertexIn0.maInputMatrix[MatrixInputMapping::VIEW];
                        memcpy(pViewMatrix, &meshDesc.mViewMatrix, sizeof(meshDesc.mViewMatrix));
                        float4x4* pProjectionMatrix = (iProjectionMatrixMapping != UINT32_MAX) ? &vertexIn0.maInputMatrix[iProjectionMatrixMapping] : &vertexIn0.maInputMatrix[MatrixInputMapping::PROJECTION];
                        memcpy(pProjectionMatrix, &meshDesc.mProjectionMatrix, sizeof(meshDesc.mProjectionMatrix));
                        float4x4* pPrevViewMatrix = (iViewMatrixMapping != UINT32_MAX) ? &vertexIn0.maInputMatrix[iViewMatrixMapping] : &vertexIn0.maInputMatrix[MatrixInputMapping::PREVIOUS_VIEW];
                        memcpy(pPrevViewMatrix, &meshDesc.mPrevViewMatrix, sizeof(meshDesc.mPrevViewMatrix));
                        float4x4* pPrevProjectionMatrix = (iProjectionMatrixMapping != UINT32_MAX) ? &vertexIn0.maInputMatrix[iProjectionMatrixMapping] : &vertexIn0.maInputMatrix[MatrixInputMapping::PREVIOUS_PROJECTION];
                        memcpy(pPrevProjectionMatrix, &meshDesc.mPrevProjectionMatrix, sizeof(meshDesc.mPrevProjectionMatrix));
                        meshDesc.mVertexShaderFunc(vertexOut0, vertexIn0, *meshDesc.mpShaderResources);

                        VertexShaderOutput& vertexOut1 = aVertexShaderOutputs[iTri + 1];
                        VertexShaderInput vertexIn1;
                        vertexIn1.maUserData = meshDesc.maUserDataInputMap;
                        vertexIn1.miMeshID = iMeshID;
                        pPosition = (iWorldPositionInputMapping != UINT32_MAX) ? &vertexIn1.maInput[iWorldPositionInputMapping] : &vertexIn1.maInput[Render::Common::Utils::VertexShaderOutputMapping::WORLD_POSITION];
                        pNormal = (iNormalInputMapping != UINT32_MAX) ? &vertexIn1.maInput[iNormalInputMapping] : &vertexIn1.maInput[Render::Common::Utils::VertexShaderOutputMapping::NORMAL];
                        pUV = (iUVInputMapping != UINT32_MAX) ? &vertexIn1.maInput[iUVInputMapping] : &vertexIn1.maInput[Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD0];
                        pColor = (iColorInputMapping != UINT32_MAX) ? &vertexIn1.maInput[iColorInputMapping] : &vertexIn1.maInput[Render::Common::Utils::VertexShaderOutputMapping::COLOR];
                        memcpy(pPosition, &pos1, sizeof(pos1));
                        memcpy(pNormal, &norm1, sizeof(norm1));
                        memcpy(pUV, &uv1, sizeof(uv1));
                        memcpy(pColor, &meshDesc.maColors[iColor1], sizeof(meshDesc.maColors[iColor1]));
                        
                        pViewMatrix = (iViewMatrixMapping != UINT32_MAX) ? &vertexIn1.maInputMatrix[iViewMatrixMapping] : &vertexIn1.maInputMatrix[MatrixInputMapping::VIEW];
                        memcpy(pViewMatrix, &meshDesc.mViewMatrix, sizeof(meshDesc.mViewMatrix));
                        pProjectionMatrix = (iProjectionMatrixMapping != UINT32_MAX) ? &vertexIn1.maInputMatrix[iProjectionMatrixMapping] : &vertexIn1.maInputMatrix[MatrixInputMapping::PROJECTION];
                        memcpy(pProjectionMatrix, &meshDesc.mProjectionMatrix, sizeof(meshDesc.mProjectionMatrix));
                        pPrevViewMatrix = (iViewMatrixMapping != UINT32_MAX) ? &vertexIn1.maInputMatrix[iViewMatrixMapping] : &vertexIn1.maInputMatrix[MatrixInputMapping::PREVIOUS_VIEW];
                        memcpy(pPrevViewMatrix, &meshDesc.mPrevViewMatrix, sizeof(meshDesc.mPrevViewMatrix));
                        pPrevProjectionMatrix = (iProjectionMatrixMapping != UINT32_MAX) ? &vertexIn1.maInputMatrix[iProjectionMatrixMapping] : &vertexIn1.maInputMatrix[MatrixInputMapping::PREVIOUS_PROJECTION];
                        memcpy(pPrevProjectionMatrix, &meshDesc.mPrevProjectionMatrix, sizeof(meshDesc.mPrevProjectionMatrix));
                        meshDesc.mVertexShaderFunc(vertexOut1, vertexIn1, *meshDesc.mpShaderResources);

                        VertexShaderOutput& vertexOut2 = aVertexShaderOutputs[iTri + 2];
                        VertexShaderInput vertexIn2;
                        vertexIn2.maUserData = meshDesc.maUserDataInputMap;
                        vertexIn2.miMeshID = iMeshID;
                        pPosition = (iWorldPositionInputMapping != UINT32_MAX) ? &vertexIn2.maInput[iWorldPositionInputMapping] : &vertexIn2.maInput[Render::Common::Utils::VertexShaderOutputMapping::WORLD_POSITION];
                        pNormal = (iNormalInputMapping != UINT32_MAX) ? &vertexIn2.maInput[iNormalInputMapping] : &vertexIn2.maInput[Render::Common::Utils::VertexShaderOutputMapping::NORMAL];
                        pUV = (iUVInputMapping != UINT32_MAX) ? &vertexIn2.maInput[iUVInputMapping] : &vertexIn2.maInput[Render::Common::Utils::VertexShaderOutputMapping::TEXCOORD0];
                        pColor = (iColorInputMapping != UINT32_MAX) ? &vertexIn2.maInput[iColorInputMapping] : &vertexIn2.maInput[Render::Common::Utils::VertexShaderOutputMapping::COLOR];
                        memcpy(pPosition, &pos2, sizeof(pos2));
                        memcpy(pNormal, &norm2, sizeof(norm2));
                        memcpy(pUV, &uv2, sizeof(uv2));
                        memcpy(pColor, &meshDesc.maColors[iColor2], sizeof(meshDesc.maColors[iColor2]));
                        
                        pViewMatrix = (iViewMatrixMapping != UINT32_MAX) ? &vertexIn2.maInputMatrix[iViewMatrixMapping] : &vertexIn2.maInputMatrix[MatrixInputMapping::VIEW];
                        memcpy(pViewMatrix, &meshDesc.mViewMatrix, sizeof(meshDesc.mViewMatrix));
                        pProjectionMatrix = (iProjectionMatrixMapping != UINT32_MAX) ? &vertexIn2.maInputMatrix[iProjectionMatrixMapping] : &vertexIn2.maInputMatrix[MatrixInputMapping::PROJECTION];
                        memcpy(pProjectionMatrix, &meshDesc.mProjectionMatrix, sizeof(meshDesc.mProjectionMatrix));
                        pPrevViewMatrix = (iViewMatrixMapping != UINT32_MAX) ? &vertexIn2.maInputMatrix[iViewMatrixMapping] : &vertexIn2.maInputMatrix[MatrixInputMapping::PREVIOUS_VIEW];
                        memcpy(pPrevViewMatrix, &meshDesc.mPrevViewMatrix, sizeof(meshDesc.mPrevViewMatrix));
                        pPrevProjectionMatrix = (iProjectionMatrixMapping != UINT32_MAX) ? &vertexIn2.maInputMatrix[iProjectionMatrixMapping] : &vertexIn2.maInputMatrix[MatrixInputMapping::PREVIOUS_PROJECTION];
                        memcpy(pPrevProjectionMatrix, &meshDesc.mPrevProjectionMatrix, sizeof(meshDesc.mPrevProjectionMatrix));
                        meshDesc.mVertexShaderFunc(vertexOut2, vertexIn2, *meshDesc.mpShaderResources);

                    }
                };

                auto rasterizeTriangle = [&](uint32_t iNumTriangles)
                {
                    RasterTriangleDescriptor rasterTriangleDesc;
                    rasterTriangleDesc.miMode = 0;
                    rasterTriangleDesc.miImageWidth = meshDesc.miImageWidth;
                    rasterTriangleDesc.miImageHeight = meshDesc.miImageHeight;
                    rasterTriangleDesc.mafDepthBuffer = meshDesc.mafDepthBuffer;
                    rasterTriangleDesc.mFragmentShaderFunc = meshDesc.mFragmentShaderFunc;
                    rasterTriangleDesc.mpShaderResources = meshDesc.mpShaderResources;

                    for(uint32_t i = 0; i < MAX_RASTERIZER_RENDER_TARGETS; i++)
                    {
                        rasterTriangleDesc.maaImageBuffers[i] = meshDesc.maaImageBuffers[i];
                    }

                    for(;;)
                    {
                        int32_t iTri = siCurrTri.fetch_add(1);
                        if(iTri >= static_cast<int32_t>(iNumTriangles))
                        {
                            break;
                        }

                        iTri *= 3;

                        rasterTriangleDesc.maClipSpacePositions[0] = aClipSpacePositions[iTri];
                        rasterTriangleDesc.maClipSpacePositions[1] = aClipSpacePositions[iTri + 1];
                        rasterTriangleDesc.maClipSpacePositions[2] = aClipSpacePositions[iTri + 2];

                        rasterTriangleDesc.maColors[0] = aColors[iTri];
                        rasterTriangleDesc.maColors[1] = aColors[iTri + 1];
                        rasterTriangleDesc.maColors[2] = aColors[iTri + 2];
                       
                        rasterTriangleDesc.maWorldPositions[0] = aWorldPositions[iTri];
                        rasterTriangleDesc.maWorldPositions[1] = aWorldPositions[iTri + 1];
                        rasterTriangleDesc.maWorldPositions[2] = aWorldPositions[iTri + 2];

                        rasterTriangleDesc.maNormals[0] = aNormals[iTri + 0];
                        rasterTriangleDesc.maNormals[1] = aNormals[iTri + 1];
                        rasterTriangleDesc.maNormals[2] = aNormals[iTri + 2];

                        rasterTriangleDesc.maVertexShaderOutput[0] = aVertexShaderOutputs[iTri];
                        rasterTriangleDesc.maVertexShaderOutput[1] = aVertexShaderOutputs[iTri + 1];
                        rasterTriangleDesc.maVertexShaderOutput[2] = aVertexShaderOutputs[iTri + 2];

                        rasterTriangle(rasterTriangleDesc);
                    }
                };

                siCurrTri.store(0);
                uint32_t iNumThreads = std::thread::hardware_concurrency();
                //uint32_t iNumThreads = 1;
                std::vector<std::unique_ptr<std::thread>> apVertexShaderThreads(iNumThreads);
                for(uint32_t iThread = 0; iThread < iNumThreads; iThread++)
                {
                    apVertexShaderThreads[iThread] = std::make_unique<std::thread>(
                        [&](uint32_t iThread)
                        {
                            xformTriangles(meshDesc.miNumTriangles, meshDesc.miMeshID);
                        }, iThread);
                }

                for(uint32_t iThread = 0; iThread < iNumThreads; iThread++)
                {
                    if(apVertexShaderThreads[iThread]->joinable())
                    {
                        apVertexShaderThreads[iThread]->join();
                    }
                }

                siCurrTri.store(0);

                iNumThreads = std::thread::hardware_concurrency();
                std::vector<std::unique_ptr<std::thread>> apFragmentShaderThreads(iNumThreads);
                for(uint32_t iThread = 0; iThread < iNumThreads; iThread++)
                {
                    apFragmentShaderThreads[iThread] = std::make_unique<std::thread>(
                        [&](uint32_t iThread)
                        {
                            rasterizeTriangle(meshDesc.miNumTriangles);
                        }, iThread);
                }

                for(uint32_t iThread = 0; iThread < iNumThreads; iThread++)
                {
                    if(apFragmentShaderThreads[iThread]->joinable())
                    {
                        apFragmentShaderThreads[iThread]->join();
                    }
                }

            }

        }   // Utils

    }   // Common

}   // Render

#ifdef __APPLE__
#undef FLT_MAX
#endif // __APPLE__
