#include "input.h"

#include "event.h"

#include <iostream>

std::unique_ptr<input_system::input_state> input_system::state_ptr;

input_system::input_system()
{
	state_ptr = std::make_unique<input_state>();
}

input_system::~input_system()
{
}

void input_system::process_key(keys key_code, b8 is_pressed)
{
    // Only handle this if the state actually changed.
    if (state_ptr && state_ptr->keyboard_current.keys[key_code] != is_pressed) {
        // Update internal state_ptr->
        state_ptr->keyboard_current.keys[key_code] = is_pressed;

        if (key_code == KEY_SPACE) {
            if (is_pressed)
                std::cout << "space is pressed" << std::endl;
            else
                std::cout << "space is released" << std::endl;
        }

        // Fire off an event for immediate processing.
        event_context context;
        context.data.u16[0] = key_code;
        event_system::fire_event(is_pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, context);
        
    }
}

void input_system::process_button(buttons button, b8 pressed) {
    // If the state changed, fire an event.
    if (state_ptr->mouse_current.buttons[button] != pressed) {
        state_ptr->mouse_current.buttons[button] = pressed;

        // Fire the event.
        event_context context;
        context.data.u16[0] = button;
        event_system::fire_event(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, context);
    }
}

void input_system::process_mouse_move(i16 x, i16 y) {
    // Only process if actually different
    if (state_ptr->mouse_current.x != x || state_ptr->mouse_current.y != y) {
        // NOTE: Enable this if debugging.
        //std::cout << "Mouse pos:" << x << ", " <<  y << std::endl;

        // Update internal state_ptr->
        state_ptr->mouse_current.x = x;
        state_ptr->mouse_current.y = y;

        // Fire the event.
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_system::fire_event(EVENT_CODE_MOUSE_MOVED, context);
    }
}

void input_system::process_mouse_wheel(i8 z_delta) {
    // NOTE: no internal state to update.

    // Fire the event.
    event_context context;
    context.data.i8[0] = z_delta;
    event_system::fire_event(EVENT_CODE_MOUSE_WHEEL, context);
}

b8 input_system::is_key_down(keys key) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->keyboard_current.keys[key] == true;
}

b8 input_system::is_key_up(keys key) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->keyboard_current.keys[key] == false;
}

b8 input_system::was_key_down(keys key) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->keyboard_previous.keys[key] == true;
}

b8 input_system::was_key_up(keys key) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->keyboard_previous.keys[key] == false;
}

// mouse input
b8 input_system::is_button_down(buttons button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_current.buttons[button] == true;
}

b8 input_system::is_button_up(buttons button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_current.buttons[button] == false;
}

b8 input_system::was_button_down(buttons button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_previous.buttons[button] == true;
}

b8 input_system::was_button_up(buttons button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_previous.buttons[button] == false;
}

void input_system::get_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_current.x;
    *y = state_ptr->mouse_current.y;
}

void input_system::get_previous_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_previous.x;
    *y = state_ptr->mouse_previous.y;
}