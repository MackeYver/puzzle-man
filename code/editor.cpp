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

struct Mouse_Input {
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
    Mouse_Input mouse;
    Tile *hot_tile = nullptr;
    Selected_Object selected_objects[2];
    u32 render_mode;
    Level_Editor_State state;    
};




//
// Implementations
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
// Utility functions for editing
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




//
// Editing
//


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
    Mouse_Input *mouse = &editor->mouse;

    
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
                if (editor->mouse.left_button.curr == Mouse_Button_Pressed) {
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
                else if (editor->mouse.right_button.curr == Mouse_Button_Pressed) {
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
        // Draw tiles
        {
            char const *text = "Tiles";
            v2u text_dim = get_text_dim(font, text);
            u32 y = (Pc.y * cell_size) + ((cell_size - text_dim.y) / 2);
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
