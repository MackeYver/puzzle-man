//
// Mathematics
// (c) Marcus Larsson
//

#include <math.h>



//
// v2u
//

union v2u {
    struct {
       u32 x;
        u32 y;
    };
    u32 e[2];

    b32 operator == (v2u& B) {
        b32 result = (this->x == B.x) && (this->y == B.y);
        return result;
    }

    b32 operator != (v2u& B) {
        b32 result = (this->x != B.x) && (this->y != B.y);
        return result;
    }
};

v2u V2u(u32 x, u32 y) {
    v2u result = {x, y};
    return result;
}

v2u const v2u_00   = {0, 0};

v2u operator / (v2u A, u32 b) {
    v2u result = V2u(A.x / b, A.y / b);
    return result;
}

v2u operator - (v2u A, v2u B) {
    v2u result = V2u(A.x - B.x, A.y - B.y);
    return result;
}

v2u operator + (v2u A, v2u B) {
    v2u result = V2u(A.x + B.x, A.y + B.y);
    return result;
}




//
// v2s
//

union v2s {
    struct {
        s32 x;
        s32 y;
    };
    s32 e[2];

    v2s() {};

    v2s(v2u a) {
        this->x = static_cast<s32>(a.x);
        this->y = static_cast<s32>(a.y);
    }

    v2s(s32 _x, s32 _y) : x(_x), y(_y) {}

    b32 operator == (v2s& B) {
        b32 result = (this->x == B.x) && (this->y == B.y);
        return result;
    }

    b32 operator != (v2s& B) {
        b32 result = (this->x != B.x) && (this->y != B.y);
        return result;
    }
};

v2s operator + (v2s A, v2s B) {
    v2s result = V2u(A.x + B.x, A.y + B.y);
    return result;
}





//
// v3
//

union v3 {
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
    };
    f32 e[3];    
};

v3 V3(f32 x, f32 y, f32 z) {
    v3 result = {x, y, z};
    return result;
};

v3 operator * (v3 A, v3 B) {
    v3 result = {A.x * B.x, A.y * B.y, A.z * B.z};
    return result;
}

v3 operator * (f32 a, v3 B) {
    v3 result = {a * B.x, a * B.y, a * B.z};
    return result;
}

v3 operator * (v3 A, f32 b) {
    v3 result = b * A;
    return result;
}

v3 operator + (v3 A, v3 B) {
    v3 result = {A.x + B.x, A.y + B.y, A.z + B.z};
    return result;
}

v3 clamp_01(v3 A) {
    v3 result = {clamp_01(A.x), clamp_01(A.y), clamp_01(A.z)};
    return result;
}




//
// v4
//

union v4 {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;        
    };
    f32 e[4];

    v3 xyz() {
        v3 result = V3(x, y, z);
        return result;
    }
};

v4 V4(f32 x, f32 y, f32 z, f32 w) {
    v4 result = {x, y, z, w};
    return result;
};

v4 operator * (v4& A, v4& B) {
    v4 result = {A.x * B.x, A.y * B.y, A.z * B.z, A.w * B.w};
    return result;
}

v4 operator * (v4 const& A, v4& B) {
    v4 result = {A.x * B.x, A.y * B.y, A.z * B.z, A.w * B.w};
    return result;
}

v4 operator * (f32 a, v4& B) {
    v4 result = {a * B.x, a * B.y, a * B.z, a * B.w};
    return result;
}

v4 operator * (v4& A, f32 b) {
    v4 result = b * A;
    return result;
}

v4 operator + (v4 const& A, v4 const& B) {
    v4 result = {A.x + B.x, A.y + B.y, A.z + B.z, A.w + B.w};
    return result;
}

v4 clamp_01(v4 A) {
    v4 result = {clamp_01(A.x), clamp_01(A.y), clamp_01(A.z), clamp_01(A.w)};
    return result;
}




//
// v4u8
//

union v4u8 {
    struct {
        u8 blue;
        u8 green;
        u8 red;
        u8 alpha;
    };
    u32 _u32;
};


v4u8 const v4u8_red    = {  0,   0, 255, 255};
v4u8 const v4u8_green  = {  0, 255,   0, 255};
v4u8 const v4u8_blue   = {255,   0,   0, 255};
v4u8 const v4u8_white  = {255, 255, 255, 255};
v4u8 const v4u8_black  = {  0,   0,   0, 255};
v4u8 const v4u8_yellow = {  0, 255, 255, 255};



v4u8 V4u8(u8 blue, u8 green, u8 red, u8 alpha) {
    v4u8 result = {blue, green, red, alpha};
    return result;
}


f32 constexpr one_over_255 = 1.0f / 255.0f;

v4 v4_from_v4u8(v4u8 *p) {
    v4 result = V4(static_cast<f32>(p->red)   * one_over_255,
                   static_cast<f32>(p->green) * one_over_255,
                   static_cast<f32>(p->blue)  * one_over_255,
                   static_cast<f32>(p->alpha) * one_over_255);
    return result;
}


v4u8 v4u8_from_v4(v4 *A) {
    v4u8 result = {static_cast<u8>(255.0f * A->b),
                    static_cast<u8>(255.0f * A->g),
                    static_cast<u8>(255.0f * A->r),
                    static_cast<u8>(255.0f * A->w)};
    return result;
}


u32 fp_mul_non_premul_src(u32 dst, u32 src) {    
    u32 constexpr mask_b = 0x000000FF;
    u32 constexpr mask_g = 0x0000FF00;
    u32 constexpr mask_r = 0x00FF0000;
    u32 constexpr mask_a = 0xFF000000;

    u32 sa = (src & mask_a) >> 24;
    u32 da = (dst & mask_a) >> 24;

    u32 sb = ((src & mask_b)  * sa) >> 8;
    u32 sg = (((src & mask_g) * sa) >> 8) >>  8;
    u32 sr = (((src & mask_r) * sa) >> 8) >> 16;

    u32 db = (dst & mask_b);
    u32 dg = (dst & mask_g) >>  8;
    u32 dr = (dst & mask_r) >> 16;

    u32 b = (sb * db) >> 8;
    u32 g = (sg * dg) >> 8;
    u32 r = (sr * dr) >> 8;
    u32 a = (sa * da) >> 8;

    u32 result = (a << 24) | (r << 16) | (g << 8) | b;    
    return result;
}


u32 fp_lerp_non_premul_src(u32 dst, u32 src) {    
    u32 constexpr mask_b = 0x000000FF;
    u32 constexpr mask_g = 0x0000FF00;
    u32 constexpr mask_r = 0x00FF0000;
    u32 constexpr mask_a = 0xFF000000;

    u32 sa = (src & mask_a) >> 24;
    u32 da = (dst & mask_a) >> 24;
    u32 nsa = 255 - sa;

    u32 sb = ((src & mask_b)  * sa) >> 8;
    u32 sg = (((src & mask_g) * sa) >> 8) >>  8;
    u32 sr = (((src & mask_r) * sa) >> 8) >> 16;

    u32 db = (dst & mask_b);
    u32 dg = (dst & mask_g) >>  8;
    u32 dr = (dst & mask_r) >> 16;

    u32 b = sb + ((db * (nsa + 1)) >> 8);
    u32 g = sg + ((dg * (nsa + 1)) >> 8);
    u32 r = sr + ((dr * (nsa + 1)) >> 8);
    u32 a = sa + ((da * (nsa + 1)) >> 8);

    u32 result = (a << 24) | (r << 16) | (g << 8) | b;    
    return result;
}


u32 fp_lerp_premul(u32 dst, u32 src) {    
    u32 constexpr mask_b = 0x000000FF;
    u32 constexpr mask_g = 0x0000FF00;
    u32 constexpr mask_r = 0x00FF0000;
    u32 constexpr mask_a = 0xFF000000;

    u32 sa = (src & mask_a) >> 24;
    u32 da = (dst & mask_a) >> 24;
    u32 nsa = 255 - sa;

    u32 sb = (src & mask_b);
    u32 sg = (src & mask_g) >>  8;
    u32 sr = (src & mask_r) >> 16;

    u32 db = (dst & mask_b);
    u32 dg = (dst & mask_g) >>  8;
    u32 dr = (dst & mask_r) >> 16;

    u32 b = sb + ((db * (nsa + 1)) >> 8);
    u32 g = sg + ((dg * (nsa + 1)) >> 8);
    u32 r = sr + ((dr * (nsa + 1)) >> 8);
    u32 a = sa + ((da * (nsa + 1)) >> 8);

    u32 result = (a << 24) | (r << 16) | (g << 8) | b;    
    return result;
}
