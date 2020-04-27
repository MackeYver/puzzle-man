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


static void edit_level(Level_Editor *editor, Render_State *render_state, Input input, u32 microseconds_since_start) {
    Level *level = &editor->level;
    Font *font = &level->resources->font;
    Mouse_Input *mouse = &editor->mouse;

    
    //
    // TODO: handle window and cell sizes properly!
    u32 cell_size = render_state->backbuffer_width / level->width;
    assert(cell_size == cell_size);


    
    //
    // Print in the right corner to indicate that we're in the editor now
    {
        char const *text = "Editor";
        v2u text_dim = get_text_dim(font, text);
        print(render_state, font, V2u(render_state->backbuffer_width - text_dim.x - 10, 10), text);
    }


    
    //
    // Open/close the menu
    if (input == Input_Space) {
        editor->state = editor->state == Level_Editor_State_Editing ? Level_Editor_State_Menu : Level_Editor_State_Editing;
    }


    
    //
    // Editing
    //
    if (editor->state == Level_Editor_State_Editing) {
        b32 changed_in_this_frame = false;
        editor->hot_tile = nullptr;   
        
        if (mouse->is_inside) {
            v2u Pmc = V2u(static_cast<u32>(floorf(mouse->position.x / cell_size)),
                          static_cast<u32>(floorf(mouse->position.y / cell_size)));

            if (Pmc.x < level->width && Pmc.y < level->height) {
                v2u P = V2u(cell_size * Pmc.x, cell_size * Pmc.y);

                Tile *tile = get_tile_at(&editor->level, Pmc);
                editor->hot_tile = tile;

                draw_rectangle_outline(render_state, P, static_cast<u32>(cell_size), static_cast<u32>(cell_size), v4u8_yellow);
            }

            if (editor->hot_tile) {
                if (editor->mouse.left_button.curr == Mouse_Button_Pressed) {
                    if (editor->selected_objects[0].type == Object_Type_Tile) {
                        editor->hot_tile->type = static_cast<Tile_Type>(editor->selected_objects[0].value);
                    }
                    else if (editor->selected_objects[0].type == Object_Type_Item) {
                        Item_Type item_type = static_cast<Item_Type>(editor->selected_objects[0].value);
                        editor->hot_tile->item.type = item_type;
                        if (item_type == Item_Type_Dot_Small)  editor->level.current_state->score += kDot_Small_Value;
                        if (item_type == Item_Type_Dot_Large)  editor->level.current_state->score += kDot_Large_Value;
                    }
                    changed_in_this_frame = true;                
                }
                else if (editor->mouse.right_button.curr == Mouse_Button_Pressed) {
                    if (editor->selected_objects[1].type == Object_Type_Tile) {
                        editor->hot_tile->type = static_cast<Tile_Type>(editor->selected_objects[1].value);
                    }
                    else if (editor->selected_objects[1].type == Object_Type_Item) {
                        Item_Type item_type = static_cast<Item_Type>(editor->selected_objects[1].value);
                        editor->hot_tile->item.type = item_type;
                        if (item_type == Item_Type_Dot_Small)  editor->level.current_state->score += kDot_Small_Value;
                        if (item_type == Item_Type_Dot_Large)  editor->level.current_state->score += kDot_Large_Value;
                    }
                    changed_in_this_frame = true;
                }
            }
        }


        if (changed_in_this_frame) {        
            adjust_walls_in_level(level, level->current_state, level->resources);
            create_maps_off_level(&editor->level);
        }

    
        //
        // Render
        draw_level(render_state, level, editor->render_mode, microseconds_since_start);
        draw_maps(render_state, level, level->maps, level->current_map_index);
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
        Tile_Type  constexpr tiles[] = {Tile_Type_Wall_0, Tile_Type_Floor, Tile_Type_None};
        Item_Type  constexpr items[] = {Item_Type_Dot_Small, Item_Type_Dot_Large};
        //Actor_Type constexpr actor[] = {};

        char const *text = "Tiles and items";
        v2u text_dim = get_text_dim(font, text);
        print(render_state, font, V2u(cell_size, render_state->backbuffer_height - ((cell_size + text_dim.y) / 2)), text);


        //
        // Draw palette of tiles/items and actors
        v2u Pc = V2u(1, level->height - 2);        
        Tile tile;

        u32 tile_count = Array_Count(tiles);
        u32 item_count = Array_Count(items);
        v4u8 colour = v4u8_white;
        
        for (u32 index = 0; index < max(tile_count, item_count); ++index) {
            if (index < tile_count) {
                v2u P = V2u(cell_size * Pc.x, cell_size * Pc.y);
                tile           = empty_tile_of_type(tiles[index]);                
                draw_tile(render_state, editor->level.resources, &tile, P);

                if (mouse_is_inside && Pmc.x == (index + 1) && Pmc.y == level->height - 2) {
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
                draw_rectangle_outline(render_state, P, cell_size, cell_size, colour);
            }

            if (index < item_count) {
                v2u P = V2u(cell_size * Pc.x, cell_size * (Pc.y - 1));
                tile           = empty_tile_of_type(Tile_Type_None);
                tile.item.type = items[index];
                draw_item(render_state, editor->level.resources, &tile, P);
                
                if (mouse_is_inside && Pmc.x == (index + 1) && Pmc.y == level->height - 3) {
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
                draw_rectangle_outline(render_state, P, cell_size, cell_size, colour);
            }            

            ++Pc.x;
            if (Pc.x >= level->width) {
                Pc.x = 0;
                ++Pc.y;
            }            
        }
    }
}
 
