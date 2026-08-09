// todo(collision): desc: collision
// todo(collision): deps: collision_winding_order 

// todo(entity): desc: entity
// todo(entity): deps: entity_misc entity_chaser entity_door

    // todo(entity_misc): desc: misc
    // todo(entity_misc): deps: entity_overflow entity_destructors

    // todo(entity_chaser): desc: chaser
    // todo(entity_chaser): deps: chaser_body_hits chaser_head_hits

    // todo(entity_door): desc: door
    // todo(entity_door): deps: door_delta_time

// todo(math): desc: math essentials
// todo(math): deps: fixed_point vec2 vec3

    // todo(fixed_point): desc: fixed point
    // todo(fixed_point): deps: fixed_point_overflow_check fixed_point_ps1_asm_opt

    // todo(vec2): desc: vec2
    // todo(vec2): deps: vec2_gte_opt

    // todo(vec3): desc: vec3
    // todo(vec3): deps: vec3_gte_opt

// todo(in_game): desc: in_game
// todo(in_game): deps: in_game_cleanup

// todo(input): desc: input
// todo(input): deps: input_remap input_remap_menu

// todo(models): desc: models
// todo(models): deps: mesh_find_hash_opt model_format_shared_vertices pc_mesh_generate_normals

// todo(music): desc: music and audio
// todo(music): deps: music_loop_cleanup music_channel_allocation_improvement music_move_sfx_enum music_pc

    // todo(music_pc): desc: pc
    // todo(music_pc): deps: pc_audio_userdata
    
    // todo(music_nds): desc: nds
    // todo(music_nds): deps: mixer_init mixer_upload_sample_data mixer_global_set_volume mixer_set_music_tempo mixer_channel_set_sample_rate mixer_channel_set_volume mixer_channel_set_sample mixer_channel_key_on mixer_channel_key_off mixer_channel_is_idle 

// todo(particles): desc: particles
// todo(particles): deps: particle_test_system particle_system_new

// todo(renderer): desc: renderer
// todo(renderer): deps: renderer_pc renderer_psx renderer_nds renderer_shared
    
    // todo(renderer_pc): desc: pc
    // todo(renderer_pc): deps: pc_renderer_globals pc_gl_debug pc_texture_atlas_resolution pc_quad_deprecated pc_renderer_extend_transparent pc_renderer_defer_mipmap_gen
    
    // todo(renderer_psx): desc: psx
    // todo(renderer_psx): deps: psx_renderer_pal_aspect

    // todo(renderer_nds): desc: nds
    // todo(renderer_nds): deps: nds_display_lists nds_renderer_get_delta_time_raw

    // todo(renderer_shared): desc: shared
    // todo(renderer_shared): deps: renderer_coordinate_system

    // todo(ui): desc: ui
    // todo(ui): deps: title_screen_cleanup ui_expansion

// todo(text): desc: text
// todo(text): deps: text_asset

// todo(debug): desc: debug
// todo(debug): deps: debug_renderer_fix error_dialog debug_level_load_reuse_code debug_light_defragment
