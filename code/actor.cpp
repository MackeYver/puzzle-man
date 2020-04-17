//
// Actor
// (c) Marcus Larsson
//


enum Direction {
    Direction_Right = 0,  
    Direction_Up,
    Direction_Left,
    Direction_Down,
    
    Direction_Count,
    Direction_Unknown
};


enum Actor_Type {        
    Actor_Type_Ghost_Red,
    Actor_Type_Ghost_Pink,
    Actor_Type_Ghost_Cyan,
    Actor_Type_Ghost_Orange,
    
    Actor_Type_Pacman,
    
    Actor_Type_Count,
    Actor_Type_Unknown,
};


enum Actor_State {
    Actor_State_Idle,
    Actor_State_Moving,
    Actor_State_Dead,
    Actor_State_At_Deaths_Door,
    
    Actor_State_Count,
};


enum Actor_Mode {
    Actor_Mode_Predator = 0,
    Actor_Mode_Prey = 1,

    Actor_Mode_Count,
};


struct Actor_ID {
    u16 index = 0;
    u16 salt = 0;
};



#define kActor_ID_Null {0xFFFF, 0xFFFF}
#define kTile_Index_Null 0xFFFFFFFF

struct Actor {    
    v2u position = {};
    v2u next_position = {};
    Actor_ID id;
    Actor_State state = Actor_State_Idle;
    Actor_Mode mode = Actor_Mode_Predator;
    Actor_Type type = Actor_Type_Unknown;
    Direction direction = Direction_Right;
};


struct Array_Of_Actors {    
    Actor *data = nullptr;
    u16 capacity = 0;
    u16 count = 0;
    u16 active = 0;
    u16 padding;
};





//
// Storage
//

void init_actor(Actor *actor);

b32 operator == (Actor_ID const& A, Actor_ID const& B) {
    b32 result = (A.index == B.index) && (A.salt == B.salt);
    return result;
}

b32 operator == (Actor const& A, Actor const& B) {
    b32 result = A.id == B.id;
    return result;
}

b32 copy_actors(Array_Of_Actors *dst, Array_Of_Actors *src) {
    b32 result = false;
    
    if (dst && src) {
        if (dst) {
            if (dst->data) {
                free(dst->data);
            }
        }
        
        *dst = *src;        
        size_t size = sizeof(Actor) * src->capacity;
        dst->data = static_cast<Actor *>(malloc(size));
        errno_t error = memcpy_s(dst->data, size, src->data, size);
        if (error != 0) {
            printf("%s() failed to copy actors, error = %d\n", __FUNCTION__, errno);
        }
        else {
            result = true;
        }
    }

    return result;
}


void free_array_of_actors(Array_Of_Actors *array) {
    if (array) {
        if (array->data) {
            free(array->data);
            array->data = nullptr;
        }
        
        array->capacity = 0;
        array->count = 0;
        array->active = 0;
    }
}


b32 grow_array_of_actors(Array_Of_Actors *array) {
    b32 result = false;
    
    if (array) {
        u16 new_capacity = array->capacity == 0 ? 10 : 2 * array->capacity;
        size_t new_size = sizeof(Actor) * new_capacity;
        void *new_ptr = realloc(array->data, new_size);
        if (new_ptr) {
            array->capacity = new_capacity;
            array->data = static_cast<Actor *>(new_ptr);
            result = true;

            for (u16 index = array->count; index < array->capacity; ++index) {
                array->data[index].id = {index, 0};
            }
        }
        else {
            printf("%s in %s failed to reallocate memory!\n", __FUNCTION__, __FILE__);
        }
    }

    return result;
}


Actor *new_actor(Array_Of_Actors *array) {
    Actor *result = nullptr;

    if (array) {
        if (array->active == array->count) {
            if ((array->capacity - array->count) < 1) {
                assert(grow_array_of_actors(array));
            }

            result = &array->data[array->count++];
        }
        else {
            for (u32 index = 0; index < array->count; ++index) {
                Actor *curr_actor = &array->data[index];
                if (curr_actor->state == Actor_State_Dead) {
                    result = curr_actor;
                    break;
                }
            }
        }
        
        ++array->active;
    }

    if (result)  init_actor(result);

    return result;
}


void delete_actor(Array_Of_Actors *array, Actor_ID delete_id) {
    if (array) {
        array->data[delete_id.index].state = Actor_State_Dead;
        ++array->data[delete_id.index].id.salt;

        // If we're deleteing the actor that is last, then we can reduce the count of actors.
        // We're not moving any of the actors due to other systems keeps an id into this array.
        if (delete_id.index == (array->count - 1)) {
            --array->count;
        }

        --array->active;
    }
}


Actor *get_actor(Array_Of_Actors *array, Actor_ID id) {
    Actor *result = nullptr;

    if (array && array->count > id.index) {
        Actor *curr_actor = &array->data[id.index];
        if (curr_actor->id == id) {
            result = curr_actor;
        }
    }

    return result;
}




//
// Actor stuff
//

void init_actor(Actor *actor) {
    actor->position = V2u(0, 0);
    actor->next_position = V2u(0, 0);
    actor->state = Actor_State_Idle;
    actor->mode = Actor_Mode_Predator;
    actor->type = Actor_Type_Unknown;
}

void free_actor(Actor *actor) {
    if (actor) {
        actor->position = V2u(0, 0);
        actor->state = Actor_State_Idle;
        actor->type = Actor_Type_Unknown;
    }
}


b32 actor_is_ghost(Actor *actor) {
    b32 result = false;

    if (actor->type == Actor_Type_Ghost_Red  || actor->type == Actor_Type_Ghost_Pink ||
        actor->type == Actor_Type_Ghost_Cyan || actor->type == Actor_Type_Ghost_Orange)
    {
        result = true;
    }

    return result;
}


b32 actor_is_alive(Actor *actor) {
    b32 result = false;

    if (actor && (actor->state != Actor_State_Dead && actor->state != Actor_State_At_Deaths_Door)) {
        result = true;
    }
    
    return result;
}

b32 actor_is_predator(Actor *actor) {
    b32 result = false;

    if (actor_is_alive(actor) && actor->mode == Actor_Mode_Predator) {
        result = true;
    }

    return result;
}

b32 actor_is_prey(Actor *actor) {
    b32 result = false;

    if (actor_is_alive(actor) && actor->mode == Actor_Mode_Prey) {
        result = true;
    }

    return result;
}


void draw_ghost(Render_State *render_state, Resources *resources, Actor *actor, Actor *pacman, u32 int_t) {  
    Bmp *bitmap = nullptr;
    if (actor->mode == Actor_Mode_Prey) {
        bitmap = &resources->bitmaps.ghost_as_prey;
    }
    else {
        switch (actor->type) {
            case Actor_Type_Ghost_Red:    { bitmap = &resources->bitmaps.ghost_red;    } break;
            case Actor_Type_Ghost_Pink:   { bitmap = &resources->bitmaps.ghost_pink;   } break;
            case Actor_Type_Ghost_Cyan:   { bitmap = &resources->bitmaps.ghost_cyan;   } break;
            case Actor_Type_Ghost_Orange: { bitmap = &resources->bitmaps.ghost_orange; } break;
        }
    }

    v2u Po = V2u(kCell_Size * actor->position.x, kCell_Size * actor->position.y);
    v2u P = Po;
    {
        f32 Pax = static_cast<f32>(Po.x);
        f32 Pay = static_cast<f32>(Po.y);

        f32 constexpr k = 1.0f / 1000000.0f;
        //f32 constexpr k2 = 1.0f / 3.0f;
        //f32 constexpr f32_tau = 6.28318530f;
        f32 t = static_cast<f32>(int_t) * k;
    
        f32 x = 0.0f;//sinf(3.6f*t);
        f32 y = 3.0f*cosf(1.8f*t);
        Pax += x;
        Pay += y;
        P = V2u(static_cast<u32>(Pax), static_cast<u32>(Pay));

        draw_bitmap(render_state, P, bitmap);
        draw_bitmap(render_state, P, &resources->bitmaps.ghost_eye);
    }
    
    if (actor_is_alive(pacman) && actor_is_alive(actor)) {
        f32 Pax = static_cast<f32>(Po.x);
        f32 Pay = static_cast<f32>(Po.y);
        f32 Ppx = static_cast<f32>(kCell_Size * pacman->position.x);
        f32 Ppy = static_cast<f32>(kCell_Size * pacman->position.y);

        f32 dx = Ppx - Pax;
        f32 dy = Ppy - Pay;
        f32 l = sqrtf(dx * dx + dy * dy);
        if (!almost_equal_relative(l, 0.0f)) {
            dx = dx / l;
            dy = dy / l;

            dx *= 3;
            dy *= 4;

            f32 Px = static_cast<f32>(P.x) + dx;
            f32 Py = static_cast<f32>(P.y) + dy;

            assert(Px >= 0);
            assert(Py >= 0);

            P = V2u(static_cast<u32>(Px), static_cast<u32>(Py));
        }
    }
    draw_bitmap(render_state, P, &resources->bitmaps.ghost_pupil);
}


void kill_actor(Array_Of_Actors *array, Actor *actor) {
    if (array && actor) {
        delete_actor(array, actor->id);
        actor->state = Actor_State_Dead;
    }
}


void draw_pacman(Render_State *render_state, Resources *resources, Actor *actor, u32 int_t) {
    v2u P = V2u(kCell_Size * actor->position.x, kCell_Size * actor->position.y);
    
    f32 constexpr duration = 0.6f;
    f32 constexpr frame_time = duration / 4.0f;    

    f32 constexpr k = 1.0f / 1000000.0f;
    f32 tot_t = static_cast<f32>(int_t) * k;    
    f32 t = fmodf(tot_t, duration) / frame_time;
    
    u32 x = 64 * static_cast<u32>(actor->direction);
    u32 y = 64 * static_cast<u32>(floor(t));
    draw_bitmap(render_state, P, &resources->bitmaps.pacman_atlas, x, y, x + 64, y + 64);
}
