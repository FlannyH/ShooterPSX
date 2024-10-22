#ifndef PLATFORM_H
#define PLATFORM_H
#include "../entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	entity_header_t entity_header;
    vec3_t position_start;
    vec3_t position_end;
    scalar_t velocity;
    int curr_timer_value;
    int auto_start_timer; // Time (milliseconds) to wait before automatically going from start to end 
    int auto_return_timer; // Time (milliseconds) to wait before automatically going from end to start
    int signal_id;
    unsigned int listen_to_signal : 1;
    unsigned int target_is_end : 1;
    unsigned int auto_start : 1;
    unsigned int auto_return : 1;
    unsigned int move_on_player_collision : 1;
} entity_platform_t;

entity_platform_t* entity_platform_new(void);
void entity_platform_update(int slot, player_t* player, int dt);
void entity_platform_on_hit(int slot, int hitbox_index);
void entity_platform_player_intersect(int slot, player_t* player);

#ifdef __cplusplus
}
#endif
#endif
