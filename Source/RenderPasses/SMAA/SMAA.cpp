#include "SMAA.h"
#include "RenderGraph/RenderPassHelpers.h"

extern "C" FALCOR_API_EXPORT void registerPlugin(Falcor::PluginRegistry& registry)
{
    registry.registerClass<RenderPass, SMAA>();
}

SMAA::SMAA(ref<Device> pDevice, const Properties& props) : RenderPass(pDevice)
{
    mpFbo = Fbo::create(mpDevice);

    {
        Sampler::Desc desc;
        desc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Point);
        desc.setAddressingMode(TextureAddressingMode::Clamp, TextureAddressingMode::Clamp, TextureAddressingMode::Clamp);
        mpLinearSampler = mpDevice->createSampler(desc);
    }

    {
        Sampler::Desc desc;
        desc.setFilterMode(TextureFilteringMode::Point, TextureFilteringMode::Point, TextureFilteringMode::Point);
        desc.setAddressingMode(TextureAddressingMode::Clamp, TextureAddressingMode::Clamp, TextureAddressingMode::Clamp);
        mpPointSampler = mpDevice->createSampler(desc);
    }

    mFrameDim = uint2(0);
    mFrameIndex = 0u;
}

RenderPassReflection SMAA::reflect(const CompileData& compileData)
{
    RenderPassReflection reflection;
    const uint2 sz = RenderPassHelpers::calculateIOSize(RenderPassHelpers::IOSize::Default, uint2(0), compileData.defaultTexDims);

    reflection.addInput("color", "color texture").format(ResourceFormat::RGBA32Float).bindFlags(ResourceBindFlags::ShaderResource);
    reflection.addInput("depth", "depth texture").format(ResourceFormat::D32Float).bindFlags(ResourceBindFlags::ShaderResource);

    reflection.addOutput("edge", "edge texture").format(ResourceFormat::RG32Float).bindFlags(ResourceBindFlags::RenderTarget);

    if (math::any(mFrameDim != compileData.defaultTexDims))
    {
        mFrameDim = compileData.defaultTexDims;
        recompile();
    }

    return reflection;
}

void SMAA::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pColor = renderData.getTexture("color");
    const auto& pDepth = renderData.getTexture("depth");

    const auto& pEdge = renderData.getTexture("edge");

    if (mpEdgeDetectionPass)
    {
        pRenderContext->clearRtv(pEdge->getRTV().get(), float4(0.f));

        auto var = mpEdgeDetectionPass->getRootVar();

        var["gColor"] = pColor;
        var["gDepth"] = pDepth;

        var["LinearSampler"] = mpLinearSampler;
        var["PointSampler"] = mpPointSampler;

        mpFbo->attachColorTarget(pEdge, 0);
        mpEdgeDetectionPass->execute(pRenderContext, mpFbo);
    }

    mFrameIndex++;
}

void SMAA::recompile()
{
    createEdgeDetectionPass();
    requestRecompile();
}

void SMAA::createEdgeDetectionPass()
{
    ProgramDesc desc;
    desc.addShaderLibrary("RenderPasses/SMAA/SMAA.slang").vsEntry("edgeDetectionVS").psEntry("edgeDetectionPS");

    DefineList defines;
    defines.add("SMAA_RT_METRICS", "float4(1.0 / 1920.0, 1.0 / 1080.0, 1920.0, 1080.0)");

    mpEdgeDetectionPass = FullScreenPass::create(mpDevice, desc, defines);
}
