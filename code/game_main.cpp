//
// Player profile
//

#define kPlayer_Profile_Name_Max_Length 21
#define kLevel_Result_Name_Max_Length 21



//
// #_Level_Result
//
struct Level_Result {
    char name[kLevel_Result_Name_Max_Length];
    u16 max_score = 0;
    u16 score = 0;    
};



//
// #_ Array_Of_Level_Result
//
struct Array_Of_Level_Result {
    Level_Result *data = nullptr;
    u32 capacity = 0;
    u32 count = 0;
};

void clear_level_result(Level_Result *level_result) {
    if (level_result) {
        level_result->name[0] = '\0';
        level_result->max_score = 0;
        level_result->score = 0;
    }
}

void free_array_of_level_result(Array_Of_Level_Result *array) {
    if (array) {        
        if (array->data) {
            free(array->data);
            array->data = nullptr;
        }
        array->count = 0;
        array->capacity = 0;
    }
}

void grow_array_of_level_result(Array_Of_Level_Result *array, u32 new_capacity = 0) {
    u32 old_capacity = array->capacity;
    if (new_capacity == 0) {
        if (old_capacity == 0) {
            new_capacity = 2;
        }
        else {
            new_capacity = 2 * old_capacity;
        }
    }    
    size_t new_size = new_capacity * sizeof(Level_Result);
    void *new_ptr = realloc(array->data, new_size);
    if (new_ptr) {
        array->data = static_cast<Level_Result *>(new_ptr);
        array->capacity = new_capacity;
    }
    else {
        assert(0);
    }

    size_t old_size = old_capacity * sizeof(Level_Result);
    if (old_size > 0) {
        memset(&array->data[old_capacity], 0, new_size - old_size);
    }
    else {
        memset(array->data, 0, new_size);
    }
}

Level_Result *get_level_result(Array_Of_Level_Result const *array, char const *level_name) {
    Level_Result *result = nullptr;

    if (array && level_name) {
        for (u32 index = 0; index < array->count; ++index) {
            Level_Result *level_result = &array->data[index];
            if (strcmp(level_result->name, level_name) == 0) {
                result = level_result;
                break;
            }
        }
    }

    return result;
}

//
// NOTE: We'll need the original state of the level here!
Level_Result *add_level_result(Array_Of_Level_Result *array, Level const *level, u16 score) {
    Level_Result *result = get_level_result(array, level->name);

    if (!result) {
        assert(array->capacity >= array->count);
        if (array->capacity <= array->count) {
            grow_array_of_level_result(array);
        }
        result = &array->data[array->count++];
    }
    assert(result);

    result->max_score = level->score;
    result->score = score;
    _snprintf_s(result->name, kLevel_Result_Name_Max_Length, _TRUNCATE, "%s", level->name);

    return result;
}

void remove_level_result(Array_Of_Level_Result *array, char const *level_name) {
    for (u32 index = 0; index < array->count; ++index) {
        Level_Result *level_result = &array->data[index];
        if (strcmp(level_name, level_result->name) == 0) {
            if (index == (array->count - 1)) {
                clear_level_result(level_result);                
            }
            else {
                Level_Result *last = &array->data[array->count - 1];
                *level_result = *last;
                clear_level_result(last);
            }
            --array->count;
        }
    }
}




//
// #_World_Progress
//
struct World_Progress {
    char name[kWorld_Name_Max_Length];
    Array_Of_Level_Result level_results;
};

void free_world_progress(World_Progress *world_progress) {
    free_array_of_level_result(&world_progress->level_results);
    world_progress->name[0] = '\0';
}

void init_world_progress(World_Progress *world_progress, World const *world) {
    free_world_progress(world_progress);

    u32 copied_chars = _snprintf_s(world_progress->name, kWorld_Name_Max_Length, _TRUNCATE, "%s", world->name);
    assert(copied_chars > 0);    
}

Level_Result *add_level_result(World_Progress *wp, Level const *level, u16 score) {
    Level_Result *result = add_level_result(&wp->level_results, level, score);
    return result;
}

void remove_level_result(World_Progress *wp, char const *level_name) {
    remove_level_result(&wp->level_results, level_name);
}

Level_Result *get_level_result(World_Progress const *wp, char const *level_name) {
    Level_Result *result = get_level_result(&wp->level_results, level_name);
    return result;
}



//
// #_Array_Of_World_Progress
//
struct Array_Of_World_Progress {
    World_Progress *data = nullptr;
    u32 capacity = 0;
    u32 count = 0;
};

void free_array_of_world_progress(Array_Of_World_Progress *array) {
    if (array) {
        if (array->data) {
            for (u32 world_index = 0; world_index < array->capacity; ++world_index) {
                World_Progress *world_progress = &array->data[world_index];
                free_world_progress(world_progress);
            }

            free(array->data);
            array->data = nullptr;
        }
        array->count = 0;
        array->capacity = 0;
    }
}

void grow_array_of_world_progress(Array_Of_World_Progress *array, u32 new_capacity = 0) {
    u32 old_capacity = array->capacity;
    if (new_capacity == 0) {
        if (old_capacity == 0) {
            new_capacity = 2;
        }
        else {
            new_capacity = 2 * old_capacity;
        }
    }    
    size_t new_size = new_capacity * sizeof(World_Progress);
    void *new_ptr = realloc(array->data, new_size);
    if (new_ptr) {
        array->data = static_cast<World_Progress *>(new_ptr);
        array->capacity = new_capacity;
    }
    else {
        assert(0);
    }

    size_t old_size = old_capacity * sizeof(World_Progress);
    if (old_size > 0) {
        memset(&array->data[old_capacity], 0, new_size - old_size);
    }
    else {
        memset(array->data, 0, new_size);
    }
}

World_Progress *get_world_progress(Array_Of_World_Progress const *array, char const *world_name) {
    World_Progress *result = nullptr;

    if (array && world_name) {
        for (u32 world_index = 0; world_index < array->count; ++world_index) {
            World_Progress *world = &array->data[world_index];
            if (strcmp(world->name, world_name) == 0) {
                result = world;
            }
        }
    }

    return result;
}

World_Progress *add_world_progress(Array_Of_World_Progress *array, World const *world) {
    World_Progress *result = nullptr;

    if (array) {
        result = get_world_progress(array, world->name);

        if (!result) {
            assert(array->count <= array->capacity);
            if (array->capacity <= array->count) {
                grow_array_of_world_progress(array);
            }
        
            result = &array->data[array->count++];
            init_world_progress(result, world);
        }
    }

    return result;
}

Level_Result *get_level_result(Array_Of_World_Progress const *array_of_worlds, World const *world, char const *level_name) {
    Level_Result *result = nullptr;

    if (array_of_worlds && world && level_name) { 
        World_Progress *world_progress = get_world_progress(array_of_worlds, world->name);
        if (world_progress) {
            result = get_level_result(world_progress, level_name);
        }
    }

    return result;
}

void add_level_result(Array_Of_World_Progress *array, World const *world, Level const *level, u16 score) {
    if (array && world && level) { 
        World_Progress *world_progress = get_world_progress(array, world->name);
        if (world_progress) {
            add_level_result(world_progress, level, score);
        }
    }
}

void remove_level_result(Array_Of_World_Progress *array, World const *world, char const *level_name) {
    if (array && world && level_name) { 
        World_Progress *world_progress = get_world_progress(array, world->name);
        if (world_progress) {
            remove_level_result(world_progress, level_name);
        }
    }
}




//
// #_Player_Profile
//
struct Player_Profile {
    char name[kPlayer_Profile_Name_Max_Length] = {};
    Array_Of_World_Progress worlds;
};

void free_player_profile(Player_Profile *profile) {
    free_array_of_world_progress(&profile->worlds);
    profile->name[0] = '\0';
}

//
// NOTE: we need to have the original level state as input, we'll assume that level->score is the original max score of the level.
void add_level_progress(Player_Profile *profile, World const *world, Level const *level, u16 score) {
    World_Progress *world_progress = add_world_progress(&profile->worlds, world);
    assert(world_progress);

    Level_Result *level_result = add_level_result(world_progress, level, score);
    assert(level_result);
}

Level_Result *get_level_result(Player_Profile const *profile, World const *world, char const *level_name) {
    Level_Result *result = nullptr;
    
    if (profile && world && level_name) {
        result = get_level_result(&profile->worlds, world, level_name);
    }

    return result;
}


s32 get_highest_completed_level_index(Player_Profile const *profile, World const *world) {
    s32 result = -1;

    if (profile && world) {
        for (u32 level_index = 0; level_index < world->level_count; ++level_index) {
            Level *level = &world->levels[level_index];
            Level_Result *level_result = get_level_result(profile, world, level->name);
            if (level_result) {
                ++result;
            }
            else {
                break;
            }
        }
    }

    return result;
}


b32 write_player_profiles_to_disc(Player_Profile profiles[3]) {
    b32 result = false;
    
    HANDLE file = win32_open_file_for_writing("data\\player_profi.les");
    if (file != INVALID_HANDLE_VALUE) {
        char textbuffer[kPlayer_Profile_Name_Max_Length] = {};
        
        for (u32 index = 0; index < 3; ++index) {
            Player_Profile *p = &profiles[index];

            // Write name of profile
            _snprintf_s(textbuffer, kPlayer_Profile_Name_Max_Length, _TRUNCATE, "%s", p->name);
            DWORD bytes_written = 0;
            result = WriteFile(file, textbuffer, kPlayer_Profile_Name_Max_Length, &bytes_written, nullptr);
            assert(result && bytes_written == kPlayer_Profile_Name_Max_Length);

            // Write count of worlds
            result = WriteFile(file, &p->worlds.count, sizeof(u32), &bytes_written, nullptr);
            assert(result && bytes_written == sizeof(u32));

            if (p->worlds.count > 0) {
                assert(p->worlds.data);
                for (u32 world_index = 0; world_index < p->worlds.count; ++world_index) {
                    World_Progress *world_progress = &p->worlds.data[world_index];

                    // Write name of world
                    _snprintf_s(textbuffer, kWorld_Name_Max_Length, _TRUNCATE, "%s", world_progress->name);
                    result = WriteFile(file, textbuffer, kWorld_Name_Max_Length, &bytes_written, nullptr);
                    assert(result && bytes_written == kWorld_Name_Max_Length);            
                
                    // Write count of level_results (which also equals the count of completed levels)
                    result = WriteFile(file, &world_progress->level_results.count, sizeof(u32), &bytes_written, nullptr);
                    assert(result && bytes_written == sizeof(u32));

                    u32 size = sizeof(Level_Result) * world_progress->level_results.count;
                    result = WriteFile(file, world_progress->level_results.data, size, &bytes_written, nullptr);
                    assert(result && bytes_written == size);
                }
            }
        }
        
        CloseHandle(file);
        result = true;
    }

    return result;
}


b32 read_player_profiles_from_disc(Player_Profile *profiles) {
    b32 result = false;
    
    // Free old profiles
    free_player_profile(&profiles[0]);
    free_player_profile(&profiles[1]);
    free_player_profile(&profiles[2]);

    HANDLE file = win32_open_file_for_reading("data\\player_profi.les");
    if (file != INVALID_HANDLE_VALUE) {
        for (u32 index = 0; index < 3; ++index) {
            Player_Profile *p = &profiles[index];

            // Read name of profile
            DWORD bytes_read = 0;
            result = ReadFile(file, &p->name, kPlayer_Profile_Name_Max_Length, &bytes_read, nullptr);
            assert(result && bytes_read == kPlayer_Profile_Name_Max_Length);

            // Write count of worlds
            DWORD size = sizeof(u32);
            result = ReadFile(file, &p->worlds.count, size, &bytes_read, nullptr);
            assert(result && bytes_read == size);

            if (p->worlds.count > 0) {
                assert(p->worlds.capacity == 0);
                grow_array_of_world_progress(&p->worlds, p->worlds.count);
                assert(p->worlds.data);
                assert(p->worlds.capacity == p->worlds.count);
                //p->worlds.data = static_cast<World_Progress *>(calloc(p->worlds.count, sizeof(World_Progress)));
                //assert(p->worlds.data);
                //p->worlds.capacity = p->worlds.count;
            
                for (u32 world_index = 0; world_index < p->worlds.count; ++world_index) {
                    World_Progress *world_progress = &p->worlds.data[world_index];

                    // Read name of world
                    result = ReadFile(file, &world_progress->name, kWorld_Name_Max_Length, &bytes_read, nullptr);
                    assert(result && bytes_read == kWorld_Name_Max_Length);

                    // Read count of levels in world
                    result = ReadFile(file, &world_progress->level_results.count, size, &bytes_read, nullptr);
                    assert(result && bytes_read == size);

                    grow_array_of_level_result(&world_progress->level_results, world_progress->level_results.count);
                    assert(world_progress->level_results.data);
                    assert(world_progress->level_results.count == world_progress->level_results.capacity);

                    size = world_progress->level_results.count * sizeof(Level_Result);
                    //world_progress->level_results = static_cast<Level_Result *>(malloc(size));
                    result = ReadFile(file, world_progress->level_results.data, size, &bytes_read, nullptr);
                    assert(result && bytes_read == size);
                }
            }
        }

        CloseHandle(file);
        result = true;
    }    

    return result;
}




//
// #_ Dijkstra ("dijkstra-maps")
// - http://www.roguebasin.com/index.php?title=The_Incredible_Power_of_Dijkstra_Maps
//

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


void create_maps_off_level(s32 **maps, Level *level) {
    size_t map_size_in_bytes = level->width * level->height * sizeof(s32);
    for (u32 map_index = 0; map_index < Map_Count; ++map_index) {
        s32 **map = &maps[map_index];
        if (*map) {
            free(*map);
            *map = nullptr;
        }
        
        *map = static_cast<s32 *>(malloc(map_size_in_bytes));    
        assert(*map);
    }
    
    u32 width = level->width;
    u32 height = level->height;
    Tile *tiles = level->tiles;
    //s32 **maps = world->maps;
    u32 constexpr max_value = 300;


    //
    // Process the maps
    for (u32 map_index = 0; map_index < Map_Count; ++map_index) {
        s32 *map = maps[map_index];        
        
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
        s32 *map = maps[Map_Flee_Ghosts];
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




//
// #_Movement
//

struct Move {
    Actor_ID actor_id = kActor_ID_Null;
    v2u src;
};

struct Move_Set {
    Move moves[4];
    v2u dst;
    u32 move_count;
    u32 predator_count;
};

struct Array_Of_Moves {
    Move_Set *data = nullptr;
    u32 capacity = 0;
    u32 count = 0;
};


void init_move(Move *move) {
    if (move) {
        move->actor_id = kActor_ID_Null;
        move->src = v2u_00;        
    }
}


void init_move_set(Move_Set *set) {
    for (u32 index = 0; index < 4; ++index) {
        init_move(&set->moves[index]);
    }
    set->dst = v2u_00;
    set->predator_count = 0;
    set->move_count = 0;
}


void free_array_of_moves(Array_Of_Moves *array) {
    if (array) {
        if (array->data) {
            free(array->data);
            array->data = nullptr;
        }
        array->capacity = 0;
        array->count = 0;
    }
}


void init_array_of_moves(Array_Of_Moves *array) {
    assert(array);
    free_array_of_moves(array);

    u32 init_capacity = 10;
    array->capacity = init_capacity;
    array->count = 0;

    size_t size = sizeof(Move_Set) * array->capacity;
    array->data = static_cast<Move_Set *>(malloc(size));    
}


b32 grow_array_of_moves(Array_Of_Moves *array) {
    b32 result = false;
    
    if (array) {
        u32 new_capacity = array->capacity < 10 ? 10 : 2 * array->capacity;
        size_t new_size = sizeof(Move_Set) * new_capacity;
        void *new_ptr = realloc(&array->data, new_size);

        if (new_ptr) {
            array->data = static_cast<Move_Set *>(new_ptr);
            array->capacity = new_capacity;
            result = true;
        }
        else {
            printf("%s in %s failed to realloc memory for the moves. Current capacity = %d, new capacity = %d\n",
                   __FUNCTION__, __FILE__, array->capacity, new_capacity);
        }
    }

    return result;
}
 

void clear_array_of_moves(Array_Of_Moves *array) {
    if (array) {
        array->count = 0;
    }
}


//
// NOTE: Slow, but we will not have that many moves per turn (famous last words), so we'll should be ok.
Move_Set *get_move_set(Array_Of_Moves *array, v2u dst) {
    for (u32 index = 0; index < array->count; ++index) {
        Move_Set *move_set = &array->data[index];
        if (move_set->dst == dst) {
            return move_set;
        }
    }

    return nullptr;
}


b32 add_move(Array_Of_Moves *array, Actor *actor, v2u dst) {
    b32 result = false;

    if (array && actor) {
        b32 got_memory = true;
        
        if (array->count == array->capacity) {
            got_memory = grow_array_of_moves(array);
        }

        if (got_memory) {
            Move *move = nullptr;
            Move_Set *set = get_move_set(array, dst);
            if (!set) {
                set = &array->data[array->count++];
                init_move_set(set);
                set->dst = dst;
                move = &set->moves[0];
                set->move_count = 1;
            }
            else {
                assert(set->move_count < 4);
                move = &set->moves[set->move_count++];
            }            
            assert(move);
            assert(move->actor_id.index == 0xFFFF);
            assert(move->actor_id.salt == 0xFFFF);

            if (actor->mode == Actor_Mode_Predator) {
                ++set->predator_count;
            }   

            move->actor_id = actor->id;
            move->src = actor->position;         
            result = true;
        }
    }

    return result;
}




//
// #_Game
//

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


enum Input {
    Input_Right,
    Input_Up,
    Input_Left,
    Input_Down,
    Input_Select,
    Input_Escape,
    Input_Delete,

    Input_Count,
    Input_None
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
    
        // wavs
        if (result)  result = load_wav("data\\audio\\eat_large_dot.wav", &resources->wavs.eat_large_dot);
        if (result)  result = load_wav("data\\audio\\eat_small_dot.wav", &resources->wavs.eat_small_dot);
        if (result)  result = load_wav("data\\audio\\ghost_dies.wav", &resources->wavs.ghost_dies);
        if (result)  result = load_wav("data\\audio\\won.wav", &resources->wavs.won);
        if (result)  result = load_wav("data\\audio\\lost.wav", &resources->wavs.lost);
        if (result)  result = load_wav("data\\audio\\nope.wav", &resources->wavs.nope);

        if (!result)  LOG_ERROR(&game->log, "failed to load resources, is the data directory present?", 0);


        
        // NOTE: Create a voice using a (somewhat) random wav file. If playing a wav with a different
        //       format (different bit rate, etc...) the voice will be destroyed and re-created.
        //       So this will not create any requirements of the wav formats.
        result = init_voices(&game->audio_state, &game->resources.wavs.eat_small_dot);
        if (!result)  LOG_ERROR(&game->log, "failed to init voices", 0);

    
        // load worlds
        {
            init_array_of_worlds(&game->worlds, 1);
            
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

        game->microseconds_since_start = 0;
    }
    // End of resource loading
    //
    //
    

    
    //
    // Init game state
    if (!read_player_profiles_from_disc(game->player_profiles)) {
        LOG_ERROR(&game->log, "failed to load player profiles", 0);
        _snprintf_s(reinterpret_cast<char *>(&game->player_profiles[0].name), kPlayer_Profile_Name_Max_Length, _TRUNCATE, "Adam");
        _snprintf_s(reinterpret_cast<char *>(&game->player_profiles[1].name), kPlayer_Profile_Name_Max_Length, _TRUNCATE, "Bertil");
        _snprintf_s(reinterpret_cast<char *>(&game->player_profiles[2].name), kPlayer_Profile_Name_Max_Length, _TRUNCATE, "Cesar");
        write_player_profiles_to_disc(game->player_profiles);
    }

    init_array_of_moves(&game->all_the_moves);
    game->state = Game_State_Player_Select;
    game->current_map_index = Map_Count; // DEBUG

    World *world = get_current_world(game);
    Player_Profile *profile = get_current_player_profile(game);
    s32 level_index = get_highest_completed_level_index(profile, world) + 1;
    level_index = level_index < static_cast<s32>(world->level_count) ? level_index : 0;
    load_level(world, level_index);
    create_maps_off_level(game->maps, &world->levels[world->current_level_index]);

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


void change_pacman_mode(Level *level, Actor_Mode mode) {
    Actor_Mode other_mode = static_cast<Actor_Mode>(!static_cast<b32>(mode));

    for (u32 index = 0; index < level->actors.count; ++index) {
        Actor *actor = &level->actors.data[index];
        if (actor->type == Actor_Type_Pacman) {
            actor->mode = mode;
        }
        else {
            actor->mode = other_mode;
        }
    }
}




//
// #_Movement
//

v2s get_movement_vector(Actor *actor, Input input) {
    s32 constexpr X[4] = {1, 0, -1,  0};
    s32 constexpr Y[4] = {0, 1,  0, -1};

    s32 dx = X[input];
    s32 dy = Y[input];
    
    if (actor->type == Actor_Type_Ghost_Pink) {
        dx *= -1;
        dy *= -1;
    }
    // else if (actor->type == Actor_Type_Ghost_Cyan) {
    //     dx *= 2;
    //     dy *= 2;
    // }
    // else if (actor->type == Actor_Type_Ghost_Orange) {
    //     s32 temp = dx;
    //     dx = dy;
    //     dy = temp;
    // }

    v2s result = v2s(dx, dy);
    return result;
}


void collect_move(Game *game, Actor *actor, v2s dP) {
    World *world = get_current_world(game);
    Level *level = get_current_level_state(world);
    assert(level);
    
    if (move_is_possible(level, actor, dP)) {
        v2u dst = V2u(static_cast<u32>(static_cast<s32>(actor->position.x) + dP.x),
                      static_cast<u32>(static_cast<s32>(actor->position.y) + dP.y));
        add_move(&game->all_the_moves, actor, dst);
        actor->state = Actor_State_Moving;
        actor->next_position = dst;
    }
    else {
        actor->state = Actor_State_Idle;
        actor->next_position = actor->position;
    }
}


v2s get_pacman_move(Game *game, Actor *pacman) {
    Direction next_direction = Direction_Right;
    v2u Po = pacman->position;
    World *world = get_current_world(game);
    Level *level = get_current_level_state(world);

    Map_Direction closest_ghost = get_shortest_direction_on_map(level, game->maps[Map_Ghosts], pacman);
    assert(closest_ghost.direction < Direction_Count);
    assert(closest_ghost.distance >= 0);
    
    if (pacman->mode == Actor_Mode_Predator && level->mode_duration >= static_cast<u32>(closest_ghost.distance)) {
        next_direction = closest_ghost.direction;
    }
    else {
        if (level->large_dot_count > 0) {
            Map_Direction closest_large_dot = get_shortest_direction_on_map(level, game->maps[Map_Dot_Large], pacman);
            assert(closest_large_dot.direction < Direction_Count);
    
            if (closest_ghost.distance < closest_large_dot.distance) {
                Map_Direction flee = get_shortest_direction_on_map(level, game->maps[Map_Flee_Ghosts], pacman);
                next_direction = flee.direction;
            }
            else {
                next_direction = closest_large_dot.direction;
            }
        }
        else if (level->small_dot_count > 0) {
            Map_Direction closest_small_dot = get_shortest_direction_on_map(level, game->maps[Map_Dot_Small], pacman);
            assert(closest_small_dot.direction < Direction_Count);
            next_direction = closest_small_dot.direction;
        }
        else {
            Map_Direction flee = get_shortest_direction_on_map(level, game->maps[Map_Flee_Ghosts], pacman);
            next_direction = flee.direction;
        }
    }

    v2s dP = get_movement_vector(pacman, static_cast<Input>(next_direction));
    return dP;
}


void collect_all_moves(Game *game, Level *level) {
    for (u32 index = 0; index < level->actors.count; ++index) {
        Actor *actor = &level->actors.data[index];
        if (actor->state != Actor_State_Dead) {
            v2s dP;
            if (actor->type == Actor_Type_Pacman) {
                dP = get_pacman_move(game, actor);
            }
            else {
                dP = get_movement_vector(actor, game->input);
            }            
            collect_move(game, actor, dP);
        }
    }
}


u32 resolve_all_moves(Game *game, Level *level) {
    u32 valid_moves = 0;

    Array_Of_Moves *all_the_moves = &game->all_the_moves;
    
    //
    // Solve all collisions with actor (if any) standing on the destination tile
    u32 resolved_collisions;
    do {
        resolved_collisions = 0;
        for (u32 set_index = 0; set_index < all_the_moves->count; ++set_index) {
            Move_Set *set = &all_the_moves->data[set_index];
            Tile *dst_tile = get_tile_at(level, set->dst);
            Actor *dst_actor = get_actor(&level->actors, dst_tile->actor_id);

            u32 stopped_actors = 0;

            for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
                Move *move = &set->moves[move_index];
                Actor *src_actor = get_actor(&level->actors, move->actor_id);

                if (src_actor->state == Actor_State_Idle)  continue;
                if (!actor_is_alive(src_actor))            continue;

                b32 immobile = false;
                b32 collision = false;

                if (actor_is_alive(dst_actor)) {
                    immobile = dst_actor->state == Actor_State_Idle;
                    collision = immobile || ((dst_actor->state == Actor_State_Moving) && (dst_actor->next_position == src_actor->position));
                }

                // NOTE:
                // src_actor will stop if there is a collision and src is not a predator and
                // dst is not a prey 
                //
                // or in other words: we only allow src to move if:
                // - there is no collision (either if dst is moving away or if dst == nullptr)
                // - in event of a collision src may move if it is a predator and dst is a prey
            
                if (collision) {
                    if (src_actor->mode == Actor_Mode_Predator && dst_actor->mode == Actor_Mode_Prey) {                        
                        dst_actor->state = Actor_State_At_Deaths_Door;
                    }
                    else {
                        src_actor->state = Actor_State_Idle;
                        ++stopped_actors;
                    }
                    ++resolved_collisions;
                }
                else {
                    src_actor->state = Actor_State_Moving;
                }
            }
        }
    } while (resolved_collisions > 0);


    //
    // Solve moves  
    for (u32 set_index = 0; set_index < all_the_moves->count; ++set_index) {        
        Move_Set *set = &all_the_moves->data[set_index];
        if (set->move_count == 0)  continue;
        
        Tile *dst_tile = get_tile_at(level, set->dst);
        Actor *dst_actor = get_actor(&level->actors, dst_tile->actor_id);
        if (!actor_is_alive(dst_actor)) {                        
            dst_actor = nullptr;
        }

        // NOTE:
        // We know that if src is moving, then either there is no collision or src is a predator and dst is a prey,
        // so we only need to think about the potential other actors moving into this square.
        // If we only have one actor in the move then we're already done!

        for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
            Move *move = &set->moves[move_index];
            Actor *src_actor = get_actor(&level->actors, move->actor_id);
            if (!actor_is_alive(src_actor) || src_actor->state == Actor_State_Idle)  continue;

            if (set->predator_count > 1) {
                src_actor->state = Actor_State_Idle;
            }
            else if (set->predator_count == 0 && set->move_count > 1) {
                src_actor->state = Actor_State_Idle;
            }
            else if (set->predator_count == 1 && set->move_count > 1) {
                if (src_actor->mode == Actor_Mode_Prey) {
                    src_actor->state = Actor_State_At_Deaths_Door;
                    ++valid_moves;
                }
            }
            else {
                ++valid_moves;
            }
        }
    }

    return valid_moves;
}


void cancel_moves(Game *game, Level *level) {
    for (u32 move_set_index = 0; move_set_index < game->all_the_moves.count; ++move_set_index) {
        Move_Set *set = &game->all_the_moves.data[move_set_index];
        //if (!set->is_valid)  continue;

        for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
            Move *move = &set->moves[move_index];
            Actor *actor = get_actor(&level->actors, move->actor_id);
            actor->state = Actor_State_Idle;
            actor->next_position = actor->position;
        }
    }
};


Direction get_direction_from_move(v2u from, v2u to) {
    Direction result = Direction_Count;

    s32 dx = static_cast<s32>(to.x) - static_cast<s32>(from.x);
    s32 dy = static_cast<s32>(to.y) - static_cast<s32>(from.y);

    if (dx > 0 && dy == 0) {
        result = Direction_Right;
    }
    else if (dx == 0 && dy > 0) {
        result = Direction_Up;
    }
    else if (dx < 0 && dy == 0) {
        result = Direction_Left;
    }
    else if (dx == 0 && dy < 0) {
        result = Direction_Down;
    }
    else {
        assert(0);
    }

    return result;
}


void accept_moves(Game *game, Level *level) {
    for (u32 move_set_index = 0; move_set_index < game->all_the_moves.count; ++move_set_index) {
        Move_Set *set = &game->all_the_moves.data[move_set_index];

        for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
            Move *move = &set->moves[move_index];
            Actor *actor = get_actor(&level->actors, move->actor_id);
            Tile *dst_tile = get_tile_at(level, set->dst);            
            
            if (actor->state == Actor_State_Moving) {
                Tile *src_tile = get_tile_at(level, actor->position);                
                assert(src_tile);
                if (src_tile->actor_id == actor->id)  src_tile->actor_id = kActor_ID_Null;
                dst_tile->actor_id = actor->id;
                
                actor->state = Actor_State_Idle;
                actor->direction = get_direction_from_move(actor->position, actor->next_position);
                actor->position = actor->next_position;

                // Is the current actor pacman, and has the current tile any dots on it?
                if (actor->type == Actor_Type_Pacman) {
                    if (dst_tile->item.type == Item_Type_Dot_Small) {
                        --level->small_dot_count;
                        level->score -= kDot_Small_Value;
                        dst_tile->item.type = Item_Type_None;
                        play_wav(&game->audio_state, &game->resources.wavs.eat_small_dot);
                    }
                    else if (dst_tile->item.type == Item_Type_Dot_Large) {
                        --level->large_dot_count;
                        level->score -= kDot_Large_Value;
                        change_pacman_mode(level, Actor_Mode_Predator);
                        level->mode_duration = kPredator_Mode_Duration;
                        dst_tile->item.type = Item_Type_None;
                        play_wav(&game->audio_state, &game->resources.wavs.eat_large_dot);

                        // TODO: What if pacman eats a large dot, changes mode but also is
                        //       the actor to be killed?
                    }
                }
            }
            else {
                actor->next_position = actor->position;
            }
        }
    }

    //
    // Iterate through all actors and kill the ones with the state At_Deaths_Door.
    // We migh have an actor that never were part of a move but should be killed.
    for (u32 index = 0; index < level->actors.count; ++index) {
        Actor *actor = &level->actors.data[index];
        if (actor->state == Actor_State_At_Deaths_Door) {
            kill_actor(level, actor);

            if (actor_is_ghost(actor)) {
                play_wav(&game->audio_state, &game->resources.wavs.ghost_dies);
            }
        }
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

void update_all_actors(Game *game, f32 dt) {
}


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
        u32 valid_moves = 0;
        if (game->input == Input_Escape) {// && game->state == Game_State_Playing) {
            game->state = Game_State_Menu;
        }
        else if (game->input < Input_Select && game->state == Game_State_Playing) {
            collect_all_moves(game, level);
            valid_moves = resolve_all_moves(game, level);
        
            if (valid_moves == 0) {
                cancel_moves(game, level);
                play_wav(&game->audio_state, &game->resources.wavs.nope); // Invalid move!
            }
            else {
                // We're moving and thus we need to save the state and recalulate the "dijkstra maps".        
                save_current_level_state(world);
                level = get_current_level_state(world);
            
                accept_moves(game, level);

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
                else {            
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
            }
        
            clear_array_of_moves(&game->all_the_moves);            
        }
    
        update_all_actors(game, dt);
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
