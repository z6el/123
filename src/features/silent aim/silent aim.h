#pragma once

#include <src/include.h>

namespace silent_aim {
    inline bool  enabled         = false;
    inline bool  silentaim       = false;
    inline float fov             = 100.0f;
    inline bool  draw_fov        = true;
    inline bool  target_sleepers = false;
    inline bool  target_npcs     = false;

    void update();
    void render();
}
