//
// Renderer back-end, software
//

#include "renderer_frontend.h"


struct Renderer_Software_win32 : public Renderer {
    //
    // Methods
    Renderer_Software_win32() {};
    ~Renderer_Software_win32();
    b32 init(Log *log, u32 width, u32 height) final override;
    b32 init_win32(HWND hwnd, Log *log, u32 width, u32 height);

    void clear(v4u8 clear_colour) final override;

    void draw_filled_rectangle(v2u P, u32 w, u32 h, v4u8 colour) final override;
    void draw_rectangle_outline(v2u P, u32 w, u32 h, v4u8 colour) final override;

    //void create_bitmap();
    void draw_bitmap(v2u P, Bmp *bitmap) final override;
    void draw_bitmap(v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1) final override;
    void draw_coloured_bitmap(v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1, v4u8 colour) final override;
    
    v2u print(Font *font, v2u Po, char const *text, v4u8 colour_v4u8 = v4u8_white) final override;

    void draw_to_screen();

    u32 get_backbuffer_width()  final override {return backbuffer_width;}
    u32 get_backbuffer_height() final override {return backbuffer_height;}

    //
    // Members
    v4u8  *backbuffer_memory = nullptr;        
    HBITMAP backbuffer_bitmap = nullptr;
    HDC     backbuffer_hdc    = nullptr;
    u32     backbuffer_width = 0;
    u32     backbuffer_height = 0;
    HWND hwnd = nullptr;
    Log *log = nullptr;
};




//
// #_Initialization and destructor
void Renderer_Software_win32_fini(Renderer_Software_win32 *renderer) {
    if (renderer->backbuffer_bitmap) {
        DeleteObject(renderer->backbuffer_bitmap); // Renderer will free the memory used for the pixel data
    }

    if (renderer->backbuffer_hdc) {
        ReleaseDC(renderer->hwnd, renderer->backbuffer_hdc);
    }
}


b32 Renderer_Software_win32::init(Log *_log, u32 width, u32 height) {
    b32 result = true;

    this->log = _log;
    this->backbuffer_width = width;
    this->backbuffer_height = height;
        
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
        
    this->backbuffer_bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
                                                       reinterpret_cast<void **>(&this->backbuffer_memory), nullptr, 0);
    if (result && !this->backbuffer_bitmap) {
        u32 error = GetLastError();                
        LOG_ERROR(log, "failed to create the backbuffer", error);
        result = false;
    }

    this->backbuffer_hdc = CreateCompatibleDC(hdc);
    if (result && !this->backbuffer_hdc) {
        u32 error = GetLastError();
        LOG_ERROR(log, "failed to create the backbuffer's hdc", error);
        result = false;        
    }

    if (result) {
        SelectObject(this->backbuffer_hdc, this->backbuffer_bitmap);
    }
   else {
        Renderer_Software_win32_fini(this);
    }

    if (hdc) {
        ReleaseDC(hwnd, hdc);
    }

    return result;
}


b32 Renderer_Software_win32::init_win32(HWND _hwnd, Log *_log, u32 width, u32 height) {
    this->hwnd = _hwnd;
    b32 result = init(_log, width, height);
    return result;
}


Renderer_Software_win32::~Renderer_Software_win32() {
    Renderer_Software_win32_fini(this);
}




//
// #_Rendering
//

void Renderer_Software_win32::clear(v4u8 clear_colour) {
    if (this->backbuffer_memory) {
        for (u32 Index = 0; Index < (this->backbuffer_width * this->backbuffer_height); ++Index) {
            this->backbuffer_memory[Index] = clear_colour;
        }
    }
}


void Renderer_Software_win32::draw_filled_rectangle(v2u P, u32 w, u32 h, v4u8 colour) {
    for (u32 y = P.y; y < (P.y + h); ++y) {
        for (u32 x = P.x; x < (P.x + w); ++x) {
            u32 *dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[(y * this->backbuffer_width) + x]);
            *dst = fp_lerp_non_premul_src(*dst, colour._u32);
        }
    }
}


void Renderer_Software_win32::draw_rectangle_outline(v2u P, u32 w, u32 h, v4u8 colour) {
    u32 width = this->backbuffer_width;
    
    for (u32 x = P.x; x < (P.x + w); ++x) {
        u32 *dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[(P.y * width) + x]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
        
        dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[((P.y + h - 1) * width) + x]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
    }
    
    for (u32 y = P.y; y < (P.y + h); ++y) {
        u32 *dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[(y * width) + P.x]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
        
        dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[(y * width) + (P.x + w - 1)]);
        *dst = fp_lerp_non_premul_src(*dst, colour._u32);
    }
}


// we assume that bitmap is premultiplied with its alpha
// P location to draw at
// (x0, y0) starting point in source bitmap
// (x1, y1) ending point in source bitmap
//      [x0, ..., x1[  and [y0, ..., y1[  (that is, x1 and y1 is excluded from the range)
void Renderer_Software_win32::draw_bitmap(v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1) {
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
                if (dst_x < this->backbuffer_width && dst_y < this->backbuffer_height) { 
                    index = (this->backbuffer_width * dst_y) + dst_x;
                    u32 *dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[index]);
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
void Renderer_Software_win32::draw_coloured_bitmap(v2u P, Bmp *bitmap, u32 x0, u32 y0, u32 x1, u32 y1, v4u8 colour) {
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
                if (dst_x < this->backbuffer_width && dst_y < this->backbuffer_height) { 
                    index = (this->backbuffer_width * dst_y) + dst_x;
                    u32 *dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[index]);
                    src = fp_mul_non_premul_src(src, colour._u32);
                    *dst = fp_lerp_premul(*dst, src);
                    //*dst = fp_mul_non_premul_src(*dst, colour._u32);
                }
            }
        }
    }
}


// we assume that bitmap is premultiplied with its alpha
void Renderer_Software_win32::draw_bitmap(v2u P, Bmp *bitmap) {
    u32 width = this->backbuffer_width;
    u32 height = this->backbuffer_height;
    
    u32 left_x = P.x <= width  ? width  - P.x : 0;
    u32 left_y = P.y <= height ? height - P.y : 0;
    
    u32 stop_x = min(static_cast<u32>(bitmap->header.width) , left_x);
    u32 stop_y = min(static_cast<u32>(bitmap->header.height), left_y);

    //
    // Using fixed-point
    for (u32 y = 0; y < stop_y; ++y) {
        for (u32 x = 0; x < stop_x; ++x) {
            u32 index = (width * (P.y + y)) + P.x + x;
            u32 *dst = reinterpret_cast<u32 *>(&this->backbuffer_memory[index]);
            
            index = (bitmap->header.width * y) + x;
            u32 *src = reinterpret_cast<u32 *>(bitmap->data) + index;
            *dst = fp_lerp_premul(*dst, *src);
        }
    }
}


v2u Renderer_Software_win32::print(Font *font, v2u Po, char const *text, v4u8 colour_v4u8) {
    v2u P = Po;    
    for (char const *ptr = text; *ptr; ++ptr) {        
        u32 char_index = *ptr - 32;
        if (char_index < font->char_count) {
            Char_Data *c = &font->char_data[char_index];
            u32 bx = c->x;
            u32 by = font->bitmap.header.height - c->y - c->height;
            u32 x = P.x + c->offset_x;
            u32 y = P.y + (font->base - c->offset_y - c->height);
            //draw_coloured_bitmap(this, V2u(x, y), &font->bitmap, bx, by, bx + c->width, by + c->height, colour_v4u8);
            this->draw_coloured_bitmap(V2u(x, y), &font->bitmap, bx, by, bx + c->width, by + c->height, colour_v4u8);
            P.x += c->advance_x;
        }
    }

    return P;
}


void Renderer_Software_win32::draw_to_screen() {
    RECT client_rect;
    GetClientRect(this->hwnd, &client_rect);
    s32 window_w = client_rect.right - client_rect.left;
    s32 window_h = client_rect.bottom - client_rect.top;
    
    s32 dst_w = window_w;
    s32 dst_h = window_h;

    s32 src_w = this->backbuffer_width;
    s32 src_h = this->backbuffer_height;

    //
    // "float-space"
    f32 aspect_ratio = static_cast<f32>(src_w) / static_cast<f32>(src_h);
    f32 w0 = static_cast<f32>(dst_w);
    f32 h0 = static_cast<f32>(dst_h);
    f32 w = w0;
    f32 h = h0;

    h = w / aspect_ratio;
    if (h > h0) {
        h = h0;
        w = aspect_ratio * h;
    }

    dst_w = static_cast<s32>(w);
    dst_h = static_cast<s32>(h);
                    
    //
    // "s32-space"
    s32 dx = (window_w - dst_w) / 2;
    s32 dy = (window_h - dst_h) / 2;

    HDC hdc = GetDC(hwnd);
#if 1
    b32 result = StretchBlt(hdc,
                            dx, dy, dst_w, dst_h,
                            this->backbuffer_hdc,
                            0, 0, src_w, src_h,
                            SRCCOPY);
#else
    b32 result = BitBlt(hdc, 0, 0, renderer->backbuffer_width, renderer->backbuffer_height,
                        renderer->backbuffer_hdc, 0, 0, SRCCOPY);
    result;
#endif
                    
    result;
    ReleaseDC(hwnd, hdc);
}
