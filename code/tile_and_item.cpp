//
// tile_and_item.cpp
// (c) Marcus Larsson



//
// Item
//

enum Item_Type {
    Item_Type_None,
    Item_Type_Dot_Small,
    Item_Type_Dot_Large,

    Item_Type_Count,
};

struct Item {
    Item_Type type = Item_Type_None;
    u32 value = 0;
};




//
// Tile, declaration
//

enum Tile_Type {
    Tile_Type_Wall_0 = 0,
    Tile_Type_Wall_1,
    Tile_Type_Wall_2,
    Tile_Type_Wall_3,
    Tile_Type_Wall_4,
    Tile_Type_Wall_5,
    Tile_Type_Wall_6,
    Tile_Type_Wall_7,
    Tile_Type_Wall_8,
    Tile_Type_Wall_9,
    Tile_Type_Wall_10,
    Tile_Type_Wall_11,
    Tile_Type_Wall_12,
    Tile_Type_Wall_13,
    Tile_Type_Wall_14,
    Tile_Type_Wall_15,

    Tile_Type_None,  
    Tile_Type_Floor,
    
    Tile_Type_Count,
};


struct Tile {
    Item item;
    Actor_ID actor_id = {0xFFFF, 0xFFFF};
    Tile_Type type = Tile_Type_None;
};




//
// Implementation
//

b32 tile_has_wall(Tile *tile) {
    b32 result = false;

    if (tile->type >= Tile_Type_Wall_0 && tile->type <= Tile_Type_Wall_15) {
        result = true;
    }

    return result;
}


b32 tile_is_traversable(Tile *tile) {
    b32 result = tile->type == Tile_Type_Floor;
    return result;    
}


void draw_tile(Render_State *render_state, Resources *resources, Tile *tile, v2u P) {
    if (tile) {
        if (tile_has_wall(tile)) {
            draw_bitmap(render_state, P, &resources->bitmaps.walls[tile->type]);
        }

        if (tile->item.type == Item_Type_Dot_Small) {
            draw_bitmap(render_state, P, &resources->bitmaps.dot_small);
        }
        else if (tile->item.type == Item_Type_Dot_Large) {
            draw_bitmap(render_state, P, &resources->bitmaps.dot_large);
        }
    }
}
