#include "platform.h"

#if _WIN32

#define SERIES_NAME "pko_window_class"

static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

b8 platform_state::init(
    const char* application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height) {

    state = malloc(sizeof(internal_state));
    internal_state* _state = (internal_state*)state;

    _state->instance = GetModuleHandleA(0);

    // Fill out Window Class & register it
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc; // pointer to window procedure
    wc.hIcon = LoadIcon(_state->instance, IDI_APPLICATION);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = _state->instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = SERIES_NAME;

    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // size not including the border
    u32 client_x = x;
    u32 client_y = y;
    u32 client_width = width;
    u32 client_height = height;

    u32 window_x = client_x;
    u32 window_y = client_y;
    u32 window_width = client_width;
    u32 window_height = client_height;

    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    // Set style of the window & inidividual buttons
    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;


    // size of the border
    RECT border_rect = { 0,0,0,0 };
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    // In this case, the border rect is negative
    window_x += border_rect.left;
    window_y += border_rect.top;

    // Grow by the size of the OS border
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    _state->handle = CreateWindowExA(window_ex_style, SERIES_NAME, application_name, window_style, window_x, window_y, window_width, window_height, nullptr, nullptr, _state->instance, nullptr);

    if (!_state->handle) {
        MessageBoxA(0, "Window creation failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    b32 should_activate = 1; //TODO: if the window should not accept input, this should be false;
    i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;

    ShowWindow(_state->handle, show_window_command_flags);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    return true;
}

void platform_state::shutdown() {

    internal_state* _state = (internal_state*)state;

    if (_state->handle) {
        DestroyWindow(_state->handle);
        _state->handle = 0;
    }

    if (_state->instance) {
        UnregisterClassA(SERIES_NAME, _state->instance);
    }

    free(state);
}

b8 platform_state::platform_message() {
    // Main message loop
    MSG message;
    //bool resize = false;
    //bool result = true;

    if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
        // Process events
        switch (message.message) {
            // Resize
        case WM_USER + 1:
            //resize = true;
            break;
            // Close
        case WM_USER + 2:
            return false;
        }
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    else {
        //// Resize
        //if (resize) {
        //    resize = false;
        //    if (!project.OnWindowSizeChanged()) {
        //        result = false;
        //        break;
        //    }
        //}
        //// Draw
        //if (project.ReadyToDraw()) {
        //    if (!project.Draw()) {
        //        result = false;
        //        break;
        //    }
        //}
        //else {
        //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //}
    }


    return true;
}

f64 platform_state::get_absolute_time() {
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    return (f64)current_time.QuadPart * clock_frequency;
}

void platform_state::sleep(u64 ms) {
    Sleep(ms);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        // Get the updated size
        //RECT r;
        //GetClientRect(hwnd, &r);
        //u32 width = r.right - r.left;
        //u32 height = r.bottom - r.top;

        //TODO: Fire an event for window resize.
    case WM_EXITSIZEMOVE:
        PostMessage(hWnd, WM_USER + 1, wParam, lParam);
        break;
    case WM_CLOSE:
        PostMessage(hWnd, WM_USER + 2, wParam, lParam);
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        // Key pressed/released
        // bool pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
        // TODO: input processing
    } break;

    case WM_MOUSEMOVE: {
        // Mouse move
        //i32 x_position = GET_X_LPARAM(lParam);
        //i32 y_position = GET_Y_LPARAM(lParam);

        //TODO: input processing
    } break;
    case WM_MOUSEWHEEL: {
        i32 z_delta = GET_WHEEL_DELTA_WPARAM(wParam);

        if (z_delta != 0) {
            z_delta = (z_delta < 0) ? -1 : 1;
        }

        //TODO: input processing 
    } break;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
        //b8 pressed = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN;
        //TODO: input processing
    } break;

    default:
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

/*
//NOTE: Vulkan specific
void platform_create_vulkan_surface(vulkan_parameters* vulkan_parameter, platform_state* plat_state) {

    internal_state* state = (internal_state*)plat_state->state;

    VkWin32SurfaceCreateInfoKHR create_info{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    create_info.hinstance = state->instance;
    create_info.hwnd = state->handle;

    VK_CHECK(vkCreateWin32SurfaceKHR(vulkan_parameter->instance, &create_info, vulkan_parameter->allocator, &vulkan_parameter->surface));
}
*/

#endif