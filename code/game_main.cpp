//
// Game main
//


enum Game_State {
    Game_State_Playing,
    Game_State_Editor,
    Game_State_Won,
    Game_State_Lost,
    
    Game_State_Count,
};




struct Game {  
    // Systems
    Render_State render_state;
    Audio_State audio_state;    
    Resources resources;
    
    Level current_level;

    s32 *maps[Map_Count] = {};
    u32 current_map_index = Map_Count; // DEBUG

    u32 microseconds_since_start = 0;

    Array_Of_Moves all_the_moves;

    Log log;

    Game_State state = Game_State_Playing;
    u32 window_width = 0;
    u32 window_height = 0;
    Input input = Input_None;

    u32 draw_grid_mode = 0;
};


#include "editor.cpp"


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
    game->current_map_index = Map_Count; // DEBUG

    result = load_level_from_disc(&game->current_level, &game->resources, "test.level_txt");
    if (result) {
    create_maps_off_level(game->maps, &game->current_level);
    game->state = Game_State_Playing;
    }
    else {
        LOG_ERROR_STR(&game->log, "failed to load level", 0);
    }

    return result;
}


void fini_game(Game *game) {
    fini_level(&game->current_level);
    free_resources(&game->resources);

    for (u32 map_index = 0; map_index < Map_Count; ++map_index) {
        if (game->maps[map_index]) {
            free(game->maps[map_index]);
            game->maps[map_index] = nullptr;
        }
    }

    free_array_of_moves(&game->all_the_moves);
}




//
// Change game state
//

void reset(Game *game) {
    game->state = Game_State_Playing;
    
    Level *level = &game->current_level;
    u32 index = get_first_level_state_index(level);
    level->current_state_index = index;
    level->current_state = get_level_state_n(level, index);
    create_maps_off_level(game->maps, level);
}


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


void undo(Game *game) {
    Level *level = &game->current_level;
    u32 prev_index = get_prev_valid_level_state_index(level);
    
    game->state = Game_State_Playing;
    level->current_state_index = prev_index;
    level->current_state = get_level_state_n(level, prev_index);
    create_maps_off_level(game->maps, &game->current_level);
}


void redo(Game *game) {
    Level *level = &game->current_level;
    u32 next_index = get_next_valid_level_state_index(level);
    
    game->state = Game_State_Playing;
    level->current_state_index = next_index;
    level->current_state = get_level_state_n(level, next_index);
    create_maps_off_level(game->maps, &game->current_level);
}




//
// #_Draw level
//

void draw_level(Game *game) {
    Level *level = &game->current_level;
    Render_State *render_state = &game->render_state;
        
    u32 cell_size = render_state->backbuffer_width / level->width;
    assert(cell_size == kCell_Size);

    u32 level_width = level->width;
    u32 level_height = level->height;


    //
    // Draw level
    draw_current_level(render_state, level, game->microseconds_since_start);
    

        
    //
    // Draw background
#if 0
    for (u32 y = 0; y < level_height; ++y) {
        for (u32 x = 0; x < level_width; ++x) {
            v2u P = V2u(cell_size * x, cell_size * y);

            if (game.resources.background.data) {
                draw_bitmap(render_state, P, &game.resources.background);
            }
        }
    }
#endif

    
    //
    // Draw grid
    if (game->draw_grid_mode > 0) {
        v4u8 const border_colour = {255, 255, 255, 75};
        for (u32 y = 0; y < level_height; ++y) {
            for(u32 x = 0; x < level_width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                if (!tile_is_traversable(tile) && game->draw_grid_mode == 1) {                    
                }
                else {
                    draw_rectangle_outline(render_state, V2u(kCell_Size * x, kCell_Size * y), kCell_Size, kCell_Size, border_colour);
                }
            }
        }
    }


    //
    // Draw maps, DEBUG
    Font *font = &level->resources->font;
    u32 current_map_index = game->current_map_index;
    if (current_map_index < Map_Count && game->maps[current_map_index]) {
        char text[10];

        u32 constexpr kOffset_x = kCell_Size / 2;
        u32 constexpr kOffset_y = kCell_Size / 2;
        
        for (u32 y = 0; y < level_height; ++y) {
            for (u32 x = 0; x < level_width; ++x) {
                u32 index = (level_width * y) + x;
                s32 value = game->maps[current_map_index][index];
                u32 error = _snprintf_s(text, 10, _TRUNCATE, "%d", value);
                assert(error > 0);
                
                v2u text_dim = get_text_dim(font, text);
                v2u half_text_dim = (text_dim / 2);
                v2u text_pos = V2u(kCell_Size * x + kOffset_x,kCell_Size * y + kOffset_y);
                if (half_text_dim.x <= text_dim.x && half_text_dim.y <= text_dim.y) {
                    text_pos = text_pos - half_text_dim;
                }

                u32 count;
                if (value > 0) {
                    count = value > 99 ? 3 : value > 9 ? 2 : 1;
                }
                else {
                    count = value < -99 ? 3 : value < -9 ? 2 : 1;
                }

                //v2u text_offset = (text_dim / count) / 7;
                v2u text_offset = V2u(1, 1);

                Tile *curr_tile = get_tile_at(level, V2u(x, y));
                if (curr_tile && curr_tile->item.type > Item_Type_None) {
                    text_pos.y = text_pos.y - kOffset_y + (kCell_Size / 8);
                }
                
                print(render_state, font, text_pos, text, V4u8(100, 100, 100, 255));
                print(render_state, font, text_pos + text_offset, text, v4u8_white);
            }
        }


#ifdef DEBUG
        char map_name_text[50];
        u32 error = _snprintf_s(map_name_text, 50, _TRUNCATE, "Map: %s", kMap_Names[current_map_index]);
        assert(error > 0);        
        v2u text_dim = get_text_dim(font, map_name_text);
        u32 char_width = text_dim.x / static_cast<u32>(strlen(map_name_text));
        print(render_state, font, V2u(render_state->backbuffer_width - text_dim.x - char_width, 10), map_name_text);
#endif
    }

    
#if 0
    // Print actor index at the actors' location (or where the game thinks that the actor are)
    else {
        // DEBUG
        for (u32 y = 0; y < level_height; ++y) {
            for (u32 x = 0; x < level_width; ++x) {
                Tile *tile = get_tile_at(level, V2u(x, y));

                if (tile->actor_id.index != 0xFFFF) {
                    u32 constexpr kOffset_x = kCell_Size / 2;
                    u32 constexpr kOffset_y = kCell_Size / 2;        
                    char text[10];

                    u32 value = tile->actor_id.index;
                    u32 error = _snprintf_s(text, 10, "%d", value);
                    assert(error > 0);
                
                    v2u text_dim = get_text_dim(font, text);
                    v2u half_text_dim = (text_dim / 2);
                    v2u text_pos = V2u(kCell_Size * x + kOffset_x, kCell_Size * y + kOffset_y);
                    if (half_text_dim.x <= text_dim.x && half_text_dim.y <= text_dim.y) {
                        text_pos = text_pos - half_text_dim;
                    }

                    u32 count;
                    if (value > 0) {
                        count = value > 99 ? 3 : value > 9 ? 2 : 1;
                    }
                    else {
                        count = value < -99 ? 3 : value < -9 ? 2 : 1;
                    }
                    v2u text_offset = (text_dim / count) / 8;
                
                    print(render_state, font, text_pos, text, v4u8_black);
                    print(render_state, font, text_pos + text_offset, text, v4u8_white);
                }
            }
        }
    }
#endif
    
    //
    // Draw win/defeat if relevant
    if (game->state == Game_State_Won) {
        char const *win_text = "VICTORIOUS!";
        v2u text_dim = get_text_dim(font, win_text);
        print(render_state, font, V2u((render_state->backbuffer_width - text_dim.x) / 2, 300), win_text, v4u8_green);
        
        if (game->input == Input_Select) {
            reset(game);
        }
    }
    else if (game->state == Game_State_Lost) {
        char const *lost_text = "DEFEATED!";
        v2u text_dim = get_text_dim(font, lost_text);
        print(render_state, font, V2u((render_state->backbuffer_width - text_dim.x) / 2, 300), lost_text, v4u8_red);
        
        char const *text = "Press Enter or R to reset level and try again!";
        text_dim = get_text_dim(font, text);
        print(render_state, font, V2u((render_state->backbuffer_width - text_dim.x) / 2, 270), text, v4u8_white);

        if (game->input == Input_Select) {
            reset(game);
        }
    }
}




//
// #_Update
//

void update_and_render(Game *game, f32 dt, b32 *should_quit) {
    Level *level = &game->current_level;
    clear_backbuffer(&game->render_state);

    
    //
    // Play the game
    if (game->state == Game_State_Editor) {
        edit_level(game);
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
            collect_all_moves(game->maps, &game->all_the_moves, &game->input, level);
            u32 valid_moves = resolve_all_moves(&game->all_the_moves, level);
        
            if (valid_moves == 0) {
                cancel_moves(&game->all_the_moves, level);
                play_wav(&game->audio_state, &game->resources.wavs.nope); // No valid moves at all, we won't move until we have at least one valid!
            }
            else {                
                // We're moving and thus we need to save the state and recalulate the "dijkstra maps".
                save_current_level_state(level);
                level_state = level->current_state;

                accept_moves(&game->all_the_moves, &game->audio_state, &game->resources.wavs, level);
                // Update mode counter                    
                if (level_state->mode_duration > 0) {
                    --level_state->mode_duration;        
                    if (level_state->mode_duration == 0) {
                        change_pacman_mode(level, Actor_Mode_Prey);
                    }
                }                
        
                // Update "dijkstra-maps"
                create_maps_off_level(game->maps, level);
            }
            
            clear_array_of_moves(&game->all_the_moves);            
        }
    }
    
    draw_level(game);
    game->input = Input_None;  
    
    //
    // NOTE: For now we'll assume that we always are able to keep the framerate.
    // TODO: Handle framerate deviations!
    if (game->microseconds_since_start < (0xFFFFFFFF - kFrame_Time)) {
        game->microseconds_since_start += kFrame_Time;
    }
    else {
        game->microseconds_since_start = 0;
    }
}
