enum Game_State {
    Game_State_Playing,
    Game_State_World_Select,
    Game_State_Level_Select,
    Game_State_Player_Select,
    Game_State_Player_Select_Enter_Name,
    Game_State_Player_Select_Enter_Name_Done,
    Game_State_Menu,
    Game_State_Help_Screen,
    Game_State_Won,
    Game_State_Lost,
    
    Game_State_Count,
};


struct Game {
    Render_State render_state;
    Audio_State audio_state;    
    Resources resources;
    Player_Profile player_profiles[3];
    u32 current_profile_index = 0;

    Array_Of_Worlds worlds;
    u32 current_world_index = 0;

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
    u32 menu_item_index = 0;
    char input_buffer[kPlayer_Profile_Name_Max_Length];
    u32 input_buffer_curr_pos = 0;
};


World *get_current_world(Game *game) {
    World *result = nullptr;

    if (game && game->current_world_index < game->worlds.count) {
        result = &game->worlds.data[game->current_world_index];
    }

    return result;
}

Level *get_current_level_state(Game *game) {
    Level *result = nullptr;
    World *world = get_current_world(game);
    result = get_current_level_state(world);
    
    return result;    
}

Level *get_current_level_first_state(Game *game) {
    Level *result = nullptr;
    World *world = get_current_world(game);
    result = get_current_level_first_state(world);
    
    return result;    
}

Player_Profile *get_current_player_profile(Game *game) {
    game->current_profile_index = game->current_profile_index < 3 ? game->current_profile_index : 2;
    Player_Profile *result = &game->player_profiles[game->current_profile_index];
    return result;
}


void reset(Game *game) {
    game->state = Game_State_Playing;
    World *world = get_current_world(game);
    load_level(world, world->current_level_index);
    create_maps_off_level(game->maps, &world->levels[world->current_level_index]);
}


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

    
        // load worlds
        {
            init_array_of_worlds(&game->worlds, 1);
            log_str(&game->log, "Loading worlds...");
            
            WIN32_FIND_DATAA file_data;
            HANDLE search_handle = FindFirstFileA("data\\worlds\\*.wld", &file_data);
            World *world = nullptr;            
            do
            {
                world = add(&game->worlds);
                result = load_world_from_file(world, "data\\worlds", file_data.cFileName, &game->resources);
                if (!result)  LOG_ERROR_STR(&game->log, "failed to load world", file_data.cFileName);
            }
            while (FindNextFileA(search_handle, &file_data) != 0);
        }

        if (result) {
            log_str(&game->log, "All worlds loaded.");
        }

        game->microseconds_since_start = 0;
    }
    // End of resource loading
    //
    //
    

    
    //
    // Init game state
    if (!read_player_profiles_from_disc(game->player_profiles)) {        
        _snprintf_s(reinterpret_cast<char *>(&game->player_profiles[0].name), kPlayer_Profile_Name_Max_Length, _TRUNCATE, "Adam");
        _snprintf_s(reinterpret_cast<char *>(&game->player_profiles[1].name), kPlayer_Profile_Name_Max_Length, _TRUNCATE, "Bertil");
        _snprintf_s(reinterpret_cast<char *>(&game->player_profiles[2].name), kPlayer_Profile_Name_Max_Length, _TRUNCATE, "Cesar");
        write_player_profiles_to_disc(game->player_profiles);
    }

    init_array_of_moves(&game->all_the_moves);
    game->state = Game_State_Player_Select;
    game->current_map_index = Map_Count; // DEBUG

    log_str(&game->log, "Creating maps...");
    World *world = get_current_world(game);
    Player_Profile *profile = get_current_player_profile(game);
    s32 level_index = get_highest_completed_level_index(profile, world) + 1;
    level_index = level_index < static_cast<s32>(world->level_count) ? level_index : 0;
    load_level(world, level_index);
    create_maps_off_level(game->maps, &world->levels[world->current_level_index]);
    log_str(&game->log, "Maps created.");

    return result;
}


void fini_game(Game *game) {
    free_array_of_worlds(&game->worlds);
    free_resources(&game->resources);

    for (u32 map_index = 0; map_index < Map_Count; ++map_index) {//Map_Count; ++map_index) {
        if (game->maps[map_index]) {
            free(game->maps[map_index]);
            game->maps[map_index] = nullptr;
        }
    }

    free_array_of_moves(&game->all_the_moves);
    
    free_player_profile(&game->player_profiles[0]);
    free_player_profile(&game->player_profiles[1]);
    free_player_profile(&game->player_profiles[2]);
}


void change_to_next_level(Game *game) {
    if (game) {
        World *world = get_current_world(game);
        world->current_level_index = world->current_level_index < (world->level_count - 1) ? ++world->current_level_index : 0;
        reset(game);
    }
}


void change_to_prev_level(Game *game) {
    if (game) {
        World *world = get_current_world(game);
        world->current_level_index = world->current_level_index > 0 ? --world->current_level_index : world->level_count - 1;
        reset(game);
    }
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
    World *world = get_current_world(game);
    u32 curr_index = world->current_level_state_index;
    u32 prev_index = curr_index == 0 ? prev_index = (kWorld_Level_States_Count - 1) : curr_index - 1;

    if (is_valid_current_level_state_index(world, prev_index)) {
        world->current_level_state_index = prev_index;
        game->state = Game_State_Playing;

        Level *level = get_current_level_state(world);
        create_maps_off_level(game->maps, level);
    }
}


void redo(Game *game) {
    World *world = get_current_world(game);
    u32 curr_index = world->current_level_state_index;
    u32 next_index = curr_index < (kWorld_Level_States_Count - 1) ? curr_index + 1 : 0;

    if (is_valid_current_level_state_index(world, next_index)) {
        world->current_level_state_index = next_index;
    }

    
}




//
// #_Menus
//

#define kMenu_Padding 3

void draw_menu(Game *game, b32 *quit) {
    Render_State *render_state = &game->render_state;
    Font *font = &game->resources.font;
    char *menu_items[] = {"Quit", "Help", "Select level", "Select world", "Change player", "Play"};
    u32 max_menu_items = 6;
    game->menu_item_index = game->menu_item_index > max_menu_items ? max_menu_items : game->menu_item_index;
    
    if (game->input == Input_Up) {
        game->input = Input_None;
        ++game->menu_item_index;
        if (game->menu_item_index >= max_menu_items)  game->menu_item_index = 0;
    }
    else if (game->input == Input_Down) {
        game->input = Input_None;
        if (game->menu_item_index == 0) {
            game->menu_item_index = max_menu_items - 1;
        }
        else {
            --game->menu_item_index;
        }
    }
    else if (game->input == Input_Select) {
        if (game->menu_item_index == 0) {
            // quit
            *quit = true;
        }
        else if (game->menu_item_index == 1) {
            game->state = Game_State_Help_Screen;
        }
        else if (game->menu_item_index == 2) {
            game->state = Game_State_Level_Select;

            World *world = get_current_world(game);
            game->menu_item_index = world->current_level_index;
        }
        else if (game->menu_item_index == 3) {
            game->state = Game_State_World_Select;
            game->menu_item_index = game->current_world_index;
        }
        else if (game->menu_item_index == 4) {
            game->state = Game_State_Player_Select;
            game->menu_item_index = game->current_profile_index;
        }
        else if (game->menu_item_index == 5) {
            game->state = Game_State_Playing;
        }
    }
    else if (game->input == Input_Escape) {
        Level *level = get_current_level_state(game);
        if (level->ghost_count == 0) {
            game->state = Game_State_Lost;
        }
        else if (level->pacman_count == 0) {
            game->state = Game_State_Won;
        }
        else {
            game->state = Game_State_Playing;
        }
    }


    //
    // Print current world and level
    u32 y = 600;
    {
        World *world = get_current_world(game);
        Level *level = get_current_level_state(world);
        v4u8 colour = {255, 125, 125, 255};
        char text_buffer[256];

        Player_Profile *profile = get_current_player_profile(game);
        World_Progress *world_progress = get_world_progress(&profile->worlds, world->name);

        
        // Current player
        _snprintf_s(text_buffer, 256, _TRUNCATE, "Player: %s", profile->name);
        print(render_state, font, V2u(100, y), text_buffer, colour);
        y -= 40;

        
        // Current world
        if (world_progress) {
            f32 world_percentage = 100.0f * roundf(static_cast<f32>(world_progress->level_results.count) /
                                                   static_cast<f32>(world->level_count));
            _snprintf_s(text_buffer, 256, _TRUNCATE, "World: %s, completed %3.1f%%", world->name, world_percentage);
        }
        else {
            _snprintf_s(text_buffer, 256, _TRUNCATE, "World: %s", world->name);
        }
        print(render_state, font, V2u(100, y), text_buffer, colour);
        y -= 40;

        
        // Current level
        b32 copied_string = false;
        if (world_progress) {
            Level_Result *level_result = get_level_result(profile, world, level->name);
            if (level_result) {
                _snprintf_s(text_buffer, 256, _TRUNCATE, "Level: %s, best score = %u", level->name, level_result->max_score);
                copied_string = true;
            }
        }
        
        if (!copied_string) {
            _snprintf_s(text_buffer, 256, _TRUNCATE, "Level: %s", level->name);
        }
        print(render_state, font, V2u(100, y), text_buffer, colour);
    }
    

    u32 current_menu_item = 0;
    u32 start_y = 100;
    y = start_y;

    // Change profile
    for (u32 index = 0; index < max_menu_items; ++index) {
        v4u8 colour = game->menu_item_index == current_menu_item ? v4u8_yellow : v4u8_white;
        print(render_state, font, V2u(100, y), menu_items[current_menu_item++], colour);
        y += 50;
    }
}


void draw_help_screen(Game *game) {
    Render_State *render_state = &game->render_state;
    Font *font = &game->resources.font;
    print(render_state, font, V2u(100, 100), "No help here!", v4u8_white);

    if (game->input == Input_Select) {
        game->state = Game_State_Menu;
    }
    else if (game->input == Input_Escape) {
        game->state = Game_State_Playing;
    }
}


void draw_level_select(Game *game) {
    Render_State *render_state = &game->render_state;
    Font *font = &game->resources.font;
    World *world = get_current_world(game);    

    // NOTE: We're assuming that the font is monospaced
    Char_Data *m_char = &font->char_data['M' - 32];
    u32 char_width = m_char->advance_x;
    u32 char_height = m_char->height;
    u32 line_height = font->line_height;
    u32 padding_width = kMenu_Padding * char_width;
    u32 textbuffer_size = ((render_state->backbuffer_width - (2 * padding_width)) / char_width) + 1; // +1 due to null terminator
    char *textbuffer = static_cast<char *>(malloc(textbuffer_size));

    u32 y = render_state->backbuffer_height - ((kMenu_Padding + 1) * char_height);

    // Print header
    u32 name_size = textbuffer_size - 6;
    _snprintf_s(textbuffer, name_size, _TRUNCATE, "Level selection");    
    memset(&textbuffer[15], ' ', textbuffer_size - 21);
    _snprintf_s(&textbuffer[textbuffer_size - 6], 6, _TRUNCATE, "Score");    
    print(render_state, font, V2u(padding_width, y), textbuffer);
    print(render_state, font, V2u(padding_width - 1, y - 1), textbuffer);
    y -= line_height;

    // Print row of dashes
    for (u32 index = 0; index < (textbuffer_size - 1); ++index) {
        textbuffer[index] = '-';
    }
    textbuffer[textbuffer_size - 1] = '\0';
    print(render_state, font, V2u(padding_width, y), textbuffer);
    y -= line_height;

    
    v4u8 colour {220, 220, 220, 255}; // NOTE: if this is marked as constexpr then it would initalize to 0, 0, 0, 255! Compiler bug?
    u32 rows_per_page = y / line_height;
    u32 start_index = 0;
    if (world->level_count > rows_per_page && game->menu_item_index >= rows_per_page) {
        start_index = (game->menu_item_index / rows_per_page) * rows_per_page;
    }

    for (u32 level_index = start_index; level_index < world->level_count && y >= line_height; ++level_index) {
        Level *level = &world->levels[level_index];
        Player_Profile *profile = get_current_player_profile(game);        
        Level_Result *level_result = get_level_result(profile, world, level->name);
        v4u8 used_colour = colour;

        textbuffer[0] = ' ';
        textbuffer[1] = ' ';
        u32 curr_pos = 2;
        if (level_index == game->menu_item_index) {
            textbuffer[0] = '>';
            used_colour = v4u8_yellow;
        }

        curr_pos += _snprintf_s(&textbuffer[curr_pos], 7, _TRUNCATE, "%4u. ", level_index); // - 5 to allow room for score
        curr_pos += _snprintf_s(&textbuffer[curr_pos], textbuffer_size - curr_pos - 5, _TRUNCATE, level->name); // - 5 to allow room for score

        for (;curr_pos < textbuffer_size - 6; ++curr_pos)  textbuffer[curr_pos] = ' ';
        
        if (level_result) {
            curr_pos += _snprintf_s(&textbuffer[textbuffer_size - 6], 6, _TRUNCATE, "%5u", level_result->score);
        }
        else {
            curr_pos += _snprintf_s(&textbuffer[textbuffer_size - 6], 6, _TRUNCATE, "   --");
        }

        print(render_state, font, V2u(padding_width, y), textbuffer, used_colour);
        y -= line_height;
    }    

    if (game->input == Input_Up) {        
        if (game->menu_item_index == 0) {
            game->menu_item_index = world->level_count - 1;
        }
        else {
            --game->menu_item_index;
        }
    }
    if (game->input == Input_Down) {
        ++game->menu_item_index;
        if (game->menu_item_index >= world->level_count) {
            game->menu_item_index = 0;
        }
    }
    else if (game->input == Input_Select) {
        Player_Profile *profile = get_current_player_profile(game);
        u32 level_index = get_highest_completed_level_index(profile, world);
        if (game->menu_item_index <= (level_index + 1)) {
            assert(game->menu_item_index < world->level_count);
            load_level(world, game->menu_item_index);
            create_maps_off_level(game->maps, get_current_level_state(game));
            game->state = Game_State_Playing;
            game->menu_item_index = 5;
        }
        else {
            // NOPE!
            play_wav(&game->audio_state, &game->resources.wavs.nope);
        }
    }
    else if (game->input == Input_Escape) {
        game->state = Game_State_Menu;
    }

    if (textbuffer) {
        free(textbuffer);
    }
}


void draw_world_select(Game *game) {
    Render_State *render_state = &game->render_state;
    Font *font = &game->resources.font;

    // NOTE: We're assuming that the font is monospaced
    Char_Data *m_char = &font->char_data['M' - 32];
    u32 char_width = m_char->advance_x;
    u32 char_height = m_char->height;
    u32 line_height = font->line_height;
    u32 padding_width = kMenu_Padding * char_width;
    u32 textbuffer_size = (render_state->backbuffer_width / char_width) - (2 * kMenu_Padding) + 1; // +1 due to null terminator
    char *textbuffer = static_cast<char *>(malloc(textbuffer_size));
    
    u32 y = render_state->backbuffer_height - ((kMenu_Padding + 1) * char_height);
    print(render_state, font, V2u(padding_width, y), "World selection", v4u8_white);
    print(render_state, font, V2u(padding_width - 1, y - 1), "World selection", v4u8_white);
    y -= line_height;
    print(render_state, font, V2u(padding_width, y), "---------------", v4u8_white);
    y -= line_height;

    v4u8 colour {220, 220, 220, 255}; // NOTE: if this is marked as constexpr then it would initalize to 0, 0, 0, 255! Compiler bug?

    u32 rows_per_page = (y - padding_width) / line_height;
    u32 start_index = 0;
    if (game->worlds.count > rows_per_page && game->menu_item_index >= rows_per_page) {
        start_index = (game->menu_item_index / rows_per_page) * rows_per_page;
    }
    
    for (u32 world_index = start_index; world_index < game->worlds.count && y >= (line_height + padding_width) ; ++world_index) {
        World *world = &game->worlds.data[world_index];
        _snprintf_s(textbuffer, textbuffer_size, _TRUNCATE, "%3u. %s", world_index, world->name);
        v4u8 used_colour = colour;
        if (world_index == game->menu_item_index) {
            print(render_state, font, V2u(padding_width, y), ">", v4u8_yellow);
            print(render_state, font, V2u(padding_width + 2 * char_width, y), textbuffer, v4u8_yellow);
        }
        else {
            print(render_state, font, V2u(padding_width + 2 * char_width, y), textbuffer, used_colour);
        }
        y -= line_height;
    }    

    if (game->input == Input_Up) {        
        if (game->menu_item_index == 0) {
            game->menu_item_index = game->worlds.count;
        }
        else {
            --game->menu_item_index;
        }
    }
    if (game->input == Input_Down) {
        ++game->menu_item_index;
        if (game->menu_item_index >= game->worlds.count) {
            game->menu_item_index = 0;
        }
    }
    else if (game->input == Input_Select) {
        game->current_world_index = game->menu_item_index;
        game->state = Game_State_Level_Select;

        World *world = get_current_world(game);
        Player_Profile *profile = get_current_player_profile(game);
        u32 level_index = get_highest_completed_level_index(profile, world);
        level_index = level_index + 1 < world->level_count ? level_index + 1 : 0;
        game->menu_item_index = level_index;
    }
    else if (game->input == Input_Escape) {
        game->state = Game_State_Menu;
    }

    if (textbuffer) {
        free(textbuffer);
    }
}


b32 is_selecting_player(Game *game) {
    b32 result = false;

    if (game->state == Game_State_Player_Select || game->state == Game_State_Player_Select_Enter_Name ||
        game->state == Game_State_Player_Select_Enter_Name_Done)
    {
        result = true;
    }

    return result;
}

void draw_player_select(Game *game) {    
    u32 max_menu_items = 3;

    if (game->state == Game_State_Player_Select_Enter_Name_Done) {
        game->state = Game_State_Player_Select;
        
        Player_Profile *profile = &game->player_profiles[game->menu_item_index];
        _snprintf_s(profile->name, kPlayer_Profile_Name_Max_Length, _TRUNCATE, "%s", game->input_buffer);
        write_player_profiles_to_disc(game->player_profiles);

        load_level(&game->worlds.data[0], 0);
        create_maps_off_level(game->maps, get_current_level_state(game));
    }


    //
    // Process input
    if (game->state != Game_State_Player_Select_Enter_Name) {
        if (game->input == Input_Up) {
            if (game->menu_item_index == 0) {
                game->menu_item_index = max_menu_items - 1;
            }
            else {
                --game->menu_item_index;
            }
        }
        if (game->input == Input_Down) {
            ++game->menu_item_index;
            if (game->menu_item_index >= max_menu_items) {
                game->menu_item_index = 0;
            }
        }
        else if (game->input == Input_Select) {
            game->current_profile_index = game->menu_item_index;
            game->current_world_index = 0;
            game->menu_item_index = 0;
            game->state = Game_State_World_Select;
        }
        else if (game->input == Input_Escape) {
            game->state = Game_State_Menu;
            game->menu_item_index = 5;
        }
        else if (game->input == Input_Delete) {
            assert(game->menu_item_index < 3);
            free_player_profile(&game->player_profiles[game->menu_item_index]);
            game->input_buffer_curr_pos = 0;
            game->input_buffer[0] = '\0';
            game->state = Game_State_Player_Select_Enter_Name;
        }
    }


    //
    // Get some information about the current font and prepare for text rendering
    // NOTE: We're assuming that the font is monospaced
    Render_State *render_state = &game->render_state;
    Font *font = &game->resources.font;
    Char_Data *m_char = &font->char_data['M' - 32];
    u32 char_width = m_char->advance_x;
    u32 char_height = m_char->height;
    u32 line_height = font->line_height;
    
    u32 padding = kMenu_Padding * char_width;
    u32 textbuffer_size = (render_state->backbuffer_width / char_width) - (2 * kMenu_Padding) + 1; // +1 due to null terminator
    char *textbuffer = static_cast<char *>(malloc(textbuffer_size));
    u32 y = render_state->backbuffer_height - ((kMenu_Padding + 1) * char_height);

    
    //
    // Print header
    u32 name_size = textbuffer_size - 6;
    _snprintf_s(textbuffer, name_size, _TRUNCATE, "Select player");
    print(render_state, font, V2u(padding, y), textbuffer);
    print(render_state, font, V2u(padding - 1, y - 1), textbuffer);
    y -= line_height;

    _snprintf_s(textbuffer, name_size, _TRUNCATE, "(Press the Delete key to delete a profile)");
    print(render_state, font, V2u(padding, y), textbuffer);    
    y -= line_height;

    
    //
    // Print row of dashes
    for (u32 index = 0; index < strlen("(Press the Delete key to delete a profile)"); ++index) {
        textbuffer[index] = '-';
    }
    textbuffer[textbuffer_size - 1] = '\0';
    print(render_state, font, V2u(padding, y), textbuffer);
    y -= line_height;

    
    //
    // Print menu items    
    v4u8 colour {220, 220, 220, 255}; // NOTE: if this is marked as constexpr then it would initalize to {0, 0, 0, 255}!
                                      //       Compiler bug or due to me relying on an undefined behaviour with the union?

    for (u32 index = 0; index < max_menu_items; ++index) {
        Player_Profile *profile = &game->player_profiles[index];
        v4u8 used_colour = colour;

        textbuffer[0] = ' ';
        textbuffer[1] = ' ';
        u32 curr_pos = 2;
        if (index == game->menu_item_index) {
            textbuffer[0] = '>';
            used_colour = v4u8_yellow;
        }

        if (game->state == Game_State_Player_Select_Enter_Name && index == game->menu_item_index) {
            curr_pos += _snprintf_s(&textbuffer[curr_pos], textbuffer_size - curr_pos, _TRUNCATE, "%s", game->input_buffer);
            used_colour = v4u8_red;
        }
        else {
            curr_pos += _snprintf_s(&textbuffer[curr_pos], textbuffer_size - curr_pos, _TRUNCATE, "%s", profile->name);
        }
        
        print(render_state, font, V2u(padding, y), textbuffer, used_colour);
        y -= line_height;
    }           

    if (textbuffer) {
        free(textbuffer);
    }
}




//
// #_Draw level
//

void draw_level(Game *game) {
    World *world = get_current_world(game);
    Level *level = get_current_level_state(world);
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
        
        if (world->current_level_index == (world->level_count - 1)) {
            char const *text1 = "You finished the last level!";
            text_dim = get_text_dim(font, text1);
            print(render_state, font, V2u((render_state->backbuffer_width - text_dim.x) / 2, 270), text1, v4u8_white);

            char const *text2 = "Press Enter to replay the first level";
            text_dim = get_text_dim(font, text2);
            print(render_state, font, V2u((render_state->backbuffer_width - text_dim.x) / 2, 240), text2, v4u8_white);
        }
        else {
            char const *text = "Press Enter to advance to the next level";
            text_dim = get_text_dim(font, text);
            print(render_state, font, V2u((render_state->backbuffer_width - text_dim.x) / 2, 270), text, v4u8_white);
        }

        if (game->input == Input_Select) {
            change_to_next_level(game);
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
    World *world = get_current_world(game);
    Level *level = get_current_level_state(world);
    clear_backbuffer(&game->render_state);

    
    //
    // Play the game
    if (game->state == Game_State_World_Select) {
        draw_world_select(game);
    }
    
    else if (game->state == Game_State_Level_Select) {
        draw_level_select(game);
    }

    else if (is_selecting_player(game)) {
        draw_player_select(game);
    }
            
    else if (game->state == Game_State_Menu) {
        draw_menu(game, should_quit);
    } 
            
    else if (game->state == Game_State_Help_Screen) {
        draw_help_screen(game);
    }

    //else if (game->state == Game_State_Playing) {
    else {
        if (game->input == Input_Escape) {// && game->state == Game_State_Playing) {
            game->state = Game_State_Menu;
        }
        else if (game->state == Game_State_Playing) {
            //
            // Check for win-/loose-conditions
            if (level->pacman_count == 0) {
                victory(game);
                play_wav(&game->audio_state, &game->resources.wavs.won);
                    
                Player_Profile *profile = get_current_player_profile(game);
                Level *first_level_state = get_current_level_first_state(game);
                assert(profile);
                assert(first_level_state);
                add_level_progress(profile, world, first_level_state, level->score);
                write_player_profiles_to_disc(game->player_profiles);
            }
            else if (level->ghost_count == 0) {
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
                    save_current_level_state(world);
                    level = get_current_level_state(world);

                    accept_moves(&game->all_the_moves, &game->audio_state, &game->resources.wavs, level);
                    // Update mode counter
                    if (level->mode_duration > 0) {
                        --level->mode_duration;        
                        if (level->mode_duration == 0) {
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
    }

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
