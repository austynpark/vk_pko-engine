#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#include "object.h"

#include <string>
#include <unordered_map>
#include <memory>

struct object_manager {
	std::unordered_map<std::string, std::unique_ptr<object>> objects;
};

#endif // !OBJECT_MANAGER_H
