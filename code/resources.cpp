//
// Level resources
// (c) Marcus Larsson
//

struct Bitmaps {
    Bmp ghost_red;
    Bmp ghost_pink;
    Bmp ghost_cyan;
    Bmp ghost_orange;
    Bmp ghost_as_prey;
    Bmp ghost_eye;
    Bmp ghost_pupil;
    Bmp dot_large;
    Bmp dot_small;
    Bmp background;
    Bmp pacman_atlas;
    Bmp wall_atlas;
};


struct Wavs {
    Wav eat_large_dot;
    Wav eat_small_dot;
    Wav ghost_dies;
    Wav won;
    Wav lost;
    Wav nope;
};


struct Resources {
    Bitmaps bitmaps;
    Wavs wavs;
    Font font;
};


static b32 init_resources(Resources *resources) {
    b32 result = true;

    // bitmaps
    if (result)  result = load_bitmap("data\\bitmaps\\ghost_red.bmp"    , &resources->bitmaps.ghost_red);
    if (result)  result = load_bitmap("data\\bitmaps\\ghost_pink.bmp"   , &resources->bitmaps.ghost_pink);
    if (result)  result = load_bitmap("data\\bitmaps\\ghost_cyan.bmp"   , &resources->bitmaps.ghost_cyan);
    if (result)  result = load_bitmap("data\\bitmaps\\ghost_orange.bmp" , &resources->bitmaps.ghost_orange);
    if (result)  result = load_bitmap("data\\bitmaps\\ghost_as_prey.bmp", &resources->bitmaps.ghost_as_prey);
    if (result)  result = load_bitmap("data\\bitmaps\\ghost_eyes.bmp"   , &resources->bitmaps.ghost_eye);
    if (result)  result = load_bitmap("data\\bitmaps\\ghost_pupils.bmp" , &resources->bitmaps.ghost_pupil);
    if (result)  result = load_bitmap("data\\bitmaps\\dot_large.bmp"    , &resources->bitmaps.dot_large);
    if (result)  result = load_bitmap("data\\bitmaps\\dot_small.bmp"    , &resources->bitmaps.dot_small);
    if (result)  result = load_bitmap("data\\bitmaps\\background.bmp"   , &resources->bitmaps.background);
    if (result)  result = load_bitmap("data\\bitmaps\\pacman_atlas.bmp" , &resources->bitmaps.pacman_atlas);
    if (result)  result = load_bitmap("data\\bitmaps\\wall_atlas.bmp"   , &resources->bitmaps.wall_atlas);
        
    // fonts
    if (result)  result = load_font("data\\fonts", "font", &resources->font);
    
    // wavs
    if (result)  result = load_wav("data\\audio\\eat_large_dot.wav", &resources->wavs.eat_large_dot);
    if (result)  result = load_wav("data\\audio\\eat_small_dot.wav", &resources->wavs.eat_small_dot);
    if (result)  result = load_wav("data\\audio\\ghost_dies.wav", &resources->wavs.ghost_dies);
    if (result)  result = load_wav("data\\audio\\won.wav", &resources->wavs.won);
    if (result)  result = load_wav("data\\audio\\lost.wav", &resources->wavs.lost);
    if (result)  result = load_wav("data\\audio\\nope.wav", &resources->wavs.nope);

    return result;
}


static void free_resources(Resources *resources) {
    //
    // bitmaps
    free_bitmap(&resources->bitmaps.ghost_red);
    free_bitmap(&resources->bitmaps.ghost_pink);
    free_bitmap(&resources->bitmaps.ghost_cyan);
    free_bitmap(&resources->bitmaps.ghost_orange);
    free_bitmap(&resources->bitmaps.ghost_as_prey);
    free_bitmap(&resources->bitmaps.ghost_eye);
    free_bitmap(&resources->bitmaps.ghost_pupil);
    free_bitmap(&resources->bitmaps.dot_large);
    free_bitmap(&resources->bitmaps.dot_small);
    free_bitmap(&resources->bitmaps.background);
    free_bitmap(&resources->bitmaps.pacman_atlas);
    free_bitmap(&resources->bitmaps.wall_atlas);

    // font
    free_font(&resources->font);

    // wavs
    free_wav(&resources->wavs.eat_large_dot);
    free_wav(&resources->wavs.eat_small_dot);
    free_wav(&resources->wavs.ghost_dies);
    free_wav(&resources->wavs.won);
    free_wav(&resources->wavs.lost);
    free_wav(&resources->wavs.nope);
}
