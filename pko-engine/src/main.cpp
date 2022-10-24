#include <vector>

#include "core/application.h"

int main() {
	application::init("pko-engine", 100, 100, 1280, 720);

	
	application::run();
	application::shutdown();

	return 0;
}

