#include "SimpleShading.h"
#include "RenderGraph/RenderPassHelpers.h"

extern "C" FALCOR_API_EXPORT void registerPlugin(Falcor::PluginRegistry& registry)
{
    registry.registerClass<RenderPass, SimpleShading>();
}

SimpleShading::SimpleShading(ref<Device> pDevice, const Properties& props) : RenderPass(pDevice)
{
    mpState = ComputeState::create(mpDevice);
    mpSampleGenerator = SampleGenerator::create(mpDevice, SAMPLE_GENERATOR_UNIFORM);

    mFrameDim = uint2(0);
    mFrameIndex = 0u;
}

RenderPassReflection SimpleShading::reflect(const CompileData& compileData)
{
    RenderPassReflection reflection;
    const uint2 sz = RenderPassHelpers::calculateIOSize(RenderPassHelpers::IOSize::Default, uint2(0), compileData.defaultTexDims);

    reflection.addInput("packedHitInfo", "packed hit info").format(ResourceFormat::RGBA32Uint).bindFlags(ResourceBindFlags::ShaderResource);
    reflection.addOutput("output", "output texture").format(ResourceFormat::RGBA32Float).bindFlags(ResourceBindFlags::UnorderedAccess);

    if (math::any(mFrameDim != compileData.defaultTexDims))
        mFrameDim = compileData.defaultTexDims;

    return reflection;
}

void SimpleShading::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pPackedHitInfo = renderData.getTexture("packedHitInfo");
    const auto& pOutput = renderData.getTexture("output");

    pRenderContext->clearUAV(pOutput->getUAV().get(), float4(0));

    if (mpScene)
    {
        auto var = mpVars->getRootVar();
        mpScene->bindShaderDataForRaytracing(pRenderContext, var["gScene"]);

        var["CB"]["gResolution"] = mFrameDim;
        var["CB"]["gFrameIndex"] = mFrameIndex;

        var["gPackedHitInfo"] = pPackedHitInfo;
        var["gOutput"] = pOutput;

        uint3 threadGroupSize = mpProgram->getReflector()->getThreadGroupSize();
        uint3 groups = div_round_up(uint3(renderData.getDefaultTextureDims(), 1), threadGroupSize);
        pRenderContext->dispatch(mpState.get(), mpVars.get(), groups);
    }

    mFrameIndex++;
}

void SimpleShading::setScene(RenderContext* pRenderContext, const ref<Scene>& pScene)
{
    mpScene = pScene;

    if (mpScene)
    {
        ProgramDesc desc;
        desc.addShaderModules(mpScene->getShaderModules());
        desc.addShaderLibrary("RenderPasses/SimpleShading/SimpleShading.cs.slang").csEntry("csMain");
        desc.addTypeConformances(mpScene->getTypeConformances());

        mpProgram = Program::create(mpDevice, desc, mpScene->getSceneDefines());
        mpProgram->addDefines(mpSampleGenerator->getDefines());

        mpState->setProgram(mpProgram);

        mpVars = ProgramVars::create(mpDevice, mpProgram.get());
        mpSampleGenerator->bindShaderData(mpVars->getRootVar());
    }
}
