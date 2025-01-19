#pragma once 

/*
**
*/
float3 barycentric(
    in float3 p, 
    in float3 a, 
    in float3 b, 
    in float3 c)
{
    float3 v0 = b - a;
    float3 v1 = c - a;
    float3 v2 = p - a;
    float fD00 = dot(v0, v0);
    float fD01 = dot(v0, v1);
    float fD11 = dot(v1, v1);
    float fD20 = dot(v2, v0);
    float fD21 = dot(v2, v1);
    float fOneOverDenom = 1.0f / (fD00 * fD11 - fD01 * fD01);
    float fV = (fD11 * fD20 - fD01 * fD21) * fOneOverDenom;
    float fW = (fD00 * fD21 - fD01 * fD20) * fOneOverDenom;
    float fU = 1.0f - fV - fW;

    return float3(fU, fV, fW);
}