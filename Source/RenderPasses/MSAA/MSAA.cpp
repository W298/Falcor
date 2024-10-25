#include "MSAA.h"
#include "RenderGraph/RenderPassHelpers.h"

namespace
{
const std::string kColorOut = "output";
const std::string kShaderFilename = "RenderPasses/MSAA/MSAA.3d.slang";
} // namespace

MSAA::MSAA(ref<Device> pDevice, const Properties& props) : RenderPass(pDevice)
{
    // Init graphics state.
    mRenderPass.pState = GraphicsState::create(mpDevice);

    // Set rasterizer function.
    RasterizerState::Desc desc;
    desc.setFillMode(RasterizerState::FillMode::Solid);
    desc.setCullMode(RasterizerState::CullMode::Back);
    mpRasterState = RasterizerState::create(desc);

    mRenderPass.pState->setRasterizerState(mpRasterState);

    // Create FBO.
    mpFbo = Fbo::create(mpDevice);

    // Create sampler.
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Linear);
    mpLinearSampler = mpDevice->createSampler(samplerDesc);

    mFrameDim = uint2(0);
    mIsResourceDirty = true;

    mSampleCount = 4u;
}

RenderPassReflection MSAA::reflect(const CompileData& compileData)
{
    RenderPassReflection reflection;
    reflection.addOutput(kColorOut, "output texture");

    if (math::any(mFrameDim != compileData.defaultTexDims))
    {
        mIsResourceDirty = true;
        mFrameDim = compileData.defaultTexDims;
    }

    return reflection;
}

void MSAA::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pOutput = renderData.getTexture(kColorOut);

    if (mIsResourceDirty)
    {
        createFrameDimDependentResources();
        mIsResourceDirty = false;
    }

    pRenderContext->clearFbo(mpFbo.get(), float4(0), 1.f, 0, FboAttachmentType::All);
    mRenderPass.pState->setFbo(mpFbo);

    if (mpScene)
        mpScene->rasterize(pRenderContext, mRenderPass.pState.get(), mRenderPass.pVars.get(), mpRasterState, mpRasterState);

    pRenderContext->resolveResource(mpFbo->getColorTexture(0), mpResolvedTexture);
    pRenderContext->blit(mpResolvedTexture->getSRV(), pOutput->getRTV());
}

void MSAA::renderUI(Gui::Widgets& widget)
{
    Falcor::Gui::DropdownList list = { { 4u, "4" }, { 8u, "8" } };
    if (widget.dropdown("Sample Count", list, mSampleCount))
    {
        mIsResourceDirty = true;
    }
}

void MSAA::setScene(RenderContext* pRenderContext, const ref<Scene>& pScene)
{
    mpScene = pScene;

    if (mpScene)
    {
        // Create program.
        ProgramDesc desc;
        desc.addShaderModules(mpScene->getShaderModules());
        desc.addShaderLibrary("RenderPasses/MSAA/MSAA.3d.slang").vsEntry("vsMain").psEntry("psMain");
        desc.addTypeConformances(mpScene->getTypeConformances());

        mRenderPass.pProgram = Program::create(mpDevice, desc, mpScene->getSceneDefines());
        mRenderPass.pState->setProgram(mRenderPass.pProgram);

        // Create program vars.
        mRenderPass.pVars = ProgramVars::create(mpDevice, mRenderPass.pProgram.get());
    }
}

void MSAA::createFrameDimDependentResources()
{
    mpFbo->attachColorTarget(
        mpDevice->createTexture2DMS(
            mFrameDim.x,
            mFrameDim.y,
            ResourceFormat::RGBA32Float,
            mSampleCount,
            1,
            ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
        ),
        0
    );

    mpFbo->attachDepthStencilTarget(mpDevice->createTexture2DMS(
        mFrameDim.x, mFrameDim.y, ResourceFormat::D32Float, mSampleCount, 1, ResourceBindFlags::DepthStencil
    ));

    mpResolvedTexture = mpDevice->createTexture2D(mFrameDim.x, mFrameDim.y, ResourceFormat::RGBA32Float, 1, 1);
}
