#include "event.h"

#include <algorithm>

static std::vector<PFN_event> registered_events[EVENT_CODE_MAX];

b8 event_system::bind_event(event_code code, PFN_event func_ptr)
{
	if (code >= EVENT_CODE_MAX)
		return false;

	// if the function is already bounded, return false
	for (const auto& event_registered : registered_events[code]) {
		if (func_ptr == event_registered) {
			return false;
		}
	}

	registered_events[code].push_back(func_ptr);

	return true;
}

b8 event_system::unbind_event(event_code code, PFN_event func_ptr)
{
	if (code >= EVENT_CODE_MAX)
		return false;

	for (auto& event_registered : registered_events[code]) {
		if (event_registered == func_ptr) {
			if (event_registered != registered_events[code].back()) {
				std::swap(event_registered, registered_events[code].back());
			}
				
			registered_events[code].pop_back();

			return true;
		}
	}

	return false;
}

b8 event_system::fire_event(event_code code, const event_context& context)
{
	if (code >= EVENT_CODE_MAX)
		return false;


	for (const auto& event_registered : registered_events[code]) {
		b8 result = event_registered(code, context);

		if (result != true)
			return false;
	}

	return true;
}
