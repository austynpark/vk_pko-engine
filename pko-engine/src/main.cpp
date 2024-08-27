#include <vector>

#include "core/application.h"

int main() {
	App::init("pko-engine", 100, 100, 1280, 720);

	while (App::run()) {
	}

	App::shutdown();

	return 0;
}

