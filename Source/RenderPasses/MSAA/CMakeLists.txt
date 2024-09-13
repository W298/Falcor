add_plugin(MSAA)

target_sources(MSAA PRIVATE
    MSAA.cpp
    MSAA.h
    MSAA.3d.slang
)

target_copy_shaders(MSAA RenderPasses/MSAA)

target_source_group(MSAA "RenderPasses")
