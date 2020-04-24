//
// Tokenizer
// (c) Marcus Larsson
//

#define kToken_Data_Max_Length 512
#define kTokenizer_Error_String_Max_Length 512
#define kTokenizer_Path_And_Name_Max_Length 512


enum Token_type {
    Token_identifier,
    Token_dash,
    Token_point,
    Token_colon,
    Token_comma,    
    Token_curled_brace_start,
    Token_curled_brace_end,
    Token_paren_start,
    Token_paren_end,
    Token_comment,
    Token_forward_slash,
    Token_equal_sign,
    Token_string,
    Token_number,    
    Token_unknown,
    Token_error,
};


char constexpr *Token_type_str[] = {
    "Token_identifier",
    "-",  //"Token_dash",
    ".",  //"Token_point",
    ":",  //"Token_colon",
    ",",  //"Token_comma",
    "{",  //"Token_curled_brace_start",
    "}",  //"Token_curled_brace_end",
    "(",  //"Token_paren_start",
    ")",  //"Token_paren_end",
    "//", //"Token_comment",
    "/",  //"Token_forward_slash",
    "=",  //"Token_equal_sign",
    "Token_string",
    "Token_number",
    "Token_unknown",
    "Token_error",
};


struct Token {
    Token_type type = Token_unknown;
    char data[kToken_Data_Max_Length] = {};
    u32 length = 0;
    u32 line_number = 1;
    u32 line_position = 1;

};


b32 token_is_valid(Token *token) {
    b32 result = token->type != Token_error;
    return result;
}


struct Tokenizer {
    char error_string[kTokenizer_Error_String_Max_Length] = {};
    char path_and_name[kTokenizer_Path_And_Name_Max_Length] = {};
    char *data = nullptr;
    u32 size = 0;
    u32 current_position = 0;
    u32 line_number = 1;
    u32 line_position = 1;
    char curr_char = 0;
    char next_char = 0;
    b32 error = false;
};


b32 char_is_space(char c) {
    b32 result = c == ' ';
    return result;
}


b32 char_is_newline(char c) {
    b32 result = c == '\n' || c == '\r';
    return result;
}


b32 char_is_space_or_newline(char c) {
    b32 result = char_is_space(c) || char_is_newline(c);
    return result;
}


b32 char_is_digit(char c) {
    b32 result = (c >= '0') && (c <= '9');
    return result;
}


b32 char_is_letter(char c) {
    b32 result = ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
    return result;
}


b32 is_eof(Tokenizer *tokenizer) {
    b32 result = tokenizer->current_position > (tokenizer->size - 1);
    if (result) {
        tokenizer->error = true;
    }
    return result;
}


b32 at_last_position(Tokenizer *tokenizer) {
    b32 result = tokenizer->current_position == (tokenizer->size - 1);
    return result;
}


void advance(Tokenizer *tokenizer) {
    if (tokenizer && !tokenizer->error && !is_eof(tokenizer)) {
        ++tokenizer->current_position;
        ++tokenizer->line_position;
        
        if (tokenizer->curr_char == '\n' || tokenizer->curr_char == '\r') {            
            if (tokenizer->next_char == '\n' || tokenizer->next_char == '\r') {
                ++tokenizer->current_position;
            }            
            ++tokenizer->line_number;
            tokenizer->line_position = 1;
        }
    }
}

void reload(Tokenizer *tokenizer) {
    if (!tokenizer->error && !is_eof(tokenizer)) {
        tokenizer->curr_char = tokenizer->data[tokenizer->current_position];

        if (tokenizer->current_position < (tokenizer->size - 2)) {
            tokenizer->next_char = tokenizer->data[tokenizer->current_position + 1];
        }
    }
}


void skip_to_next_line(Tokenizer *tokenizer) {
    reload(tokenizer);
    
    while(!tokenizer->error && !is_eof(tokenizer) && !char_is_newline(tokenizer->curr_char)) {
        advance(tokenizer);
        reload(tokenizer);
    }

    while (char_is_newline(tokenizer->curr_char)) {
        advance(tokenizer);
        reload(tokenizer);
    }
}


void eat_spaces_and_newline(Tokenizer *tokenizer) {
    reload(tokenizer);
    
    if (!tokenizer->error) {
        b32 is_space_or_newline = char_is_space_or_newline(tokenizer->curr_char);
        b32 is_comment = (tokenizer->curr_char == '/' && tokenizer->next_char == '/');
        b32 more_to_eat = is_space_or_newline || is_comment;
        
        while(more_to_eat) {
            if (is_comment) {
                skip_to_next_line(tokenizer);
            }
            else {
                advance(tokenizer);            
            }
            reload(tokenizer);
            
            is_space_or_newline = char_is_space_or_newline(tokenizer->curr_char);
            is_comment = (tokenizer->curr_char == '/' && tokenizer->next_char == '/');
            more_to_eat = (is_space_or_newline || is_comment) && !is_eof(tokenizer);
        }
    }
}


Token get_token(Tokenizer *tokenizer) {
    Token result;

    if (tokenizer) {
        eat_spaces_and_newline(tokenizer);
        
        if (tokenizer->error) {
            result.type = Token_error;
        }
        else {
            result.line_number = tokenizer->line_number;
            result.line_position = tokenizer->line_position;            
            
            switch (tokenizer->curr_char) {
                case  '-': { result.type = Token_dash;               } break; // TODO, this prevents the use of negative numbers
                case  '.': { result.type = Token_point;              } break;
                case  ':': { result.type = Token_colon;              } break;                    
                case  ',': { result.type = Token_comma;              } break;
                case  '{': { result.type = Token_curled_brace_start; } break;
                case  '}': { result.type = Token_curled_brace_end;   } break;
                case  '(': { result.type = Token_paren_start;        } break;
                case  ')': { result.type = Token_paren_end;          } break;
                case  '=': { result.type = Token_equal_sign;         } break;
                    
                case  '\"': {
                    result.type = Token_string;

                    advance(tokenizer);
                    reload(tokenizer);
                            
                    result.length = 0;
                    result.data[0] = '\0';

                    while (tokenizer->curr_char != '\"') {
                        if (result.length == (kTokenizer_Path_And_Name_Max_Length - 1)) {
                            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                                        "In %s at %u:%u, found a string longer than kToken_Data_Max_Length (%u)",
                                        tokenizer->path_and_name, tokenizer->line_number, tokenizer->line_position, kToken_Data_Max_Length);
                            tokenizer->error = true;
                            result.type = Token_error;                            
                            break;
                        }
                        
                        result.data[result.length++] = tokenizer->curr_char;

                        advance(tokenizer);
                        reload(tokenizer);
                    }                    
                } break;

                case  '/': {
                    if (tokenizer->next_char == '/') {
                        result.type = Token_comment;
                    }
                    else {
                        result.type = Token_forward_slash;
                    }
                } break;

                default: {
                    if (char_is_digit(tokenizer->curr_char)) {
                        result.type = Token_number;
                        u32 index = 0;
                        result.data[index++] = tokenizer->curr_char;

                        while (char_is_digit(tokenizer->next_char)) {
                            advance(tokenizer);
                            reload(tokenizer);
                            result.data[index++] = tokenizer->curr_char;
                        }

                        if (!char_is_space_or_newline(tokenizer->next_char) && tokenizer->next_char != ',' && tokenizer->next_char != ':') {                            
                            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                                      "In %s at %u:%u, found an invalid char while tokenizing a number",
                                      tokenizer->path_and_name, tokenizer->line_number, tokenizer->line_position);
                            tokenizer->error = true;
                            result.type = Token_error;
                        }                                 
                        result.length = index - 1;
                    }
                    else if (char_is_letter(tokenizer->curr_char)) {
                        result.type = Token_identifier;
                        u32 index = 0;
                        result.data[index++] = tokenizer->curr_char;

                        while (char_is_letter(tokenizer->next_char) || char_is_digit(tokenizer->next_char) || (tokenizer->next_char == '_')) {
                            advance(tokenizer);
                            reload(tokenizer);
                            result.data[index++] = tokenizer->curr_char;
                        }

                        if (!char_is_space_or_newline(tokenizer->next_char) &&
                            tokenizer->next_char != ',' && tokenizer->next_char != ':' && tokenizer->next_char != '='
                            && tokenizer->next_char != '\"' && tokenizer->next_char != '\'' && tokenizer->next_char != '.')
                        {
                            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                                      "In %s at %u:%u, found an invalid char while tokenizing an identifier",
                                      tokenizer->path_and_name, tokenizer->line_number, tokenizer->line_position);
                            tokenizer->error = true;
                            result.type = Token_error;
                        }

                        result.length = index - 1;
                    }
                    else {
                        result.type = Token_unknown;
                    }
                } break;
            } // end of switch()
            
            advance(tokenizer);
            reload(tokenizer);            
        }
    }

    return result;
}


b32 require_token(Tokenizer *tokenizer, Token *token, Token_type token_type) {
    b32 result = false;

    if (!tokenizer->error) {
        *token = get_token(tokenizer);

        // Skip comments
        while (token->type == Token_comment) {
            skip_to_next_line(tokenizer);
            *token = get_token(tokenizer);
        }

        if (token->type != token_type) {                        
            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                      "In %s at %u:%u, expected '%s' got '%s'",
                      tokenizer->path_and_name, token->line_number, token->line_position,
                      Token_type_str[token_type], Token_type_str[token->type]);
            token->type = Token_error;
            tokenizer->error = true;
        }
        else {
            result = true;
        }
    }

    return result;
}


b32 require_identifier_with_exact_name(Tokenizer *tokenizer, Token *token, char const *name) {
    b32 result = false;
    
    if (!tokenizer->error) {
        *token = get_token(tokenizer);

        // Skip comments
        while (token->type == Token_comment) {
            skip_to_next_line(tokenizer);
            *token = get_token(tokenizer);
        }
    
        if (token->type == Token_identifier) {
            if (strcmp(token->data, name) == 0) {
                result = true; // Yay, this is the one!
            }
            else {                
                _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                          "In %s at %u:%u, expected an identifier named %s but got %s",                          
                          tokenizer->path_and_name, token->line_number, token->line_position, name, token->data);
                token->type = Token_error;
                tokenizer->error = true;
            }
        }
        else {            
            _snprintf_s(tokenizer->error_string, kTokenizer_Error_String_Max_Length, _TRUNCATE,
                      "In %s at %u:%u, expected an identifier named '%s' got '%s'",
                      tokenizer->path_and_name, token->line_number, token->line_position, name, Token_type_str[token->type]);
            token->type = Token_error;
            tokenizer->error = true;
        }
    }

    return result;
}


u32 get_u32_from_token(Token *token) {
    // TODO: add better error handling here!

    u32 result = 0; 
    size_t read = sscanf_s(token->data, "%u", &result);
    assert(read == 1);
    
    return result;
}


b32 init_tokenizer(Tokenizer *tokenizer, char const *path, char const *name) {
    b32 result = false;

    size_t path_len = strlen(path);
    size_t name_len = strlen(name);
    char path_and_name[512];
    assert((path_len + name_len) < 511);    
    _snprintf_s(path_and_name, 512, _TRUNCATE, "%s\\%s", path, name);
    _snprintf_s(tokenizer->path_and_name, kTokenizer_Path_And_Name_Max_Length, _TRUNCATE, "%s\\%s", path, name);

#if 0
    FILE *file;
    errno_t error = fopen_s(&file, path_and_name, "rb");
    if (error != 0) {
        printf("%s() failed to open file %s, errno = %d\n", __FUNCTION__, name, errno);
    }
    else {
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        tokenizer->data = static_cast<char *>(malloc(size));
        size_t read = fread_s(tokenizer->data, size, 1, size, file);
        if (read != size) {
            printf("%s() failed to read from file %s, errno = %d\n", __FUNCTION__, name, errno);
        }
        else {
            fclose(file);
            reload(tokenizer);
            assert(size < 4294967295);
            tokenizer->size = static_cast<u32>(size);
            tokenizer->line_number = 1;
            tokenizer->line_position = 1;
            result = true;
        }
    }
#else

    result = win32_read_entire_file(path_and_name, reinterpret_cast<u8 **>(&tokenizer->data), &tokenizer->size);
    if (result) {
        reload(tokenizer);
        tokenizer->line_number = 1;
        tokenizer->line_position = 1;
        result = true;
    }
#endif

    return result;
}


void fini_tokenizer(Tokenizer *tokenizer) {
    if (tokenizer) {
        if (tokenizer->data) {
            free(tokenizer->data);
            tokenizer->data = nullptr;
        }
        ZeroMemory(tokenizer, sizeof(Tokenizer));
    }        
}


void reset_tokenizer(Tokenizer *tokenizer) {
    tokenizer->current_position = 0;
    tokenizer->line_number = 1;
    tokenizer->line_position = 1;
    tokenizer->error = false;
    tokenizer->error_string[0] = '\0';
}
