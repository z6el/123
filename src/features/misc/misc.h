#pragma once

#include <src/include.h>

namespace misc {
    
    inline bool no_recoil = false;
    inline bool admin_flags = false;
    inline bool spider_man = false;
    inline bool bright_night = false;
    inline bool third_person = false;

    /* Call throttled or per-frame based on what is updating */
    void update();
}
