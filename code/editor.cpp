//
// Level editor
//




//
// Declarations
//

//
// Mouse
enum Mouse_Button_State {
    Mouse_Button_Up = 0,
    Mouse_Button_Pressed,
    Mouse_Button_Down,
    Mouse_Button_Released,
};

struct Mouse_Button {
    Mouse_Button_State prev;
    Mouse_Button_State curr;
};

struct Input_Mouse {
    b32 is_inside = false;
    v2 position;

    union {
        struct {
            Mouse_Button left_button;
            Mouse_Button right_button;
        };
        Mouse_Button buttons[2];
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
};

struct Level_Editor {
    Level level;
    Editor_Input input;
    Tile *hot_tile = nullptr;
    Selected_Object selected_objects[2];
    u32 render_mode;
    Level_Editor_State state;    
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

static void fini_editor(Level_Editor *editor) {
    fini_level(&editor->level);
}

static void begin_editing(Level_Editor *editor, Level *level) {
    editor->state = Level_Editor_State_Editing;
    editor->render_mode = Level_Render_Mode_All;
    editor->selected_objects[0].type  = Object_Type_Tile;
    editor->selected_objects[0].value = Tile_Type_Wall_0;
    editor->selected_objects[1].type  = Object_Type_Tile;
    editor->selected_objects[1].value = Tile_Type_Floor;
    
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
        tile->item.type = Item_Type_None;
    }
    else if (tile->item.type == Item_Type_Dot_Small) {
        state->score -= kDot_Large_Value;
        tile->item.type = Item_Type_None;
    }
    
    if (tile->actor_id.index < 0xFFF) {
        Actor *actor = get_actor(&state->actors, tile->actor_id);
        if (actor_is_ghost(actor)) {
            state->score -= kGhost_Value;
        }

        delete_actor(&state->actors, tile->actor_id);        
        tile->actor_id = kActor_ID_Null;        
    }
}


static void validate_level(Level_State *state) {
    
}


char get_char_from_actor_type(Actor *actor) {
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
        }
    }
    
    return result;
}


char get_char_from_tile_type(Tile *tile) {
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

    HANDLE file_handle;
    result = win32_open_file_for_writing("data//levels//save_test.level_txt", &file_handle);
    if (result) {
        DWORD bytes_written;
        char buffer[512];
        DWORD bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "Name:%s\n", level->name);
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "Width:%u\n", level->width);
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "Height:%u\n", level->width);
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);

        Level_State *state = level->current_state;

        //
        // Tiles        
        bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "Layer_tiles:\n");
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);        
        
        for (u32 y = 0; y < level->height; ++y) {
            for (u32 x = 0; x < level->width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "%c", get_char_from_tile_type(tile));
                result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
                assert(bytes_to_write == bytes_written);
            }
            WriteFile(file_handle, "\n", 1, &bytes_written, nullptr);
        }

        //
        // Items
        bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "Layer_items:\n");
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);
        
        for (u32 y = 0; y < level->height; ++y) {
            for (u32 x = 0; x < level->width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "%c", get_char_from_item_type(tile));
                result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
                assert(bytes_to_write == bytes_written);
            }
            WriteFile(file_handle, "\n", 1, &bytes_written, nullptr);
        }

        //
        // Actors
        bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "Layer_actors:\n");
        result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
        assert(bytes_to_write == bytes_written);
        
        for (u32 y = 0; y < level->height; ++y) {
            for (u32 x = 0; x < level->width; ++x) {
                Tile *tile = get_tile_at(level, x, y);
                Actor *actor = get_actor(&state->actors, tile->actor_id);
                bytes_to_write = _snprintf_s(buffer, 512, _TRUNCATE, "%c", get_char_from_actor_type(actor));
                result = WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, nullptr);
                assert(bytes_to_write == bytes_written);
            }
            WriteFile(file_handle, "\n", 1, &bytes_written, nullptr);
        }
        
        CloseHandle(file_handle);
    }

    return result;
}


static void edit_level(Level_Editor *editor, Renderer *renderer, Input input, u32 microseconds_since_start) {
    Level *level = &editor->level;
    Font *font = &level->resources->font;
    Input_Mouse *mouse = &editor->input.mouse;

    
    //
    // TODO: handle window and cell sizes properly!
    u32 cell_size = renderer->get_backbuffer_width() / level->width;
    assert(cell_size == cell_size);
 
   
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
        
        if (mouse->is_inside) {
            v2u Pmc = V2u(static_cast<u32>(floorf(mouse->position.x / cell_size)),
                          static_cast<u32>(floorf(mouse->position.y / cell_size)));

            if (Pmc.x < level->width && Pmc.y < level->height) {
                v2u P = V2u(cell_size * Pmc.x, cell_size * Pmc.y);

                Tile *tile = get_tile_at(&editor->level, Pmc);
                editor->hot_tile = tile;

                renderer->draw_rectangle_outline(P, static_cast<u32>(cell_size), static_cast<u32>(cell_size), v4u8_yellow);
            }

            if (editor->hot_tile) {
                if (mouse->left_button.curr == Mouse_Button_Pressed) {
                    if (editor->selected_objects[0].type == Object_Type_Tile) {
                        clear_tile(editor->level.current_state, editor->hot_tile);
                        editor->hot_tile->type = static_cast<Tile_Type>(editor->selected_objects[0].value);
                    }
                    else if (editor->selected_objects[0].type == Object_Type_Item) {
                        Item_Type item_type = static_cast<Item_Type>(editor->selected_objects[0].value);
                        clear_tile(editor->level.current_state, editor->hot_tile);
                        
                        editor->hot_tile->item.type = item_type;
                        
                        if (item_type == Item_Type_Dot_Small)  editor->level.current_state->score += kDot_Small_Value;
                        if (item_type == Item_Type_Dot_Large)  editor->level.current_state->score += kDot_Large_Value;
                    }
                    else if (editor->selected_objects[0].type == Object_Type_Actor) {
                        Actor_Type type = static_cast<Actor_Type>(editor->selected_objects[0].value);
                        clear_tile(editor->level.current_state, editor->hot_tile);                        
                        add_actor(nullptr, &editor->level, editor->level.current_state, Pmc.x, Pmc.y, type, false);
                    }
                    changed_in_this_frame = true;
                }
                else if (mouse->right_button.curr == Mouse_Button_Pressed) {
                    if (editor->selected_objects[1].type == Object_Type_Tile) {
                        clear_tile(editor->level.current_state, editor->hot_tile);
                        editor->hot_tile->type = static_cast<Tile_Type>(editor->selected_objects[1].value);
                    }
                    else if (editor->selected_objects[1].type == Object_Type_Item) {
                        Item_Type item_type = static_cast<Item_Type>(editor->selected_objects[1].value);
                        clear_tile(editor->level.current_state, editor->hot_tile);
                        
                        editor->hot_tile->item.type = item_type;
                        
                        if (item_type == Item_Type_Dot_Small)  editor->level.current_state->score += kDot_Small_Value;
                        if (item_type == Item_Type_Dot_Large)  editor->level.current_state->score += kDot_Large_Value;
                    }
                    else if (editor->selected_objects[1].type == Object_Type_Actor) {
                        Actor_Type type = static_cast<Actor_Type>(editor->selected_objects[1].value);
                        clear_tile(editor->level.current_state, editor->hot_tile);                        
                        add_actor(nullptr, &editor->level, editor->level.current_state, Pmc.x, Pmc.y, type, false);
                    }
                    changed_in_this_frame = true;
                }
            }
        }


        if (changed_in_this_frame) {        
            adjust_walls_in_level(level, level->current_state, level->resources);
            create_maps_off_level(&editor->level);
        }  
    }



    //
    // Menu
    //
    else if (editor->state == Level_Editor_State_Menu) {
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

        
        //
        // Print name
        {
            char buffer[512];
            u32 static debug_counter = 0;
            v4u8 text_colour = colour;

            if ((mouse->left_button.curr == Mouse_Button_Pressed) && mouse_is_inside) {
                if ((Pmc.y >= Pc.y) && (editor->input.keyboard.state != Input_Keyboard_State_Receive)) {
                    begin_keyboard_input(&editor->input.keyboard, level->name);
                }
                else if (editor->input.keyboard.state == Input_Keyboard_State_Receive) {
                    cancel_keyboard_input(&editor->input.keyboard);
                }
            }

            if (editor->input.keyboard.state == Input_Keyboard_State_Receive) {
                _snprintf_s(buffer, kLevel_Name_Max_Length, _TRUNCATE, "%s", editor->input.keyboard.buffer);
                text_colour = v4u8_yellow;
            }
            else if (editor->input.keyboard.state == Input_Keyboard_State_Done) {
                _snprintf_s(editor->level.name, kLevel_Name_Max_Length, _TRUNCATE, "%s", editor->input.keyboard.buffer);
                cancel_keyboard_input(&editor->input.keyboard);
            }   
            else {
                _snprintf_s(buffer, 512, _TRUNCATE, "%s", editor->level.name);
            }

            char const *caption = "Name: ";
            v2u caption_dim = get_text_dim(font, caption);
            v2u name_dim = get_text_dim(font, buffer);

            v2u Pt = Pc * cell_size;

            if (editor->input.keyboard.state == Input_Keyboard_State_Receive) {
                u32 cursor_x = get_cursor_position_in_pixels(&editor->input.keyboard, font, nullptr);
                renderer->draw_filled_rectangle(Pt + V2u(caption_dim.x + cursor_x, 0) - V2u(0, 5), 12, 24, v4u8_red);
            }
            
            renderer->print(font, Pt, "Name: ", colour);
            renderer->print(font, Pt + V2u(caption_dim.x, 0), buffer, text_colour);            
            
            Pc.y -= 2;
            
            ++debug_counter;
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

                if (mouse_is_inside && Pmc.x == Pc.x && Pmc.y == Pc.y) {
                    colour = v4u8_yellow;

                    if (mouse->left_button.curr == Mouse_Button_Down) {
                        editor->selected_objects[0].type = Object_Type_Tile;
                        editor->selected_objects[0].value = tiles[index];
                    }
                    if (mouse->right_button.curr == Mouse_Button_Down) {
                        editor->selected_objects[1].type = Object_Type_Tile;
                        editor->selected_objects[1].value = tiles[index];
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
                
                if (mouse_is_inside && Pmc.x == Pc.x && Pmc.y == Pc.y) {
                    colour = v4u8_yellow;
                    
                    if (mouse->left_button.curr == Mouse_Button_Down) {
                        editor->selected_objects[0].type = Object_Type_Item;
                        editor->selected_objects[0].value = items[index];
                    }
                    if (mouse->right_button.curr == Mouse_Button_Down) {
                        editor->selected_objects[1].type = Object_Type_Item;
                        editor->selected_objects[1].value = items[index];
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
                
                if (mouse_is_inside && Pmc.x == Pc.x && Pmc.y == Pc.y) {
                    colour = v4u8_yellow;
                    
                    if (mouse->left_button.curr == Mouse_Button_Down) {
                        editor->selected_objects[0].type = Object_Type_Actor;
                        editor->selected_objects[0].value = actors[index];
                    }
                    if (mouse->right_button.curr == Mouse_Button_Down) {
                        editor->selected_objects[1].type = Object_Type_Actor;
                        editor->selected_objects[1].value = actors[index];
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
            char const *text[3] = {"New", "Save", "Load"};
            
            for (u32 index = 0; index < 3; ++index) {
                v2u text_dim = get_text_dim(font, text[index]);
                Pc = V2u(2 * index + 1, 1);
                v2u P = V2u(cell_size * Pc.x, cell_size * Pc.y);
                v2u offset = V2u((cell_size - text_dim.x) / 2, (cell_size - text_dim.y) / 2);
                colour = v4u8_white;

                if (mouse_is_inside && Pmc.x == Pc.x && Pmc.y == Pc.y) {
                    colour = v4u8_yellow;
                
                    if (mouse->left_button.curr == Mouse_Button_Down) {
                        if (index == 0) {
                        }
                        else if (index == 1) {
                            save_level(level);
                        }
                        if (index == 2) {
                        }

                    }
                
                    renderer->draw_rectangle_outline(P, cell_size, cell_size, colour);
                }

                renderer->print(font, P + offset + V2u(1, 1), text[index], V4u8(125, 125, 125, 255));
                renderer->print(font, P + offset, text[index], colour);
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
}
