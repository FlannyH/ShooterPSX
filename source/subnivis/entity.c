#include "entity.h"

#include "entities/chaser.h"
#include "entities/cinecam.h"
#include "entities/crate.h"
#include "entities/door.h"
#include "entities/pickup.h"
#include "entities/platform.h"
#include "entities/trigger.h"

#ifdef _LEVEL_EDITOR
    #define REGISTER_ENTITY(name, type) \
        entity_register_type(ENTITY_##name, (entity_reg_t){ \
            sizeof(entity_##type##_t), \
            entity_##type##_new, \
            entity_##type##_deserialize, \
            entity_##type##_serialize, \
            entity_##type##_update, \
            entity_##type##_on_hit, \
            entity_##type##_player_intersect, \
            entity_##type##_inspect \
        })
#else
    #define REGISTER_ENTITY(name, type) \
        entity_register_type(ENTITY_##name, (entity_reg_t){ \
            sizeof(entity_##type##_t), \
            entity_##type##_new, \
            entity_##type##_deserialize, \
            entity_##type##_serialize, \
            entity_##type##_update, \
            entity_##type##_on_hit, \
            entity_##type##_player_intersect \
        })
#endif

void subnivis_register_entities(void) {
    REGISTER_ENTITY(DOOR, door);
    REGISTER_ENTITY(PICKUP, pickup);
    REGISTER_ENTITY(CRATE, crate);
    REGISTER_ENTITY(CHASER, chaser);
    REGISTER_ENTITY(PLATFORM, platform);
    REGISTER_ENTITY(TRIGGER, trigger);
    REGISTER_ENTITY(CINECAM, cinecam);
}

#undef REGISTER_ENTITY
