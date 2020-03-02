//
// Level resources
// (c) Marcus Larsson
//

struct Bitmaps {
    Bmp walls[16];
    Bmp ghost_red;
    Bmp ghost_pink;
    Bmp ghost_cyan;
    Bmp ghost_orange;
    Bmp ghost_as_prey;
    Bmp ghost_eye;
    Bmp ghost_pupil;
    Bmp pacman; // TODO: Remove if unused!
    Bmp dot_large;
    Bmp dot_small;
    Bmp background; // TODO: Remove if unused!
    Bmp pacman_atlas;
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


void free_resources(Resources *resources) {
    //
    // bitmaps
    for (u32 index = 0; index < Array_Count(resources->bitmaps.walls); ++index) {
        free_bitmap(&resources->bitmaps.walls[index]);
    }

    free_bitmap(&resources->bitmaps.ghost_red);
    free_bitmap(&resources->bitmaps.ghost_pink);
    free_bitmap(&resources->bitmaps.ghost_cyan);
    free_bitmap(&resources->bitmaps.ghost_orange);
    free_bitmap(&resources->bitmaps.ghost_as_prey);
    free_bitmap(&resources->bitmaps.ghost_eye);
    free_bitmap(&resources->bitmaps.ghost_pupil);
    free_bitmap(&resources->bitmaps.pacman); // TODO: Remove if unused!
    free_bitmap(&resources->bitmaps.dot_large);
    free_bitmap(&resources->bitmaps.dot_small);
    free_bitmap(&resources->bitmaps.background); // TODO: Remove if unused!
    free_bitmap(&resources->bitmaps.pacman_atlas);

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
