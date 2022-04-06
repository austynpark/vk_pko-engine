#include <vector>

#include "core/application.h"

int main() {
	application::init("pko-engine", 100, 100, 1980, 1080);

	while (application::run()) {
	}

	application::shutdown();

	return 0;
}

