//
// Declarations
//


//
// Maps
enum Map_Type {
    Map_Ghosts = 0,
    Map_Flee_Ghosts,
    Map_Dot_Small,
    Map_Dot_Large,

    Map_Count,
};


struct Map_Direction {
    Direction direction = Direction_Unknown;
    s32 distance = 0;
};


#ifdef DEBUG
char static *kMap_Names[] = {
    "Map_Ghosts",
    "Map_Flee_Ghosts",
    "Map_Dot_Small",
    "Map_Dot_Large",
    "No_map",
};
#endif


//
// Level
enum Level_Render_Mode {
    Level_Render_Mode_Tiles     = 1 << 1,
    Level_Render_Mode_Items     = 1 << 2,
    Level_Render_Mode_Actors    = 1 << 3,
    Level_Render_Mode_Grid      = 1 << 4,
    Level_Render_Mode_Actor_IDs = 1 << 5,
    Level_Render_Mode_All       = Level_Render_Mode_Tiles | Level_Render_Mode_Items | Level_Render_Mode_Actors,

    Level_Render_Mode_count,
};


struct Level_State {
    Array_Of_Actors actors;
    Tile *tiles = nullptr;
    u32 tile_count = 0;

    u16 score           = 0;
    u16 mode_duration   = 0;
    u16 large_dot_count = 0;
    u16 small_dot_count = 0;
    u16 ghost_count     = 0;
    u16 pacman_count    = 0;
};


#define kLevel_States_Count 100

struct Level {
    Level_State states[kLevel_States_Count];
    Level_State original_state;

    s32 *maps[Map_Count] = {};
    u32 current_map_index = Map_Count; // DEBUG

    Level_State *current_state  = nullptr;
    u32 first_valid_state_index = 0;
    u32 last_valid_state_index  = 0;
    u32 current_state_index     = 0;

    char name[kLevel_Name_Max_Length] = {'\0'};

    Resources *resources;

    Actor_ID pacman_id = {0xFFFF, 0xFFFF};

    u32 id;
    u32 width = 0;
    u32 height = 0;
};




//
// Implementation, Level state
//

void free_level_state(Level_State *state) {
    if (state) {
        if (state->tiles) {
            free(state->tiles);
            state->tiles = nullptr;
            state->tile_count = 0;
        }

        free_array_of_actors(&state->actors);

        state->score           = 0;
        state->mode_duration   = 0;
        state->large_dot_count = 0;
        state->small_dot_count = 0;
        state->ghost_count     = 0;
        state->pacman_count    = 0;
    }
}


void free_all_level_states(Level *level) {
    for (u32 index = 0; index < kLevel_States_Count; ++index) {
        free_level_state(&level->states[index]);
    }

    level->current_state_index = 0;
    level->first_valid_state_index = 0;
    level->last_valid_state_index = 0;
    level->current_state = nullptr;
}


b32 is_valid_level_state_index(Level *level, u32 n) {
    b32 result = false;

    if (level && n < kLevel_States_Count) {
        u32 first_index = level->first_valid_state_index;
        u32 last_index  = level->last_valid_state_index;

        if (first_index <= last_index) {
            result = n >= first_index && n <= last_index;
        }
        else if (last_index < first_index) {
            result = !(n > last_index && n < first_index);
        }
    }

    return result;
}


// NOTE: returns nullptr if level state n is not a valid level state
Level_State *get_valid_level_state_n(Level *level, u32 n) {
    Level_State *result = nullptr;

    if (level && is_valid_level_state_index(level, n)) {
        result = &level->states[n];
    }

    return result;
}


// NOTE: returns nullptr if the current level state is not a valid level state
Level_State *get_current_level_state(Level *level) {
    Level_State *result = get_valid_level_state_n(level, level->current_state_index);
    level->current_state = result;
    return result;
}


static void undo_one_level_state(Level *level) {
    u32 curr_index = level->current_state_index;
    u32 prev_index = curr_index == 0 ? prev_index = (kLevel_States_Count - 1) : curr_index - 1;
    Level_State *prev_state = get_valid_level_state_n(level, prev_index);
    if (prev_state) {
        level->current_state_index = prev_index;
        level->current_state = prev_state;
    }
}


b32 copy_level_state(Level_State *dst, Level_State *src) {
    b32 result = false;

    if (dst && src) {
        dst->score = src->score;
        dst->mode_duration = src->mode_duration;
        dst->large_dot_count = src->large_dot_count;
        dst->small_dot_count = src->small_dot_count;
        dst->ghost_count = src->ghost_count;
        dst->pacman_count = src->pacman_count;
        dst->tile_count = src->tile_count;

        // tiles
        if (dst->tiles)  free(dst->tiles);
        size_t size = src->tile_count * sizeof(Tile);
        dst->tiles = static_cast<Tile *>(calloc(1, size));
        errno_t error = memcpy_s(dst->tiles, size, src->tiles, size);
        if (error != 0) {
            printf("%s() failed to copy tiles from %p to %p, error = %d\n", __FUNCTION__, src, dst, errno);
            return false;
        }
        else {
            result = true;
        }

        // actors
        b32 b32_result = copy_actors(&dst->actors, &src->actors);
        if (result && !b32_result) {
            printf("%s() failed to copy actors from %p to %p, error = %d\n", __FUNCTION__, src, dst, errno);
            result = false;
        }
    }

    return result;
}


void save_current_level_state(Level *level) {
    // We assume that:
    //   first_index, last_index, current_index < kWorld_Level_States_Count
    //   current_index >= first_index && current_index <= last_index
    //

    u32 first_index = level->first_valid_state_index;
    u32 last_index  = level->last_valid_state_index;
    u32 curr_index  = level->current_state_index;
    u32 next_index = (curr_index + 1) < kLevel_States_Count ? curr_index + 1 : 0;

    // Free levels
    // We're adding a new level state in between older states, we need to free the states after the new one.
    if (curr_index != last_index) {
        if (curr_index < last_index) {
            for (u32 index = curr_index + 1; index <= last_index; ++index) {
                free_level_state(&level->states[index]);
            }
            last_index = curr_index;
        }
        else if (curr_index > last_index) {
            for (u32 index = curr_index + 1; index < kLevel_States_Count; ++index) {
                free_level_state(&level->states[index]);
            }
            for (u32 index = 0; index <= last_index; ++index) {
                free_level_state(&level->states[index]);
            }
            last_index = curr_index;
        }
        else {
            assert(0); // Shouldn't happen?
        }
    }
    else if (next_index == first_index) { // Will we wrap around?
        free_level_state(get_valid_level_state_n(level, first_index));
        first_index = (first_index + 1) < kLevel_States_Count ? first_index + 1 : 0;
    }

    Level_State *curr_level_state = &level->states[curr_index];
    Level_State *next_level_state = &level->states[next_index];
    copy_level_state(next_level_state, curr_level_state);

    level->current_state_index = next_index;
    level->current_state = next_level_state;
    level->first_valid_state_index = first_index;
    level->last_valid_state_index = level->current_state_index;
}


static void redo_one_level_state(Level *level) {
    u32 curr_index = level->current_state_index;
    u32 next_index = curr_index < (kLevel_States_Count - 1) ? curr_index + 1 : 0;
    Level_State *next_level_state = get_valid_level_state_n(level, next_index);
    if (next_level_state) {
        level->current_state_index = next_index;
        level->current_state = next_level_state;
    }
}


static void reset_level(Level *level) {
    free_all_level_states(level);
    copy_level_state(&level->states[0], &level->original_state);
    level->current_state = &level->states[0];
}




//
// Level
//

void fini_level(Level *level) {
    if (level) {
        free_all_level_states(level);
        free_level_state(&level->original_state);

        level->first_valid_state_index = 0;
        level->last_valid_state_index = 0;
        level->current_state_index = 0;
        level->current_state = nullptr;

        level->width = 0;
        level->height = 0;
        level->name[0] = '\0';
        level->pacman_id.index = 0xFFFF;
        level->pacman_id.salt = 0xFFFF;

        for (u32 map_index = 0; map_index < Map_Count; ++map_index) {
            if (level->maps[map_index]) {
                free(level->maps[map_index]);
                level->maps[map_index] = nullptr;
            }
        }
    }
};


void init_level(Level *level, Resources *resources) {
    fini_level(level);

    level->resources = resources;
    level->pacman_id.index = 0xFFFF;
    level->pacman_id.salt = 0xFFFF;

    level->current_state_index = level->first_valid_state_index;
    level->current_state = get_valid_level_state_n(level, level->current_state_index);

    level->current_map_index = Map_Count; // DEBUG
}


static void init_level_as_copy_of_level(Level *dst, Level *src, Level_State *state) {
    init_level(dst, src->resources);

    copy_level_state(&dst->original_state, state);
    copy_level_state(&dst->states[0], &dst->original_state);

    dst->pacman_id = src->pacman_id;
    dst->width     = src->width;
    dst->height    = src->height;
    dst->id        = src->id;

    _snprintf_s(dst->name, kLevel_Name_Max_Length, _TRUNCATE, "%s", src->name);
}




//
// Queries
//

Tile *get_tile_at(Level *level, Level_State *state, v2u P) {
    Tile *result = nullptr;

    if (state && state && state->tiles) {
        if (P.x < level->width && P.y < level->height) {
            result = &state->tiles[(level->width * P.y) + P.x];
        }
    }

    return result;
}


Tile *get_tile_at(Level *level, v2u P) {
    Tile *result = nullptr;

    if (level && level->current_state && level->current_state->tiles) {
        if (P.x < level->width && P.y < level->height) {
            result = &level->current_state->tiles[(level->width * P.y) + P.x];
        }
    }

    return result;
}


Tile *get_tile_at(Level *level, u32 x, u32 y) {
    return get_tile_at(level, V2u(x, y));
}


Tile *get_tile_at(Level *level, s32 x, s32 y) {
    Tile *result = nullptr;

    if (x >= 0 && y >= 0) {
        result = get_tile_at(level, static_cast<u32>(x), static_cast<u32>(y));
    }

    return result;
}


b32 move_is_possible(Level *level, Actor *actor, v2s dP) {
    b32 result = false;

    s32 width = static_cast<s32>(level->width);
    s32 height = static_cast<s32>(level->height);
    v2s next_pos = actor->position + dP;

    if (next_pos.x >= 0 && next_pos.x < width && next_pos.y >= 0 && next_pos.y < height) {
        Tile *tile = &level->current_state->tiles[(width * next_pos.y) + next_pos.x];
        if (tile_is_traversable(tile)) {
            result = true;
        }
    }

    return result;
}


Actor *get_pacman(Level *level) {
    Actor *result = nullptr;
    result = get_actor(&level->current_state->actors, level->pacman_id);

    return result;
}


Actor *get_actor_at(Level *level, v2u P) {
    Actor *result = nullptr;

    Tile *tile = get_tile_at(level, P);
    if (tile) {
        result = get_actor(&level->current_state->actors, tile->actor_id);
    }

    return result;
}


inline Actor *get_actor_at(Level *level, u32 x, u32 y) {
    return get_actor_at(level, V2u(x, y));
}




//
// Change level state
//

void change_pacman_mode(Level *level, Actor_Mode mode) {
    Actor_Mode other_mode = static_cast<Actor_Mode>(!static_cast<b32>(mode));

    Level_State *state = level->current_state;
    for (u32 index = 0; index < state->actors.count; ++index) {
        Actor *actor = &state->actors.data[index];
        if (actor->type == Actor_Type_Pacman) {
            actor->mode = mode;
        }
        else {
            actor->mode = other_mode;
        }
    }
}


void kill_actor(Level *level, Actor *actor) {
    Tile *tile = get_tile_at(level, actor->position);
    if (tile) {
        if (tile->actor_id == actor->id) {
            tile->actor_id = kActor_ID_Null;
        }
    }

    Level_State *state = level->current_state;
    kill_actor(&state->actors, actor);

    if (actor_is_ghost(actor)) {
        state->score -= kGhost_Value;
        --state->ghost_count;
    }
    else if (actor->type == Actor_Type_Pacman) {
        --state->pacman_count;
    }
}




//
// Rendering
//

void draw_level(Renderer *renderer, Level *level, u32 render_mode, u32 microseconds_since_start) {
    Level_State *state = level->current_state;
    Font *font = &level->resources->font;
    Resources *resources = level->resources;
    u32 width = level->width;
    u32 height = level->height;


    //
    // Draw background
#if 0
    u32 cell_size = renderer->backbuffer_width / level->width;
    assert(cell_size == kCell_Size);

    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            v2u P = V2u(cell_size * x, cell_size * y);

            if (bitmaps->background.data) {
                renderer->draw_bitmap(&resources->bitmaps->background);
            }
        }
    }
#endif

    //
    // Draw all tiles
    if ((render_mode & Level_Render_Mode_Tiles) | (render_mode & Level_Render_Mode_Items)) {
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                v2u P = V2u(kCell_Size * x, kCell_Size * y);
                if (render_mode & Level_Render_Mode_Tiles) {
                    draw_tile(renderer, resources, tile, P);
                }
                if (render_mode & Level_Render_Mode_Items) {
                    draw_item(renderer, resources, tile, P);
                }
            }
        }
    }


    //
    // Draw grid
    if (render_mode & Level_Render_Mode_Grid) {
        v4u8 const border_colour = {255, 255, 255, 75};
        for (u32 y = 0; y < height; ++y) {
            for(u32 x = 0; x < width; ++x) {
                renderer->draw_rectangle_outline(V2u(kCell_Size * x, kCell_Size * y), kCell_Size, kCell_Size, border_colour);
            }
        }
    }


    //
    // Draw all actors
    if (render_mode & Level_Render_Mode_Actors) {
        Actor *pacman = get_pacman(level);

        // Timings for ghost animation
        f32 constexpr k = 1.0f / 1000000.0f;
        f32 t = static_cast<f32>(microseconds_since_start) * k;
        f32 x_offset = 0.0f;//sinf(3.6f*t);
        f32 y_offset = 3.0f*cosf(1.8f*t);

        for (u32 index = 0; index < state->actors.count; ++index) {
            Actor *actor = &state->actors.data[index];
            if (actor_is_alive(actor)) {
                if (actor_is_ghost(actor)) {
                    draw_ghost(renderer, resources, actor, pacman, x_offset, y_offset);
                }
                else if (actor->type == Actor_Type_Pacman) {
                    draw_pacman(renderer, resources, actor, microseconds_since_start);
                }
            }
        }
    }


    if (render_mode & Level_Render_Mode_Actor_IDs) {
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
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

                    text_pos = V2u(kCell_Size * x, kCell_Size * y);
                    renderer->print(font, text_pos, text, v4u8_black);
                    renderer->print(font, text_pos + text_offset, text, v4u8_white);
                }
            }
        }
    }


    //
    // Score
    {
        char text[50];
        _snprintf_s(text, 50, _TRUNCATE, "Score %u", state->score);
        renderer->print(font, V2u(10, 10), text);
    }
}


static void draw_level_win_text (Renderer *renderer, Font *font) {
    char const *win_text = "VICTORIOUS!";
    v2u text_dim = get_text_dim(font, win_text);
    v2u Pt = V2u((renderer->get_backbuffer_width() - text_dim.x) / 2, 300);

    v2u offset = V2u(50, 50);
    v2u rectangle_size = text_dim + (2*offset);
    renderer->draw_filled_rectangle(Pt - offset - V2u(1, 1), rectangle_size.x + 2, rectangle_size.y + 2, v4u8_green);
    renderer->draw_filled_rectangle(Pt - offset            , rectangle_size.x    , rectangle_size.y    , v4u8_black);

    renderer->print(font, Pt, win_text, v4u8_green);
}


static void draw_level_lost_text(Renderer *renderer, Font *font) {
    v2u Pc = V2u(renderer->get_backbuffer_width(), renderer->get_backbuffer_height()) / 2;
    u32 row_offset = 30;
    u32 half_row_offset = row_offset / 2;

    char const *defeat_text = "DEFEATED!";
    v2u defeat_text_dim = get_text_dim(font, defeat_text);
    v2u Ptd = Pc;
    Ptd.x -= (defeat_text_dim.x / 2);
    Ptd.y += half_row_offset;

    char const *reset_text = "Press Enter or R to reset level and try again!";
    v2u reset_text_dim = get_text_dim(font, reset_text);
    v2u Ptr = Pc;
    Ptr.x -= (reset_text_dim.x / 2);
    Ptr.y -= (half_row_offset + defeat_text_dim.y);

    v2u text_dim = V2u(max(defeat_text_dim.x, reset_text_dim.x), defeat_text_dim.y + reset_text_dim.y + row_offset);

    v2u offset = V2u(25, 25);
    v2u rectangle_size = text_dim + (2*offset);
    renderer->draw_filled_rectangle(Ptr - offset - V2u(1, 1), rectangle_size.x + 2, rectangle_size.y + 2, v4u8_red);
    renderer->draw_filled_rectangle(Ptr - offset            , rectangle_size.x    , rectangle_size.y    , v4u8_black);
    renderer->print(font, Ptd, defeat_text, v4u8_red);
    renderer->print(font, Ptr, reset_text, v4u8_red);
}




//
// Loading
//

void adjust_walls_in_level(Level *level, Level_State *state) {
    for (u32 y = 0; y < level->height; ++y) {
        for (u32 x = 0; x < level->width; ++x) {
            u32 curr_index = (level->width * y) + x;
            u32 sum = 0;

            if (tile_has_wall(&state->tiles[curr_index])) {
                if (x < (level->width - 1)) {
                    u32 right_index = (level->width * y) + x + 1;
                    if (tile_has_wall(&state->tiles[right_index])) {
                        sum += 1;
                    }
                }
                if (y < (level->height - 1)) {
                    u32 up_index = (level->width * (y + 1)) + x;
                    if (tile_has_wall(&state->tiles[up_index])) {
                        sum += 2;
                    }
                }
                if (x > 0) {
                    u32 left_index = (level->width * y) + x - 1;
                    if (tile_has_wall(&state->tiles[left_index])) {
                        sum += 4;
                    }
                }
                if (y > 0) {
                    u32 down_index = (level->width * (y - 1)) + x;
                    if (tile_has_wall(&state->tiles[down_index])) {
                        sum += 8;
                    }
                }

                state->tiles[curr_index].type = static_cast<Tile_Type>(sum);
            }
        }
    }
}


static char get_char_from_actor_type(Actor *actor) {
    char result = '?';

    if (!actor) {
        result = '.';
    }
    else {
        switch (actor->type) {
            case Actor_Type_Ghost_Red:    { result = '1'; } break;
            case Actor_Type_Ghost_Pink:   { result = '2'; } break;
            case Actor_Type_Ghost_Cyan:   { result = '3'; } break;
            case Actor_Type_Ghost_Orange: { result = '4'; } break;
            case Actor_Type_Pacman:       { result = 'P'; } break;
            default:                      { result = '?'; } break;
        }
    }

    return result;
}


static Actor_Type get_actor_type_from_char(char c) {
    Actor_Type result = Actor_Type_Unknown;

    switch (c) {
        case '1': { result = Actor_Type_Ghost_Red;    } break;
        case '2': { result = Actor_Type_Ghost_Pink;   } break;
        case '3': { result = Actor_Type_Ghost_Cyan;   } break;
        case '4': { result = Actor_Type_Ghost_Orange; } break;
        case 'P': { result = Actor_Type_Pacman;       } break;
        default:  { result = Actor_Type_Unknown;      } break;
    }

    return result;
}


static char get_char_from_tile_type(Tile *tile) {
    char result = '?';

    if (tile_has_wall(tile)) {
        result = 'W';
    }
    else {
        switch (tile->type) {
            case Tile_Type_None:  { result = '.'; } break;
            case Tile_Type_Floor: { result = '-'; } break;
        }
    }

    return result;
}


char get_char_from_item_type(Tile *tile) {
    char result = '?';

    switch (tile->item.type) {
        case Item_Type_None:      { result = '.'; } break;
        case Item_Type_Dot_Small: { result = '+'; } break;
        case Item_Type_Dot_Large: { result = 'X'; } break;
    }

    return result;
}


static b32 save_level(Level *level) {
    b32 result = false;

    u32 const buffer_size = 512;
    char buffer[buffer_size];
    _snprintf_s(buffer, buffer_size, _TRUNCATE, "data//levels//%u.level_txt", level->id);

    HANDLE file_handle;
    result = win32_open_file_for_writing(buffer, &file_handle);
    if (result) {
        DWORD bytes_written;
        DWORD bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "Name:\"%s\"\n", level->name);
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "ID:%u\n", level->id);
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "Width:%u\n", level->width);
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "Height:%u\n", level->width);
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        Level_State *state = level->current_state;

        //
        // Tiles
        bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "Layer_tiles:\n");
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        for (u32 y = 0; y < level->height; ++y) {
            for (u32 x = 0; x < level->width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "%c", get_char_from_tile_type(tile));
                result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
                assert(bytes_to_write == bytes_written);
            }
            WriteFile(file_handle, "\n", 1, &bytes_written, nullptr);
        }

        //
        // Items
        bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "Layer_items:\n");
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        for (u32 y = 0; y < level->height; ++y) {
            for (u32 x = 0; x < level->width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "%c", get_char_from_item_type(tile));
                result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
                assert(bytes_to_write == bytes_written);
            }
            WriteFile(file_handle, "\n", 1, &bytes_written, nullptr);
        }

        //
        // Actors
        bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "Layer_actors:\n");
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        for (u32 y = 0; y < level->height; ++y) {
            for (u32 x = 0; x < level->width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                Actor *actor = get_actor(&state->actors, tile->actor_id);
                bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "%c", get_char_from_actor_type(actor));
                result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
                assert(bytes_to_write == bytes_written);
            }
            WriteFile(file_handle, "\n", 1, &bytes_written, nullptr);
        }

        CloseHandle(file_handle);
    }

    return result;
}


static b32 add_actor(Tokenizer *tokenizer, Level *level, Level_State *state, u32 x, u32 y, Actor_Type type) {
    b32 result = false;

    if (type < Actor_Type_Count ) {
        Actor *curr_actor = new_actor(&state->actors);
        assert(curr_actor);

        result = true;
        curr_actor->position = V2u(x, y);
        curr_actor->type = type;
        curr_actor->state = Actor_State_Idle;
        curr_actor->direction = Direction_Right;

        Tile *curr_tile = get_tile_at(level, state, V2u(x, y));
        assert(curr_tile);
        curr_tile->actor_id = curr_actor->id;


        //
        // Ghost
        if (actor_type_is_ghost(type)) {
            curr_actor->mode = Actor_Mode_Predator;
            state->score += kGhost_Value;
            ++state->ghost_count;
        }


        //
        // Pacman
        else {
            if (state->pacman_count == 0) {
                curr_actor->mode = Actor_Mode_Prey;
                level->pacman_id = curr_actor->id;
                ++state->pacman_count;
            }
            else {
                if (tokenizer) {
                    tokenizer->error = true;
                    _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE, "In %s at %u:%u, found more than one pacman",
                                tokenizer->path_and_name, tokenizer->line_number, tokenizer->line_position);
                }
                result = false;
            }
        }
    }

    return result;
}


static b32 load_level(Level *level, Resources *resources, char const *name) {
    b32 result = false;

    Tokenizer tokenizer;
    result = init_tokenizer(&tokenizer, "data\\levels\\", name);
    if (result) {
        fini_level(level);
        init_level(level, resources);

        eat_spaces_and_newline(&tokenizer);
        Token token;

        // Name of level
        require_identifier_with_exact_name(&tokenizer, &token, "Name");
        require_token(&tokenizer, &token, Token_colon);
        require_token(&tokenizer, &token, Token_string);
        _snprintf_s(level->name, kLevel_Name_Max_Length, _TRUNCATE, "%s", token.data);

        // Level id
        require_identifier_with_exact_name(&tokenizer, &token, "ID");
        require_token(&tokenizer, &token, Token_colon);
        require_token(&tokenizer, &token, Token_number);
        level->id = get_u32_from_token(&token);

        //
        // Width
        require_identifier_with_exact_name(&tokenizer, &token, "Width");
        require_token(&tokenizer, &token, Token_colon);
        require_token(&tokenizer, &token, Token_number);
        level->width = get_u32_from_token(&token);

        //
        // Height
        require_identifier_with_exact_name(&tokenizer, &token, "Height");
        require_token(&tokenizer, &token, Token_colon);
        require_token(&tokenizer, &token, Token_number);
        level->height = get_u32_from_token(&token);


        //
        // Tiles
        require_identifier_with_exact_name(&tokenizer, &token, "Layer_tiles");
        require_token(&tokenizer, &token, Token_colon);
        eat_spaces_and_newline(&tokenizer);
        reload(&tokenizer);

        Level_State *state = &level->original_state;
        state->tile_count = level->width * level->height;
        state->tiles = static_cast<Tile *>(calloc(state->tile_count, sizeof(Tile)));
        assert(state->tiles);

        b32 should_loop = !tokenizer.error;
        b32 should_advance = true;

        //for (s32 y = level->height - 1; (y >= 0) && should_loop; --y) {
        for (u32 y = 0; (y < level->height) && should_loop; ++y) {
            for (u32 x = 0; (x < level->width) && should_loop; ++x) {
                Tile *tile = &state->tiles[(y * level->width) + x];

                switch (tokenizer.curr_char) {
                    case '.': {
                        tile->type = Tile_Type_None;
                    } break;

                    case '-': {
                        tile->type = Tile_Type_Floor;
                    } break;

                    case 'W': {
                        tile->type = Tile_Type_Wall_0;
                    } break;

                    default: {
                        token = get_token(&tokenizer);
                        if (token.type == Token_comment) {
                            skip_to_next_line(&tokenizer);
                            should_advance = false;
                        }
                        else {
                            tokenizer.error = true;
                            _snprintf_s(tokenizer.error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE, "In %s at %u:%u, found invalid token in the tile layer",
                                        tokenizer.path_and_name, token.line_number, token.line_position);
                            should_loop = false;
                        }
                    } break;
                }

                if (should_advance) {
                    advance(&tokenizer);
                }
                should_advance = true;

                eat_spaces_and_newline(&tokenizer);
                reload(&tokenizer);
            }
        }


        //
        // Items
        require_identifier_with_exact_name(&tokenizer, &token, "Layer_items");
        require_token(&tokenizer, &token, Token_colon);
        eat_spaces_and_newline(&tokenizer);
        reload(&tokenizer);

        should_loop = !tokenizer.error;
        should_advance = true;

        //for (s32 y = level->height - 1; (y >= 0) && should_loop; --y) {
        for (u32 y = 0; (y < level->height) && should_loop; ++y) {
            for (u32 x = 0; (x < level->width) && should_loop; ++x) {
                Tile *tile = &state->tiles[(y * level->width) + x];

                switch (tokenizer.curr_char) {
                    case '.': {
                        tile->item.type = Item_Type_None;
                        tile->item.value = 0;
                    } break;

                    case '+': {
                        tile->item.type = Item_Type_Dot_Small;
                        tile->item.value = kDot_Small_Value;
                        state->score += tile->item.value;
                        ++state->small_dot_count;
                    } break;

                    case 'X': {
                        tile->item.type = Item_Type_Dot_Large;
                        tile->item.value = kDot_Large_Value;
                        state->score += tile->item.value;
                        ++state->large_dot_count;
                    } break;

                    default: {
                        token = get_token(&tokenizer);
                        if (token.type == Token_comment) {
                            skip_to_next_line(&tokenizer);
                            should_advance = false;
                        }
                        else {
                            tokenizer.error = true;
                            _snprintf_s(tokenizer.error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE, "In %s at %u:%u, found invalid token in the items layer",
                                        tokenizer.path_and_name, token.line_number, token.line_position);
                            should_loop = false;
                        }
                    } break;
                }

                if (should_advance) {
                    advance(&tokenizer);
                }
                should_advance = true;

                eat_spaces_and_newline(&tokenizer);
                reload(&tokenizer);
            }
        }


        //
        // Actors
        require_identifier_with_exact_name(&tokenizer, &token, "Layer_actors");
        require_token(&tokenizer, &token, Token_colon);
        eat_spaces_and_newline(&tokenizer);
        reload(&tokenizer);

        should_loop = !tokenizer.error;
        should_advance = true;

        //for (s32 y = level->height - 1; (y >= 0) && should_loop; --y) {
        for (u32 y = 0; (y < level->height) && should_loop; ++y) {
            for (u32 x = 0; (x < level->width) && should_loop; ++x) {
                Tile *tile = &state->tiles[(y * level->width) + x];

                switch (tokenizer.curr_char) {
                    case '.': {
                        tile->actor_id = kActor_ID_Null;
                    } break;

                    case 'P':
                    case '1':
                    case '2':
                    case '3':
                    case '4': {
                        Actor_Type type = get_actor_type_from_char(tokenizer.curr_char);
                        should_loop = add_actor(&tokenizer, level, state, x, y, type);
                    } break;

                    default: {
                        token = get_token(&tokenizer);
                        if (token.type == Token_comment) {
                            skip_to_next_line(&tokenizer);
                            should_advance = false;
                        }
                        else {
                            tokenizer.error = true;
                            _snprintf_s(tokenizer.error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE, "In %s at %u:%u, found invalid token in the actor layer",
                                        tokenizer.path_and_name, token.line_number, token.line_position);
                            should_loop = false;
                        }
                    } break;
                }

                if (should_advance) {
                    advance(&tokenizer);
                }
                should_advance = true;

                eat_spaces_and_newline(&tokenizer);
                reload(&tokenizer);
            }
        }



        //
        // Done
        adjust_walls_in_level(level, &level->original_state);
        copy_level_state(&level->states[0], &level->original_state);
        level->first_valid_state_index = 0;
        level->last_valid_state_index = 0;
        level->current_state_index = 0;
        level->current_state = &level->states[0];

        fini_tokenizer(&tokenizer);
    }

    return result;
}




//
// #_Maps
// Dijkstra ("dijkstra-maps")
// - http://www.roguebasin.com/index.php?title=The_Incredible_Power_of_Dijkstra_Maps
//

void process_map(s32 *map, Tile *tiles, u32 width, u32 height, u32 max_value = 999) {
    u32 change_count = 1;
    while (change_count > 0) {
        change_count = 0;
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                u32 curr_index = (width * y) + x;

                Tile *tile = &tiles[curr_index];
                if (!tile_is_traversable(tile))  continue;

                s32 min_value = max_value;

                if (x < (width - 1)) {
                    u32 next_index = (width * y) + x + 1;
                    s32 *next_value = &map[next_index];
                    min_value = min_value > *next_value ? *next_value : min_value;
                }

                if (y < (height - 1)) {
                    u32 next_index = (width * (y + 1)) + x;
                    s32 *next_value = &map[next_index];
                    min_value = min_value > *next_value ? *next_value : min_value;
                }

                if (x > 0) {
                    u32 next_index = (width * y) + x - 1;
                    s32 *next_value = &map[next_index];
                    min_value = min_value > *next_value ? *next_value : min_value;
                }

                if (y > 0) {
                    u32 next_index = (width * (y - 1)) + x;
                    s32 *next_value = &map[next_index];
                    min_value = min_value > *next_value ? *next_value : min_value;
                }

                s32 *curr_value = &map[curr_index];
                if (*curr_value > (min_value + 1)) {
                    *curr_value = min_value + 1;
                    ++change_count;
                }
            }
        }
    }
}


void create_maps_off_level(Level *level) {
    size_t map_size_in_bytes = level->width * level->height * sizeof(s32);
    for (u32 map_index = 0; map_index < Map_Count; ++map_index) {
        s32 **map = &level->maps[map_index];
        if (*map) {
            free(*map);
            *map = nullptr;
        }

        *map = static_cast<s32 *>(malloc(map_size_in_bytes));
        assert(*map);
    }

    u32 width = level->width;
    u32 height = level->height;
    Tile *tiles = level->current_state->tiles;
    u32 constexpr max_value = 300;


    //
    // Process the maps
    for (u32 map_index = 0; map_index < Map_Count; ++map_index) {
        s32 *map = level->maps[map_index];

        //
        // Set initial values
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                u32 index = (width * y) + x;
                s32 *value = &map[index];
                Tile *tile = &tiles[index];
                *value = max_value;

                if (tile_is_traversable(tile)) {
                    if (map_index == Map_Ghosts || map_index == Map_Flee_Ghosts) {
                        Actor *actor = get_actor_at(level, x, y);
                        if (actor && actor_is_ghost(actor)) {
                            *value = 0;
                        }
                    }
                    else if (map_index == Map_Dot_Small) {
                        if (tile->item.type == Item_Type_Dot_Small) {
                            *value = 0;
                        }
                    }
                    else if (map_index == Map_Dot_Large) {
                        if (tile->item.type == Item_Type_Dot_Large) {
                            *value = 0;
                        }
                    }
                }
            }
        }

        process_map(map, tiles, width, height, max_value);
    }


    //
    // For the flee map, we want to "invert" the values so that when "rolling down" it, we will
    // move away from the ghosts but moving away in a manner that doesn't always lead to the corners.
    {
        s32 *map = level->maps[Map_Flee_Ghosts];
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                u32 curr_index = (width * y) + x;

                Tile *tile = &tiles[curr_index];
                if (!tile_is_traversable(tile))  continue;

                map[curr_index] = -1 * map[curr_index];
            }
        }

        process_map(map, tiles, width, height, max_value);
    }
}


Map_Direction get_shortest_direction_on_map(Level *level, s32 *map, Actor *actor) {
    Map_Direction result = {Direction_Unknown, 0x7FFFFFFF};
    //Level *level = get_current_level_state(world);

    s32 constexpr X[] = {1, 0, -1, 0};
    s32 constexpr Y[] = {0, 1, 0, -1};

    v2u P = actor->position;

    if (P.x < level->width && P.y < level->height) {
        for (u32 index = 0; index < 4; ++index) {
            v2s dP = v2s(X[index], Y[index]);
            if (move_is_possible(level, actor, dP)) {
                v2s new_P = P + dP;
                s32 value = map[(level->width * new_P.y) + new_P.x];
                if (value < result.distance) {
                    result.distance = value;
                    result.direction = static_cast<Direction>(index);
                }
            }
        }
    }

    return result;
};


static void draw_maps(Renderer *renderer, Level *level, s32 **maps, u32 map_index) {
    //
    // Draw maps, DEBUG
    u32 level_width  = level->width;
    u32 level_height = level->height;
    Font *font = &level->resources->font;

    if (map_index < Map_Count && maps[map_index]) {
        char text[10];

        u32 constexpr kOffset_x = kCell_Size / 2;
        u32 constexpr kOffset_y = kCell_Size / 2;

        for (u32 y = 0; y < level_height; ++y) {
            for (u32 x = 0; x < level_width; ++x) {
                u32 index = (level_width * y) + x;
                s32 value = maps[map_index][index];
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

                renderer->print(font, text_pos, text, V4u8(100, 100, 100, 255));
                renderer->print(font, text_pos + text_offset, text, v4u8_white);
            }
        }


#ifdef DEBUG
        char map_name_text[50];
        u32 error = _snprintf_s(map_name_text, 50, _TRUNCATE, "Map: %s", kMap_Names[map_index]);
        assert(error > 0);
        v2u text_dim = get_text_dim(font, map_name_text);
        u32 char_width = text_dim.x / static_cast<u32>(strlen(map_name_text));
        renderer->print(font, V2u(renderer->get_backbuffer_width() - text_dim.x - char_width, 10), map_name_text);
#endif
    }
}




//
// A dynamic array of levels
//

struct Array_Of_Levels {
    Level *data = nullptr;
    u16 capacity = 0;
    u16 count = 0;
};


static void free_array_of_levels(Array_Of_Levels *levels) {
    if (levels) {
        for (u32 index = 0; index < levels->capacity; ++index) {
            Level *level = &levels->data[index];
            fini_level(level);
        }
    }
}


static void init_array_of_levels(Array_Of_Levels *levels, u16 capacity = 10) {
    if (levels) {
        if (levels->data)  free_array_of_levels(levels);
        levels->capacity = capacity;
        levels->count = 0;
        levels->data = static_cast<Level *>(calloc(levels->capacity, sizeof(Level)));
    }
}


b32 grow_array_of_levels(Array_Of_Levels *array) {
    b32 result = false;

    if (array) {
        u16 new_capacity = array->capacity == 0 ? 10 : 2 * array->capacity;
        size_t new_size = sizeof(Level) * new_capacity;
        void *new_ptr = realloc(array->data, new_size);
        if (new_ptr) {
            array->capacity = new_capacity;
            array->data = static_cast<Level *>(new_ptr);
            memset(array->data, 0, array->capacity * sizeof(Level));
            result = true;
        }
        else {
            printf("%s in %s failed to reallocate memory!\n", __FUNCTION__, __FILE__);
        }
    }

    return result;
}


Level *get_next_empty_level(Array_Of_Levels *levels) {
    Level *result = nullptr;

    if (levels) {
        if ((levels->capacity == 0) || (levels->count == (levels->capacity -1))) {
            if (grow_array_of_levels(levels)) {
                result = &levels->data[levels->count++];
            }
        }
        else {
            result = &levels->data[levels->count++];
        }
    }

    return result;
}


static Level *get_level_with_id(Array_Of_Levels *levels, u32 level_id) {
    Level *result = nullptr;

    if (levels) {
        for (u32 index = 0; index < levels->capacity; ++index) {
            Level *level = &levels->data[index];
            if (level->id == level_id) {
                result = level;
                break;
            }
        }
    }

    return result;
}


static Level *get_level_with_exact_name(Array_Of_Levels *levels, char const *name) {
    Level *result = nullptr;

    if (levels) {
        for (u32 index = 0; index < levels->capacity; ++index) {
            Level *level = &levels->data[index];
            if (strcmp(level->name, name) == 0) {
                result = level;
                break;
            }
        }
    }

    return result;
}


static u32 load_levels_from_disc(Array_Of_Levels *levels, Resources *resources) {
    u32 loaded_levels = 0;

    if (levels) {
        free_array_of_levels(levels);

        WIN32_FIND_DATAA find_data;
        HANDLE find_handle = FindFirstFileA("data\\levels\\*.level_txt", &find_data);
        b32 load_result = true;
        b32 find_result = true;

        while ((load_result) && (find_result) && (find_handle != INVALID_HANDLE_VALUE)) {
            Level *level = get_next_empty_level(levels);
            if (level) {
                load_result = load_level(level, resources, find_data.cFileName);
                if (load_result)  ++loaded_levels;
            }
            find_result = FindNextFileA(find_handle, &find_data);
        }

        FindClose(find_handle);
    }

    return loaded_levels;
}




//
// #_Darray
//

struct u32_darray {
    u32 *data = nullptr;
    u32 capacity = 0;
    u32 count = 0;

    u32 operator [] (u32 index) {
        u32 result = data[index];
        return result;
    }
};


static void free_darray(u32_darray *array) {
    if (array) {
        if (array->data) {
            free(array->data);
            array->data = nullptr;
        }
        array->capacity = 0;
        array->count = 0;
    }
}


static void init_darray(u32_darray *array, u32 capacity = 10) {
    if (array && capacity > 0) {
        if (array->data)  free_darray(array);
        array->capacity = capacity;
        array->count = 0;
        array->data = static_cast<u32 *>(malloc(array->capacity * sizeof(u32)));
    }
}

static void grow_darray(u32_darray *array) {
    if (array) {
        u32 new_capacity = array->capacity == 0 ? 10 : 2 * array->capacity;
        size_t new_size = sizeof(u32) * new_capacity;
        void *new_ptr = realloc(array->data, new_size);
        if (new_ptr) {
            array->capacity = new_capacity;
            array->data = static_cast<u32 *>(new_ptr);
        }
        else {
            assert(0); // TODO: handle this
        }
    }
}

static void push_u32(u32_darray *array, u32 value) {
    if (array) {
        if (array->count >= array->capacity) {
            grow_darray(array);
        }
        array->data[array->count++] = value;
    }
}


static b32 read_level_set_from_disc(u32_darray *u32_array, char const *level_set_name) {
    b32 result = false;

    if (u32_array && level_set_name) {
        Tokenizer tokenizer;
        result = init_tokenizer(&tokenizer, "data\\levels\\", level_set_name);
        while (!is_eof(&tokenizer) && result) {
            Token token;
            result = require_token(&tokenizer, &token, Token_number);
            s32 level_id = -1;
            if (result) {
                level_id = get_u32_from_token(&token);
                if (level_id >= 0) {
                    push_u32(u32_array, level_id);
                }
            }

            if (!is_eof(&tokenizer)) {
                result = require_token(&tokenizer, &token, Token_comma);
            }
        }

        fini_tokenizer(&tokenizer);
    }

    return result;
}
