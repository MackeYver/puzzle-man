// 
// MIT License
// 
// Copyright (c) 2018 Marcus Larsson
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <assert.h>
#else
#ifndef assert
#define assert(x)
#endif
#endif

#define UNUSED_VAR(x) x


enum Wav_Type {
    Wav_Type_Wave = 0,
    Wav_Type_XWMA,
    Wav_Type_Unknown,
    
    Wav_Type_Count,
};


enum Wav_Audio_Format {
    Wav_Audio_Format_Unknown    = 0x0000,
    Wav_Audio_Format_PCM        = 0x0001,
    Wav_Audio_Format_IEE_FLOAT  = 0x0003,
    Wav_Audio_Format_ALAW       = 0x0006, 
    Wav_Audio_Format_MULAW      = 0x0007,
    Wav_Audio_Format_Extensible = 0xFFFE,
};


struct Wav_Format {
    u16 audio_format = Wav_Audio_Format_Unknown;
    u16 number_of_channels = 0;
    u32 sample_rate = 0;
    u32 average_bytes_per_second = 0;
    u16 block_align = 0;
    u16 bits_per_sample = 0;
    u16 extra_format_size = 0;
    u16 valid_bits_per_sample = 0;
    u32 channel_mask = 0;
    u32 subformat[4] = {};
};


struct Wav {
    u8 *data = nullptr;
    size_t data_size = 0;
    Wav_Type type = Wav_Type_Unknown;
    Wav_Format format;
};



//
// Chunks fourcc
//

u32 constexpr kWav_FourCC_RIFF = 'FFIR';
u32 constexpr kWav_FourCC_DATA = 'atad';
u32 constexpr kWav_FourCC_FMT  = ' tmf';
u32 constexpr kWav_FourCC_WAVE = 'EVAW';
u32 constexpr kWav_FourCC_XWMA = 'AMWX';




//
// Structs
//

struct Wav_Data_Info {
    u8 *start_of_buffer = nullptr;
    size_t current_index = 0;
    size_t size_of_buffer = 0;
    b32 error = false;
};




//
// Helper funtions
//

b32 is_EOF(Wav_Data_Info *data, u32 bytes_to_advance = 0) {
    b32 result = false;
    
    if (!data->start_of_buffer || ((data->current_index + bytes_to_advance) > data->size_of_buffer)) {
        data->error = true;
        result = true;
    }
    
    return result;
}


b32 is_valid(Wav_Data_Info *data) {
    b32 result = false;
    
    if (!data->error || !is_EOF(data)) {
        result = true;
    }
    
    return result;
}


void advance(Wav_Data_Info *data, u32 bytes_to_advance) {
    if (!is_EOF(data, bytes_to_advance)) {
        data->current_index += bytes_to_advance;
    }
}


u8 *get_current_address(Wav_Data_Info *data) {
    u8 *result = nullptr;
    
    if (is_valid(data)) {
        result = data->start_of_buffer + data->current_index;
    }
    
    return result;
}


u32 peek_u32(Wav_Data_Info *data) {
    u32 result = 0;
    
    if (is_valid(data)) {
        result = *(reinterpret_cast<u32 *>(get_current_address(data)));
    }
    
    return result;
}


u32 get_u32(Wav_Data_Info *data) {
    u32 result = 0;
    
    if (is_valid(data)) {
        result = *(reinterpret_cast<u32 *>(get_current_address(data)));
        advance(data, 4);
    }
    
    return result;
}


u16 get_u16(Wav_Data_Info *data) {
    u16 result = 0;
    
    if (is_valid(data)) {
        result = *(reinterpret_cast<u16 *>(get_current_address(data)));
        advance(data, 2);
    }
    
    return result;
}


b32 parse_RIFF(Wav_Data_Info *data, Wav *output) {
    b32 result = false;
    
    if (is_valid(data)) {
        u32 fourCC = peek_u32(data);
        if (fourCC == kWav_FourCC_RIFF) {
            advance(data, 4);
            
            u32 file_size = get_u32(data);
            UNUSED_VAR(file_size);
            // From MSDN: "The value of fileSize includes the size of fileType FOURCC plus the size of the data that follows, but
            // does not include the size of the "RIFF" FOURCC or the size of fileSize. The data consists of chunks in any order".
            //output->file_size = file_size - ? or +, we probably don't need to store this
            
            u32 file_type = get_u32(data);
            if (file_type == kWav_FourCC_WAVE) { // We only support wav for now            
                output->type = Wav_Type_Wave;
                result = true;
            }
        }
    }
    
    data->error = result;
    return result;
}


b32 parse_FMT(Wav_Data_Info *data, Wav *output) {
    b32 result = false;
    
    if (is_valid(data)) {
        u32 fourCC = peek_u32(data);
        if (fourCC == kWav_FourCC_FMT) {
            advance(data, 4);
            
            u32 chunk_size = get_u32(data); // size excluding the id (u32) and the size field itself (u32)
            
            Wav_Format *fmt = &output->format;
            fmt->audio_format = static_cast<Wav_Audio_Format>(get_u16(data));
            fmt->number_of_channels = get_u16(data);
            fmt->sample_rate = get_u32(data);
            fmt->average_bytes_per_second = get_u32(data);
            fmt->block_align = get_u16(data);
            fmt->bits_per_sample = get_u16(data);
            
            if (chunk_size > 16) { // Extra bytes maybe, should only be in the case of compressed data
                fmt->extra_format_size = get_u16(data);
                if (fmt->extra_format_size == 22) {
                    fmt->bits_per_sample = get_u16(data);
                    fmt->channel_mask = get_u32(data);
                    fmt->subformat[0] = get_u32(data);
                    fmt->subformat[1] = get_u32(data);
                    fmt->subformat[2] = get_u32(data);
                    fmt->subformat[3] = get_u32(data);
                }
                else {
                    advance(data, fmt->extra_format_size); // Hm, unsupported extension!
                }
            }
            
            if (fmt->audio_format != Wav_Audio_Format_PCM && fmt->audio_format != Wav_Audio_Format_Extensible) { // We only support uncompressed data for the moment.
                assert(0);
            }
            
            if (fmt->bits_per_sample % 8 != 0) { // Byte aligned?
                assert(0);
            }
            
            result = true;
        }
    }
    
    data->error = result;
    return result;
}


b32 parse_DATA(Wav_Data_Info *data, Wav *output) {
    b32 result = false;
    
    if (is_valid(data)) {
        u32 fourCC = peek_u32(data);
        if (fourCC == kWav_FourCC_DATA) {
            advance(data, 4);
            
            output->data_size = get_u32(data);
            output->data = static_cast<u8 *>(malloc(output->data_size));
            assert(output->data);
            
            memcpy(output->data, get_current_address(data), output->data_size);
            
            result = true;
        }
    }
    
    return result;
}




//
// "Public" functions
//

b32 parse_WAV(u8 *data, size_t data_size, Wav *output) {
    b32 result = false;
    
    if (output && data && data_size > 0) {
        Wav_Data_Info data_info;
        data_info.start_of_buffer = data;
        data_info.current_index = 0;
        data_info.size_of_buffer = data_size;
        
        result = parse_RIFF(&data_info, output);
        assert(result);
        
        result = parse_FMT(&data_info, output);
        assert(result);
        
        result = parse_DATA(&data_info, output);
        assert(result);
    }
    
    return result;
}


b32 load_wav(char const *path_and_name, Wav *output) {
    b32 result = false;

    u8 *data;
    u32 size;
    result = win32_read_entire_file(path_and_name, &data, &size);
    if (result) {
        result = parse_WAV(data, size, output);
    }

    if (data) {
        free(data);
    }

    return result;
}


void free_wav(Wav *data) {
    if (data) {
        if (data->data) {
            free(data->data);
            memset(data, 0, sizeof(Wav));
        }
    }
}
