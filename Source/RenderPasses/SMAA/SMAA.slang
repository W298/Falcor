#define SMAA_HLSL_4_1
#define SMAA_PRESET_HIGH

#include "SMAA.hlsli"

Texture2D<float4> gColor;

Texture2D gArea;
Texture2D gSearch;

Texture2D<float4> gEdge;
Texture2D<float4> gBlend;

struct VSOut
{
    float4 posH : SV_POSITION;
    float2 texC : TEXCOORD0;
    float4 offset[3] : TEXCOORD1;
};

VSOut edgeDetectionVS(float4 posS: POSITION, float2 texC: TEXCOORD)
{
    VSOut vsOut;

    vsOut.posH = posS;
    vsOut.texC = texC;
    SMAAEdgeDetectionVS(texC, vsOut.offset); 

    return vsOut;
}

float2 edgeDetectionPS(VSOut vsOut) : SV_TARGET
{
    return SMAALumaEdgeDetectionPS(vsOut.texC, vsOut.offset, gColor);
}

struct BlendVSOut
{
    float4 posH : SV_Position;
    float2 texC : TEXCOORD0;
    float2 pixC : TEXCOORD1;
    float4 offset[3] : TEXCOORD2;
};

BlendVSOut blendingWeightCalculationVS(float4 posS: POSITION, float2 texC: TEXCOORD)
{
    BlendVSOut blendVSOut;

    blendVSOut.posH = posS;
    blendVSOut.texC = texC;
    SMAABlendingWeightCalculationVS(texC, blendVSOut.pixC, blendVSOut.offset);

    return blendVSOut;
}

float4 blendingWeightCalculationPS(BlendVSOut blendVSOut) : SV_TARGET
{
    return SMAABlendingWeightCalculationPS(
        blendVSOut.texC,
        blendVSOut.pixC,
        blendVSOut.offset,
        gEdge,
        gArea,
        gSearch,
        float4(0)   // TODO: Should be changed.
    );
}

struct NeighborBlendVSOut
{
    float4 posH : SV_Position;
    float2 texC : TEXCOORD0;
    float4 offset : TEXCOORD1; 
}

NeighborBlendVSOut neighborhoodBlendingVS(float4 posS: POSITION, float2 texC: TEXCOORD)
{
    NeighborBlendVSOut neiBlendingVSOut;

    neiBlendingVSOut.posH = posS;
    neiBlendingVSOut.texC = texC;
    SMAANeighborhoodBlendingVS(texC, neiBlendingVSOut.offset);

    return neiBlendingVSOut;
}

float4 neighborhoodBlendingPS(NeighborBlendVSOut neiBlendingVSOut) : SV_TARGET
{
    return SMAANeighborhoodBlendingPS(
        neiBlendingVSOut.texC,
        neiBlendingVSOut.offset,
        gColor,
        gBlend
    );
}
