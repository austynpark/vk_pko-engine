#include "input.h"

#include "event.h"

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
    if (state_ptr && state_ptr->keyboard.keys[key_code] != is_pressed) {
        // Update internal state_ptr->
        state_ptr->keyboard.keys[key_code] = is_pressed;

        // Fire off an event for immediate processing.
        event_context context;
        context.data.u16[0] = key_code;
        event_system::fire_event(is_pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, context);
        
    }
}