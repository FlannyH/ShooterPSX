#include "main.h"

#include "renderer.h"
#include "memory.h"
#include "input.h"
#include "music.h"
#include "text.h"
#include "ui.h"

#include <string.h>

#ifdef _PSX
#include <psxcd.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxspu.h>
#include <psxpad.h>
#endif

#ifdef _WINDOWS
#include "windows/psx.h"
#include "windows/debug_layer.h"
#endif

#ifdef _NDS
#include "nds/psx.h"
#include <filesystem.h>
#endif

void state_enter_credits(void) {
	state.global.fade_level = 255;
	while (state.global.fade_level > 0) {
		renderer_begin_frame(&id_transform);
		ui_render_background();
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
		renderer_end_frame();
	}
	state.global.fade_level = 0;
	state.credits.scroll = 0;
	return;
}
void state_update_credits(int dt) {
	renderer_begin_frame(&id_transform);

	ui_render_background();

	// Draw text
	for (int i = 0; i < n_text_credits; ++i) {
		renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1, white);
	}

	// Scroll text
	state.credits.scroll -= dt * 140;

	renderer_end_frame();
	input_update();
	if (input_pressed(0xFFFF, 0) || state.credits.scroll < -5800000) {
		current_state = STATE_TITLE_SCREEN;
	}
	return;
}
void state_exit_credits(void) {
	music_stop();

	// Fade
	state.global.fade_level = 0;
	while (state.global.fade_level < 255) {
		renderer_begin_frame(&id_transform);
		
		ui_render_background();

		// Draw text
		for (int i = 0; i < n_text_credits; ++i) {
			renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1, white);
		}
		input_update();
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level += FADE_SPEED;
		renderer_end_frame();
	}
	state.global.fade_level = 255;
	state.title_screen.assets_in_memory = 0;
}
