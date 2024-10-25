#include "DeferredMSAA.h"
#include "RenderGraph/RenderPassHelpers.h"

DeferredMSAA::DeferredMSAA(ref<Device> pDevice, const Properties& props) : RenderPass(pDevice)
{
    mpState = ComputeState::create(mpDevice);
    mpSampleGenerator = SampleGenerator::create(mpDevice, SAMPLE_GENERATOR_UNIFORM);

    mFrameDim = uint2(0);
    mFrameIndex = 0u;

    mSampleCount = 8u;
    mDepthThreshold = 0.01f;
    mNormalThreshold = 0.99f;

    for (const auto& [key, value] : props)
    {
        if (key == "sampleCount")           mSampleCount = value;
        else if (key == "depthThreshold")   mDepthThreshold = value;
        else if (key == "normalThreshold")  mNormalThreshold = value;
    }
}

RenderPassReflection DeferredMSAA::reflect(const CompileData& compileData)
{
    RenderPassReflection reflection;
    const uint2 sz = RenderPassHelpers::calculateIOSize(RenderPassHelpers::IOSize::Default, uint2(0), compileData.defaultTexDims);

    reflection.addInput("complex", "complex pixel mask").format(ResourceFormat::R16Float);
    reflection.addInput("packedHitInfo", "packed hit info")
        .format(ResourceFormat::RGBA32Uint)
        .bindFlags(ResourceBindFlags::RenderTarget | ResourceBindFlags::ShaderResource)
        .texture2D(sz.x, sz.y, mSampleCount);

    reflection.addOutput("output", "output texture")
        .format(ResourceFormat::RGBA32Float)
        .bindFlags(ResourceBindFlags::UnorderedAccess | ResourceBindFlags::RenderTarget);
    reflection.addOutput("edge", "edge pixel mask").format(ResourceFormat::R16Float).bindFlags(ResourceBindFlags::UnorderedAccess);

    if (math::any(mFrameDim != compileData.defaultTexDims))
        mFrameDim = compileData.defaultTexDims;

    return reflection;
}

void DeferredMSAA::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pComplex = renderData.getTexture("complex");
    const auto& pPackedHitInfo = renderData.getTexture("packedHitInfo");
    const auto& pOutput = renderData.getTexture("output");
    const auto& pEdge = renderData.getTexture("edge");

    pRenderContext->clearUAV(pOutput->getUAV().get(), float4(0));

    if (mpScene)
    {
        auto var = mpVars->getRootVar();
        mpScene->bindShaderDataForRaytracing(pRenderContext, var["gScene"]);

        var["CB"]["gResolution"] = mFrameDim;
        var["CB"]["gFrameIndex"] = mFrameIndex;
        var["CB"]["gDepthThreshold"] = mDepthThreshold;
        var["CB"]["gNormalThreshold"] = mNormalThreshold;

        var["gComplex"] = pComplex;
        var["gPackedHitInfo"] = pPackedHitInfo;
        var["gOutput"] = pOutput;
        var["gEdge"] = pEdge;

        uint3 threadGroupSize = mpProgram->getReflector()->getThreadGroupSize();
        uint3 groups = div_round_up(uint3(renderData.getDefaultTextureDims(), 1), threadGroupSize);
        pRenderContext->dispatch(mpState.get(), mpVars.get(), groups);
    }

    mFrameIndex++;
}

void DeferredMSAA::renderUI(Gui::Widgets& widget)
{
    widget.text("Sample Count: " + std::to_string(mSampleCount));
    widget.var("Depth Threshold", mDepthThreshold, 0.f, 0.01f);
    widget.var("Normal Threshold", mNormalThreshold, 0.f, 1.f);
}

void DeferredMSAA::setScene(RenderContext* pRenderContext, const ref<Scene>& pScene)
{
    mpScene = pScene;

    if (mpScene)
    {
        ProgramDesc desc;
        desc.addShaderModules(mpScene->getShaderModules());
        desc.addShaderLibrary("RenderPasses/MSAA/DeferredMSAA.cs.slang").csEntry("csMain");
        desc.addTypeConformances(mpScene->getTypeConformances());

        mpProgram = Program::create(mpDevice, desc, mpScene->getSceneDefines());
        mpProgram->addDefines(mpSampleGenerator->getDefines());
        mpProgram->addDefine("SAMPLE_COUNT", std::to_string(mSampleCount));

        mpState->setProgram(mpProgram);

        mpVars = ProgramVars::create(mpDevice, mpProgram.get());
        mpSampleGenerator->bindShaderData(mpVars->getRootVar());
    }
}
