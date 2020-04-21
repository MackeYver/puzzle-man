//
// Declaration
//

struct Level {
    char name[kLevel_Name_Max_Length] = {'\0'};
    Tile *tiles = nullptr;
    
    Resources *resources;
    Array_Of_Actors actors;
    Actor_ID pacman_id = {0xFFFF, 0xFFFF};
    u32 width = 0;
    u32 height = 0;
    u16 score = 0;
    u16 mode_duration = 0;
    u16 large_dot_count = 0;
    u16 small_dot_count = 0;
    u16 ghost_count = 0;
    u16 pacman_count = 0;
};




//
// Implementation
//

void free_level(Level *level) {
    if (level) {
        if (level->tiles) {
            free(level->tiles);
            level->tiles = nullptr;
        }

        // for (u32 index = 0; index < level->actor_count; ++index) {
        //     Actor *actor = &level->actors[index];
        //     free_actor(actor);
        // }
        free_array_of_actors(&level->actors);
        
        level->width = 0;
        level->height = 0;
        level->name[0] = '\0';
        level->score = 0;
        level->ghost_count = 0;
        level->pacman_count = 0;
        level->pacman_id.index = 0xFFFF;
        level->pacman_id.salt = 0xFFFF;
    }
};


void init_level(Level *level, Resources *resources) {
    free_level(level);
    level->resources = resources;
    level->pacman_id.index = 0xFFFF;
    level->pacman_id.salt = 0xFFFF;
}


b32 copy_level(Level *dst, Level *src) {
    b32 result = false;
    
    if (dst && src) {
        dst->width = src->width;
        dst->height = src->height;
        dst->resources = src->resources;
        dst->score = src->score;
        dst->ghost_count = src->ghost_count;
        dst->pacman_count = src->pacman_count;
        dst->pacman_id = src->pacman_id;
        dst->mode_duration = src->mode_duration;
        dst->large_dot_count = src->large_dot_count;
        dst->small_dot_count = src->small_dot_count;
        
        // name
        errno_t error = memcpy_s(dst->name, kLevel_Name_Max_Length, src->name, kLevel_Name_Max_Length);
        if (error != 0) {
            printf("%s() failed to copy the name of the level named \'%s\', error = %d\n", __FUNCTION__, src->name, errno);
            return false;
        }

        // tiles
        size_t size = dst->width * dst->height * sizeof(Tile);
        dst->tiles = static_cast<Tile *>(malloc(size));
        error = memcpy_s(dst->tiles, size, src->tiles, size);
        if (error != 0) {
            printf("%s() failed to copy the tiles from the level with with name \'%s\', error = %d\n", __FUNCTION__, src->name, errno);
            return false;
        }        

        // actors
        b32 b32_result = copy_actors(&dst->actors, &src->actors);
        if (result && !b32_result) {
            printf("%s() failed to copy the level with with name \'%s\', error = %d\n", __FUNCTION__, src->name, errno);
            return false;
        }

        result = true;
    }

    return result;
}


Tile *get_tile_at(Level *level, v2u P) {
    Tile *result = nullptr;
    
    if (level->tiles && P.x < level->width && P.y < level->height) {
        result = &level->tiles[(level->width * P.y) + P.x];
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
        Tile *tile = &level->tiles[(width * next_pos.y) + next_pos.x];
        if (tile_is_traversable(tile)) {
            result = true;
        }
    }

    return result;
}


Actor *get_pacman(Level *level) {
    Actor *result = nullptr;

    if (level) {
        result = get_actor(&level->actors, level->pacman_id);
    }

    return result;
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


Actor *get_actor_at(Level *level, v2u P) {
    Actor *result = nullptr;

    Tile *tile = get_tile_at(level, P);
    if (tile) {
        result = get_actor(&level->actors, tile->actor_id);
    }

    return result;
}


inline Actor *get_actor_at(Level *level, u32 x, u32 y) {
    return get_actor_at(level, V2u(x, y));    
}


void kill_actor(Level *level, Actor *actor) {
    if (level && actor) {
        Tile *tile = get_tile_at(level, actor->position);
        if (tile->actor_id == actor->id) {
            tile->actor_id = kActor_ID_Null;
        }
        
        kill_actor(&level->actors, actor);
        
        if (actor_is_ghost(actor)) {
            level->score -= kGhost_Value;
            --level->ghost_count;
        }
        else if (actor->type == Actor_Type_Pacman) {
            --level->pacman_count;
        }
    }
}


void draw_current_level(Render_State *render_state, Level *level, u32 microseconds_since_start) {
    Font *font = &level->resources->font;    
    u32 width = level->width;
    u32 height = level->height;
    
    //
    // Draw all tiles
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            Tile *tile = get_tile_at(level, x, y);
            draw_tile(render_state, level->resources, tile, V2u(kCell_Size * x, kCell_Size * y));
        }
    }

    
    //
    // Draw all actors
    Actor *pacman = get_pacman(level);
    for (u32 index = 0; index < level->actors.count; ++index) {
        Actor *actor = &level->actors.data[index];
        if (actor_is_alive(actor)) {
            if (actor_is_ghost(actor)) {
                draw_ghost(render_state, level->resources, actor, pacman, microseconds_since_start);
            }
            else if (actor->type == Actor_Type_Pacman) {
                draw_pacman(render_state, level->resources, actor, microseconds_since_start);
            }
        }
    }

    
    //
    // Score
    {
        char text[50];
        _snprintf_s(text, 50, _TRUNCATE, "Level %s, score %u", level->name, level->score);
        print(render_state, font, V2u(10, 10), text);
    }
}



//
// Loading
//

//
// Lexer, lexing levels
enum Level_Token {
    // NOTE: Everything below this line and until the actors will be static data
    Level_Token_Unused_Cell,
    Level_Token_Wall, 
    Level_Token_Floor,       
    
    Level_Token_Dot_Small,
    Level_Token_Dot_Large,

    Level_Token_Actors,
    // NOTE: Everything below this line will be actors
    Level_Token_Ghost_Red,
    Level_Token_Ghost_Pink,
    Level_Token_Ghost_Cyan,
    Level_Token_Ghost_Orange, 
    Level_Token_Pacman,

    Level_Token_Count,
    Level_Token_Unknown,
};


Level_Token get_level_token_from_char(char c) {
    Level_Token result = Level_Token_Unknown;
    
    switch (c) {
        case 'w': { result = Level_Token_Wall;         } break;
        case '.': { result = Level_Token_Floor;        } break;
            
        case '+': { result = Level_Token_Dot_Small;    } break;
        case 'X': { result = Level_Token_Dot_Large;    } break;
            
        case '1': { result = Level_Token_Ghost_Red;    } break;
        case '2': { result = Level_Token_Ghost_Pink;   } break;
        case '3': { result = Level_Token_Ghost_Cyan;   } break;
        case '4': { result = Level_Token_Ghost_Orange; } break;
        case 'P': { result = Level_Token_Pacman;       } break;
            
        case '-': { result = Level_Token_Unused_Cell;  } break;

        default:  { result = Level_Token_Unknown;      } break;
    }
    
    return result;
}


b32 token_is_static_level_data(Level_Token token) {
    b32 result = (token < Level_Token_Actors);
    return result;
}


b32 token_is_an_actor(Level_Token token) {
    b32 result = (token > Level_Token_Actors && token < Level_Token_Count);
    return result;
}


Actor_Type get_actor_type_from_level_token(Level_Token token) {
    Actor_Type actor_type = Actor_Type_Unknown;
    
    switch (token) {        
        case Level_Token_Ghost_Red:    { actor_type = Actor_Type_Ghost_Red;    } break;
        case Level_Token_Ghost_Pink:   { actor_type = Actor_Type_Ghost_Pink;   } break;
        case Level_Token_Ghost_Cyan:   { actor_type = Actor_Type_Ghost_Cyan;   } break;
        case Level_Token_Ghost_Orange: { actor_type = Actor_Type_Ghost_Orange; } break;
        case Level_Token_Pacman:       { actor_type = Actor_Type_Pacman;       } break;
    }

    return actor_type;
}


b32 parse_level_header(Tokenizer *tokenizer, Level *level) {
    assert(tokenizer);
    assert(level);
    
    eat_spaces_and_newline(tokenizer);
    Token token;

    //
    // read the name of the level    
    require_identifier_with_exact_name(tokenizer, &token, "name");
    require_token(tokenizer, &token, Token_colon);
    require_token(tokenizer, &token, Token_string);
    _snprintf_s(level->name, kLevel_Name_Max_Length, _TRUNCATE, "%s", token.data);
    require_token(tokenizer, &token, Token_comma);


    //
    // width
    require_identifier_with_exact_name(tokenizer, &token, "width");
    require_token(tokenizer, &token, Token_colon);
    require_token(tokenizer, &token, Token_number);
    level->width = get_u32_from_token(&token);
    require_token(tokenizer, &token, Token_comma);

    //
    // height
    require_identifier_with_exact_name(tokenizer, &token, "height");
    require_token(tokenizer, &token, Token_colon);
    require_token(tokenizer, &token, Token_number);
    level->height = get_u32_from_token(&token);

    require_token(tokenizer, &token, Token_curled_brace_start);
    eat_spaces_and_newline(tokenizer);

    if (tokenizer->error && !is_eof(tokenizer)) {        
        return false;
    }
    else {
        return true;
    }
}

b32 add_actor(Tokenizer *tokenizer, Level *level, u32 x, u32 y, Actor_Type actor_type) {
    assert(actor_type < Actor_Type_Count);    

    if (actor_type >= Actor_Type_Count) {
        printf("%s() got an invalid actor type (%u)\n", __FUNCTION__, actor_type);
        return false;
    }
    
    u32 reversed_y = level->height - 1 - y;
    Actor *curr_actor = new_actor(&level->actors);
    assert(curr_actor);
    curr_actor->position = V2u(x, reversed_y);
    curr_actor->type = actor_type;
    curr_actor->state = Actor_State_Idle;
    curr_actor->direction = Direction_Right;
        
    //
    // Ghost
    if (actor_type < Actor_Type_Pacman) {
        curr_actor->mode = Actor_Mode_Predator;
        level->score += kGhost_Value;
        ++level->ghost_count;
    }

    
    //
    // Pacman
    else if (actor_type == Actor_Type_Pacman) {            
        if (level->pacman_count == 0) {
            curr_actor->mode = Actor_Mode_Prey;
            level->pacman_id = curr_actor->id;
            ++level->pacman_count;
        }
        else {
            tokenizer->error = true;
            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE, "In %s at %u:%u, found more than one pacman",
                      tokenizer->path_and_name, tokenizer->line_number, tokenizer->line_position);
            return false;
        }
    }

        
    //
    // Tile beneath the actor
    u32 curr_tile_index = (level->width * reversed_y) + x;
    Tile *curr_tile = &level->tiles[curr_tile_index];
    curr_tile->type = Tile_Type_Floor;
    curr_tile->item.type = Item_Type_None;
    curr_tile->actor_id = curr_actor->id;

    return true;
}


b32 parse_level(Tokenizer *tokenizer, Level *level) {
    assert(tokenizer);
    assert(level);

    b32 result = parse_level_header(tokenizer, level);
    if (result) {
        u32 level_size = level->width * level->height;
        level->tiles = static_cast<Tile *>(calloc(1, level_size * sizeof(Tile)));
        assert(level->tiles);

        u32 x = 0;
        u32 y = 0;

        //
        // NOTE: the level is read top-down from the text file but we want to have
        //       it in bottom up,otherwise the level will be rendered upside-down.
        //
        b32 should_loop = true;
        while (y < level->height && should_loop) {
            u32 tile_index = (level->width * (level->height - 1 - y)) + x;
            Tile *curr_tile = &level->tiles[tile_index];
            b32 should_advance = true;

            
            //
            // Process token
            Level_Token level_token = get_level_token_from_char(tokenizer->curr_char);
            switch (level_token) {
                case Level_Token_Unused_Cell: {
                    curr_tile->type = Tile_Type_None;
                    curr_tile->item.type = Item_Type_None;
                    curr_tile->item.value = 0;
                    curr_tile->actor_id = kActor_ID_Null;
                } break;
                    
                case Level_Token_Wall: {
                    curr_tile->type = Tile_Type_Wall_0;
                    curr_tile->item.type = Item_Type_None;
                    curr_tile->item.value = 0;
                    curr_tile->actor_id = kActor_ID_Null;
                } break;
                    
                case Level_Token_Floor: {
                    curr_tile->type = Tile_Type_Floor;
                    curr_tile->item.type = Item_Type_None;
                    curr_tile->item.value = 0;
                    curr_tile->actor_id = kActor_ID_Null;
                } break;
                    
                case Level_Token_Dot_Small: {
                    curr_tile->type = Tile_Type_Floor;
                    level->score += kDot_Small_Value;
                    ++level->small_dot_count;
                    curr_tile->item.type = Item_Type_Dot_Small;
                    curr_tile->item.value = kDot_Small_Value;
                    curr_tile->actor_id = kActor_ID_Null;
                } break;
                    
                case Level_Token_Dot_Large: {
                    curr_tile->type = Tile_Type_Floor;
                    level->score += kDot_Large_Value;
                    ++level->large_dot_count;
                    curr_tile->item.type = Item_Type_Dot_Large;
                    curr_tile->item.value = kDot_Large_Value;
                    curr_tile->actor_id = kActor_ID_Null;
                } break;
                    
                //
                // NOTE: Beneath every actor there is a floor, holding them up.
                case Level_Token_Ghost_Red:    { if (!add_actor(tokenizer, level, x, y, Actor_Type_Ghost_Red))     should_loop = false; } break;
                case Level_Token_Ghost_Pink:   { if (!add_actor(tokenizer, level, x, y, Actor_Type_Ghost_Pink))    should_loop = false; } break;
                case Level_Token_Ghost_Cyan:   { if (!add_actor(tokenizer, level, x, y, Actor_Type_Ghost_Cyan))    should_loop = false; } break;
                case Level_Token_Ghost_Orange: { if (!add_actor(tokenizer, level, x, y, Actor_Type_Ghost_Orange))  should_loop = false; } break;
                case Level_Token_Pacman:       { if (!add_actor(tokenizer, level, x, y, Actor_Type_Pacman))        should_loop = false; } break;

                default: {                    
                    Token token = get_token(tokenizer);
                    if (token.type == Token_comment) {
                        skip_to_next_line(tokenizer);
                        should_advance = false;
                    }
                    else {
                        tokenizer->error = true;
                        _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE, "In %s at %u:%u, found invalid token",
                                  tokenizer->path_and_name, token.line_number, token.line_position);
                        should_loop = false;
                    }
                } break;
            }
            
            if (should_advance) {
                advance(tokenizer);
                ++x;
                if (x == level->width) {
                    ++y;
                    x = 0;
                }                
            }

            eat_spaces_and_newline(tokenizer);
            reload(tokenizer);
        }

        Token token;
        require_token(tokenizer, &token, Token_curled_brace_end);
        eat_spaces_and_newline(tokenizer);

        if (tokenizer->error && !is_eof(tokenizer)) {
            printf("%s(): %s\n", __FUNCTION__, tokenizer->error_string);
            result = false;            
        }
    }

    return result;
}

void adjust_walls_in_level(Level *level, Resources *resources) {
    for (u32 y = 0; y < level->height; ++y) {
        for (u32 x = 0; x < level->width; ++x) {
            u32 curr_index = (level->width * y) + x;
            u32 sum = 0;

            if (tile_has_wall(&level->tiles[curr_index])) {
                if (x < (level->width - 1)) {
                    u32 right_index = (level->width * y) + x + 1;
                    if (tile_has_wall(&level->tiles[right_index])) {
                        sum += 1;
                    }
                }
                if (y < (level->height - 1)) {
                    u32 up_index = (level->width * (y + 1)) + x;
                    if (tile_has_wall(&level->tiles[up_index])) {
                        sum += 2;
                    }
                }
                if (x > 0) {
                    u32 left_index = (level->width * y) + x - 1;
                    if (tile_has_wall(&level->tiles[left_index])) {
                        sum += 4;
                    }
                }
                if (y > 0) {
                    u32 down_index = (level->width * (y - 1)) + x;
                    if (tile_has_wall(&level->tiles[down_index])) {
                        sum += 8;
                    }
                }

                level->tiles[curr_index].type = static_cast<Tile_Type>(sum);
            }
        }
    }
}
