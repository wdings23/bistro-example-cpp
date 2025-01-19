#pragma once

/*
**
*/
uint32_t murmurHash13(
    in uint3 src)
{
    uint32_t M = 0x5bd1e995u;
    uint32_t h = 1190494759u;

    src *= M; src.x ^= src.x >> 24u; src.y ^= src.y >> 24u; src.z ^= src.z >> 24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z;
    h ^= h >> 13u; h *= M; h ^= h >> 15u;
    return h;
}

/*
**
*/
uint32_t hash13(
    in float3 src,
    in uint32_t iNumSlots)
{
    uint3 srcU32 = uint3(
        asuint(src.x),
        asuint(src.y),
        asuint(src.z)
    );

    uint32_t h = uint32_t(murmurHash13(srcU32));
    float fValue = asfloat((h & 0x007ffffffu) | 0x3f800000u) - 1.0f;
    uint32_t iRet = clamp(uint32_t(fValue * float(iNumSlots - 1)), 0u, iNumSlots - 1);
    return iRet;
}