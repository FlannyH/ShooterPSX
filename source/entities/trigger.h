#ifndef TRIGGER_H
#define TRIGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../entity.h"
#include "../texture.h"

typedef enum {
    ENTITY_TRIGGER_TYPE_NONE,
    ENTITY_TRIGGER_TYPE_TEXT,
    ENTITY_TRIGGER_TYPE_SIGNAL,
    ENTITY_TRIGGER_TYPE_TELEPORT,
} entity_trigger_type_t;

#ifdef _LEVEL_EDITOR
extern const char* entity_trigger_type_names[];
#endif

typedef struct {
    entity_header_t entity_header;
    uint8_t trigger_type;
    uint8_t destroy_on_player_intersect : 1;
    uint8_t intersecting_curr : 1;
    uint8_t intersecting_prev : 1;
    uint8_t activated : 1;
    union {
        struct {
            pixel32_t color;
            int id; // Which level text entry to use for this
            scalar_t total_display_time;
            scalar_t curr_display_time;
        } data_text;
        struct {
            int id;
            int value_to_send;
        } signal;
        struct {
            vec3_t destination_pos;
            vec3_t destination_rotation_offset;
        } teleport;
    };
} entity_trigger_t;

entity_trigger_t* entity_trigger_new(void);
void entity_trigger_update(int slot, player_t* player, scalar_t dt);
void entity_trigger_on_hit(int slot, int hitbox_index);
void entity_trigger_player_intersect(int slot, player_t* player);

#ifdef __cplusplus
}
#endif

#endif
