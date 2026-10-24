#ifndef offset_h
#define offset_h

#include <src/include.h>

using offset = const uint32_t;

namespace offsets {
    offset il2cpp_handle_base = 0xdb6ee20;
    offset base_networkable = 0xd81ab88;
    offset base_camera = 0xd85feb8;
    offset tod_sky = 0xd8a7230;
    offset graphics_typeinfo = 0xd883e80;

    namespace base_networkable_class {
        offset static_fields = 0xB8;
        offset wrapper_fields = 0x10;
        offset parent_static_fields = 0x10;
        offset entity = 0x20;
    }

    namespace entity_list {
        offset array = 0x10;
        offset size = 0x18;
    }

    namespace base_camera_class {
        offset static_fields = 0xB8;
        offset wrapper_class = 0x90;
        offset parent_static_fields = 0x10;
        offset matrix = 0x30C;
        offset position = 0x454;
    }

    namespace tod_ambient_parameters {
        offset mode = 0x10;
        offset Saturation = 0x14;
        offset UpdateInterval = 0x18;
    }

    namespace tod_sky_class {
        offset static_fields = 0xB8;
        offset wrapper_class = 0xC0;
        offset parent_static_fields = 0x10;
        offset TOD_AmbientParameters = 0x98;  // updated
        offset TOD_NightParameters = 0x60;

        namespace ambient_params {
            offset update_int = 0x18;
        }

        offset TOD_CycleParameters = 0x40;
        namespace cycle_params {
            offset time = 0x10;
        }
    }

    namespace base_entity {
        offset bounds = 0xC0;
        offset model = 0xE8;   // BaseEntity::Model (_Model)
    }

    namespace base_combat_entity {
        offset lifestate = 0x268;
        offset _health = 0x274;
        offset _maxHealth = 0x278;
    }

    namespace base_player {
        offset currentTeam = 0x4B0;   // CurrentTeam (playerTeam)
        offset playerFlags = 0x620;
        offset playerModel = 0x650;   // PlayerModel
        offset inventoryValue = 0x468;   // PlayerInventory
        offset clActiveItem = 0x4E0;
        offset _displayName = 0x660;
        offset eyesValue = 0x300;   // PlayerEyes
        offset input = 0x350;   // PlayerInput
        offset movement = 0x2E0;   // BaseMovement (PlayerMovement)
    }

    namespace player_model {
        offset position = 0x1F8;
        offset _multiMesh = 0x4B0;   // SkinnedMultiMesh (keep for compatibility)
        offset new_velocity = 0x21C;
        offset InGesture = 0x2A0;
        offset CurrentGestureConfig = 0x2A8;
    }

    namespace player_eyes {
        offset eye_offset = 0x400;
        offset eye_rotation = 0x6C;
        offset body_rotation = 0x50;
        offset view_offset = 0x40;
    }

    namespace graphic {
        offset fov = 0x5C4;   // Convar.Graphics._fov
    }

    namespace base_projectile {
        offset recoil = 0x3A0;
        offset automatic = 0x330;
        offset primaryMagazine = 0x378;
        offset sightAimConeScale = 0x40C;
        offset hipAimConeScale = 0x414;
    }

    namespace player_inventory {
        offset Belt = 0x28;
        offset Wear = 0x60;
        offset ItemList = 0x20;
        offset ItemDefinition = 0xC8;
    }

    namespace model {
        offset boneTransforms = 0x50;
    }

    namespace base_movement {
        offset waterLevel = 0x24;
        offset gravityMultiplier = 0x84;
        offset capsuleHeight = 0x68;
        offset capsuleCenter = 0x6C;
        offset clothingMoveSpeedReduction = 0x708;
    }
}

#endif // !offset_h
