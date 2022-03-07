#ifndef PLATFORM_H
#define PLATFORM_H

#include "defines.h"

#ifdef _WIN32

#include <Windows.h>
#include <windowsx.h>

typedef HMODULE vulkan_library;

typedef struct internal_state {
    HINSTANCE instance;
    HWND      handle;
} internal_state;

#endif

struct platform_state {
    public:
        b8 init(const char* application_name, i32 x, i32 y, i32 width, i32 height);
        void shutdown();

        b8 platform_message();

        f64 get_absolute_time();

        // Sleep on the main thread for the microsecond
        // only use for giving time back to the OS for unused update power
        void sleep(u64 ms);

        // Internal state
        void* state;
    private:
        u32 width;
        u32 height;
};

#endif // !PLATFORM_H

