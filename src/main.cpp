#include <windows.h>

LRESULT CALLBACK main_window_callback(HWND window, UINT msg, WPARAM w_param, LPARAM l_param) {
    LRESULT res = 0;

    switch (msg) {
    case WM_SIZE: {
        OutputDebugStringA("WM_SIZE\n");
        break;
    }
    case WM_DESTROY: {
        OutputDebugStringA("WM_DESTROY\n");
        break;
    }
    case WM_CLOSE: {
        OutputDebugStringA("WM_CLOSE\n");
        break;
    }
    case WM_ACTIVATEAPP: {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
        break;
    }
    default: {
        res = DefWindowProc(window, msg, w_param, l_param);
        break;
    }
    }

    return res;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_code) {
    WNDCLASS window = {
        .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = main_window_callback,
        .hInstance = instance,
        .lpszClassName = "HandmadeHerowindowClass",
    };

    if(!RegisterClass(&window)) {
        OutputDebugStringA("Failed to register window class\n");
        return 1;
    }

    HWND window_handle = CreateWindowEx(
        0,
        window.lpszClassName,
        "Handmade Hero",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        instance,
        0
    );

    if(!window_handle) {
        OutputDebugStringA("Failed to create window handle\n");
        return 1;
    }

    MSG message;
    while(true) {
        BOOL message_result = GetMessage(&message, 0, 0, 0);
        if(message_result > 0) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        } else {
            break;
        }
    }



    return 0;
}
