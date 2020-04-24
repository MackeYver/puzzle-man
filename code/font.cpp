//
// Font
//

// NOTE: we're wasting a lot of space in the struct
// TODO: optimize storage
struct Char_Data {
    u32 x = 0;
    u32 y = 0;
    u32 width = 0;
    u32 height = 0;
    u32 offset_x = 0;
    u32 offset_y = 0;
    u32 advance_x = 0;
};

#define kFont_Bitmap_Name_Max_Length 31

struct Font {
    Char_Data *char_data = nullptr;
    char bitmap_name[kFont_Bitmap_Name_Max_Length] = {};
    Bmp bitmap = {};
    u32 char_count = 0;
    u32 line_height = 0;
    u32 spacing_x = 0;
    u32 spacing_y = 0;
    u32 bitmap_width = 0;
    u32 bitmap_height = 0;
    u32 base = 0;
};

void free_font(Font *font) {
    if (font) {
        free_bitmap(&font->bitmap);
        font->bitmap_name[0] = '\0';
        font->bitmap_width = 0;
        font->bitmap_height = 0;
        
        if (font->char_data) {
            free(font->char_data);
            font->char_data = nullptr;
        }
        font->char_count = 0;
        
        font->line_height = 0;
        font->spacing_x = 0;
        font->spacing_y = 0;
    }
}

b32 require_font_attribute_u32(Tokenizer *tokenizer, Token *token) {
    b32 result = false;
    
    if (!tokenizer->error) {
        result = require_token(tokenizer, token, Token_identifier);
        result = require_token(tokenizer, token, Token_equal_sign);
        result = require_token(tokenizer, token, Token_number);
    }
    
    return result;
}

b32 require_font_attribute_string(Tokenizer *tokenizer, Token *token, char **string, size_t string_size) {
    b32 result = false;
    
    if (!tokenizer->error) {
        result = require_token(tokenizer, token, Token_identifier);
        result = require_token(tokenizer, token, Token_equal_sign);

        #if 1
        result = require_token(tokenizer, token, Token_string);
        if (string && *string && string_size > 0) {
            _snprintf_s(*string, string_size, _TRUNCATE, "%s", token->data);
        }
        #else
        result = require_token(tokenizer, token, Token_double_quote);

        u32 start_line_number = tokenizer->line_number;
        u32 start_line_position = tokenizer->line_position - 1;

        u32 string_index = 0;
        while (!tokenizer->error && tokenizer->curr_char != '\"') {

            // copy string
            if (string && *string && string_index < string_size) {
                (*string)[string_index++] = tokenizer->curr_char;
            }

            advance(tokenizer);
            reload(tokenizer);
        }

        if (is_eof(tokenizer)) {
            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                      "In %s at %d:%d, start of string (\") but reached the end of the file before end of string",
                      tokenizer->path_and_name, start_line_number, start_line_position);
            token->type = Token_error;
            tokenizer->error = true;
        }
        else if (tokenizer->error) {
            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                      "In %s at %d:%d, error when parsing string",
                      tokenizer->path_and_name, start_line_number, start_line_position);
            token->type = Token_error;
            tokenizer->error = true;
        }

        advance(tokenizer);
        eat_spaces_and_newline(tokenizer);
        reload(tokenizer);
        #endif
        result = true;
    }
    
    return result;
}

b32 require_font_attribute_padding(Tokenizer *tokenizer, Token *token) {
    b32 result = false;
    if (!tokenizer->error) {
        result = require_font_attribute_u32(tokenizer, token);
        result = require_token(tokenizer, token, Token_comma);
        result = require_token(tokenizer, token, Token_number);
        result = require_token(tokenizer, token, Token_comma);
        result = require_token(tokenizer, token, Token_number);
        result = require_token(tokenizer, token, Token_comma);
        result = require_token(tokenizer, token, Token_number);
    }

    return result;
}

b32 require_font_attribute_spacing(Tokenizer *tokenizer, Token *token, u32 *x, u32 *y) {
    b32 result = false;

    if (!tokenizer->error) {
        result = require_font_attribute_u32(tokenizer, token);
        if (x)  *x = get_u32_from_token(token);    
        result = require_token(tokenizer, token, Token_comma);
        result = require_token(tokenizer, token, Token_number);
        if (y)  *y = get_u32_from_token(token);
    }

    return result;
}

b32 load_font(char const *path, char const *name, Font *font) {
    free_font(font);
    b32 result = false;

    u32 constexpr temp_string_max_length = 512;
    char temp_string[temp_string_max_length];
    _snprintf_s(temp_string, temp_string_max_length, _TRUNCATE, "%s.fnt", name);

    Tokenizer tokenizer;
    result = init_tokenizer(&tokenizer, path, temp_string);
    if (result) {
        Token token;
        //
        // parse header

        
        // info line
        require_identifier_with_exact_name(&tokenizer, &token, "info");
        
        require_font_attribute_string(&tokenizer, &token, nullptr, 0); // face
        require_font_attribute_u32(&tokenizer, &token);    // size
        require_font_attribute_u32(&tokenizer, &token);    // bold
        require_font_attribute_u32(&tokenizer, &token);    // italic
        require_font_attribute_string(&tokenizer, &token, nullptr, 0); // charset
        require_font_attribute_u32(&tokenizer, &token);    // unicode
        require_font_attribute_u32(&tokenizer, &token);    // stretchH
        require_font_attribute_u32(&tokenizer, &token);    // smooth
        require_font_attribute_u32(&tokenizer, &token);    // aa
        require_font_attribute_padding(&tokenizer, &token);
        require_font_attribute_spacing(&tokenizer, &token, &font->spacing_x, &font->spacing_y);
        require_font_attribute_u32(&tokenizer, &token);    // outline

        
        // common line
        require_identifier_with_exact_name(&tokenizer, &token, "common");

        require_font_attribute_u32(&tokenizer, &token);    // lineHeight
        font->line_height = get_u32_from_token(&token);
        require_font_attribute_u32(&tokenizer, &token);    // base
        font->base = get_u32_from_token(&token);
        require_font_attribute_u32(&tokenizer, &token);    // scaleW
        require_font_attribute_u32(&tokenizer, &token);    // scaleH
        require_font_attribute_u32(&tokenizer, &token);    // pages
        require_font_attribute_u32(&tokenizer, &token);    // packed
        require_font_attribute_u32(&tokenizer, &token);    // alphaChnl
        require_font_attribute_u32(&tokenizer, &token);    // redChnl
        require_font_attribute_u32(&tokenizer, &token);    // greenChnl
        require_font_attribute_u32(&tokenizer, &token);    // blueChnl
        
        // page line
        require_identifier_with_exact_name(&tokenizer, &token, "page");

        require_font_attribute_u32(&tokenizer, &token);
        
        b32 loaded_bitmap = false;
        {
            char *string = &font->bitmap_name[0];
            b32 got_name = require_font_attribute_string(&tokenizer, &token, &string, kFont_Bitmap_Name_Max_Length); // bitmap name
            if (got_name) {
                if (_snprintf_s(temp_string, temp_string_max_length, _TRUNCATE, "%s\\%s", path, font->bitmap_name) <= 0) {
                    printf("%s() failed to concatenate bitmap name, error = %d\n", __FUNCTION__, errno);
                }
                else {
                    free_bitmap(&font->bitmap);
                    loaded_bitmap = load_bitmap(temp_string, &font->bitmap);                    
                }
            }
        }

        if (loaded_bitmap) {
            // chars line
            require_identifier_with_exact_name(&tokenizer, &token, "chars");
            require_font_attribute_u32(&tokenizer, &token);
            font->char_count = get_u32_from_token(&token);
            assert(font->char_count > 0); // TODO: handle this
            font->char_data = static_cast<Char_Data *>(malloc(font->char_count * sizeof(Char_Data)));
            assert(font->char_data);

        
            // read char data
            u32 char_index = 0;
            for (u32 line_count = 0; line_count < font->char_count; ++line_count) {
                Char_Data *c = &font->char_data[char_index++];
            
                require_identifier_with_exact_name(&tokenizer, &token, "char");
                require_font_attribute_u32(&tokenizer, &token); // id
            
                require_font_attribute_u32(&tokenizer, &token); // x position in bitmap
                c->x = get_u32_from_token(&token);
            
                require_font_attribute_u32(&tokenizer, &token); // y position in bitmap
                c->y = get_u32_from_token(&token);

                require_font_attribute_u32(&tokenizer, &token); // width
                c->width = get_u32_from_token(&token);

                require_font_attribute_u32(&tokenizer, &token); // height
                c->height = get_u32_from_token(&token);

                require_font_attribute_u32(&tokenizer, &token); // x offset
                c->offset_x = get_u32_from_token(&token);

                require_font_attribute_u32(&tokenizer, &token); // y offset
                c->offset_y = get_u32_from_token(&token);

                require_font_attribute_u32(&tokenizer, &token); // advance
                c->advance_x = get_u32_from_token(&token);

                require_font_attribute_u32(&tokenizer, &token); // page
                require_font_attribute_u32(&tokenizer, &token); // chnl

                if (tokenizer.error)  break;
            }
            assert(char_index == font->char_count);
        }
        
        if (tokenizer.error) {
            printf("%s(): invalid file format, %s\n", __FUNCTION__, tokenizer.error_string);
        }
    }

    fini_tokenizer(&tokenizer);

    return result;
};

v2u get_text_dim(Font *font, char const *text) {
    v2u result = v2u_00;    
    for (char const *ptr = text; *ptr; ++ptr) {        
        u32 char_index = *ptr - 32;
        if (char_index < font->char_count) {
            Char_Data *c = &font->char_data[char_index];
            //u32 bx = c->x;
            //u32 by = font->bitmap.header.height - c->y - c->height;
            //u32 x = result.x + c->offset_x;
            //u32 y = result.y + (font->base - c->offset_y - c->height);
            //draw_coloured_bitmap(render_state, V2u(x, y), &font->bitmap, bx, by, bx + c->width, by + c->height, colour_v4u8);
            result.x += c->advance_x;
            result.y = max(result.y, c->height);
        }
    }

    return result;
}
