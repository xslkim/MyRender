#pragma once
#include <string>

class Config {
public:
    // Render resolution. Runtime-configurable (not a compile-time constant) so the
    // interactive fly-through can drop to a lower internal resolution for heavy
    // scenes and let SDL stretch it to the window. Must be set BEFORE Render::Init
    // (i.e. before the first Scene::Load*). Defaults match the tutorial framebuffer.
    inline static int kScreenWidth  = 960;
    inline static int kScreenHeight = 540;
    inline static std::string scene_path;
};
