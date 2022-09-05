#include <vector>

#include "core/application.h"

static application app;

int main() {
	app.init("pko-engine", 100, 100, 1280, 720);

	while (app.run()) {
	}

	app.shutdown();

	return 0;
}

