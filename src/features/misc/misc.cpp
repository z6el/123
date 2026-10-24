#include "misc.h"
#include <src/features/esp/esp.h>

namespace misc {

    void update() 
    {
        if (g_local_player) {
            if (spider_man) {
                uint64_t movement = m.read<uint64_t>(g_local_player + offsets::base_player::movement);
                if (movement) {
                    m.write<float>(movement + 0x108, 0.0f);
                }
            }

            if (admin_flags || third_person) {
                int flags = m.read<int>(g_local_player + offsets::base_player::playerFlags);
                if (!(flags & 64) && !(flags & 16)) { // Not wounded and not sleeping
                    if (admin_flags) flags |= 4;
                    if (third_person) flags |= 1024;
                    m.write<int>(g_local_player + offsets::base_player::playerFlags, flags);
                }
            }
        }

        if (bright_night) {
            uint64_t tod_sky_type = m.read<uint64_t>(globals::game_assembly + offsets::tod_sky);
            if (tod_sky_type) {
                uint64_t static_fields = m.read<uint64_t>(tod_sky_type + offsets::tod_sky_class::static_fields);
                if (static_fields) {
                    uint64_t instances = m.read<uint64_t>(static_fields + 0x0); // List of TOD_Sky
                    if (instances) {
                        uint64_t list = m.read<uint64_t>(instances + 0x10);
                        int count = m.read<int>(instances + 0x18);
                        if (list && count > 0) {
                            uint64_t sky = m.read<uint64_t>(list + 0x20); // First TOD_Sky
                            if (sky) {
                                uint64_t night_params = m.read<uint64_t>(sky + offsets::tod_sky_class::TOD_NightParameters);
                                uint64_t ambient_params = m.read<uint64_t>(sky + offsets::tod_sky_class::TOD_AmbientParameters);
                                
                                if (night_params) {
                                    m.write<float>(night_params + 0x10, 13.0f); // AmbientMultiplier
                                }
                                if (ambient_params) {
                                    m.write<float>(ambient_params + offsets::tod_ambient_parameters::Saturation, 0.0f);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}
