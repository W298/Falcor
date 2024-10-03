#pragma once
#include "Falcor.h"
#include "RenderGraph/RenderPass.h"
#include "Utils/Sampling/SampleGenerator.h"

using namespace Falcor;

/**
 * Deferred Multi Sampling AA class
 */
class DeferredMSAA : public RenderPass
{
public:
    FALCOR_PLUGIN_CLASS(DeferredMSAA, "DeferredMSAA", "Deferred Multi Sampling Anti-Aliasing");

    static ref<DeferredMSAA> create(ref<Device> pDevice, const Properties& props) { return make_ref<DeferredMSAA>(pDevice, props); }

    DeferredMSAA(ref<Device> pDevice, const Properties& props);

    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const ref<Scene>& pScene) override;

private:
    ref<Scene>              mpScene;

    ref<ComputeState>       mpState;
    ref<Program>            mpProgram;
    ref<ProgramVars>        mpVars;

    ref<SampleGenerator>    mpSampleGenerator;

    uint2                   mFrameDim;
    uint                    mFrameIndex;

    uint                    mSampleCount;
    float                   mDepthThreshold;
    float                   mNormalThreshold;
};
