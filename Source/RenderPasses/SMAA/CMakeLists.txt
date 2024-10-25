add_plugin(SMAA)

target_sources(SMAA PRIVATE
    SMAA.cpp
    SMAA.h
    SMAA.slang
    SMAA.hlsli
)

target_copy_shaders(SMAA RenderPasses/SMAA)

target_source_group(SMAA "RenderPasses")
