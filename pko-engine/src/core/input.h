#pragma once

/*
* 1. function pointer
* 2. input event function form (key_code, b8 is_pressed)
* 3. std::unordered_map<key_code, std::vector<void*>
*/

#include <memory>

#include "defines.h"

enum keys;
enum buttons {
    /** @brief The left mouse button */
    BUTTON_LEFT,
    /** @brief The right mouse button */
    BUTTON_RIGHT,
    /** @brief The middle mouse button (typically the wheel) */
    BUTTON_MIDDLE,
    BUTTON_MAX_BUTTONS
};

class InputSystem {

private:
    struct keyboard_state {
        b8 keys[256];
    };

    struct mouse_state {
        i16 x;
        i16 y;
        u8 buttons[BUTTON_MAX_BUTTONS];
    };

    struct input_state {
        keyboard_state keyboard_current;
        mouse_state mouse_current;
        keyboard_state keyboard_previous;
        mouse_state mouse_previous;
    };

public:
    InputSystem();
    ~InputSystem();

    static void process_key(keys key_code, b8 is_pressed);

    // mouse input
    static void get_previous_mouse_position(i32* x, i32* y);
    static void get_mouse_position(i32* x, i32* y);
    static b8 was_button_up(buttons button);

    static void process_button(buttons button, b8 pressed);
    static void process_mouse_move(i16 x, i16 y);
    static void process_mouse_wheel(i8 z_delta);

    static b8 is_key_down(keys key);
    static b8 is_key_up(keys key);
    static b8 was_key_down(keys key);
    static b8 was_key_up(keys key);
    static b8 is_button_down(buttons button);
    static b8 is_button_up(buttons button);
    static b8 was_button_down(buttons button);

    static std::unique_ptr<input_state> state_ptr;

};

enum keys {
    /** @brief The backspace key. */
    KEY_BACKSPACE = 0x08,
    /** @brief The enter key. */
    KEY_ENTER = 0x0D,
    /** @brief The tab key. */
    KEY_TAB = 0x09,
    /** @brief The shift key. */
    KEY_SHIFT = 0x10,
    /** @brief The Control/Ctrl key. */
    KEY_CONTROL = 0x11,

    /** @brief The pause key. */
    KEY_PAUSE = 0x13,
    /** @brief The Caps Lock key. */
    KEY_CAPITAL = 0x14,

    /** @brief The Escape key. */
    KEY_ESCAPE = 0x1B,

    KEY_CONVERT = 0x1C,
    KEY_NONCONVERT = 0x1D,
    KEY_ACCEPT = 0x1E,
    KEY_MODECHANGE = 0x1F,

    /** @brief The spacebar key. */
    KEY_SPACE = 0x20,
    KEY_PRIOR = 0x21,
    KEY_NEXT = 0x22,
    /** @brief The end key. */
    KEY_END = 0x23,
    /** @brief The home key. */
    KEY_HOME = 0x24,
    /** @brief The left arrow key. */
    KEY_LEFT = 0x25,
    /** @brief The up arrow key. */
    KEY_UP = 0x26,
    /** @brief The right arrow key. */
    KEY_RIGHT = 0x27,
    /** @brief The down arrow key. */
    KEY_DOWN = 0x28,
    KEY_SELECT = 0x29,
    KEY_PRINT = 0x2A,
    //KEY_EXECUTE = 0x2B,
    KEY_SNAPSHOT = 0x2C,
    /** @brief The insert key. */
    KEY_INSERT = 0x2D,
    /** @brief The delete key. */
    KEY_DELETE = 0x2E,
    KEY_HELP = 0x2F,

    /** @brief The A key. */
    KEY_A = 0x41,
    /** @brief The B key. */
    KEY_B = 0x42,
    /** @brief The C key. */
    KEY_C = 0x43,
    /** @brief The D key. */
    KEY_D = 0x44,
    /** @brief The E key. */
    KEY_E = 0x45,
    /** @brief The F key. */
    KEY_F = 0x46,
    /** @brief The G key. */
    KEY_G = 0x47,
    /** @brief The H key. */
    KEY_H = 0x48,
    /** @brief The I key. */
    KEY_I = 0x49,
    /** @brief The J key. */
    KEY_J = 0x4A,
    /** @brief The K key. */
    KEY_K = 0x4B,
    /** @brief The L key. */
    KEY_L = 0x4C,
    /** @brief The M key. */
    KEY_M = 0x4D,
    /** @brief The N key. */
    KEY_N = 0x4E,
    /** @brief The O key. */
    KEY_O = 0x4F,
    /** @brief The P key. */
    KEY_P = 0x50,
    /** @brief The Q key. */
    KEY_Q = 0x51,
    /** @brief The R key. */
    KEY_R = 0x52,
    /** @brief The S key. */
    KEY_S = 0x53,
    /** @brief The T key. */
    KEY_T = 0x54,
    /** @brief The U key. */
    KEY_U = 0x55,
    /** @brief The V key. */
    KEY_V = 0x56,
    /** @brief The W key. */
    KEY_W = 0x57,
    /** @brief The X key. */
    KEY_X = 0x58,
    /** @brief The Y key. */
    KEY_Y = 0x59,
    /** @brief The Z key. */
    KEY_Z = 0x5A,

    /** @brief The left Windows/Super key. */
    KEY_LWIN = 0x5B,
    /** @brief The right Windows/Super key. */
    KEY_RWIN = 0x5C,
    KEY_APPS = 0x5D,

    /** @brief The sleep key. */
    KEY_SLEEP = 0x5F,

    /** @brief The numberpad 0 key. */
    KEY_NUMPAD0 = 0x60,
    /** @brief The numberpad 1 key. */
    KEY_NUMPAD1 = 0x61,
    /** @brief The numberpad 2 key. */
    KEY_NUMPAD2 = 0x62,
    /** @brief The numberpad 3 key. */
    KEY_NUMPAD3 = 0x63,
    /** @brief The numberpad 4 key. */
    KEY_NUMPAD4 = 0x64,
    /** @brief The numberpad 5 key. */
    KEY_NUMPAD5 = 0x65,
    /** @brief The numberpad 6 key. */
    KEY_NUMPAD6 = 0x66,
    /** @brief The numberpad 7 key. */
    KEY_NUMPAD7 = 0x67,
    /** @brief The numberpad 8 key. */
    KEY_NUMPAD8 = 0x68,
    /** @brief The numberpad 9 key. */
    KEY_NUMPAD9 = 0x69,
    /** @brief The numberpad multiply key. */
    KEY_MULTIPLY = 0x6A,
    /** @brief The numberpad add key. */
    KEY_ADD = 0x6B,
    /** @brief The numberpad separator key. */
    KEY_SEPARATOR = 0x6C,
    /** @brief The numberpad subtract key. */
    KEY_SUBTRACT = 0x6D,
    /** @brief The numberpad decimal key. */
    KEY_DECIMAL = 0x6E,
    /** @brief The numberpad divide key. */
    KEY_DIVIDE = 0x6F,

    /** @brief The F1 key. */
    KEY_F1 = 0x70,
    /** @brief The F2 key. */
    KEY_F2 = 0x71,
    /** @brief The F3 key. */
    KEY_F3 = 0x72,
    /** @brief The F4 key. */
    KEY_F4 = 0x73,
    /** @brief The F5 key. */
    KEY_F5 = 0x74,
    /** @brief The F6 key. */
    KEY_F6 = 0x75,
    /** @brief The F7 key. */
    KEY_F7 = 0x76,
    /** @brief The F8 key. */
    KEY_F8 = 0x77,
    /** @brief The F9 key. */
    KEY_F9 = 0x78,
    /** @brief The F10 key. */
    KEY_F10 = 0x79,
    /** @brief The F11 key. */
    KEY_F11 = 0x7A,
    /** @brief The F12 key. */
    KEY_F12 = 0x7B,
    /** @brief The F13 key. */
    KEY_F13 = 0x7C,
    /** @brief The F14 key. */
    KEY_F14 = 0x7D,
    /** @brief The F15 key. */
    KEY_F15 = 0x7E,
    /** @brief The F16 key. */
    KEY_F16 = 0x7F,
    /** @brief The F17 key. */
    KEY_F17 = 0x80,
    /** @brief The F18 key. */
    KEY_F18 = 0x81,
    /** @brief The F19 key. */
    KEY_F19 = 0x82,
    /** @brief The F20 key. */
    KEY_F20 = 0x83,
    /** @brief The F21 key. */
    KEY_F21 = 0x84,
    /** @brief The F22 key. */
    KEY_F22 = 0x85,
    /** @brief The F23 key. */
    KEY_F23 = 0x86,
    /** @brief The F24 key. */
    KEY_F24 = 0x87,

    /** @brief The number lock key. */
    KEY_NUMLOCK = 0x90,

    /** @brief The scroll lock key. */
    KEY_SCROLL = 0x91,

    /** @brief The numberpad equal key. */
    KEY_NUMPAD_EQUAL = 0x92,

    /** @brief The left shift key. */
    KEY_LSHIFT = 0xA0,
    /** @brief The right shift key. */
    KEY_RSHIFT = 0xA1,
    /** @brief The left control key. */
    KEY_LCONTROL = 0xA2,
    /** @brief The right control key. */
    KEY_RCONTROL = 0xA3,
    /** @brief The left alt key. */
    KEY_LALT = 0xA4,
    /** @brief The right alt key. */
    KEY_RALT = 0xA5,

    /** @brief The semicolon key. */
    KEY_SEMICOLON = 0xBA,
    /** @brief The plus key. */
    KEY_PLUS = 0xBB,
    /** @brief The comma key. */
    KEY_COMMA = 0xBC,
    /** @brief The minus key. */
    KEY_MINUS = 0xBD,
    /** @brief The period key. */
    KEY_PERIOD = 0xBE,
    /** @brief The slash key. */
    KEY_SLASH = 0xBF,

    /** @brief The grave key. */
    KEY_GRAVE = 0xC0,

    KEYS_MAX_KEYS
};

