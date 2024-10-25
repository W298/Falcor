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

Texture2D<float> gComplex;
Texture2DMS<uint4> gPackedHitInfo;
RWTexture2D<float4> gOutput;
RWTexture2D<float> gEdge;

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

    // Truncate non-edge pixel.
    if (gComplex[pixelPos] <= 0.f)
    {
        // Execute one-sample shading.
        uint4 packed = gPackedHitInfo.Load(pixelPos, 0);
        gOutput[pixelPos] = float4(evalAnalyticLight(pixelPos, HitInfo(packed), sg), 1.f);
        gEdge[pixelPos] = 0.f;
        return;
    }

    // Check sample has environment hit. If so, the pixel is always edge.
    bool isEdge = false;

    HitInfo sampleData[SAMPLE_COUNT];
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        uint4 packed = gPackedHitInfo.Load(pixelPos, i);
        HitInfo hitInfo = HitInfo(packed);
        if (!hitInfo.isValid())
            isEdge = true;

        sampleData[i] = hitInfo;
    }

    // Check pixel is edge or not. By comparing depth and normal between samples.
    if (!isEdge)
    {
        VertexData v1 = {};
        VertexData v2 = gScene.getVertexData(sampleData[0].getTriangleHit());

        for (uint i = 0; i < SAMPLE_COUNT; ++i)
        {
            v1 = v2;
            v2 = gScene.getVertexData(sampleData[(i + 1) % SAMPLE_COUNT].getTriangleHit());

            if (abs(getDepth(v1) - getDepth(v2)) >= gDepthThreshold)
            {
                isEdge = true;
                break;
            }

            if (dot(v1.normalW, v2.normalW) <= gNormalThreshold)
            {
                isEdge = true;
                break;
            }
        }
    }

    if (isEdge)
    {
        // Execute multi-sample shading.
        float3 Lr = 0.f;
        for (uint i = 0; i < SAMPLE_COUNT; ++i)
            Lr += evalAnalyticLight(pixelPos, sampleData[i], sg);

        gOutput[pixelPos] = float4(Lr / SAMPLE_COUNT, 1.f);
        gEdge[pixelPos] = 1.f;

        return;
    }

    // Execute one-sample shading.
    gOutput[pixelPos] = float4(evalAnalyticLight(pixelPos, sampleData[0], sg), 1.f);
    gEdge[pixelPos] = 0.f;
}
