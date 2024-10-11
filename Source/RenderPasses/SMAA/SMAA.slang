#define SMAA_HLSL_4_1
#define SMAA_PRESET_HIGH

#include "SMAA.hlsli"

Texture2D<float4> gColor;
Texture2D<float> gDepth;

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
