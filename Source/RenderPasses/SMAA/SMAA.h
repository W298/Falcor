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
    void loadImage();
    void recompile();
    void createPasses();

    ref<FullScreenPass>     mpEdgeDetectionPass;
    ref<FullScreenPass>     mpBlendingWeightCalcPass;
    ref<FullScreenPass>     mpNeighborhoodBlendingPass;

    ref<Fbo>                mpFbo;

    ref<Sampler>            mpLinearSampler;
    ref<Sampler>            mpPointSampler;

    ref<Texture>            mpAreaTexture;
    ref<Texture>            mpSearchTexture;

    uint2                   mFrameDim;
    uint                    mFrameIndex;
};
