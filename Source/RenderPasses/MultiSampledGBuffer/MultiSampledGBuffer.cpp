#include "MultiSampledGBuffer.h"

extern "C" FALCOR_API_EXPORT void registerPlugin(Falcor::PluginRegistry& registry)
{
    registry.registerClass<RenderPass, MultiSampledGBuffer>();
}

MultiSampledGBuffer::MultiSampledGBuffer(ref<Device> pDevice, const Properties& props) : RenderPass(pDevice)
{
    mpState = GraphicsState::create(mpDevice);
    mpFbo = Fbo::create(mpDevice);

    mFrameDim = uint2(0);
    mIsResourceDirty = true;

    mSampleCount = 4u;
}

RenderPassReflection MultiSampledGBuffer::reflect(const CompileData& compileData)
{
    RenderPassReflection reflector;

    reflector.addOutput("complex", "complex pixel mask").format(ResourceFormat::R16Float);
    reflector.addOutput("packedHitInfo", "packed hit info")
        .format(ResourceFormat::RGBA32Uint)
        .bindFlags(ResourceBindFlags::RenderTarget)
        .texture2D(1920, 1080, mSampleCount);

    if (math::any(mFrameDim != compileData.defaultTexDims))
    {
        mIsResourceDirty = true;
        mFrameDim = compileData.defaultTexDims;
    }

    return reflector;
}

void MultiSampledGBuffer::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pComplex = renderData.getTexture("complex");
    const auto& pPackedHitInfo = renderData.getTexture("packedHitInfo");

    if (mIsResourceDirty)
    {
        createFrameDimDependentResources();
        mpFbo->attachColorTarget(pPackedHitInfo, 1);
        mIsResourceDirty = false;
    }

    pRenderContext->clearFbo(mpFbo.get(), float4(0), 1.f, 0, FboAttachmentType::All);
    mpState->setFbo(mpFbo);

    if (mpScene)
        mpScene->rasterize(pRenderContext, mpState.get(), mpVars.get());

    pRenderContext->resolveResource(mpFbo->getColorTexture(0), pComplex);
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
        mpState->setProgram(mpProgram);

        mpVars = ProgramVars::create(mpDevice, mpProgram.get());
    }
}

void MultiSampledGBuffer::createFrameDimDependentResources()
{
    mpFbo->attachColorTarget(
        mpDevice->createTexture2DMS(mFrameDim.x, mFrameDim.y, ResourceFormat::R16Float, mSampleCount, 1, ResourceBindFlags::RenderTarget), 0
    );

    mpFbo->attachDepthStencilTarget(
        mpDevice->createTexture2DMS(mFrameDim.x, mFrameDim.y, ResourceFormat::D32Float, mSampleCount, 1, ResourceBindFlags::DepthStencil)
    );
}
