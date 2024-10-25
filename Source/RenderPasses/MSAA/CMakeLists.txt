add_plugin(MSAA)

target_sources(MSAA PRIVATE
    MSAA.cpp
    MSAA.h
    MSAA.3d.slang

    DeferredMSAA.cpp
    DeferredMSAA.h
    DeferredMSAA.cs.slang

    Base.cpp
)

target_copy_shaders(MSAA RenderPasses/MSAA)

target_source_group(MSAA "RenderPasses")
