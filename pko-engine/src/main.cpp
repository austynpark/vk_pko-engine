#include <vector>

#include "core/application.h"

int main() {
	app.init("pko-engine", 100, 100, 1280, 720);

	while (application::run()) {
	}

	application::shutdown();

	return 0;
}

