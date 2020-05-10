//
// Level editor
//




//
// Declarations
//

//
// Mouse
enum Input_Mouse_Button_State {
    Input_Mouse_Button_Up = 0,
    Input_Mouse_Button_Pressed,
    Input_Mouse_Button_Down,
    Input_Mouse_Button_Released,
};

struct Mouse_Button {
    Input_Mouse_Button_State prev;
    Input_Mouse_Button_State curr;
};

struct Input_Mouse {
    b32 is_inside = false;
    v2 position;

    union {
        struct {
            Mouse_Button left_button;
            Mouse_Button middle_button;
            Mouse_Button right_button;
        };
        Mouse_Button buttons[3];
    };
};


//
// Keyboard

#define kInput_Keyboard_Buffer_Length kLevel_Name_Max_Length

enum Input_Keyboard_State {
    Input_Keyboard_State_Inactive = 0,
    Input_Keyboard_State_Receive,
    Input_Keyboard_State_Done,
    Input_Keyboard_State_Cancel,

    Input_Keyboard_State_Count,
};

enum Input_Keyboard_Mode {
    Input_Keyboard_Mode_Alphanumeric,
    Input_Keyboard_Mode_Integer,
    Input_Keyboard_Mode_Float,
    
    Input_Keyboard_Mode_Count,
};

struct Input_Keyboard {
    char buffer[kInput_Keyboard_Buffer_Length];
    u32 cursor_position = 0; // Location of cursor position, will always be in the range [0, ..., end_position - 1] 
    u32 end_position = 0;    // One past the last char in the buffer (position of the null terminator). Will always be at least cursor_position + 1.
    b32 decimal_entered = false;
    Input_Keyboard_State state = Input_Keyboard_State_Inactive;
    Input_Keyboard_Mode mode = Input_Keyboard_Mode_Alphanumeric;
};

static void cancel_keyboard_input(Input_Keyboard *keyboard);
static void clear_keyboard_input(Input_Keyboard *keyboard);
static void scan_for_decimal(Input_Keyboard *keyboard);

static void begin_keyboard_input(Input_Keyboard *keyboard, Input_Keyboard_Mode mode);
static void begin_keyboard_input(Input_Keyboard *keyboard, f32 value);

static void add_char_at_cursor(Input_Keyboard *keyboard, char const c);
static void del_at_cursor(Input_Keyboard *keyboard);
static void backspace_at_cursor(Input_Keyboard *keyboard);
static void move_cursor_left(Input_Keyboard *keyboard, u32 n = 1);
static void move_cursor_right(Input_Keyboard *keyboard, u32 n = 1);
static void move_cursor_to_start(Input_Keyboard *keyboard);
static void move_cursor_to_end(Input_Keyboard *keyboard);

struct Editor_Input {
    Input_Keyboard keyboard;
    Input_Mouse mouse;
};



// 
// Selected object
enum Object_Type {
    Object_Type_None,
    Object_Type_Tile,
    Object_Type_Item,
    Object_Type_Actor,

    Object_Type_Count
};

struct Selected_Object {
    Object_Type type;
    u32 value;
};



//
// Level Editor
enum Level_Editor_State {
    Level_Editor_State_Editing,
    Level_Editor_State_Menu,
    Level_Editor_State_Menu_Input_Name,
    Level_Editor_State_Menu_Input_Load,
};

enum Level_Editor_Message {
    Level_Editor_Message_None,
    Level_Editor_Message_Unsaved_Changes,
    Level_Editor_Message_Load_Failed,

    Level_Editor_Message_Count,
};

struct Level_Editor {
    Level level;
    Editor_Input input;
    Tile *hot_tile = nullptr;
    Selected_Object selected_object;
    u32 render_mode;
    Level_Editor_State state;
    Level_Editor_Message message = Level_Editor_Message_None;
    u32 current_level_count = 0;
    b32 level_has_unsaved_changes = false;
};




//
// #_Keyboard input
//

static void cancel_keyboard_input(Input_Keyboard *keyboard) {
    keyboard->state = Input_Keyboard_State_Cancel;
    clear_keyboard_input(keyboard);
}


static void clear_keyboard_input(Input_Keyboard *keyboard) {
    keyboard->end_position = 0;
    keyboard->buffer[0] = '\0';
    keyboard->cursor_position = 0;
}


static void begin_keyboard_input(Input_Keyboard *keyboard, Input_Keyboard_Mode mode) {
    clear_keyboard_input(keyboard);
    keyboard->state = Input_Keyboard_State_Receive;
    keyboard->mode = mode;
}

static void begin_keyboard_input(Input_Keyboard *keyboard, f32 value) {
    begin_keyboard_input(keyboard, Input_Keyboard_Mode_Float);
    
    u32 char_num = _snprintf_s(keyboard->buffer, kInput_Keyboard_Buffer_Length, _TRUNCATE, "%g", value);
    keyboard->cursor_position = char_num;
    keyboard->end_position = char_num;

    scan_for_decimal(keyboard);
}


static void begin_keyboard_input(Input_Keyboard *keyboard, u32 value) {
    begin_keyboard_input(keyboard, Input_Keyboard_Mode_Integer);
    
    u32 char_num = _snprintf_s(keyboard->buffer, kInput_Keyboard_Buffer_Length, _TRUNCATE, "%u", value);
    keyboard->cursor_position = char_num;
    keyboard->end_position = char_num;

    scan_for_decimal(keyboard);
}


static void begin_keyboard_input(Input_Keyboard *keyboard, char const *text) {
    begin_keyboard_input(keyboard, Input_Keyboard_Mode_Alphanumeric);
    
    u32 char_num = _snprintf_s(keyboard->buffer, kInput_Keyboard_Buffer_Length, _TRUNCATE, "%s", text);
    keyboard->cursor_position = char_num;
    keyboard->end_position = char_num;

    scan_for_decimal(keyboard);
}


static void scan_for_decimal(Input_Keyboard *keyboard) {
    keyboard->decimal_entered = false;

    // NOTE: we're only checking for one decimal, what if there are more?
    for (char *ptr = keyboard->buffer; *ptr; ++ptr) {
        if (*ptr == '.') {
            keyboard->decimal_entered = true;
            break;            
        }
    }
}


static void add_char_at_cursor(Input_Keyboard *keyboard, char const c) {
    if (keyboard->end_position != kInput_Keyboard_Buffer_Length) {
        u32 c_as_int = static_cast<u32>(c);
        
        if ((c == '.' || c == ',') && keyboard->mode != Input_Keyboard_Mode_Integer) {
            if (keyboard->mode == Input_Keyboard_Mode_Float && keyboard->decimal_entered) {
            }
            else {
                keyboard->buffer[keyboard->end_position++] = '.';
                ++keyboard->cursor_position;
                keyboard->decimal_entered = true;
            }
        }
        else if((c_as_int >= 0x30 && c_as_int <= 0x39) || (c_as_int >= 0x41 && c_as_int <= 0x5a) || (c_as_int >= 0x61 && c_as_int <= 0x7a) || (c_as_int == 32)) {
            ++keyboard->end_position;
            if (keyboard->end_position > 1) {
                for (u32 index = keyboard->end_position; index > keyboard->cursor_position; --index) {
                    keyboard->buffer[index] = keyboard->buffer[index - 1];
                }
            }
            
            keyboard->buffer[keyboard->cursor_position] = c;
            keyboard->buffer[keyboard->end_position] = '\0';

            if ((keyboard->cursor_position + 1) == keyboard->end_position)  ++keyboard->cursor_position;

            //keyboard->buffer[keyboard->end_position++] = c;
            //++keyboard->cursor_position;
        }
    }
}


static void del_at_cursor(Input_Keyboard *keyboard) {
    u32 end_position = keyboard->end_position > 1 ? keyboard->end_position - 1 : 0;
    
    for (u32 index = keyboard->cursor_position; index < end_position; ++index) {
        keyboard->buffer[index] = keyboard->buffer[index + 1];
    }    
    
    if (keyboard->end_position > 0) {
        --keyboard->end_position;
    }
    if ((keyboard->cursor_position > 0) && (keyboard->cursor_position >= keyboard->end_position)) {
        --keyboard->cursor_position;
    }

    keyboard->buffer[keyboard->end_position] = '\0';

    if (keyboard->mode == Input_Keyboard_Mode_Float)  scan_for_decimal(keyboard);
}


static void backspace_at_cursor(Input_Keyboard *keyboard) {    
    if (keyboard->cursor_position > 0) {
        u32 end_position = keyboard->end_position > 1 ? keyboard->end_position - 1 : 0;
        
        for (u32 index = keyboard->cursor_position - 1; index < end_position; ++index) {
            keyboard->buffer[index] = keyboard->buffer[index + 1];
        }
        
        --keyboard->cursor_position;
        keyboard->buffer[--keyboard->end_position] = '\0';

        if (keyboard->mode == Input_Keyboard_Mode_Float)  scan_for_decimal(keyboard);
    }
}

static void move_cursor_left(Input_Keyboard *keyboard, u32 n) {
    if (n >= keyboard->cursor_position) {
        keyboard->cursor_position = 0;
    }
    else {        
        keyboard->cursor_position -= n;
    }
}

static void move_cursor_right(Input_Keyboard *keyboard, u32 n) {
    u32 new_position = keyboard->cursor_position + n;
    u32 last_position = keyboard->end_position > 1 ? keyboard->end_position : 0;
        
    if (new_position <= last_position) {
        keyboard->cursor_position = new_position;
    }
    else {
        keyboard->cursor_position = last_position;
    }
}

static void move_cursor_to_start(Input_Keyboard *keyboard) {
    move_cursor_left(keyboard, keyboard->end_position);
}
    
static void move_cursor_to_end(Input_Keyboard *keyboard) {
    move_cursor_right(keyboard, keyboard->end_position);
}

static u32 get_cursor_position_in_pixels(Input_Keyboard *keyboard, Font *font, Char_Data *char_data_output) {
    u32 result = 0;
    
    for (u32 index = 0; index < keyboard->cursor_position; ++index) {
        u32 curr_char_index = keyboard->buffer[index] - 32;
        Char_Data *curr_char_data = &font->char_data[curr_char_index];
        result += curr_char_data->advance_x;

        if ((index == (keyboard->cursor_position - 1) && char_data_output)) {
            *char_data_output = *curr_char_data;
        }
    }

    return result;
}




//
// Init and fini
//

static b32 init_editor(Level_Editor *editor) {
    Tokenizer tokenizer;
    b32 result = init_tokenizer(&tokenizer, "data\\levels\\", "editor_data.txt");
    if (result) {
        Token token;
        require_identifier_with_exact_name(&tokenizer, &token, "Current_level_count");
        require_token(&tokenizer, &token, Token_colon);
        require_token(&tokenizer, &token, Token_number);
        editor->current_level_count = get_u32_from_token(&token, &result);

        editor->message = Level_Editor_Message_None;
    }
    
    fini_tokenizer(&tokenizer);

    return result;
}


static void fini_editor(Level_Editor *editor) {   
    fini_level(&editor->level);   


    //
    // Write editor state to file
    HANDLE file_handle;
    b32 result = win32_open_file_for_writing("data\\levels\\editor_data.txt", &file_handle);
    assert(result);    

    u32 constexpr buffer_size = 512;
    char buffer[buffer_size];
    DWORD bytes_written;
    DWORD bytes_to_write = _snprintf_s(buffer, buffer_size, _TRUNCATE, "Current_level_count:%u", editor->current_level_count);
    WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
    assert(bytes_to_write == bytes_written);
    
    CloseHandle(file_handle);
}


static void begin_editing(Level_Editor *editor, Level *level, Level_Render_Mode render_mode) {
    editor->state = Level_Editor_State_Editing;
    editor->render_mode = render_mode;
    editor->selected_object.type  = Object_Type_Tile;
    editor->selected_object.value = Tile_Type_Wall_0;
    
    init_level_as_copy_of_level(&editor->level, level, &level->original_state);
    create_maps_off_level(&editor->level);
}


static void end_editing(Level_Editor *editor, Level *level) {    
    init_level_as_copy_of_level(level, &editor->level, editor->level.current_state);
    create_maps_off_level(level);
}




//
// Editing
//

static void clear_tile(Level_State *state, Tile *tile) {
    if (tile->item.type == Item_Type_Dot_Small) {
        state->score -= kDot_Small_Value;
        --state->small_dot_count;
        tile->item.type = Item_Type_None;
    }
    else if (tile->item.type == Item_Type_Dot_Large) {
        state->score -= kDot_Large_Value;
        --state->large_dot_count;
        tile->item.type = Item_Type_None;
    }
    
    if (tile->actor_id.index < 0xFFF) {
        Actor *actor = get_actor(&state->actors, tile->actor_id);
        if (actor_is_ghost(actor)) {
            state->score -= kGhost_Value;
            --state->ghost_count;
        }
        else if (actor->type == Actor_Type_Pacman) {
            --state->pacman_count;
        }

        delete_actor(&state->actors, tile->actor_id);        
        tile->actor_id = kActor_ID_Null;        
    }
}


static void validate_level(Level_State *state) {
    
}


static void edit_level(Level_Editor *editor, Renderer *renderer, Input input, u32 microseconds_since_start) {
    Level *level = &editor->level;
    Font *font = &level->resources->font;
    Input_Mouse *mouse = &editor->input.mouse;

    
    //
    // TODO: handle window and cell sizes properly!
    u32 cell_size = kCell_Size;//renderer->get_backbuffer_width() / level->width;
    //assert(cell_size == kCell_Size);
 
   
    //
    // Open/close the menu
    if (input == Input_Space) {
        editor->state = editor->state == Level_Editor_State_Editing ? Level_Editor_State_Menu : Level_Editor_State_Editing;
    }


    
    //
    // Editing
    //
    if (editor->state == Level_Editor_State_Editing) {
        //
        // Render tiles and map
        draw_level(renderer, level, editor->render_mode, microseconds_since_start);
        draw_maps(renderer, level, level->maps, level->current_map_index);

        b32 changed_in_this_frame = false;
        editor->hot_tile = nullptr;
        Tile *hot_tile = editor->hot_tile;
        Level_State *current_state = editor->level.current_state;
        Selected_Object *selected_object = &editor->selected_object;
        
        if (mouse->is_inside) {
            v2u Pmc = V2u(static_cast<u32>(floorf(mouse->position.x / cell_size)),
                          static_cast<u32>(floorf(mouse->position.y / cell_size)));

            if (Pmc.x < level->width && Pmc.y < level->height) {
                v2u P = V2u(cell_size * Pmc.x, cell_size * Pmc.y);

                Tile *tile = get_tile_at(&editor->level, Pmc);
                hot_tile = tile;

                renderer->draw_rectangle_outline(P, static_cast<u32>(cell_size), static_cast<u32>(cell_size), v4u8_yellow);
            }            

            if (hot_tile) {
                if (mouse->left_button.curr == Input_Mouse_Button_Pressed) {
                    if (selected_object->type == Object_Type_Tile) {
                        clear_tile(current_state, hot_tile);
                        hot_tile->type = static_cast<Tile_Type>(selected_object->value);
                    }
                    else if (selected_object->type == Object_Type_Item) {
                        Item_Type item_type = static_cast<Item_Type>(selected_object->value);
                        clear_tile(current_state, hot_tile);
                        
                        hot_tile->item.type = item_type;
                        
                        if (item_type == Item_Type_Dot_Small) {
                            current_state->score += kDot_Small_Value;
                            ++current_state->small_dot_count;
                        }
                        
                        if (item_type == Item_Type_Dot_Large) {
                            current_state->score += kDot_Large_Value;
                            ++current_state->large_dot_count;
                        }
                    }
                    else if (selected_object->type == Object_Type_Actor) {
                        clear_tile(current_state, hot_tile);
                        Actor_Type type = static_cast<Actor_Type>(selected_object->value);
                        if (type == Actor_Type_Pacman && current_state->pacman_count > 0) {
                            Actor *pacman = get_pacman(&editor->level);
                            if (pacman) {
                                Tile *old_tile = get_tile_at(level, current_state, pacman->position);
                                if (!old_tile) {
                                    int a = 0;
                                    ++a;
                                }

                                old_tile->actor_id = kActor_ID_Null;
                                pacman->position = Pmc;                                
                                hot_tile->actor_id = pacman->id;
                            }
                            else {
                                int a = 0;
                                ++a;
                            }
                        }
                        else {
                            clear_tile(current_state, hot_tile);     
                            add_actor(nullptr, &editor->level, current_state, Pmc.x, Pmc.y, type);
                        }
                    }
                    changed_in_this_frame = true;
                    editor->level_has_unsaved_changes = true;
                }
                else if (mouse->middle_button.curr == Input_Mouse_Button_Pressed) {
                    if (hot_tile->item.type != Item_Type_None) {
                        selected_object->type = Object_Type_Item;
                        selected_object->value = hot_tile->item.type;
                    }
                    else if (hot_tile->actor_id.index < 0xFFFF) {
                        selected_object->type = Object_Type_Actor;
                        Actor *actor = get_actor(&current_state->actors, hot_tile->actor_id);
                        selected_object->value = actor->type;
                    }
                    else {
                        selected_object->type = Object_Type_Tile;
                        selected_object->value = hot_tile->type;
                    }
                }
                else if (mouse->right_button.curr == Input_Mouse_Button_Pressed) {
                    if ((hot_tile->item.type != Item_Type_None) || (hot_tile->actor_id.index < 0xFFFF)) {
                        hot_tile->type = Tile_Type_Floor;
                    }                    
                    else if (hot_tile->type == Tile_Type_None) {
                        hot_tile->type = Tile_Type_Floor;
                    }
                    else if (hot_tile->type == Tile_Type_Floor) {
                        hot_tile->type = Tile_Type_None;
                    }
                    else {
                        hot_tile->type = Tile_Type_Floor;
                    }
                    
                    clear_tile(current_state, hot_tile);
                    changed_in_this_frame = true;
                    editor->level_has_unsaved_changes = true;
                }
            }
        }


        if (changed_in_this_frame) {        
            adjust_walls_in_level(level, current_state);
            create_maps_off_level(&editor->level);
        }  
    }



    //
    // Menu
    //
    else if (editor->state >= Level_Editor_State_Menu) {
        v2u Pmc = V2u(static_cast<u32>(floorf(mouse->position.x / cell_size)),
                      static_cast<u32>(floorf(mouse->position.y / cell_size)));
        b32 mouse_is_inside = mouse->is_inside;
                        
        //
        // Draw the tiles and stuff
        Tile_Type  constexpr tiles[]  = {Tile_Type_Wall_0, Tile_Type_Floor, Tile_Type_None};
        u32 tile_count = Array_Count(tiles);
        
        Item_Type  constexpr items[]  = {Item_Type_Dot_Small, Item_Type_Dot_Large};
        u32 item_count = Array_Count(items);
        
        Actor_Type constexpr actors[] = {Actor_Type_Ghost_Red, Actor_Type_Ghost_Pink, Actor_Type_Ghost_Cyan, Actor_Type_Ghost_Orange, Actor_Type_Pacman};
        u32 actor_count = Array_Count(actors);

        v2u Pc = V2u(1, level->height - 1);
        v4u8 colour = v4u8_white;

        //Tile *hot_tile = editor->hot_tile;
        //Level_State *current_state = editor->level.current_state;
        Selected_Object *selected_object = &editor->selected_object;
        Input_Keyboard *keyboard = &editor->input.keyboard;

        
        //
        // Print name
        if (mouse_is_inside && (mouse->left_button.curr == Input_Mouse_Button_Pressed)) {
            if ((Pmc.y >= Pc.y) && (editor->state != Level_Editor_State_Menu_Input_Name)) {
                cancel_keyboard_input(keyboard);
                begin_keyboard_input(keyboard, level->name);
                editor->state = Level_Editor_State_Menu_Input_Name;
            }
            else if ((Pmc.y < Pc.y) && (editor->state == Level_Editor_State_Menu_Input_Name)) {
                cancel_keyboard_input(keyboard);
                keyboard->state = Input_Keyboard_State_Inactive;
                editor->state = Level_Editor_State_Menu;
            }
        }

        {
            char buffer[512];
            v4u8 text_colour = colour;            

            if ((editor->state == Level_Editor_State_Menu_Input_Name)  && (keyboard->state == Input_Keyboard_State_Receive)) {
                _snprintf_s(buffer, kLevel_Name_Max_Length, _TRUNCATE, "%s", keyboard->buffer);
                text_colour = v4u8_yellow;
            }
            else if ((editor->state == Level_Editor_State_Menu_Input_Name)  && (keyboard->state == Input_Keyboard_State_Done)) {
                _snprintf_s(editor->level.name, kLevel_Name_Max_Length, _TRUNCATE, "%s", keyboard->buffer);
                cancel_keyboard_input(keyboard);
            }   
            else {
                _snprintf_s(buffer, 512, _TRUNCATE, "%s", editor->level.name);
            }

            char const *caption = "Name: ";
            v2u caption_dim = get_text_dim(font, caption);
            v2u name_dim = get_text_dim(font, buffer);

            v2u Pt = Pc * cell_size;

            if ((editor->state == Level_Editor_State_Menu_Input_Name)  && (keyboard->state == Input_Keyboard_State_Receive)) {
                u32 cursor_x = get_cursor_position_in_pixels(keyboard, font, nullptr);
                renderer->draw_filled_rectangle(Pt + V2u(caption_dim.x + cursor_x, 0) - V2u(0, 5), 12, 24, v4u8_red);
            }
            
            renderer->print(font, Pt, "Name: ", colour);
            renderer->print(font, Pt + V2u(caption_dim.x, 0), buffer, text_colour);
            --Pc.y;

            Pt = Pc * cell_size;
            char const *id_caption = "ID: ";
            v2u id_caption_dim = get_text_dim(font, id_caption);

            _snprintf_s(buffer, kLevel_Name_Max_Length, _TRUNCATE, "%u", level->id);
            v2u id_value_dim = get_text_dim(font, buffer);
            renderer->print(font, Pt + V2u(0               , (cell_size - id_value_dim.y) / 2), id_caption, colour);
            renderer->print(font, Pt + V2u(id_caption_dim.x, (cell_size - id_value_dim.y) / 2), buffer, text_colour);
            --Pc.y;
        }
        

        //
        // Draw the "tiles-palette"
        {
            char const *text = "Tiles";
            v2u text_dim = get_text_dim(font, text);
            u32 y = (Pc.y * cell_size) + ((cell_size - text_dim.y) / 2);
            renderer->print(font, V2u(Pc.x * cell_size, y) + V2u(1, 1), text, V4u8(125, 125, 125, 255));
            renderer->print(font, V2u(Pc.x * cell_size, y), text);
            --Pc.y;
        
            for (u32 index = 0; index < tile_count; ++index) {
                v2u P = V2u(cell_size * Pc.x, cell_size * Pc.y);
                Tile tile = empty_tile_of_type(tiles[index]);                
                draw_tile(renderer, editor->level.resources, &tile, P);

                if (mouse_is_inside && (Pmc.x == Pc.x && Pmc.y == Pc.y) && (editor->state == Level_Editor_State_Menu)) {
                    colour = v4u8_yellow;

                    if (mouse->left_button.curr == Input_Mouse_Button_Down) {
                        selected_object->type = Object_Type_Tile;
                        selected_object->value = tiles[index];
                    }                    
                }
                else {
                    colour = v4u8_white;
                }
            
                renderer->draw_rectangle_outline(P, cell_size, cell_size, colour);
            
                ++Pc.x;
                if (Pc.x >= level->width) {
                    Pc.x = 0;
                    ++Pc.y;
                }
            }
        }

        
        //
        // Draw items
        {
            char const *text = "Items";
            v2u text_dim = get_text_dim(font, text);
            Pc = V2u(1, Pc.y - 1);
            u32 y = (Pc.y * cell_size) + ((cell_size - text_dim.y) / 2);
            renderer->print(font, V2u(Pc.x * cell_size, y) + V2u(1, 1), text, V4u8(125, 125, 125, 255));
            renderer->print(font, V2u(Pc.x * cell_size, y), text);
            --Pc.y;

            for (u32 index = 0; index < item_count; ++index) {
                v2u P = V2u(cell_size * Pc.x, cell_size * Pc.y);
                Tile tile = empty_tile_of_type(Tile_Type_None);
                tile.item.type = items[index];
                draw_item(renderer, editor->level.resources, &tile, P);
                
                if (mouse_is_inside && (Pmc.x == Pc.x && Pmc.y == Pc.y) && (editor->state == Level_Editor_State_Menu)) {
                    colour = v4u8_yellow;
                    
                    if (mouse->left_button.curr == Input_Mouse_Button_Down) {
                        selected_object->type = Object_Type_Item;
                        selected_object->value = items[index];
                    }                    
                }
                else {
                    colour = v4u8_white;
                }
            
                renderer->draw_rectangle_outline(P, cell_size, cell_size, colour);

                ++Pc.x;
                if (Pc.x >= level->width) {
                    Pc.x = 0;
                    ++Pc.y;
                }
            }
        }


        //
        // Draw actors
        {
            char const *text = "Actors";
            v2u text_dim = get_text_dim(font, text);
            Pc = V2u(1, Pc.y - 1);
            u32 y = (Pc.y * cell_size) + ((cell_size - text_dim.y) / 2);
            renderer->print(font, V2u(Pc.x * cell_size, y) + V2u(1, 1), text, V4u8(125, 125, 125, 255));
            renderer->print(font, V2u(Pc.x * cell_size, y), text);
            --Pc.y;

            Actor actor;
            init_actor(&actor);

            for (u32 index = 0; index < actor_count; ++index) {
                v2u P = V2u(cell_size * Pc.x, cell_size * Pc.y);
                actor.position = Pc;
                actor.type = actors[index];
                if (actor_is_ghost(&actor)) {
                    draw_ghost(renderer, editor->level.resources, &actor, nullptr, 0, 0);
                }
                else {
                    draw_pacman(renderer, editor->level.resources, &actor, 1000000);
                }
                
                if (mouse_is_inside && (Pmc.x == Pc.x && Pmc.y == Pc.y) && (editor->state == Level_Editor_State_Menu)) {
                    colour = v4u8_yellow;
                    
                    if (mouse->left_button.curr == Input_Mouse_Button_Down) {
                        selected_object->type = Object_Type_Actor;
                        selected_object->value = actors[index];
                    }
                }
                else {
                    colour = v4u8_white;
                }
            
                renderer->draw_rectangle_outline(P, cell_size, cell_size, colour);

                ++Pc.x;
                if (Pc.x >= level->width) {
                    Pc.x = 0;
                    ++Pc.y;
                }
            }
        }


        //
        // New, Save and Load
        {
            b32 static pressed_load_once = false; // DEBUG
            
            char const *text[4] = {"New", "Save", "Load", "Copy"};
            
            for (u32 index = 0; index < Array_Count(text); ++index) {
                v2u text_dim = get_text_dim(font, text[index]);
                Pc = V2u(2 * index + 1, 1);
                v2u P = V2u(cell_size * Pc.x, cell_size * Pc.y);
                v2u offset = V2u((cell_size - text_dim.x) / 2, (cell_size - text_dim.y) / 2);
                colour = v4u8_white;

                if (mouse_is_inside && (Pmc.x == Pc.x && Pmc.y == Pc.y) && (editor->state == Level_Editor_State_Menu)) {
                    colour = v4u8_yellow;
                
                    if (mouse->left_button.curr == Input_Mouse_Button_Pressed) {
                        if (index == 0) {
                            //
                            // New level
                            Resources *resources = level->resources;
                            
                            fini_level(level);
                            init_level(level, resources);
                            level->id = ++editor->current_level_count;
                            _snprintf_s(level->name, kLevel_Name_Max_Length, _TRUNCATE, "Level_%u", level->id);

                            // TODO: handle different level sizes!
                            level->width = 11;
                            level->height = 11;
                            Level_State *state = &level->original_state;
                            state->tile_count = level->width * level->height;
                            state->tiles = static_cast<Tile *>(malloc(state->tile_count * sizeof(Tile)));
                            for (u32 tile_index = 0; tile_index < state->tile_count; ++tile_index) {
                                empty_tile(&state->tiles[tile_index]);
                            }

                            copy_level_state(level->current_state, &level->original_state);
                        }
                        else if (index == 1) {
                            //
                            // Save level
                            copy_level_state(&level->original_state, level->current_state);
                            save_level(level);
                            editor->level_has_unsaved_changes = false;                            
                        }
                        if (index == 2) {
                            //
                            // Load level
                            if (editor->level_has_unsaved_changes && !pressed_load_once) {
                                pressed_load_once = true;
                            }
                            else {                                
                                editor->state = Level_Editor_State_Menu_Input_Load;
                                pressed_load_once = false;
                            }

                            if (editor->state == Level_Editor_State_Menu_Input_Load) {
                                colour = v4u8_yellow;
                            }
                        }
                        else if (index == 3) {
                            //
                            // Copy level
                            if (editor->level_has_unsaved_changes && !pressed_load_once) {
                                pressed_load_once = true;
                            }
                            else {
                                if (editor->level_has_unsaved_changes) {
                                    copy_level_state(&level->original_state, level->current_state);
                                    save_level(level);
                                    editor->level_has_unsaved_changes = false;
                                }                                

                                char temp_text_buffer[kLevel_Name_Max_Length];
                                _snprintf_s(temp_text_buffer, kLevel_Name_Max_Length, _TRUNCATE, "Copy of %s", level->name);
                                _snprintf_s(level->name, kLevel_Name_Max_Length, _TRUNCATE, "%s", temp_text_buffer);
                                level->id = ++editor->current_level_count;
                            }
                        }
                    }
                
                    renderer->draw_rectangle_outline(P, cell_size, cell_size, colour);
                }

                renderer->print(font, P + offset + V2u(1, 1), text[index], V4u8(125, 125, 125, 255));
                renderer->print(font, P + offset, text[index], colour);
            }
        }


        //
        // Editor message
        if ((editor->message == Level_Editor_Message_None) && (editor->level_has_unsaved_changes)) {
            editor->message = Level_Editor_Message_Unsaved_Changes;
        }
    
        if (editor->message != Level_Editor_Message_None) {
            switch (editor->message) {
                case Level_Editor_Message_Unsaved_Changes: {
                    if (editor->level_has_unsaved_changes) {
                    char constexpr *unsaved_text = "Level has unsaved changes";
                    v2u text_dim = get_text_dim(font, unsaved_text);
                    renderer->print(font, V2u(cell_size * 1, (cell_size * 1) - text_dim.y), unsaved_text, v4u8_red);
                    }
                    else {
                        editor->message = Level_Editor_Message_None;
                    }
                } break;

                case Level_Editor_Message_Load_Failed: {
                    char constexpr *unsaved_text = "Load failed";
                    v2u text_dim = get_text_dim(font, unsaved_text);
                    renderer->print(font, V2u(cell_size * 1, (cell_size * 1) - text_dim.y), unsaved_text, v4u8_red);
                }
            }
        }
    }


    //
    // Load input
    if (editor->state == Level_Editor_State_Menu_Input_Load) {
        Input_Keyboard *keyboard = &editor->input.keyboard;
        
        if (keyboard->state == Input_Keyboard_State_Cancel) {
            editor->state = Level_Editor_State_Menu;
            cancel_keyboard_input(keyboard);
            keyboard->state = Input_Keyboard_State_Inactive;
            if (editor->message == Level_Editor_Message_Load_Failed)  editor->message = Level_Editor_Message_None;
        }        
        else {                        
            char buffer[512];
            buffer[0] = '\0';
                    
            if (keyboard->state == Input_Keyboard_State_Inactive) {
                begin_keyboard_input(keyboard, Input_Keyboard_Mode_Integer);
            }
            else if (keyboard->state == Input_Keyboard_State_Receive) {
                _snprintf_s(buffer, kLevel_Name_Max_Length, _TRUNCATE, "%s", keyboard->buffer);
            }                    
            else if (keyboard->state == Input_Keyboard_State_Done) {
                u32 constexpr file_to_load_size = kLevel_Name_Max_Length + 11;
                char file_to_load[file_to_load_size];
                _snprintf_s(file_to_load, file_to_load_size, _TRUNCATE, "%s.level_txt", keyboard->buffer);
                        
                b32 load_result = load_level(&editor->level, editor->level.resources, file_to_load);
                if (load_result) {
                    editor->level_has_unsaved_changes = false;
                    editor->state = Level_Editor_State_Menu;
                    editor->message = Level_Editor_Message_None;
                    create_maps_off_level(level);
                }
                else {
                    editor->message = Level_Editor_Message_Load_Failed;
                }

                cancel_keyboard_input(keyboard);
                keyboard->state = Input_Keyboard_State_Inactive;
            }                       

            char const *caption = "Load ID: ";
            v2u caption_dim = get_text_dim(font, caption);
            v2u name_dim = get_text_dim(font, buffer);
                    
            v2u Pt = V2u(1, 2) * cell_size;

            if (keyboard->state == Input_Keyboard_State_Receive) {
                u32 cursor_x = get_cursor_position_in_pixels(keyboard, font, nullptr);
                renderer->draw_filled_rectangle(Pt + V2u(caption_dim.x + cursor_x, 0) - V2u(0, 5), 12, 24, v4u8_red);
            }

            v4u8 colour = v4u8_yellow;
            renderer->print(font, Pt, caption, colour);
            renderer->print(font, Pt + V2u(caption_dim.x, 0), buffer, colour);
        }
    }

    
    //
    // Print "Editor" in the bottom-right corner to indicate that we're in the editor now
    {
        char const *text = "Editor";
        v2u text_dim = get_text_dim(font, text);
        renderer->print(font, V2u(renderer->get_backbuffer_width() - text_dim.x - 10, 10), text);
    }
}
