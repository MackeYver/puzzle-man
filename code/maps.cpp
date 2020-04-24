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
    Tile *tiles = level->current_state->tiles;
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
