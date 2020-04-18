//
// win32_main.cpp
// (c) Marcus Larsson
//

#define kFrame_Time 16667 // in microseconds (for some reason)
//#define kPrintFPS

#define kCell_Size 64
#define kLevel_Size 11

// TODO: make this dynamic
#define kWindow_Client_Area_Width  11 * kCell_Size
#define kWindow_Client_Area_Height kWindow_Client_Area_Width

#define kLevel_Name_Max_Length 31
#define kLevel_Max_Actors 100 // DEBUG

#define kDot_Small_Value 10
#define kDot_Large_Value 50
#define kGhost_Value 200

#define kPredator_Mode_Duration 10

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>


//
// Check for memory leaks
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif




//
// Types
//

//
// Scalars
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef u32 b32;

typedef float f32;

f32 clamp_01(f32 a) {
    f32 result = (a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a));
    return result;
}

#define Array_Count(x) sizeof(x) / sizeof(x[0])




//
// Utilities, win32
//

b32 win32_read_entire_file(char const *path_and_name, u8 **data, u32 *size) {
    b32 result = false;

    size_t length = strlen(path_and_name) + 1;
    assert(length < MAX_PATH);
    wchar_t text_buffer[MAX_PATH];
    size_t converted;
    // NOTE: It seems like mbstowcs_s includes the null termintor in the count of converted chars?
    mbstowcs_s(&converted, text_buffer, sizeof(text_buffer) / sizeof(wchar_t), path_and_name, length);
    assert(converted == length);
    
    HANDLE file = CreateFile(text_buffer, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        u32 error = GetLastError();
        printf("%s() failed to open file %s, error = %u\n", __FUNCTION__, path_and_name, error);
    }
    else {
        *size = GetFileSize(file, nullptr);
        *data = static_cast<u8 *>(malloc(*size));
        DWORD bytes_read = 0;
        result = ReadFile(file, *data, *size, &bytes_read, nullptr);
        if (result == 0) {
            u32 error = GetLastError();
            printf("%s() failed to read %u bytes from file %s, error = %u\n", __FUNCTION__, *size, path_and_name, error);
            assert(0);
        }

        // DEBUG
        assert(result && *data && bytes_read == *size);
        CloseHandle(file);
    }    

    return result;
}

HANDLE win32_open_file_for_writing(char const *path_and_name) {
    size_t length = strlen(path_and_name) + 1;
    assert(length < MAX_PATH);
    wchar_t text_buffer[MAX_PATH];
    size_t converted;
    // NOTE: It seems like mbstowcs_s includes the null termintor in the count of converted chars?
    mbstowcs_s(&converted, text_buffer, sizeof(text_buffer) / sizeof(wchar_t), path_and_name, length);
    assert(converted == length);
    
    HANDLE file = CreateFile(text_buffer, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        u32 error = GetLastError();
        printf("%s() failed to create file %s, error = %u\n", __FUNCTION__, path_and_name, error);
    }

    return file;
}


HANDLE win32_open_file_for_reading(char const *path_and_name) {
    size_t length = strlen(path_and_name) + 1;
    assert(length < MAX_PATH);
    wchar_t text_buffer[MAX_PATH];
    size_t converted;
    // NOTE: It seems like mbstowcs_s includes the null termintor in the count of converted chars?
    mbstowcs_s(&converted, text_buffer, sizeof(text_buffer) / sizeof(wchar_t), path_and_name, length);
    assert(converted == length);
    
    HANDLE file = CreateFile(text_buffer, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        u32 error = GetLastError();
        printf("%s() failed to create file %s, error = %u\n", __FUNCTION__, path_and_name, error);
    }

    return file;
}




//
// Includes
//

#include "log.h"
#include "wav.cpp"
#include "win32_audio.cpp"
#include "tokenizer.cpp"
#include "mathematics.cpp"
#include "bitmap.cpp"
#include "font.cpp"
#include "win32_software_renderer.cpp"
#include "resources.cpp"
#include "actor.cpp"
#include "tile_and_item.cpp"
#include "level.cpp"
#include "world.cpp"
#include "game_main.cpp"




//
// Windows
//

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT *create_struct = reinterpret_cast<CREATESTRUCT*>(l_param);
            Game *game = reinterpret_cast<Game *>(create_struct->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)game);
        } break;
            
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        } break;

        // case WM_MOUSEMOVE: {
        // } break;

        // case WM_SIZE: {
        // } break;

        case WM_PAINT: {
            Game *game = reinterpret_cast<Game *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (game) {
                Render_State *render_state = &game->render_state;
                if (render_state) {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);

                    #if 0
                    b32 result = BitBlt(hdc, 0, 0, render_state->backbuffer_width, render_state->backbuffer_height,
                                        render_state->backbuffer_hdc, 0, 0, SRCCOPY);
                    result;
                    #else                    
                    
                    RECT client_rect;
                    GetClientRect(hwnd, &client_rect);
                    s32 window_w = client_rect.right - client_rect.left;
                    s32 window_h = client_rect.bottom - client_rect.top;
                    
                    s32 dst_w = window_w;
                    s32 dst_h = window_h;

                    s32 src_w = render_state->backbuffer_width;
                    s32 src_h = render_state->backbuffer_height;

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
                    
                    b32 result = StretchBlt(hdc,
                                            dx, dy, dst_w, dst_h,
                                            render_state->backbuffer_hdc,
                                            0, 0, src_w, src_h,
                                            SRCCOPY);
                    result;
#endif
                    EndPaint(hwnd, &ps);
                }
            }
        } break;

        case WM_KEYDOWN: {
            Game *game = reinterpret_cast<Game *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            b32 ctrl_is_down = GetKeyState(VK_CONTROL) & 0x8000;
            b32 shift_is_down = GetKeyState(VK_SHIFT) & 0x8000;

            if (game->state == Game_State_Player_Select_Enter_Name) {
                if (w_param == VK_ESCAPE) {
                    game->state = Game_State_Player_Select;
                }
                else if (w_param == VK_BACK) {
                    if (game->input_buffer_curr_pos > 0) {
                        --game->input_buffer_curr_pos;
                    }
                    game->input_buffer[game->input_buffer_curr_pos] = '\0';
                }
                else if (w_param == VK_RETURN) {
                    game->state = Game_State_Player_Select_Enter_Name_Done;
                }
                else if ((w_param >= 0x41 && w_param <= 0x5A) || (w_param == VK_SPACE)) {
                    char curr_char = static_cast<char>(w_param);                    
                    if (w_param != VK_SPACE && !shift_is_down)  curr_char += 32;
                    
                    if (game->input_buffer_curr_pos < (kPlayer_Profile_Name_Max_Length - 1)) {
                        game->input_buffer[game->input_buffer_curr_pos++] = curr_char;
                    }
                    game->input_buffer[game->input_buffer_curr_pos] = '\0';
               }
                else {
                    play_wav(&game->audio_state, &game->resources.wavs.nope);
                }
            }
            else {
                if (w_param == VK_RIGHT) {
                    game->input = Input_Right;                
                }
                else if (w_param == VK_UP) {
                    game->input = Input_Up;
                }
                else if (w_param == VK_LEFT) {
                    game->input = Input_Left;
                }
                else if (w_param == VK_DOWN) {
                    game->input = Input_Down;
                }
                else if (w_param == VK_RETURN) {
                    game->input = Input_Select;
                }
                else if (w_param == VK_ESCAPE) {
                    game->input = Input_Escape;
                }
                else if (w_param == VK_DELETE) {
                    game->input = Input_Delete;
                }
                else if (w_param == 0x52) { // R
                    reset(game);
                }
                else if (w_param == 0x47) { // G
                    ++game->draw_grid_mode;
                    game->draw_grid_mode = game->draw_grid_mode > 2 ? 0 : game->draw_grid_mode;
                }
                else if (w_param == VK_BACK || (!shift_is_down && ctrl_is_down && w_param == 0x5A)) {
                    // 0x5A == 'z
                    undo(game);
                }
                else if ((shift_is_down && ctrl_is_down && w_param == 0x5A) || ctrl_is_down && w_param == 0x59) {
                    // 0x59 == 'y'
                    redo(game);
                }
                else if (w_param == VK_F5) {
                    // Save
                    write_player_profiles_to_disc(game->player_profiles);
                }
                else if (w_param == VK_F6) {
                    // Load
                    read_player_profiles_from_disc(game->player_profiles);
                }                
                else if (game->state == Game_State_Lost && w_param == VK_RETURN) {
                    reset(game);
                }
                else if (game->state == Game_State_Won && w_param == VK_RETURN) {
                    change_to_next_level(game);
                }            
                else if (w_param == VK_F1) { // DEBUG
                    change_to_prev_level(game);
                }
                else if (w_param == VK_F2) { // DEBUG
                    change_to_next_level(game);
                }
                else if (w_param == VK_F3) { // DEBUG
                    game->current_map_index = game->current_map_index == 0 ? Map_Count : game->current_map_index - 1;
                }
                else if (w_param == VK_F4) { // DEBUG
                    game->current_map_index = (game->current_map_index + 1) % (Map_Count + 1);
                }
            }
        } break;
    }

    return DefWindowProc(hwnd, msg, w_param, l_param);
}


int WINAPI wWinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PWSTR CmdLine, int CmdShow)
{
    Game game;
    open_log(&game.log);
    
    wchar_t const *class_name = L"puzzle-man";


    
    //
    // Allocate a console and direct the standard output to it
#ifdef DEBUG
    AllocConsole();
    FILE* pcout;
    freopen_s(&pcout, "conout$", "w", stdout);
    HWND console_hwnd = GetConsoleWindow();
    SetWindowPos(console_hwnd, HWND_TOPMOST, 300, 500, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    game.log.print_log = true;
#endif  


    log_str(&game.log, "Commencing win32 start-up");
    
   
    //
    // Window class
    RECT client_rect = {0, 0, kWindow_Client_Area_Width, kWindow_Client_Area_Height};
    HWND hwnd = nullptr;    
    {        
        WNDCLASSEX window_class;
        ZeroMemory(&window_class, sizeof(WNDCLASSEX));
        window_class.cbSize = sizeof(WNDCLASSEX);
        window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = WindowProc;
        window_class.hInstance = Instance;
        window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
        window_class.lpszClassName = class_name;
    
        u32 result = RegisterClassEx(&window_class);
        if (result == 0) {
            u32 error = GetLastError();
            LOG_ERROR(&game.log, "failed to register WNDCLASSEX", error);
            return -1;
        }
        
        //
        // Create window
        DWORD window_style = WS_CAPTION | WS_SYSMENU | WS_SIZEBOX;
        AdjustWindowRectEx(&client_rect, window_style, false, 0);
        game.window_width = client_rect.right - client_rect.left;
        game.window_height = client_rect.bottom - client_rect.top;
        
        hwnd = CreateWindowEx(0,
                              class_name,
                              class_name,
                              window_style,
                              1800,//CW_USEDEFAULT, // DEBUG
                              500,//CW_USEDEFAULT,
                              game.window_width,
                              game.window_height,
                              nullptr,
                              nullptr,
                              Instance,
                              &game);
        if (hwnd == 0) {
            int error = GetLastError();
            LOG_ERROR(&game.log, "failed to create the window", error);
            return -2;
        }
    }

    ShowWindow(hwnd, CmdShow);
    UpdateWindow(hwnd);
    log_str(&game.log, "Done with win32 start-up.");


    
    //
    // Init renderer and audio
    {
        b32 result = init_renderer(&game.render_state, &game.log, hwnd, kWindow_Client_Area_Width, kWindow_Client_Area_Height);
        if (!result) {            
            LOG_ERROR(&game.log, "failed to initialize the software renderer", result);
            return result;
        }
        log_str(&game.log, "Software renderer initialized.");
            
        result = init_audio(&game.audio_state, &game.log);
        if (!result) {
            //printf("Error, failed to initialize the audio system\n");
            LOG_ERROR(&game.log, "failed to initialize the audio system", result);            
            return -1;
        }
        log_str(&game.log, "XAudio2 initilized.");
    }
    

    
    //
    // Init game
    {        
        init_game(&game);
    }

    
    
    //
    // Main loop
    LARGE_INTEGER frequency;
    LARGE_INTEGER frame_start_time;
    QueryPerformanceFrequency(&frequency);

#ifdef kPrintFPS
    u32 frame_counter = 0;
    u32 accumulated_frame_time = 0;
#endif

    MSG msg = {};
    b32 running = true;
    while (running) {
        QueryPerformanceCounter(&frame_start_time);


        //
        // Message pump        
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
            running = false;
            }
        }


        //
        // Update and render
        b32 should_quit = false;
        update_and_render(&game, kFrame_Time, &should_quit);
        if (should_quit) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
      
                
        //
        // Draw backbuffer to screen
        InvalidateRect(hwnd, nullptr, false);
        UpdateWindow(hwnd);


        //
        // Timing
        LARGE_INTEGER frame_end_time, elapsed_time;
        QueryPerformanceCounter(&frame_end_time);
        elapsed_time.QuadPart = frame_end_time.QuadPart - frame_start_time.QuadPart;
        elapsed_time.QuadPart *= 1000000;
        elapsed_time.QuadPart /= frequency.QuadPart;

        while (elapsed_time.QuadPart < kFrame_Time) {
            QueryPerformanceCounter(&frame_end_time);
            elapsed_time.QuadPart = frame_end_time.QuadPart - frame_start_time.QuadPart;
            elapsed_time.QuadPart *= 1000000;
            elapsed_time.QuadPart /= frequency.QuadPart;
        }
        

#ifdef kPrintFPS
        ++frame_counter;
        accumulated_frame_time += static_cast<u32>(elapsed_time.QuadPart);
        if (accumulated_frame_time >= 1000000) {
            printf("--------------------------------------\n%d fps\n\n", frame_counter);
            frame_counter = 0;
            accumulated_frame_time = 0;

            metrics = game.metrics;
            metrics.total.QuadPart *= 1000000;
            metrics.total.QuadPart /= frequency.QuadPart;
            metrics.draw_tiles.QuadPart *= 1000000;
            metrics.draw_tiles.QuadPart /= frequency.QuadPart;
            metrics.draw_actors.QuadPart *= 1000000;
            metrics.draw_actors.QuadPart /= frequency.QuadPart;

            printf("TOTAL\n");
            printf(" Total =          %10lld\n    draw_tiles =  %10lld (%5.1f%%)\n    draw_actors = %10lld (%5.1f%%)\n\n",
                   metrics.total.QuadPart,
                   metrics.draw_tiles.QuadPart, 100.0f * ((f32)metrics.draw_tiles.QuadPart   / (f32)metrics.total.QuadPart),
                   metrics.draw_actors.QuadPart, 100.0f * ((f32)metrics.draw_actors.QuadPart / (f32)metrics.total.QuadPart));


            f32 total = (f32)metrics.total.QuadPart / (f32)metrics.count;
            f32 dt = (f32)metrics.draw_tiles.QuadPart / (f32)metrics.count;
            f32 da = (f32)metrics.draw_actors.QuadPart / (f32)metrics.count;;
            
            printf("AVERAGE\n");
            printf(" Total =          %7.1f\n    draw_tiles =  %7.1f (%5.1f%%)\n    draw_actors = %7.1f (%5.1f%%)\n\n",
                   total,
                   dt, 100.0f * dt / total,
                   da, 100.0f * da / total);
        }
#endif
    }   

    
    //
    // Quit the program
    fini_game(&game);    
    fini_audio(&game.audio_state);    
    fini_renderer(&game.render_state);

#ifdef DEBUG
    _CrtDumpMemoryLeaks();

    fclose(pcout);
    FreeConsole();
#endif

    close_log(&game.log);
   
    return 0;
}
