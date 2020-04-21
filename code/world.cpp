//
// world.cpp
// (c) Marcus Larsson
//

//
// #_World
//

#define kWorld_Level_States_Count 100
#define kWorld_Name_Max_Length 21

struct World {
    char name[kWorld_Name_Max_Length] = {};
    Level *levels = nullptr;    
    
    Level current_level_states[kWorld_Level_States_Count];
    u32 first_valid_level_state_index = 0;
    u32 last_valid_level_state_index = 0;
    u32 current_level_state_index = 0;
    u32 level_count = 0;
    u32 current_level_index = 0;
};


void free_all_current_level_states(World *world) {
    for (u32 index = 0; index < kWorld_Level_States_Count; ++index) {
        free_level(&world->current_level_states[index]);
    }
    
    world->current_level_state_index = 0;
    world->first_valid_level_state_index = 0;
    world->last_valid_level_state_index = 0;
}


void free_world(World *world) {
    if (world) {
        free_all_current_level_states(world);        
        
        for (u32 index = 0; index < world->level_count; ++index) {
            free_level(&world->levels[index]);
        }
        free(world->levels);
        world->levels = nullptr;
        world->level_count = 0;
    }    
}


void load_level(World *world, u32 index) {
    if (index < world->level_count) {
        free_all_current_level_states(world);
        
        copy_level(&world->current_level_states[0], &world->levels[index]);
        world->current_level_index = index;        
    }
}


Level *get_level_with_exact_name(World *world, char const *level_name) {
    Level *result = nullptr;

    if (world && level_name) {
        for (u32 index = 0; index < world->level_count; ++index) {
            Level *current_level = &world->levels[index];
            if (strcmp(current_level->name, level_name) == 0) {
                result = current_level;
                break;
            }
        }
    }

    return result;
}


b32 is_valid_current_level_state_index(World *world, u32 n) {
    b32 result = false;

    if (world && n < kWorld_Level_States_Count) {
        u32 first_index = world->first_valid_level_state_index;
        u32 last_index = world->last_valid_level_state_index;
        
        if (first_index <= last_index) {
            result = n >= first_index && n <= last_index;
        }
        else if (last_index < first_index) {
            result = !(n > last_index && n < first_index);
        }
    }

    return result;
}


Level *get_current_level_state_n(World *world, u32 n) {
    Level *result = nullptr;

    if (world && is_valid_current_level_state_index(world, n)) {
        result = &world->current_level_states[n];
    }

    return result;
}


Level *get_current_level_state(World *world) {
    Level *result = get_current_level_state_n(world, world->current_level_state_index);
    return result;
}


Level *get_current_level_first_state(World *world) {
    Level *result = get_current_level_state_n(world, 0);
    return result;
}


void save_current_level_state(World *world) {
    // We assume that:
    //   first_index, last_index, current_index < kWorld_Level_States_Count
    //   current_index >= first_index && current_index <= last_index
    //

    u32 first_index = world->first_valid_level_state_index;
    u32 last_index  = world->last_valid_level_state_index;
    u32 curr_index  = world->current_level_state_index;
    u32 next_index = (curr_index + 1) < kWorld_Level_States_Count ? curr_index + 1 : 0;

    // Free levels
    // We're adding a new level state in between older states, we need to free the states after the new one.
    if (curr_index != last_index) {
        if (curr_index < last_index) {            
            for (u32 index = curr_index + 1; index <= last_index; ++index) {
                free_level(&world->current_level_states[index]);
            }
            last_index = curr_index;
        }
        else if (curr_index > last_index) {
            for (u32 index = curr_index + 1; index < kWorld_Level_States_Count; ++index) {
                free_level(&world->current_level_states[index]);
            }
            for (u32 index = 0; index <= last_index; ++index) {
                free_level(&world->current_level_states[index]);
            }
            last_index = curr_index;
        }
        else {
            assert(0); // Shouldn't happen?
        }
    }
    else if (next_index == first_index) { // Will we wrap around?
        free_level(get_current_level_state_n(world, first_index));
        first_index = (first_index + 1) < kWorld_Level_States_Count ? first_index + 1 : 0;
    }
    
    Level *curr_level_state = get_current_level_state(world);    
    Level *next_level_state = &world->current_level_states[next_index];
    
    copy_level(next_level_state, curr_level_state);
    world->current_level_state_index = next_index;
    world->first_valid_level_state_index = first_index;
    world->last_valid_level_state_index = world->current_level_state_index;
}


void reset_level(World *world) {
    load_level(world, world->current_level_index);
}


//
// NOTE: We pass a pointer to a resources struct, this will be kept by each level. For now this will be
//       the same struct for all levels, but in a near future this will, potentially, be unique for each
//       level.Which resources that should be used for a level should be controlled in the levels file.
//
b32 load_world_from_file(World *world, char const *path, char const *name, Resources *resources) {
    free_world(world);
    
    Tokenizer tokenizer;
    b32 result = init(&tokenizer, path, name);

    //
    // NOTE: we are using a two-pass solution here, the first pass counts the number of levels
    //       and the second one tokinizes the file.
    // TODO: Can we do this in only one pass?
    //

    if (result) {
        world->level_count = 0;

        Token token;
        // Parse world name
        if (require_identifier_with_exact_name(&tokenizer, &token, "world")) {
            eat_spaces_and_newline(&tokenizer);
            require_identifier_with_exact_name(&tokenizer, &token, "name");
            require_token(&tokenizer, &token, Token_colon);
            require_token(&tokenizer, &token, Token_string);
            _snprintf_s(world->name, kWorld_Name_Max_Length, _TRUNCATE, "%s", token.data);
        }
        require_token(&tokenizer, &token, Token_curled_brace_start);
            
        while(!tokenizer.error) {
            // See how many levels that are in this file            
            if (require_identifier_with_exact_name(&tokenizer, &token, "level")) {
                Level temp_level;                
                if (parse_level_header(&tokenizer, &temp_level)) {
                    ++world->level_count;
                    for (u32 line_index = 0; line_index < (temp_level.height); ++line_index) {
                        skip_to_next_line(&tokenizer);
                    }
                    
                    require_token(&tokenizer, &token, Token_curled_brace_end);
                    eat_spaces_and_newline(&tokenizer);
                }                
            }
        }

        if (world->level_count == 0 || (tokenizer.error && tokenizer.current_position < tokenizer.size)) {
            printf("%s(): %s\n", __FUNCTION__, tokenizer.error_string);
            return false;
        }

        world->levels = static_cast<Level *>(calloc(1, world->level_count * sizeof(Level)));
        reset_tokenizer(&tokenizer);

        // Skip name of the world
        require_identifier_with_exact_name(&tokenizer, &token, "world");
        eat_spaces_and_newline(&tokenizer);
        require_identifier_with_exact_name(&tokenizer, &token, "name");
        require_token(&tokenizer, &token, Token_colon);
        require_token(&tokenizer, &token, Token_string);
        require_token(&tokenizer, &token, Token_curled_brace_start);
        
        u32 level_index = 0;
        while(!tokenizer.error && result) {
            if (require_identifier_with_exact_name(&tokenizer, &token, "level")) {
                Level *curr_level = &world->levels[level_index++];
                init_level(curr_level, resources);
                result = parse_level(&tokenizer, curr_level);
                if (!result) {
                    free_level(curr_level);
                }
                else {
                    adjust_walls_in_level(curr_level, resources);                    
                }
            }            
        }

        require_token(&tokenizer, &token, Token_curled_brace_end);
        fini_tokenizer(&tokenizer);
    }

    return result;
}




//
// #_Storage
//

struct Array_Of_Worlds {
    World *data = nullptr;
    u32 count = 0;
    u32 capacity = 0;
};


void free_array_of_worlds(Array_Of_Worlds *array) {
    if (array->data) {
        for (u32 world_index = 0; world_index < array->capacity; ++world_index) {
            World *world = &array->data[world_index];
            free_world(world);
        }
        free(array->data);
        array->data = nullptr;
    }

    array->count = 0;
    array->capacity = 0;
}


void init_array_of_worlds(Array_Of_Worlds *array, u32 capacity) {
    free_array_of_worlds(array);
    size_t old_size = sizeof(World) * array->capacity;
    array->capacity = capacity >= 1 ? capacity : 1;
    size_t size = sizeof(World) * array->capacity;
    array->data = static_cast<World *>(malloc(size));

    // Zero the newly allocated memory
    memset(array->data + old_size, 0, sizeof(World) * array->capacity);
}


b32 grow_array_of_worlds(Array_Of_Worlds *array, u32 delta_capacity = 1) {
    b32 result = false;
    
    u32 new_capacity = array->capacity + delta_capacity;
    new_capacity = new_capacity > array->capacity ? new_capacity : array->capacity + 1;
    size_t new_size = sizeof(World) * new_capacity;
    void *new_ptr = realloc(array->data, new_size);
    if (new_ptr) {
        array->data = static_cast<World *>(new_ptr);
        array->capacity = new_capacity;
        result = true;
    }

    return result;
}


World *add(Array_Of_Worlds *array) {
    World *result = nullptr;
    b32 got_memory = array->count <= (array->capacity - 1);
    
    if (!got_memory) {
        got_memory = grow_array_of_worlds(array);
    }

    if (got_memory) {
        result = &array->data[array->count++];
        memset(result, 0, sizeof(World));
    }

    return result;
}


World *get_world_with_index(Array_Of_Worlds *array, u32 index) {
    World *result = nullptr;

    if (array && index < array->count) {
        result = &array->data[index];
    }

    return result;
}
