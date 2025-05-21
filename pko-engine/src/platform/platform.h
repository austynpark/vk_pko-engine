#ifndef PLATFORM_H
#define PLATFORM_H

#include "defines.h"

#ifdef _WIN32

#include <windows.h>

typedef HMODULE vulkan_library;

typedef struct InternalState {
  HINSTANCE instance;
  HWND handle;

  u32 width;
  u32 height;
} InternalState;

#endif  //_WIN32

struct PlatformState {
 public:
  static InternalState& GetInternalState();
  static void on_window_resize(u32 width, u32 height);
  b8 init(const char* application_name, i32 x, i32 y, i32 width, i32 height);
  void shutdown();

  b8 platform_message();

  f64 get_absolute_time();

  // Sleep on the main thread for the microsecond
  // only use for giving time back to the OS for unused update power
  void sleep(u64 ms);
};

#endif  // !PLATFORM_H
