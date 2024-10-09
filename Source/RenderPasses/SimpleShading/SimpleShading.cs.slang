import Scene.RaytracingInline;
import Rendering.Lights.EnvMapSampler;
import Rendering.Lights.LightHelpers;
import Utils.Geometry.GeometryHelpers;
import Utils.Sampling.SampleGenerator;

cbuffer CB
{
    uint2 gResolution;
    uint gFrameIndex;
    float gDepthThreshold;
    float gNormalThreshold;
}

Texture2D<uint4> gPackedHitInfo;
RWTexture2D<float4> gOutput;

// [Sample ALL light sources]
float3 evalAnalyticLight(const ShadingData sd, const IMaterialInstance mi, inout SampleGenerator sg)
{
    const uint lightCount = gScene.getLightCount();
    if (lightCount == 0)
        return float3(0.f);

    float3 Lr = float3(0.f);
    for (uint i = 0; i < lightCount; ++i)
    {
        AnalyticLightSample ls;
        if (!sampleLight(sd.posW, gScene.getLight(i), sg, ls))
            continue;

        const uint lobeTypes = mi.getLobeTypes(sd);
        const bool hasReflection = lobeTypes & uint(LobeType::Reflection);
        const bool hasTransmission = lobeTypes & uint(LobeType::Transmission);
        float NdotL = dot(sd.getOrientedFaceNormal(), ls.dir);
        if ((NdotL <= kMinCosTheta && !hasTransmission) || (NdotL >= -kMinCosTheta && !hasReflection))
            continue;

        Lr += mi.eval(sd, ls.dir, sg) * ls.Li;
    }

    return Lr;
}

float3 evalAnalyticLight(uint2 pixelPos, HitInfo hitInfo, inout SampleGenerator sg)
{
    if (!hitInfo.isValid())
    {
        const float3 ray = gScene.camera.computeRayPinhole(pixelPos, gResolution).dir;
        return gScene.envMap.eval(ray);
    }

    TriangleHit triangleHit = hitInfo.getTriangleHit();
    VertexData v = gScene.getVertexData(triangleHit);
    float3 viewDir = normalize(gScene.camera.getPosition() - v.posW);
    uint materialID = gScene.getMaterialID(triangleHit.instanceID);
    ShadingData sd = gScene.materials.prepareShadingData(v, materialID, viewDir);
    let mi = gScene.materials.getMaterialInstance(sd, ExplicitLodTextureSampler(0.f));

    return evalAnalyticLight(sd, mi, sg);
}

float getDepth(VertexData v)
{
    float4 curPosH = mul(gScene.camera.data.viewProjMatNoJitter, float4(v.posW, 1.f));
    return curPosH.z / curPosH.w;
}

[numthreads(32, 32, 1)]
void csMain(
    uint3 dispatchThreadId: SV_DispatchThreadID,
    uint groupIndex: SV_GroupIndex,
    uint3 groupThreadID: SV_GroupThreadID,
    uint3 groupdId: SV_GroupID
)
{
    if (dispatchThreadId.x >= gResolution.x || dispatchThreadId.y >= gResolution.y)
        return;

    uint2 pixelPos = dispatchThreadId.xy;
    SampleGenerator sg = SampleGenerator(pixelPos, gFrameIndex);

    // Execute one-sample shading.
    uint4 packed = gPackedHitInfo[pixelPos];
    gOutput[pixelPos] = float4(evalAnalyticLight(pixelPos, HitInfo(packed), sg), 1.f);
}
