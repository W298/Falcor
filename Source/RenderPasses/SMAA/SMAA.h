#pragma once
#include "Falcor.h"
#include "RenderGraph/RenderPass.h"
#include "Core/Pass/FullScreenPass.h"

using namespace Falcor;

/**
 * SMAA
 */
class SMAA : public RenderPass
{
public:
    FALCOR_PLUGIN_CLASS(SMAA, "SMAA", "SMAA");

    static ref<SMAA> create(ref<Device> pDevice, const Properties& props) { return make_ref<SMAA>(pDevice, props); }

    SMAA(ref<Device> pDevice, const Properties& props);

    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;

private:
    void recompile();
    void createEdgeDetectionPass();

    ref<FullScreenPass>     mpEdgeDetectionPass;
    ref<Fbo>                mpFbo;

    ref<Sampler>            mpLinearSampler;
    ref<Sampler>            mpPointSampler;

    uint2                   mFrameDim;
    uint                    mFrameIndex;
};
