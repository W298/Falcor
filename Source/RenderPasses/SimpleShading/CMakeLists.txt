add_plugin(SimpleShading)

target_sources(SimpleShading PRIVATE
    SimpleShading.cpp
    SimpleShading.h
    SimpleShading.cs.slang
)

target_copy_shaders(SimpleShading RenderPasses/SimpleShading)

target_source_group(SimpleShading "RenderPasses")
