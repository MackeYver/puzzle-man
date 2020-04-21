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
