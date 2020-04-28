//
// Game main
//


enum Game_State {
    Game_State_Playing,
    Game_State_Begin_Editing,
    Game_State_Editing,
    Game_State_End_Editing,
    Game_State_Won,
    Game_State_Lost,
    
    Game_State_Count,
};


struct Game {  
    // Systems
    //Render_State render_state;
    Renderer *renderer = nullptr;
    Audio_State audio_state;    
    Resources resources;
    Log log;
    
    Level current_level;
    Level_Editor editor;

    u32 microseconds_since_start = 0;

    Array_Of_Moves all_the_moves;

    Game_State state  = Game_State_Playing;
    u32 render_mode   = Level_Render_Mode_All;
    u32 window_width  = 0;
    u32 window_height = 0;
    Input input = Input_None;
};


b32 init_game(Game *game) {
    b32 result = true;
    
    //
    // Load resources
    // TODO: add error handling!
    {   
        Resources *resources = &game->resources;

        log_str(&game->log, "loading resources...");
        
        // bitmaps
        if (result)  result = load_bitmap("data\\bitmaps\\pacman.bmp", &resources->bitmaps.pacman);        
        if (result)  result = load_bitmap("data\\bitmaps\\ghost_red.bmp", &resources->bitmaps.ghost_red);
        if (result)  result = load_bitmap("data\\bitmaps\\ghost_pink.bmp", &resources->bitmaps.ghost_pink);
        if (result)  result = load_bitmap("data\\bitmaps\\ghost_cyan.bmp", &resources->bitmaps.ghost_cyan);
        if (result)  result = load_bitmap("data\\bitmaps\\ghost_orange.bmp", &resources->bitmaps.ghost_orange);
        if (result)  result = load_bitmap("data\\bitmaps\\ghost_as_prey.bmp", &resources->bitmaps.ghost_as_prey);
        if (result)  result = load_bitmap("data\\bitmaps\\ghost_eyes.bmp", &resources->bitmaps.ghost_eye);
        if (result)  result = load_bitmap("data\\bitmaps\\ghost_pupils.bmp", &resources->bitmaps.ghost_pupil);
        if (result)  result = load_bitmap("data\\bitmaps\\dot_large.bmp", &resources->bitmaps.dot_large);
        if (result)  result = load_bitmap("data\\bitmaps\\dot_small.bmp", &resources->bitmaps.dot_small);
        if (result)  result = load_bitmap("data\\bitmaps\\background.bmp", &resources->bitmaps.background);
        if (result)  result = load_bitmap("data\\bitmaps\\pacman_atlas.bmp", &resources->bitmaps.pacman_atlas);

        char path[24];
        for (u32 index = 0; result && (index < 16); ++index) {
            _snprintf_s(path, 24, _TRUNCATE, "data\\bitmaps\\wall%02u.bmp", index);
            result = load_bitmap(path, &resources->bitmaps.walls[index]);
        }
        
        // fonts
        if (result)  result = load_font("data\\fonts", "font", &resources->font);
        log_str(&game->log, "font loaded");
    
        // wavs
        if (result)  result = load_wav("data\\audio\\eat_large_dot.wav", &resources->wavs.eat_large_dot);
        if (result)  result = load_wav("data\\audio\\eat_small_dot.wav", &resources->wavs.eat_small_dot);
        if (result)  result = load_wav("data\\audio\\ghost_dies.wav", &resources->wavs.ghost_dies);
        if (result)  result = load_wav("data\\audio\\won.wav", &resources->wavs.won);
        if (result)  result = load_wav("data\\audio\\lost.wav", &resources->wavs.lost);
        if (result)  result = load_wav("data\\audio\\nope.wav", &resources->wavs.nope);

        if (!result) {
            LOG_ERROR(&game->log, "failed to load resources, is the data directory present?", 0);
        }
        else {
            log_str(&game->log, "audio loaded");
            log_str(&game->log, "all resources loaded");
        }        


        
        // NOTE: Create a voice using a (somewhat) random wav file. If playing a wav with a different
        //       format (different bit rate, etc...) the voice will be destroyed and re-created.
        //       So this will not create any requirements of the wav formats.
        result = init_voices(&game->audio_state, &game->resources.wavs.eat_small_dot);
        if (!result)  LOG_ERROR(&game->log, "failed to init voices", 0);

        game->microseconds_since_start = 0;
    }
    // End of resource loading
    //
    //
    

    
    //
    // Init game state
    init_array_of_moves(&game->all_the_moves);
    game->state = Game_State_Playing;    

    result = load_level_from_disc(&game->current_level, &game->resources, "test.level_txt");
    if (result) {
        create_maps_off_level(&game->current_level);
        game->state = Game_State_Playing;
    }
    else {
        LOG_ERROR_STR(&game->log, "failed to load level", 0);
    }

    return result;
}


void fini_game(Game *game) {
    fini_editor(&game->editor);
    fini_level(&game->current_level);
    free_resources(&game->resources);
    free_array_of_moves(&game->all_the_moves);
}




//
// Change game state
//

void change_to_next_level(Game *game) {    
}


void change_to_prev_level(Game *game) {
}


void victory(Game *game) {
    game->state = Game_State_Won;
    game->input = Input_None;    
}


void defeat(Game *game) {
    game->state = Game_State_Lost;
    game->input = Input_None;
}


void reset(Game *game) {
    game->state = Game_State_Playing;
    reset_level(&game->current_level);
    create_maps_off_level(&game->current_level);
}


void undo(Game *game) {    
    game->state = Game_State_Playing;
    undo_one_level_state(&game->current_level);
    create_maps_off_level(&game->current_level);
}


void redo(Game *game) {
    game->state = Game_State_Playing;
    redo_one_level_state(&game->current_level);
    create_maps_off_level(&game->current_level);
}




//
// #_Draw level
//

void draw_level(Game *game) {
    Level *level = &game->current_level;
    //Render_State *render_state = &game->render_state;
    Renderer *renderer = game->renderer;
    Font *font = &level->resources->font;


    //
    // Draw level
    draw_level(renderer, level, game->render_mode, game->microseconds_since_start);    


    //
    // Draw maps
    draw_maps(renderer, level, level->maps, level->current_map_index);

    
    //
    // Draw win/defeat if relevant
    if (game->state == Game_State_Won) {
        draw_level_win_text(renderer, font);
    }
    else if (game->state == Game_State_Lost) {
        draw_level_lost_text(renderer, font);
    }
}




//
// #_Update
//

void update_and_render(Game *game, f32 dt, b32 *should_quit) {
    Level *level = &game->current_level;
    //clear_backbuffer(&game->render_state);
    game->renderer->clear(v4u8_black);

    
    //
    // Play the game    
    if (game->state == Game_State_Won) {
        if (game->input == Input_Select) {
            reset(game);
        }
    }
    else if (game->state == Game_State_Lost) {
        if (game->input == Input_Select) {
            reset(game);
        }
    }
    else if (game->state == Game_State_Playing) {
        Level_State *level_state = level->current_state;
            
        //
        // Check for win-/loose-conditions
        if (level_state->pacman_count == 0) {
            victory(game);
            play_wav(&game->audio_state, &game->resources.wavs.won);
        }
        else if (level_state->ghost_count == 0) {
            defeat(game);
            play_wav(&game->audio_state, &game->resources.wavs.lost);
        }            
        else if (game->input < Input_Select) {
            #ifdef DEBUG
            debug_check_all_actors(level);
            #endif
                
            collect_all_moves(level->maps, &game->all_the_moves, &game->input, level);
            u32 valid_moves = resolve_all_moves(&game->all_the_moves, level);
                
            #ifdef DEBUG
            debug_check_all_move_sets(&game->all_the_moves);
            debug_check_all_actors(level);
            #endif
        
            if (valid_moves == 0) {
                cancel_moves(&game->all_the_moves, level);
                play_wav(&game->audio_state, &game->resources.wavs.nope); // No valid moves at all, we won't move until we have at least one valid!
            }
            else {                
                // We're moving and thus we need to save the state and recalulate the "dijkstra maps".
                save_current_level_state(level);
                level_state = level->current_state;

                debug_check_all_actors(level);
                accept_moves(&game->all_the_moves, &game->audio_state, &game->resources.wavs, level);
                debug_check_all_actors(level);
                
                // Update mode counter                    
                if (level_state->mode_duration > 0) {
                    --level_state->mode_duration;        
                    if (level_state->mode_duration == 0) {
                        change_pacman_mode(level, Actor_Mode_Prey);
                    }
                }                
        
                // Update "dijkstra-maps"
                create_maps_off_level(level);
            }
            
            clear_array_of_moves(&game->all_the_moves);            
        }
    }
    

    if (game->state == Game_State_Begin_Editing) {
        begin_editing(&game->editor, level);
        game->state = Game_State_Editing;
    }
    else if (game->state == Game_State_End_Editing) {
        end_editing(&game->editor, &game->current_level);
        game->state = Game_State_Playing;
    }
    else if (game->state == Game_State_Editing) {
        edit_level(&game->editor, game->renderer, game->input, game->microseconds_since_start);
    }
    else {
        draw_level(game);
    }
    
    
    //
    // NOTE: For now we'll assume that we always are able to keep the framerate.
    // TODO: Handle framerate deviations!
    if (game->microseconds_since_start < (0xFFFFFFFF - kFrame_Time)) {
        game->microseconds_since_start += kFrame_Time;
    }
    else {
        game->microseconds_since_start = 0;
    }

    game->input = Input_None;
}
