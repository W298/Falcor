#include "SMAA.h"
#include "RenderGraph/RenderPassHelpers.h"
#include "Core/AssetResolver.h"

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

    loadImage();

    mFrameDim = uint2(0);
    mFrameIndex = 0u;
}

RenderPassReflection SMAA::reflect(const CompileData& compileData)
{
    RenderPassReflection reflection;
    const uint2 sz = RenderPassHelpers::calculateIOSize(RenderPassHelpers::IOSize::Default, uint2(0), compileData.defaultTexDims);

    reflection.addInput("color", "color texture").format(ResourceFormat::RGBA32Float).bindFlags(ResourceBindFlags::ShaderResource);

    reflection.addOutput("edge", "edge texture")
        .format(ResourceFormat::RG32Float)
        .bindFlags(ResourceBindFlags::RenderTarget | ResourceBindFlags::ShaderResource);
    reflection.addOutput("blend", "blending weight texture")
        .format(ResourceFormat::RGBA32Float)
        .bindFlags(ResourceBindFlags::RenderTarget | ResourceBindFlags::ShaderResource);

    reflection.addOutput("output", "output texture").format(ResourceFormat::RGBA32Float).bindFlags(ResourceBindFlags::RenderTarget);

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

    const auto& pEdge = renderData.getTexture("edge");
    const auto& pBlend = renderData.getTexture("blend");

    const auto& pOutput = renderData.getTexture("output");

    if (mpEdgeDetectionPass)
    {
        pRenderContext->clearRtv(pEdge->getRTV().get(), float4(0.f));

        auto var = mpEdgeDetectionPass->getRootVar();

        var["gColor"] = pColor;

        var["LinearSampler"] = mpLinearSampler;
        var["PointSampler"] = mpPointSampler;

        mpFbo->attachColorTarget(pEdge, 0);
        mpEdgeDetectionPass->execute(pRenderContext, mpFbo);
    }

    if (mpBlendingWeightCalcPass)
    {
        pRenderContext->clearRtv(pBlend->getRTV().get(), float4(0.f));

        auto var = mpBlendingWeightCalcPass->getRootVar();

        var["gArea"] = mpAreaTexture;
        var["gSearch"] = mpSearchTexture;
        var["gEdge"] = pEdge;

        mpFbo->attachColorTarget(pBlend, 0);
        mpBlendingWeightCalcPass->execute(pRenderContext, mpFbo);
    }

    if (mpNeighborhoodBlendingPass)
    {
        pRenderContext->clearRtv(pOutput->getRTV().get(), float4(0.f));

        auto var = mpNeighborhoodBlendingPass->getRootVar();

        var["gColor"] = pColor;
        var["gBlend"] = pBlend;

        mpFbo->attachColorTarget(pOutput, 0);
        mpNeighborhoodBlendingPass->execute(pRenderContext, mpFbo);
    }

    mFrameIndex++;
}

void SMAA::loadImage()
{
    {
        std::filesystem::path resolvedPath =
            AssetResolver::getDefaultResolver().resolvePath("F:\\Falcor\\Source\\RenderPasses\\SMAA\\Textures\\AreaTexDX10.png");
        if (std::filesystem::exists(resolvedPath))
        {
            mpAreaTexture = Texture::createFromFile(mpDevice, resolvedPath, true, false, ResourceBindFlags::ShaderResource);
        }
        else
        {
            msgBox("Error", fmt::format("Failed to load image"), MsgBoxType::Ok, MsgBoxIcon::Warning);
        }
    }

    {
        std::filesystem::path resolvedPath =
            AssetResolver::getDefaultResolver().resolvePath("F:\\Falcor\\Source\\RenderPasses\\SMAA\\Textures\\SearchTex.png");
        if (std::filesystem::exists(resolvedPath))
        {
            mpSearchTexture = Texture::createFromFile(mpDevice, resolvedPath, true, false, ResourceBindFlags::ShaderResource);
        }
        else
        {
            msgBox("Error", fmt::format("Failed to load image"), MsgBoxType::Ok, MsgBoxIcon::Warning);
        }
    }
}

void SMAA::recompile()
{
    createPasses();
    requestRecompile();
}

void SMAA::createPasses()
{
    {
        ProgramDesc desc;
        desc.addShaderLibrary("RenderPasses/SMAA/SMAA.slang").vsEntry("edgeDetectionVS").psEntry("edgeDetectionPS");

        DefineList defines;
        defines.add(
            "SMAA_RT_METRICS",
            "float4(1.0 / " + std::to_string(mFrameDim.x) + ", 1.0 / " + std::to_string(mFrameDim.y) + ", " + std::to_string(mFrameDim.x) +
                ", " + std::to_string(mFrameDim.y) + ")"
        );

        mpEdgeDetectionPass = FullScreenPass::create(mpDevice, desc, defines);
    }

    {
        ProgramDesc desc;
        desc.addShaderLibrary("RenderPasses/SMAA/SMAA.slang").vsEntry("blendingWeightCalculationVS").psEntry("blendingWeightCalculationPS");

        DefineList defines;
        defines.add(
            "SMAA_RT_METRICS",
            "float4(1.0 / " + std::to_string(mFrameDim.x) + ", 1.0 / " + std::to_string(mFrameDim.y) + ", " + std::to_string(mFrameDim.x) +
                ", " + std::to_string(mFrameDim.y) + ")"
        );

        mpBlendingWeightCalcPass = FullScreenPass::create(mpDevice, desc, defines);
    }

    {
        ProgramDesc desc;
        desc.addShaderLibrary("RenderPasses/SMAA/SMAA.slang").vsEntry("neighborhoodBlendingVS").psEntry("neighborhoodBlendingPS");

        DefineList defines;
        defines.add(
            "SMAA_RT_METRICS",
            "float4(1.0 / " + std::to_string(mFrameDim.x) + ", 1.0 / " + std::to_string(mFrameDim.y) + ", " + std::to_string(mFrameDim.x) +
                ", " + std::to_string(mFrameDim.y) + ")"
        );

        mpNeighborhoodBlendingPass = FullScreenPass::create(mpDevice, desc, defines);
    }
}
