#include "backends/imgui_impl_win32.h"
#include "core/event.h"
#include "core/input.h"
#include "imgui.h"
#include "platform.h"

#if _WIN32

#include <windowsx.h>

#define SERIES_NAME "pko_window_class"

static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

InternalState& PlatformState::GetInternalState() {
  static InternalState state{};
  return state;
}

b8 PlatformState::init(const char* application_name, i32 x, i32 y, i32 width,
                       i32 height) {
  InternalState& state = GetInternalState();
  state.instance = GetModuleHandleA(0);

  // Fill out Window Class & register it
  WNDCLASSA wc;
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = WndProc;  // pointer to window procedure
  wc.hIcon = LoadIcon(state.instance, IDI_APPLICATION);
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = state.instance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = SERIES_NAME;

  if (!RegisterClassA(&wc)) {
    MessageBoxA(0, "Window registration failed", "Error",
                MB_ICONEXCLAMATION | MB_OK);
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
  RECT border_rect = {0, 0, 0, 0};
  AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

  // In this case, the border rect is negative
  window_x += border_rect.left;
  window_y += border_rect.top;

  // Grow by the size of the OS border
  window_width += border_rect.right - border_rect.left;
  window_height += border_rect.bottom - border_rect.top;

  state.handle =
      CreateWindowExA(window_ex_style, SERIES_NAME, application_name,
                      window_style, window_x, window_y, window_width,
                      window_height, nullptr, nullptr, state.instance, nullptr);

  if (!state.handle) {
    MessageBoxA(0, "Window creation failed", "Error",
                MB_ICONEXCLAMATION | MB_OK);
    return false;
  }

  b32 should_activate =
      1;  // TODO: if the window should not accept input, this should be false;
  i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;

  ShowWindow(state.handle, show_window_command_flags);

  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  clock_frequency = 1.0 / (f64)frequency.QuadPart;
  QueryPerformanceCounter(&start_time);

  return true;
}

void PlatformState::shutdown() {
  InternalState& state = GetInternalState();
  if (state.handle) {
    DestroyWindow(state.handle);
    state.handle = 0;
  }

  if (state.instance) {
    UnregisterClassA(SERIES_NAME, state.instance);
  }
}

b8 PlatformState::platform_message() {
  // Main message loop
  MSG message;
  // bool resize = false;
  // bool result = true;

  if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
    // Process events
    switch (message.message) {
        // Resize
      case WM_USER + 1:
        // resize = true;
        break;
        // Close
      case WM_USER + 2:
        return false;
    }
    TranslateMessage(&message);
    DispatchMessage(&message);
  } else {
    //// Resize
    // if (resize) {
    //     resize = false;
    //     if (!project.OnWindowSizeChanged()) {
    //         result = false;
    //         break;
    //     }
    // }
    //// Draw
    // if (project.ReadyToDraw()) {
    //     if (!project.Draw()) {
    //         result = false;
    //         break;
    //     }
    // }
    // else {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // }
  }

  return true;
}

f64 PlatformState::get_absolute_time() {
  LARGE_INTEGER current_time;
  QueryPerformanceCounter(&current_time);
  return (f64)current_time.QuadPart * clock_frequency;
}

void PlatformState::sleep(u64 ms) { Sleep(ms); }

void PlatformState::on_window_resize(u32 width, u32 height) {
  InternalState& state = GetInternalState();

  if (state.instance == nullptr) return;

  state.width = width;
  state.height = height;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

  switch (message) {
    case WM_SIZE: {
      // Get the updated size
      RECT r;
      GetClientRect(hWnd, &r);
      u32 width = r.right - r.left;
      u32 height = r.bottom - r.top;

      event_context context;
      context.data.u32[0] = width;
      context.data.u32[1] = height;

      PlatformState::on_window_resize(width, height);

      event_system::fire_event(event_code::EVENT_CODE_ONRESIZED, context);
      break;
    }
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
      b8 pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);

      InputSystem::process_key((keys)wParam, pressed);
    } break;

    case WM_MOUSEMOVE: {
      // Mouse move
      i32 x_position = GET_X_LPARAM(lParam);
      i32 y_position = GET_Y_LPARAM(lParam);

      InputSystem::process_mouse_move(x_position, y_position);
    } break;
    case WM_MOUSEWHEEL: {
      i32 z_delta = GET_WHEEL_DELTA_WPARAM(wParam);

      if (z_delta != 0) {
        z_delta = (z_delta < 0) ? -1 : 1;
        InputSystem::process_mouse_wheel(z_delta);
      }

    } break;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
      b8 pressed = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
                   message == WM_MBUTTONDOWN;
      buttons mouse_button = BUTTON_MAX_BUTTONS;
      switch (message) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
          mouse_button = BUTTON_LEFT;
          break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
          mouse_button = BUTTON_MIDDLE;
          break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
          mouse_button = BUTTON_RIGHT;
          break;
      }
      // TODO: input processing
      if (mouse_button != BUTTON_MAX_BUTTONS) {
        InputSystem::process_button(mouse_button, pressed);
      }

    } break;
    case WM_CANCELMODE:
      break;
    default:
      return DefWindowProcA(hWnd, message, wParam, lParam);
  }
  return 0;
}

#endif