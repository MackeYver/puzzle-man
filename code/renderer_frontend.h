//
// Front-end for the rendering
//


struct Renderer {
    virtual b32 init(Log *log, u32 width, u32 height) = 0;

    virtual void clear(v4u8 clear_colour) = 0;

    virtual void draw_filled_rectangle(v2u P, u32 w, u32 h, v4u8 colour) = 0;
    virtual void draw_rectangle_outline(v2u P, u32 w, u32 h, v4u8 colour) = 0;
    
    virtual void draw_bitmap(v2u P, Bmp *bitmap) = 0;
    virtual void draw_bitmap(v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1) = 0;
    virtual void draw_coloured_bitmap(v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1, v4u8 colour) = 0;

    virtual v2u print(Font *font, v2u Po, char const *text, v4u8 colour_v4u8 = v4u8_white) = 0;

    virtual void draw_to_screen() = 0;

    virtual u32 get_backbuffer_width() = 0;
    virtual u32 get_backbuffer_height() = 0;
};
 
