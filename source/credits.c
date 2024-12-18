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
	renderer_start_fade_in(FADE_SPEED);
	while (renderer_is_fading()) {
		renderer_begin_frame(&id_transform);
		ui_render_background();
		renderer_end_frame();
	}
	state.credits.scroll = 0;
	return;
}

void state_update_credits(int dt) {
	renderer_begin_frame(&id_transform);
	ui_render_background();

	for (int i = 0; i < n_text_credits; ++i) {
		renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1, white);
	}

	state.credits.scroll -= dt * 140;
	renderer_end_frame();
	input_update();

	// If any input is pressed or the credits text is over, go back to title screen
	if (input_pressed(0xFFFF, 0) || state.credits.scroll < -5800000) {
		set_current_state(STATE_TITLE_SCREEN);
	}
	return;
}
void state_exit_credits(void) {
	music_stop();

	// Fade
	renderer_start_fade_out(FADE_SPEED);
	while (renderer_is_fading()) {
		renderer_begin_frame(&id_transform);
		ui_render_background();

		for (int i = 0; i < n_text_credits; ++i) {
			renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1, white);
		}

		// In case the player skipped the credits by pressing a button, tick the input
		// system. That way the button isn't registered as released on frame 1 on the 
		// title screen, fixing a bug where it'd immediately press the credits button again
		input_update();

		renderer_end_frame();
	}
	state.title_screen.assets_in_memory = 0;
}
