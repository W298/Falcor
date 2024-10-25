#pragma once
#include "Falcor.h"
#include "RenderGraph/RenderPass.h"

using namespace Falcor;

/**
 * Multi Sampling AA class
 */
class MSAA : public RenderPass
{
public:
    FALCOR_PLUGIN_CLASS(MSAA, "MSAA", "Multi Sampling Anti-Aliasing");

    static ref<MSAA> create(ref<Device> pDevice, const Properties& props) { return make_ref<MSAA>(pDevice, props); }

    MSAA(ref<Device> pDevice, const Properties& props);

    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const ref<Scene>& pScene) override;

private:
    void createFrameDimDependentResources();

    struct
    {
        ref<GraphicsState>  pState;
        ref<Program>        pProgram;
        ref<ProgramVars>    pVars;
    } mRenderPass;

    ref<Scene>              mpScene;

    ref<Fbo>                mpFbo;
    ref<RasterizerState>    mpRasterState;

    ref<Texture>            mpResolvedTexture;
    ref<Sampler>            mpLinearSampler;

    uint2                   mFrameDim;
    bool                    mIsResourceDirty;

    uint32_t                mSampleCount;
};
