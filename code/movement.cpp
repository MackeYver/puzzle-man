//
// Input
//

enum Input {
    Input_Right,
    Input_Up,
    Input_Left,
    Input_Down,
    Input_Select,
    Input_Space,
    Input_Save,
    Input_Escape,
    Input_Delete,

    Input_Count,
    Input_None
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
    b32 cancelled = false;
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
    set->cancelled = false;
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


b32 grow_array_of_moves(Array_Of_Moves *moves) {
    b32 result = false;

    if (moves) {
        u32 new_capacity = moves->capacity < 10 ? 10 : 2 * moves->capacity;
        size_t new_size = sizeof(Move_Set) * new_capacity;
        void *new_ptr = realloc(moves->data, new_size);

        if (new_ptr) {
            moves->data = static_cast<Move_Set *>(new_ptr);
            moves->capacity = new_capacity;
            result = true;
        }
        else {
            printf("%s in %s failed to realloc memory for the moves. Current capacity = %d, new capacity = %d\n",
                   __FUNCTION__, __FILE__, moves->capacity, new_capacity);
        }
    }

    return result;
}


void clear_array_of_moves(Array_Of_Moves *moves) {
    if (moves) {
        moves->count = 0;
    }
}


// DEBUG
static void debug_check_all_move_sets(Array_Of_Moves *moves) {
    for (u32 index = 0; index < moves->count; ++index) {
        Move_Set *set = &moves->data[index];
        if (set->move_count > 1 && (set->predator_count == 0 || set->predator_count == set->move_count)) {
            printf("Move set %u contains no predators and has more than one moves\n", index);
        }
    }
}

// DEBUG
static void debug_check_all_actors(Level *level) {
    Level_State *state = level->current_state;
    Array_Of_Actors *array = &state->actors;
    for (u32 outer_index = 0; outer_index < array->count; ++outer_index) {
        Actor *outer_actor = &array->data[outer_index];

        if (!actor_is_alive(outer_actor) && !actor_will_die(outer_actor)) {
            int a = 0;
            ++a;
        }

        for (u32 inner_index = 0; inner_index < array->count; ++inner_index) {
            if (outer_index == inner_index)  continue;

            Actor *inner_actor = &array->data[inner_index];
            if (inner_actor->position == outer_actor->position) {
                if (actor_is_alive(outer_actor) && actor_is_alive(inner_actor)) {
                    int a = 0;
                    ++a;
                }
            }
        }
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


b32 add_move(Array_Of_Moves *array, Actor *actor, v2u dst, Level *level) {
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


void collect_move(Array_Of_Moves *all_the_moves, Level *level, Actor *actor, v2s dP) {
    if (move_is_possible(level, actor, dP)) {
        v2u dst = V2u(static_cast<u32>(static_cast<s32>(actor->position.x) + dP.x),
                      static_cast<u32>(static_cast<s32>(actor->position.y) + dP.y));
        add_move(all_the_moves, actor, dst, level);
        actor->pending_state = Actor_State_Moving;
        actor->next_position = dst;
    }
    else {
        actor->state = Actor_State_Idle;
        actor->pending_state = actor->state;
        actor->next_position = actor->position;
    }
}


v2s get_pacman_move(s32 **maps, Level *level, Actor *pacman) {
    Direction next_direction = Direction_Right;
    v2u Po = pacman->position;

    Map_Direction closest_ghost = get_shortest_direction_on_map(level, maps[Map_Ghosts], pacman);
    assert(closest_ghost.direction < Direction_Count);
    assert(closest_ghost.distance >= 0);

    Level_State *state = level->current_state;

    if (pacman->mode == Actor_Mode_Predator && state->mode_duration >= static_cast<u32>(closest_ghost.distance)) {
        next_direction = closest_ghost.direction;
    }
    else {
        if (state->large_dot_count > 0) {
            Map_Direction closest_large_dot = get_shortest_direction_on_map(level, maps[Map_Dot_Large], pacman);
            assert(closest_large_dot.direction < Direction_Count);

            if (closest_ghost.distance < closest_large_dot.distance) {
                Map_Direction flee = get_shortest_direction_on_map(level, maps[Map_Flee_Ghosts], pacman);
                next_direction = flee.direction;
            }
            else {
                next_direction = closest_large_dot.direction;
            }
        }
        else if (state->small_dot_count > 0) {
            Map_Direction closest_small_dot = get_shortest_direction_on_map(level, maps[Map_Dot_Small], pacman);
            assert(closest_small_dot.direction < Direction_Count);
            next_direction = closest_small_dot.direction;
        }
        else {
            Map_Direction flee = get_shortest_direction_on_map(level, maps[Map_Flee_Ghosts], pacman);
            next_direction = flee.direction;
        }
    }

    v2s dP = get_movement_vector(pacman, static_cast<Input>(next_direction));
    return dP;
}


void collect_all_moves(s32 **maps, Array_Of_Moves *all_the_moves, Input *input, Level *level) {
    Level_State *state = level->current_state;
    for (u32 index = 0; index < state->actors.count; ++index) {
        Actor *actor = &state->actors.data[index];
        if (actor->state != Actor_State_Dead) {
            v2s dP;
            if (actor->type == Actor_Type_Pacman) {
                dP = get_pacman_move(maps, level, actor);
            }
            else {
                dP = get_movement_vector(actor, *input);
            }
            collect_move(all_the_moves, level, actor, dP);
        }
    }
}


void cancel_move_set(Level *level, Move_Set *set) {
    for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
        Move *move = &set->moves[move_index];
        Actor *actor = get_actor(&level->current_state->actors, move->actor_id);
        actor->pending_state = actor->state;
        actor->next_position = actor->position;
        set->cancelled = true;
    }
}

void cancel_moves(Array_Of_Moves *all_the_moves, Level *level) {
    for (u32 move_set_index = 0; move_set_index < all_the_moves->count; ++move_set_index) {
        Move_Set *set = &all_the_moves->data[move_set_index];
        cancel_move_set(level, set);
    }
};


u32 resolve_all_moves(Array_Of_Moves *all_the_moves, Level *level) {
    u32 valid_moves = 0;

    //
    // Solve all collisions with actor (if any) standing on the destination tile
    Level_State *state = level->current_state;
    u32 resolved_collisions;

    do {
        resolved_collisions = 0;

        for (u32 set_index = 0; set_index < all_the_moves->count; ++set_index) {
            Move_Set *set = &all_the_moves->data[set_index];
            if (set->cancelled)  continue;

            Tile *dst_tile = get_tile_at(level, set->dst);
            Actor *dst_actor = get_actor(&state->actors, dst_tile->actor_id);

            if ((set->predator_count == 0 || set->predator_count > 1) && set->move_count > 1) {
                cancel_move_set(level, set);
                ++resolved_collisions;
                printf("cancelled move set!\n");
            }
            else {
                for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
                    Move *move = &set->moves[move_index];
                    Actor *src_actor = get_actor(&state->actors, move->actor_id);

                    if (src_actor->pending_state != Actor_State_Moving)          continue; // TODO: Do we need to check this?
                    if (!actor_is_alive(src_actor) || actor_will_die(src_actor)) continue;

                    b32 immobile = false;
                    b32 collision = false;

                    // Check for collision with the actor standing on the destination tile
                    if (dst_actor && actor_is_alive(dst_actor) && !actor_will_die(dst_actor)) {
                        immobile = dst_actor->pending_state == Actor_State_Idle;
                        collision = immobile || ((dst_actor->pending_state == Actor_State_Moving) && (dst_actor->next_position == src_actor->position));
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
                            dst_actor->pending_state = Actor_State_At_Deaths_Door;
                        }
                        else {
                            src_actor->pending_state = Actor_State_Idle;
                            if (set->move_count == 1)  set->cancelled = true;
                        }
                        ++resolved_collisions;
                    }
                    else {
                        src_actor->pending_state = Actor_State_Moving;
                    }
                }
            }
        }
    } while (resolved_collisions > 0);


    //
    // Solve moves
    for (u32 set_index = 0; set_index < all_the_moves->count; ++set_index) {
        Move_Set *set = &all_the_moves->data[set_index];
        if (set->cancelled)  continue;
        if (set->move_count == 0)  continue;

        Tile *dst_tile = get_tile_at(level, set->dst);
        Actor *dst_actor = get_actor(&state->actors, dst_tile->actor_id);
        if (!actor_is_alive(dst_actor)) {
            dst_actor = nullptr;
        }

        // NOTE:
        // We know that if src is moving, then either there is no collision or src is a predator and dst is a prey,
        // so we only need to think about the potential other actors moving into this square.
        // If we only have one actor in the move then we're already done!

        for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
            Move *move = &set->moves[move_index];
            Actor *src_actor = get_actor(&state->actors, move->actor_id);
            if (!actor_is_alive(src_actor) || actor_will_die(src_actor) || src_actor->pending_state == Actor_State_Idle)  continue;

            if (set->predator_count > 1) {
                src_actor->pending_state = Actor_State_Idle;
            }
            else if (set->predator_count == 0 && set->move_count > 1) {
                src_actor->pending_state = Actor_State_Idle;
            }
            else if (set->predator_count == 1 && set->move_count > 1) {
                if (src_actor->mode == Actor_Mode_Prey) {
                    src_actor->pending_state = Actor_State_At_Deaths_Door;
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


void accept_moves(Array_Of_Moves *all_the_moves, Audio *audio, Wavs *wavs, Level *level) {
    Level_State *state = level->current_state;

    for (u32 move_set_index = 0; move_set_index < all_the_moves->count; ++move_set_index) {
        Move_Set *set = &all_the_moves->data[move_set_index];
        if (set->cancelled)  continue;

        for (u32 move_index = 0; move_index < set->move_count; ++move_index) {
            Move *move = &set->moves[move_index];
            Actor *actor = get_actor(&state->actors, move->actor_id);
            Tile *dst_tile = get_tile_at(level, set->dst);

            if (actor->pending_state == Actor_State_Moving) {
                Tile *src_tile = get_tile_at(level, actor->position);
                assert(src_tile);
                if (src_tile->actor_id == actor->id)  src_tile->actor_id = kActor_ID_Null;
                dst_tile->actor_id = actor->id;

                actor->state = Actor_State_Idle;
                actor->pending_state = Actor_State_Idle;
                actor->direction = get_direction_from_move(actor->position, actor->next_position);
                actor->position = actor->next_position;


                // Is the current actor pacman, and has the current tile any dots on it?
                if (actor->type == Actor_Type_Pacman) {
                    if (dst_tile->item.type == Item_Type_Dot_Small) {
                        --state->small_dot_count;
                        state->score -= kDot_Small_Value;
                        dst_tile->item.type = Item_Type_None;
                        play_wav(audio, &wavs->eat_small_dot);
                    }
                    else if (dst_tile->item.type == Item_Type_Dot_Large) {
                        --state->large_dot_count;
                        state->score -= kDot_Large_Value;
                        change_pacman_mode(level, Actor_Mode_Predator);
                        state->mode_duration = kPredator_Mode_Duration;
                        dst_tile->item.type = Item_Type_None;
                        play_wav(audio, &wavs->eat_large_dot);

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
    for (u32 index = 0; index < state->actors.count; ++index) {
        Actor *actor = &state->actors.data[index];
        if (actor->pending_state == Actor_State_At_Deaths_Door) {
            kill_actor(level, actor);

            if (actor_is_ghost(actor)) {
                play_wav(audio, &wavs->ghost_dies);
            }
        }
    }
}
