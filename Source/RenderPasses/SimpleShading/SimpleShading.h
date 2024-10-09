#pragma once
#include "Falcor.h"
#include "RenderGraph/RenderPass.h"
#include "Utils/Sampling/SampleGenerator.h"

using namespace Falcor;

/**
 * Deferred Simple Shading
 */
class SimpleShading : public RenderPass
{
public:
    FALCOR_PLUGIN_CLASS(SimpleShading, "SimpleShading", "Deferred Simple Shading");

    static ref<SimpleShading> create(ref<Device> pDevice, const Properties& props) { return make_ref<SimpleShading>(pDevice, props); }

    SimpleShading(ref<Device> pDevice, const Properties& props);

    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void setScene(RenderContext* pRenderContext, const ref<Scene>& pScene) override;

private:
    ref<Scene>              mpScene;

    ref<ComputeState>       mpState;
    ref<Program>            mpProgram;
    ref<ProgramVars>        mpVars;

    ref<SampleGenerator>    mpSampleGenerator;

    uint2                   mFrameDim;
    uint                    mFrameIndex;
};
