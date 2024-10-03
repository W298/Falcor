import Scene.Raster;

struct GBufferPSOut
{
    float coverage  : SV_TARGET0;
    uint4 packedHitInfo  : SV_TARGET1;
};

VSOut vsMain(VSIn vsIn)
{
    return defaultVS(vsIn);
}

GBufferPSOut psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID, float3 barycentrics: SV_Barycentrics, uint cover: SV_Coverage)
{
    float3 faceNormal = gScene.getFaceNormalW(vsOut.instanceID, triangleIndex);
    VertexData v = prepareVertexData(vsOut, faceNormal);
    let lod = ExplicitLodTextureSampler(0.f);

    // Alpha test.
    if (gScene.materials.alphaTest(v, vsOut.materialID, lod))
        discard;

    TriangleHit triangleHit;
    triangleHit.instanceID = vsOut.instanceID;
    triangleHit.primitiveIndex = triangleIndex;
    triangleHit.barycentrics = barycentrics.yz;

    GBufferPSOut psOut;

#if SAMPLE_COUNT == 4
    psOut = { float(cover != 0x0000000F), triangleHit.pack() };
#elif SAMPLE_COUNT == 8
    psOut = { float(cover != 0x000000FF), triangleHit.pack() };
#endif

    return psOut;
}
