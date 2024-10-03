#include "RenderGraph/RenderPass.h"
#include "MSAA.h"
#include "DeferredMSAA.h"

extern "C" FALCOR_API_EXPORT void registerPlugin(Falcor::PluginRegistry& registry)
{
    registry.registerClass<RenderPass, MSAA>();
    registry.registerClass<RenderPass, DeferredMSAA>();
}
