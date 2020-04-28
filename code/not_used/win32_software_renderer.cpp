//
// Renderer
// (c) Marcus Larsson
//


struct Render_State {
    v4u8  *backbuffer_memory = nullptr;        
    HBITMAP backbuffer_bitmap = nullptr;
    HDC     backbuffer_hdc    = nullptr;
    u32     backbuffer_width = 0;
    u32     backbuffer_height = 0;
    HWND hwnd = nullptr;
    Log *log = nullptr;
};


void fini_renderer(Render_State *render_state) {
    if (render_state->backbuffer_bitmap) {
        DeleteObject(render_state->backbuffer_bitmap); // This will free the memory used for the pixel data
    }

    if (render_state->backbuffer_hdc) {
        ReleaseDC(render_state->hwnd, render_state->backbuffer_hdc);
    }
}


b32 init_renderer(Render_State *render_state, Log *log, HWND hwnd, u32 width, u32 height) {
    b32 result = true;

    render_state->log = log;
    render_state->backbuffer_width = width;
    render_state->backbuffer_height = height;
    render_state->hwnd = hwnd;
        
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;    
    bmi.bmiHeader.biSize = sizeof(bmi);

    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        u32 error = GetLastError();        
        LOG_ERROR(log, "failed to get the window's hdc", error);
        ReleaseDC(hwnd, hdc);
        result = false;
    }
        
    render_state->backbuffer_bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
                                                       reinterpret_cast<void **>(&render_state->backbuffer_memory), nullptr, 0);
    if (result && !render_state->backbuffer_bitmap) {
        u32 error = GetLastError();                
        LOG_ERROR(log, "failed to create the backbuffer", error);
        result = false;
    }

    render_state->backbuffer_hdc = CreateCompatibleDC(hdc);
    if (result && !render_state->backbuffer_hdc) {
        u32 error = GetLastError();
        LOG_ERROR(log, "failed to create the backbuffer's hdc", error);
        result = false;        
    }

    if (result) {
        SelectObject(render_state->backbuffer_hdc, render_state->backbuffer_bitmap);
    }
   else {
        fini_renderer(render_state);
    }

    if (hdc) {
        ReleaseDC(hwnd, hdc);
    }

    return result;
}


void clear_backbuffer(Render_State *render_state, v4u8 clear_colour = {0, 0, 0, 255}) {
    if (render_state && render_state->backbuffer_memory) {
        for (u32 Index = 0; Index < (render_state->backbuffer_width * render_state->backbuffer_height); ++Index) {
            render_state->backbuffer_memory[Index] = clear_colour;
        }
    }
}


void draw_filled_rectangle(Render_State *render_state, v2u P, u32 w, u32 h, v4u8 colour) {
    for (u32 y = P.y; y < (P.y + h); ++y) {
        for (u32 x = P.x; x < (P.x + w); ++x) {
            u32 *dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[(y * render_state->backbuffer_width) + x]);
            *dst = fp_lerp_non_premul_src(*dst, colour._u32);
        }
    }
}


void draw_rectangle_outline(Render_State *render_state, v2u P, u32 w, u32 h, v4u8 colour) {
    u32 width = render_state->backbuffer_width;
    
    for (u32 x = P.x; x < (P.x + w); ++x) {
        u32 *dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[(P.y * width) + x]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
        
        dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[((P.y + h - 1) * width) + x]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
    }
    
    for (u32 y = P.y; y < (P.y + h); ++y) {
        u32 *dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[(y * width) + P.x]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
        
        dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[(y * width) + (P.x + w - 1)]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
    }
}


// we assume that bitmap is premultiplied with its alpha
// P location to draw at
// (x0, y0) starting point in source bitmap
// (x1, y1) ending point in source bitmap
//      [x0, ..., x1[  and [y0, ..., y1[  (that is, x1 and y1 is excluded from the range)
void draw_bitmap(Render_State *render_state, v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1) {
    if (x0 < x1 && y0 < y1) {
        u32 stop_y = min(static_cast<u32>(bitmap->header.height), y1 - y0);
        u32 stop_x = min(static_cast<u32>(bitmap->header.width) , x1 - x0);
        
        //
        // Using fixed-point, 0.8
        for (u32 y = 0; y < stop_y; ++y) {
            for (u32 x = 0; x < stop_x; ++x) {
                u32 index = (bitmap->header.width * (y0 + y)) + (x0 + x);
                u32 *src = reinterpret_cast<u32 *>(bitmap->data) + index;

                u32 dst_x = P.x + x;
                u32 dst_y = P.y + y;
                if (dst_x < render_state->backbuffer_width && dst_y < render_state->backbuffer_height) { 
                    index = (render_state->backbuffer_width * dst_y) + dst_x;
                    u32 *dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[index]);
                    *dst = fp_lerp_premul(*dst, *src);

                }
            }
        }
    }
}


// we assume that bitmap is premultiplied with its alpha
// P location to draw at
// (x0, y0) starting point in source bitmap
// (x1, y1) ending point in source bitmap
//      [x0, ..., x1[  and [y0, ..., y1[  (that is, x1 and y1 is excluded from the range)
void draw_coloured_bitmap(Render_State *render_state, v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1, v4u8 colour) {
    if (x0 < x1 && y0 < y1) {
        u32 stop_y = min(static_cast<u32>(bitmap->header.height), y1 - y0);
        u32 stop_x = min(static_cast<u32>(bitmap->header.width) , x1 - x0);
        
        //
        // Using fixed-point, 0.8
        for (u32 y = 0; y < stop_y; ++y) {
            for (u32 x = 0; x < stop_x; ++x) {
                u32 index = (bitmap->header.width * (y0 + y)) + (x0 + x);
                u32 src = *(reinterpret_cast<u32 *>(bitmap->data) + index);

                u32 dst_x = P.x + x;
                u32 dst_y = P.y + y;
                if (dst_x < render_state->backbuffer_width && dst_y < render_state->backbuffer_height) { 
                    index = (render_state->backbuffer_width * dst_y) + dst_x;
                    u32 *dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[index]);
                    src = fp_mul_non_premul_src(src, colour._u32);
                    *dst = fp_lerp_premul(*dst, src);
                    //*dst = fp_mul_non_premul_src(*dst, colour._u32);
                }
            }
        }
    }
}


// we assume that bitmap is premultiplied with its alpha
void draw_bitmap(Render_State *render_state, v2u P, Bmp *bitmap) {
    u32 width = render_state->backbuffer_width;
    u32 height = render_state->backbuffer_height;
    
    u32 left_x = P.x <= width  ? width  - P.x : 0;
    u32 left_y = P.y <= height ? height - P.y : 0;
    
    u32 stop_x = min(static_cast<u32>(bitmap->header.width) , left_x);
    u32 stop_y = min(static_cast<u32>(bitmap->header.height), left_y);

    //
    // Using fixed-point
    for (u32 y = 0; y < stop_y; ++y) {
        for (u32 x = 0; x < stop_x; ++x) {
            u32 index = (width * (P.y + y)) + P.x + x;
            u32 *dst = reinterpret_cast<u32 *>(&render_state->backbuffer_memory[index]);
            
            index = (bitmap->header.width * y) + x;
            u32 *src = reinterpret_cast<u32 *>(bitmap->data) + index;
            *dst = fp_lerp_premul(*dst, *src);
        }
    }
}

v2u print(Render_State *render_state, Font *font, v2u Po, char const *text, v4u8 colour_v4u8 = v4u8_white) {
    v2u P = Po;    
    for (char const *ptr = text; *ptr; ++ptr) {        
        u32 char_index = *ptr - 32;
        if (char_index < font->char_count) {
            Char_Data *c = &font->char_data[char_index];
            u32 bx = c->x;
            u32 by = font->bitmap.header.height - c->y - c->height;
            u32 x = P.x + c->offset_x;
            u32 y = P.y + (font->base - c->offset_y - c->height);
            draw_coloured_bitmap(render_state, V2u(x, y), &font->bitmap, bx, by, bx + c->width, by + c->height, colour_v4u8);
            P.x += c->advance_x;
        }
    }

    return P;
}
