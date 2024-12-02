/////
float3 ACESFilm(
    float3 radiance)
{
    float fA = 2.51f;
    float fB = 0.03f;
    float fC = 2.43f;
    float fD = 0.59f;
    float fE = 0.14f;

    return saturate((radiance * (fA * radiance + fB)) / (radiance * (fC * radiance + fD) + fE));
}