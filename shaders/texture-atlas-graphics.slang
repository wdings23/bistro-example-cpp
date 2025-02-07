
#include "default-uniform-headers.slang"
#include "octahedron-map-headers.slang"

struct HashEntry
{
    uint32_t miPageCoordinate;
    uint32_t miPageIndex;
    uint32_t miTextureIDAndMIP;
    uint32_t miUpdateFrame;
};

struct TextureAtlasOutput
{
    float4 mColor;
    bool mbLoaded;
};

[[vk::binding(0, 0)]] Texture2D<float4> atlasTexture0;
[[vk::binding(1, 0)]] Texture2D<float4> atlasTexture1;
[[vk::binding(2, 0)]] Texture2D<float4> atlasTexture2;
[[vk::binding(3, 0)]] Texture2D<float4> atlasTexture3;
[[vk::binding(4, 0)]] Texture2D<float4> pageInfoTextureMIP;
[[vk::binding(5, 0)]] Texture2D<float4> pageInfoNormalTextureMIP;
[[vk::binding(6, 0)]] Texture2D<float4> texCoordTexture;
[[vk::binding(7, 0)]] RWStructuredBuffer<HashEntry> aPageHashTableMIP;
[[vk::binding(8, 0)]] Texture2D<float4> initialTextureAtlas;
[[vk::binding(9, 0)]] Texture2D<float4> skyTexture;
[[vk::binding(10, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] StructuredBuffer<int2> aTextureDimension;
[[vk::binding(1, 1)]] StructuredBuffer<int2> aNormalTextureDimension;
[[vk::binding(2, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData;

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 mAlbedo : SV_TARGET0;
};

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
    float4 texCoord;
};

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    output.sv_position = assembledVertex.position;
    output.texCoord = assembledVertex.texCoord;

    return output;
}

[shader("fragment")]
PSOutput PSMain(VertexStageOutput in)
{
    PSOutput out;

    float4 sampleTexCoord = texCoordTexture.Sample(
        textureSampler,
        float2(in.texCoord.x, in.texCoord.y)
    );

    if (sampleTexCoord.w <= 0.0f)
    {
        float2 sampleUV = float2(in.texCoord.x, 1.0f - in.texCoord.y);
        float2 convertedUV = sampleUV * 2.0f - float2(1.0f, 1.0f);
        convertedUV.y = -convertedUV.y;

        // float4x4 inverseViewProjectionMatrix = inverse(transpose(defaultUniformData.mViewProjectionMatrix));
        float4 worldSpace = mul(float4(convertedUV.x, convertedUV.y, 1.0f, 1.0f), defaultUniformData.mInverseViewProjectionMatrix);
        worldSpace.x /= worldSpace.w;
        worldSpace.y /= worldSpace.w;
        worldSpace.z /= worldSpace.w;

        float3 direction = normalize(worldSpace.xyz - defaultUniformData.mCameraPosition.xyz);

        float2 uv = octahedronMap2(direction);
        float4 skyRadiance = skyTexture.Sample(
            textureSampler,
            uv);

        out.mAlbedo = float4(skyRadiance.xyz, 1.0f);
    }
    else
    {
        out.mAlbedo = outputTextureMIP(
            float2(in.texCoord.x, in.texCoord.y),
            0u
        ) .mColor;
    }

    return out;
}

/////
TextureAtlasOutput outputTextureMIP(
    float2 texCoord, 
    uint32_t iTextureType)
{
    TextureAtlasOutput ret;

    float fPageSize = 64.0f;
    float fTextureAtlasSize = 8192.0f;
    float fNumPagePerRow = fTextureAtlasSize / fPageSize;
    int32_t iNumPagesPerRow = int32_t(fNumPagePerRow);
    float fOneOverNumPagePerRow = 1.0f / fNumPagePerRow;

    // texture page info 
    float4 pageInfoMIP = pageInfoTextureMIP.Sample(
        textureSampler,
        texCoord
    );
    if(iTextureType == 1u)
    {
        pageInfoMIP = pageInfoNormalTextureMIP.Sample(
            textureSampler,
            texCoord
        );
    }

    float2 uv = texCoordTexture.Sample(
        textureSampler,
        texCoord
    ).xy;

    // texture id, hash id, mip and page uv
    int32_t iTextureIDMIP = int32_t(ceil(pageInfoMIP.z - 0.5f));
    int32_t iHashIndexMIP = int32_t(ceil(pageInfoMIP.w - 0.5f));
    int32_t iPageIndexMIP = int32_t(aPageHashTableMIP[iHashIndexMIP].miPageIndex);
    uint32_t iMIP = aPageHashTableMIP[iHashIndexMIP].miTextureIDAndMIP & 0xf;

    if(uint32_t(iPageIndexMIP) == 0xffffffffu || iPageIndexMIP <= 0)
    {
        uint32_t iInitialAtlasWidth = 1, iInitialAtlasHeight = 1;
        initialTextureAtlas.GetDimensions(iInitialAtlasWidth, iInitialAtlasHeight);

        float fInitialTextureAtlasSize = 512.0f;
        int32_t iInitialPageSize = 16;
        int32_t iNumInitialPagesPerRow = iInitialAtlasWidth / iInitialPageSize; 

        // get the uv difference from start of the page to current sample uv
        int2 initialTextureDimension = int2(iInitialPageSize, iInitialPageSize);
        int2 initialImageCoord = int2(
            int32_t(uv.x * float(initialTextureDimension.x)),
            int32_t(uv.y * float(initialTextureDimension.y))
        );
        int2 initialImagePageIndex = int2(
            initialImageCoord.x / iInitialPageSize,
            initialImageCoord.y / iInitialPageSize
        );
        int2 initialStartPageImageCoord = int2(
            initialImagePageIndex.x * iInitialPageSize,
            initialImagePageIndex.y * iInitialPageSize
        ); 
        int2 initialImageCoordDiff = initialImageCoord - initialStartPageImageCoord;
        
        // uv of the page in the atlas
        // page x, y
        int32_t iInitialAtlasPageX = iTextureIDMIP % iNumInitialPagesPerRow;
        int32_t iInitialAtlasPageY = iTextureIDMIP / iNumInitialPagesPerRow;
        
        // atlas uv
        int32_t iInitialAtlasX = iInitialAtlasPageX * iInitialPageSize;
        int32_t iInitialAtlasY = iInitialAtlasPageY * iInitialPageSize;
        float2 initialAtlasUV = float2(
            float(iInitialAtlasX + initialImageCoordDiff.x) / fInitialTextureAtlasSize,
            float(iInitialAtlasY + initialImageCoordDiff.y) / fInitialTextureAtlasSize 
        );

        ret.mbLoaded = false;
        ret.mColor = initialTextureAtlas.Sample(
            textureSampler,
            initialAtlasUV);

        if(iTextureType == 1u)
        {
            ret.mColor = float4(0.5f, 0.5f, 1.0f, 1.0f);
        }

//ret.mColor = float4(1.0f, 0.0f, 0.0f, 1.0f);

        return ret; // * vec4<f32>(1.0f, 0.2f, 0.2f, 1.0f);
    }

    ret.mColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    ret.mbLoaded = true;

    // atlas texture index
    uint32_t iAtlasIndex = iPageIndexMIP / (iNumPagesPerRow * iNumPagesPerRow);
    iPageIndexMIP = iPageIndexMIP % (iNumPagesPerRow * iNumPagesPerRow);
    if(iPageIndexMIP > 0)
    {
        // offset by one since zero is the default value, this signifies the page has been loaded 
        iPageIndexMIP -= 1;

        int32_t iMIPDenom = int32_t(pow(2.0f, float(iMIP)));

        // get the uv difference from start of the page to current sample uv
        int2 textureDimension = aTextureDimension[iTextureIDMIP];
        textureDimension[0] /= iMIPDenom;
        textureDimension[1] /= iMIPDenom;
        int2 imageCoord = int2(
            int32_t(uv.x * float(textureDimension.x)),
            int32_t(uv.y * float(textureDimension.y))
        );
        int2 imagePageIndex = int2(
            imageCoord.x / int32_t(fPageSize),
            imageCoord.y / int32_t(fPageSize)
        );
        int2 startPageImageCoord = int2(
            imagePageIndex.x * int32_t(fPageSize),
            imagePageIndex.y * int32_t(fPageSize)
        ); 
        int2 imageCoordDiff = imageCoord - startPageImageCoord;
        
        // uv of the page in the atlas
        // page x, y
        int32_t iAtlasPageX = iPageIndexMIP % iNumPagesPerRow;
        int32_t iAtlasPageY = iPageIndexMIP / iNumPagesPerRow;
        
        // atlas uv
        int32_t iAtlasX = iAtlasPageX * int32_t(fPageSize);
        int32_t iAtlasY = iAtlasPageY * int32_t(fPageSize);
        float2 atlasUV = float2(
            float(iAtlasX + imageCoordDiff.x) / fTextureAtlasSize,
            float(iAtlasY + imageCoordDiff.y) / fTextureAtlasSize 
        );
        
        // sample atlas texture based on MIP index
        ret.mColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
        // if(iMIP == 0)
        if (iAtlasIndex == 0)
        {
            ret.mColor = atlasTexture0.Sample(
                textureSampler,
                atlasUV
            );
        }
        else if(iMIP == 1)
        {
            ret.mColor = atlasTexture1.Sample(
                textureSampler,
                atlasUV
            );
        }
        //else if(iMIP == 2)
        else if(iAtlasIndex == 1)
        {
            ret.mColor = atlasTexture2.Sample(
                textureSampler,
                atlasUV
            );
        }
        else if(iMIP == 3)
        {
            ret.mColor = atlasTexture3.Sample(
                textureSampler,
                atlasUV
            );
        }

        // mark page as active
        aPageHashTableMIP[iHashIndexMIP].miUpdateFrame = uint32_t(defaultUniformData.miFrame);
    }

    return ret;
}
