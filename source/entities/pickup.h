#ifndef PICKUP_H
#define PICKUP_H
#include "../entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PICKUP_TYPE_NONE,
	PICKUP_TYPE_AMMO_SMALL,
	PICKUP_TYPE_AMMO_BIG,
	PICKUP_TYPE_ARMOR_SMALL,
	PICKUP_TYPE_ARMOR_BIG,
	PICKUP_TYPE_HEALTH_SMALL,
	PICKUP_TYPE_HEALTH_BIG,
	PICKUP_TYPE_KEY_BLUE,
	PICKUP_TYPE_KEY_YELLOW,
	PICKUP_TYPE_DAMAGE,
	PICKUP_TYPE_FIRE_RATE,
	PICKUP_TYPE_INVINCIBILITY,
} pickup_types;

#ifdef _LEVEL_EDITOR
extern const char* pickup_names[];
#endif

typedef struct {
	entity_header_t entity_header;
    uint8_t type;
} entity_pickup_t;

entity_pickup_t* entity_pickup_new(void);
void entity_pickup_update(int slot, player_t* player, int dt);
void entity_pickup_on_hit(int slot, int hitbox_index);
void entity_pickup_player_intersect(int slot, player_t* player);

#ifdef __cplusplus
}
#endif
#endif
