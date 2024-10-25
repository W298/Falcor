add_plugin(MultiSampledGBuffer)

target_sources(MultiSampledGBuffer PRIVATE
    MultiSampledGBuffer.cpp
    MultiSampledGBuffer.h
    MultiSampledGBuffer.3d.slang
)

target_copy_shaders(MultiSampledGBuffer RenderPasses/MultiSampledGBuffer)

target_source_group(MultiSampledGBuffer "RenderPasses")
