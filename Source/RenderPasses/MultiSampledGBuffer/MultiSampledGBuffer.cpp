#include "MultiSampledGBuffer.h"

extern "C" FALCOR_API_EXPORT void registerPlugin(Falcor::PluginRegistry& registry)
{
    registry.registerClass<RenderPass, MultiSampledGBuffer>();
}

MultiSampledGBuffer::MultiSampledGBuffer(ref<Device> pDevice, const Properties& props) : RenderPass(pDevice)
{
    mpState = GraphicsState::create(mpDevice);
    mpFbo = Fbo::create(mpDevice);

    mSampleCount = 8u;
    if (props.has("sampleCount"))
        mSampleCount = props["sampleCount"];
}

RenderPassReflection MultiSampledGBuffer::reflect(const CompileData& compileData)
{
    RenderPassReflection reflector;
    const uint2 sz = RenderPassHelpers::calculateIOSize(RenderPassHelpers::IOSize::Default, uint2(0), compileData.defaultTexDims);

    reflector.addInternal("msComplex", "multi-sample complex")
        .format(ResourceFormat::R16Float)
        .bindFlags(ResourceBindFlags::RenderTarget)
        .texture2D(sz.x, sz.y, mSampleCount);
    reflector.addInternal("msPackedHitInfo", "multi-sample packedHitInfo")
        .format(ResourceFormat::RGBA32Uint)
        .bindFlags(ResourceBindFlags::RenderTarget | ResourceBindFlags::ShaderResource)
        .texture2D(sz.x, sz.y, mSampleCount);
    reflector.addInternal("msDepth", "multi-sample depth")
        .format(ResourceFormat::D32Float)
        .bindFlags(ResourceBindFlags::DepthStencil)
        .texture2D(sz.x, sz.y, mSampleCount);

    reflector.addOutput("complex", "complex pixel mask").format(ResourceFormat::R16Float);
    reflector.addOutput("packedHitInfo", "packed hit info")
        .format(ResourceFormat::RGBA32Uint)
        .bindFlags(ResourceBindFlags::RenderTarget)
        .texture2D(sz.x, sz.y, mSampleCount);

    return reflector;
}

void MultiSampledGBuffer::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pComplex = renderData.getTexture("complex");
    const auto& pPackedHitInfo = renderData.getTexture("packedHitInfo");

    mpFbo->attachColorTarget(renderData.getTexture("msComplex"), 0);
    mpFbo->attachColorTarget(renderData.getTexture("msPackedHitInfo"), 1);
    mpFbo->attachDepthStencilTarget(renderData.getTexture("msDepth"));

    pRenderContext->clearFbo(mpFbo.get(), float4(0), 1.f, 0, FboAttachmentType::All);
    mpState->setFbo(mpFbo);

    if (mpScene)
        mpScene->rasterize(pRenderContext, mpState.get(), mpVars.get());

    pRenderContext->resolveResource(mpFbo->getColorTexture(0), pComplex);
    pRenderContext->blit(mpFbo->getColorTexture(1)->getSRV(), pPackedHitInfo->getRTV());
}

void MultiSampledGBuffer::renderUI(Gui::Widgets& widget)
{
    widget.text("Sample Count: " + std::to_string(mSampleCount));
}

void MultiSampledGBuffer::setScene(RenderContext* pRenderContext, const ref<Scene>& pScene)
{
    mpScene = pScene;

    if (mpScene)
    {
        ProgramDesc desc;
        desc.addShaderModules(mpScene->getShaderModules());
        desc.addShaderLibrary("RenderPasses/MultiSampledGBuffer/MultiSampledGBuffer.3d.slang").vsEntry("vsMain").psEntry("psMain");
        desc.addTypeConformances(mpScene->getTypeConformances());

        mpProgram = Program::create(mpDevice, desc, mpScene->getSceneDefines());
        mpProgram->addDefine("SAMPLE_COUNT", std::to_string(mSampleCount));

        mpState->setProgram(mpProgram);

        mpVars = ProgramVars::create(mpDevice, mpProgram.get());
    }
}
