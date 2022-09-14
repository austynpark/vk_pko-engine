#ifndef EVENT_H
#define EVENT_H

#include <vector>

#include "defines.h"

struct event_context;

enum event_code {
    EVENT_CODE_KEY_RELEASED,
    EVENT_CODE_KEY_PRESSED,
    EVENT_CODE_BUTTON_PRESSED,
    EVENT_CODE_BUTTON_RELEASED,
    EVENT_CODE_MOUSE_MOVED,
    EVENT_CODE_MOUSE_WHEEL,
    EVENT_CODE_ONRESIZED,
    EVENT_CODE_MAX
};

typedef b8 (*PFN_event)(u16 code, event_context context);

class event_system {
public:
    static b8 bind_event(event_code code, PFN_event func_ptr);
    static b8 unbind_event(event_code code, PFN_event func_ptr);

    static b8 fire_event(event_code code, const event_context& context);
};

struct event_context {
	union {
        /** @brief An array of 2 64-bit signed integers. */
        i64 i64[2];
        /** @brief An array of 2 64-bit unsigned integers. */
        u64 u64[2];

        /** @brief An array of 2 64-bit floating-point numbers. */
        f64 f64[2];

        /** @brief An array of 4 32-bit signed integers. */
        i32 i32[4];
        /** @brief An array of 4 32-bit unsigned integers. */
        u32 u32[4];
        /** @brief An array of 4 32-bit floating-point numbers. */
        f32 f32[4];

        /** @brief An array of 8 16-bit signed integers. */
        i16 i16[8];

        /** @brief An array of 8 16-bit unsigned integers. */
        u16 u16[8];

        /** @brief An array of 16 8-bit signed integers. */
        i8 i8[16];
        /** @brief An array of 16 8-bit unsigned integers. */
        u8 u8[16];

        /** @brief An array of 16 characters. */
        char c[16];
	} data;
};


#endif // !EVENT_H
