//
// Bitmaps
//

#pragma pack(push, 1)
struct Bitmap_file_header {
    u16 type;        // file type must be BM
    u32 size;        // size of file in bytes
    u16 reserved1;   // must be zero
    u16 reserved2;   // must be zero
    u32 offset_bits; // byte offset from start of header to bitmap bits
};
#pragma pack(pop)


#pragma pack(push, 1)
struct Bitmap_info_header {  // This is really v3, for now we'll only support this one.
    u32 size;                  // size of the structure in bytes
    s32 width;                 // width of image in pixels
    s32 height;                // height of image in pixels
    u16 planes;                // number of bit planes, must be 1
    u16 bits_per_pixel;        // bits per pixel
    u32 compression;           // type of compression, uncompressed = BI_RGB (=0)
    u32 sizeof_image;          // size of the image in bytes
    s32 pixels_per_meter_x;    // The horizontal resolution, in pixels-per-meter
    s32 pixels_per_meter_y;    // The vertical resolution, in pixels-per-meter
    u32 colours_used;          // if all the colours are used = 0
    u32 important_colours_count;
}; // size 40
#pragma pack(pop)


struct Bmp {
    Bitmap_info_header header = {};
    u8 *data = nullptr;
    size_t row_size = 0;
    size_t data_size = 0;
};


b32 load_bitmap(char const *path_and_name, Bmp *output) {
    b32 result = false;

    //
    // Load file
    u8 *data;
    u32 data_size;
    result = win32_read_entire_file(path_and_name, &data, &data_size);

    
    //
    // parse file
    if (output && data && data_size > 0) {
        Bitmap_file_header *bf_header = reinterpret_cast<Bitmap_file_header *>(data);        
        if (bf_header->type == 0x4d42) {
            Bitmap_info_header *header = reinterpret_cast<Bitmap_info_header *>(data + sizeof(Bitmap_file_header));
            if ((header->size == 40) && (header->compression == 0)) {
                memcpy(&output->header, data + sizeof(Bitmap_file_header), sizeof(Bitmap_info_header));

                output->header.height = abs(output->header.height);
                
                output->row_size = 4 * (size_t)(((f32)(header->bits_per_pixel * output->header.width) / 32.0f) + 0.5f);
                output->data_size = output->row_size * output->header.height;
                assert(output->data_size > 0);
                
                output->data = static_cast<u8 *>(malloc(output->data_size));
                assert(output->data);
                memcpy(output->data, data + sizeof(Bitmap_file_header) + sizeof(Bitmap_info_header), output->data_size);
                
                result = true;
            }
            else {
                printf("%s: File has an invalid header, the size of the header is not 40 bytes\n", __FUNCTION__);
            }
        }
        else {
            printf("%s: file is not a bmp\n", __FUNCTION__);
        }
    }

    if (data) {
        free(data);
    }
    
    return result;
}

void free_bitmap(Bmp *bmp) {
    if (bmp && bmp->data) {
        free(bmp->data);
        bmp->data = nullptr;
        bmp->data_size = 0;
        bmp->row_size = 0;
    }    
    memset(&bmp->header, 0, sizeof(Bitmap_info_header));
}
