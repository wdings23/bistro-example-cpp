/*
**
*/
float3 decodeOctahedronMap(
    float2 uv)
{
    float2 newUV = uv * 2.0f - float2(1.0f, 1.0f);

    float2 absUV = float2(abs(newUV.x), abs(newUV.y));
    float3 v = float3(newUV.x, newUV.y, 1.0f - (absUV.x + absUV.y));

    if(absUV.x + absUV.y > 1.0f) 
    {
        v.x = (abs(newUV.y) - 1.0f) * -sign(newUV.x);
        v.y = (abs(newUV.x) - 1.0f) * -sign(newUV.y);
    }

    //v.y *= -1.0f;

    return v;
}

/*
**
*/
float2 octahedronMap2(
    float3 direction)
{
    float fDP = dot(float3(1.0f, 1.0f, 1.0f), abs(direction));
    float3 newDirection = float3(direction.x, direction.z, direction.y) / fDP;

    float2 ret = float2(
        (1.0f - abs(newDirection.z)) * sign(newDirection.x),
        (1.0f - abs(newDirection.x)) * sign(newDirection.z));

    if (newDirection.y < 0.0f)
    {
        ret = float2(
            newDirection.x,
            newDirection.z);
    }

    ret = ret * 0.5f + float2(0.5f, 0.5f);
    //ret.y = 1.0f - ret.y;

    return ret;
}
