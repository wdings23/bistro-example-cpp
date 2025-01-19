#pragma once

struct RandomResult 
{
    float mfNum;
    uint32_t miSeed;
};

/*
**
*/
RandomResult initRand(
    in uint32_t val0, 
    in uint32_t val1, 
    in uint32_t backoff)
{
    RandomResult retResult;

    uint32_t v0 = val0;
    uint32_t v1 = val1;
    uint32_t s0 = 0u;

    for(uint32_t n = 0; n < backoff; n++)
    {
        s0 += uint32_t(0x9e3779b9);
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    retResult.miSeed = v0;
    retResult.mfNum = 0.0f;

    return retResult;
}

/*
**
*/
RandomResult nextRand(
    in uint32_t s)
{
    RandomResult retResult;

    uint32_t sCopy = s;
    sCopy = (1664525u * sCopy + 1013904223u);
    retResult.mfNum = float(sCopy & 0x00FFFFFF) / float(0x01000000);
    retResult.miSeed = sCopy;

    return retResult;
}