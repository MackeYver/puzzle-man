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

// From: https://stackoverflow.com/questions/8487986/file-macro-shows-full-path, users "Jayesh" and "red1ynx".
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)



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

b32  win32_open_file_for_writing(char const *path_and_name, HANDLE *handle) {
    b32 result = true;
    
    size_t length = strlen(path_and_name) + 1;
    assert(length < MAX_PATH);
    wchar_t text_buffer[MAX_PATH];
    size_t converted;
    // NOTE: It seems like mbstowcs_s includes the null termintor in the count of converted chars?
    mbstowcs_s(&converted, text_buffer, sizeof(text_buffer) / sizeof(wchar_t), path_and_name, length);
    assert(converted == length);
    
    *handle = CreateFile(text_buffer, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (*handle == INVALID_HANDLE_VALUE) {
        u32 error = GetLastError();
        printf("%s() failed to create file %s, error = %u\n", __FUNCTION__, path_and_name, error);
        result = false;
        *handle = nullptr;
    }

    return result;
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
#include "movement.cpp"
#include "editor.cpp"
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

        // case WM_PAINT: {            
        // } break;

        case WM_KEYDOWN: {
            Game *game = reinterpret_cast<Game *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            b32 ctrl_is_down = GetKeyState(VK_CONTROL) & 0x8000;
            b32 shift_is_down = GetKeyState(VK_SHIFT) & 0x8000;
            
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
            else if (w_param == VK_SPACE) {
                game->input = Input_Space;
            }
            else if (w_param == VK_TAB) {
                if (game->state == Game_State_Editing || game->state == Game_State_Begin_Editing) {
                    game->state = Game_State_End_Editing;
                }
                else {
                    game->state = Game_State_Begin_Editing;
                }
            }
            else if (w_param == 0x31) { // 1
                u32 *render_mode = nullptr;
                if (game->state == Game_State_Editing) {
                    render_mode = &game->editor.render_mode;
                }
                else {
                    render_mode = &game->render_mode;
                }
                *render_mode ^= Level_Render_Mode_Tiles;
            }
            else if (w_param == 0x32) { // 2
                u32 *render_mode = nullptr;
                if (game->state == Game_State_Editing) {
                    render_mode = &game->editor.render_mode;
                }
                else {
                    render_mode = &game->render_mode;
                }
                *render_mode ^= Level_Render_Mode_Items;
            }
            else if (w_param == 0x33) { // 3
                u32 *render_mode = nullptr;
                if (game->state == Game_State_Editing) {
                    render_mode = &game->editor.render_mode;
                }
                else {
                    render_mode = &game->render_mode;
                }
                *render_mode ^= Level_Render_Mode_Actors;
            }
            else if (w_param == 0x34) { // 4
                u32 *render_mode = nullptr;
                if (game->state == Game_State_Editing) {
                    render_mode = &game->editor.render_mode;
                }
                else {
                    render_mode = &game->render_mode;
                }
                *render_mode |= Level_Render_Mode_All;
            }            
            else if (w_param == 0x49) { // I
                u32 *render_mode = nullptr;
                if (game->state == Game_State_Editing) {
                    render_mode = &game->editor.render_mode;
                }
                else {
                    render_mode = &game->render_mode;
                }
                *render_mode ^= Level_Render_Mode_Actor_IDs;
            }
            else if (w_param == 0x47) { // G
                u32 *render_mode = nullptr;
                if (game->state == Game_State_Editing) {
                    render_mode = &game->editor.render_mode;
                }
                else {
                    render_mode = &game->render_mode;
                }
                *render_mode ^= Level_Render_Mode_Grid;
            }
            else if (w_param == 0x52) { // R
                reset(game);
            }
            else if (w_param == VK_BACK || (!shift_is_down && ctrl_is_down && w_param == 0x5A)) {
                // 0x5A == 'z
                if (game->state != Game_State_Editing) {
                    undo(game);
                }
            }
            else if ((shift_is_down && ctrl_is_down && w_param == 0x5A) || ctrl_is_down && w_param == 0x59) {
                // 0x59 == 'y'
                if (game->state != Game_State_Editing) {
                    redo(game);
                }
            }
            else if (w_param == VK_F5) {
                // Save
                //write_player_profiles_to_disc(game->player_profiles);
            }
            else if (w_param == VK_F6) {
                // Load
                //read_player_profiles_from_disc(game->player_profiles);
            }                
            else if (game->state == Game_State_Lost && w_param == VK_RETURN) {
                if (game->state != Game_State_Editing) {
                    reset(game);
                }
            }
            else if (game->state == Game_State_Won && w_param == VK_RETURN) {
                if (game->state != Game_State_Editing) {
                    change_to_next_level(game);
                }
            }            
            else if (w_param == VK_F1) { // DEBUG                
                if (game->state != Game_State_Editing) {
                    change_to_prev_level(game);
                }
            }
            else if (w_param == VK_F2) { // DEBUG
                if (game->state != Game_State_Editing) {
                    change_to_next_level(game);
                }
            }
            else if (w_param == VK_F3) { // DEBUG
                Level *level = nullptr;
                if (game->state == Game_State_Editing) {
                    level = &game->editor.level;
                }
                else {
                    level = &game->current_level;
                }
                level->current_map_index = level->current_map_index == 0 ? Map_Count : level->current_map_index - 1;
            }
            else if (w_param == VK_F4) { // DEBUG
                Level *level = nullptr;
                if (game->state == Game_State_Editing) {
                    level = &game->editor.level;
                }
                else {
                    level = &game->current_level;
                }
                level->current_map_index = (level->current_map_index + 1) > Map_Count ? 0 : (level->current_map_index + 1);
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
    u32 error_code = 0;

    
    //
    // Allocate a console and direct the standard output to it
#ifdef DEBUG
    //CoInitialize(nullptr);
    AllocConsole();
    FILE* pcout;
    freopen_s(&pcout, "conout$", "w", stdout);
    freopen_s(&pcout, "conout$", "w", stderr);
    HWND console_hwnd = GetConsoleWindow();
    SetWindowPos(console_hwnd, HWND_TOPMOST, 300, 500, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    game.log.print_log = true;
#endif

    game.log.print_log = true;
    game.log.flush_immediately = true;


    log_str(&game.log, "Starting initializations...");
    
   
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
            error_code = 1;
        }
        
        //
        // Create window
        if (error_code == 0) {
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
                error_code = 2;
            }

            GetClientRect(hwnd, &client_rect);
            printf("%d x %d\n", client_rect.bottom, client_rect.top);

            ShowWindow(hwnd, CmdShow);
            UpdateWindow(hwnd);
            log_str(&game.log, "win32 initialized");
        }
    }


    
    //
    // Init renderer and audio
    if (error_code == 0)
    {
        game.renderer = new Renderer_Software_win32();
        b32 result = static_cast<Renderer_Software_win32 *>(game.renderer)->init_win32(hwnd, &game.log, kWindow_Client_Area_Width, kWindow_Client_Area_Height);
        if (!result) {            
            LOG_ERROR(&game.log, "failed to initialize the software renderer", result);
            error_code = 3;
        }
        else {
            log_str(&game.log, "Renderer initialized");
             
            result = init_audio(&game.audio, &game.log);
            if (!result) {
                LOG_ERROR(&game.log, "failed to initialize the audio system", result);
                error_code = 4;
            }
            else {
                log_str(&game.log, "XAudio2 initilized");
            }
        }
    }
    

    
    //
    // Init game
    if (error_code == 0)
    {        
        if (init_game(&game)) {
            log_str(&game.log, "Game initilized");
        }
        else {            
            LOG_ERROR(&game.log, "failed to initialized game", 0);
            error_code = 5;
        }
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
    b32 running = error_code == 0;
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
        if (game.state == Game_State_Begin_Editing) {
            game.editor.mouse.left_button.curr  = Mouse_Button_Up;
            game.editor.mouse.left_button.prev  = Mouse_Button_Up;
            game.editor.mouse.right_button.curr = Mouse_Button_Up;
            game.editor.mouse.right_button.prev = Mouse_Button_Up;
        }
        
        if (game.state == Game_State_Editing || game.state == Game_State_Begin_Editing) {            
            POINT Pm;
            GetCursorPos(&Pm);
            ScreenToClient(hwnd, &Pm);
            GetClientRect(hwnd, &client_rect);

            Level_Editor *editor = &game.editor;
            Mouse_Input *mouse = &editor->mouse;
            
            if (PtInRect(&client_rect, Pm)) {
                mouse->position = V2(static_cast<f32>(Pm.x), static_cast<f32>(client_rect.bottom - Pm.y));
                mouse->is_inside = true;

                u32 constexpr key_codes[] = {VK_LBUTTON, VK_RBUTTON};
                for (u32 index = 0; index < 2; ++index) {
                    mouse->buttons[index].prev = mouse->buttons[index].curr;
                    u16 button_state = GetKeyState(key_codes[index]);

                    b32 button_is_down = button_state & 0x8000;
                    if (button_is_down && (mouse->buttons[index].prev == Mouse_Button_Up)) {
                        mouse->buttons[index].curr = Mouse_Button_Pressed;
                    }
                    else if (button_is_down && (mouse->buttons[index].prev == Mouse_Button_Pressed)) {
                        mouse->buttons[index].curr = Mouse_Button_Down;
                    }
                    else if (!button_is_down && (mouse->buttons[index].prev == Mouse_Button_Down)) {
                        mouse->buttons[index].curr = Mouse_Button_Released;
                    }
                    else if (!button_is_down && (mouse->buttons[index].prev == Mouse_Button_Released)) {
                        mouse->buttons[index].curr = Mouse_Button_Up;
                    }
                }
            }
            else {
                mouse->is_inside = false;
                mouse->position = V2(-1, -1);
            }
        }


        //
        // Update and render game
        b32 should_quit = false;
        update_and_render(&game, kFrame_Time, &should_quit);
        if (should_quit) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }


        #if 0
        //
        // Draw backbuffer to screen
        InvalidateRect(hwnd, nullptr, false);
        UpdateWindow(hwnd);
        #else
        game.renderer->draw_to_screen();
#endif


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
            printf("FPS: %u\n", frame_counter);
            frame_counter = 0;
            accumulated_frame_time = 0;
        }
#endif
    }   

    
    //
    // Quit the program
    fini_game(&game);    
    fini_audio(&game.audio);    
    delete game.renderer;

#ifdef DEBUG
    _CrtDumpMemoryLeaks();

    fclose(pcout);
    fclose(stderr);
    FreeConsole();
    //CoUninitialize();
#endif

    close_log(&game.log);
   
    return error_code;
}
