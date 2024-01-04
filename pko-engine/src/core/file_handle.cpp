#define _CRT_SECURE_NO_WARNINGS
#include "file_handle.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>

void read_file(char* out_buffer, const std::string& filename) {
	std::ifstream file(filename, std::ios::ate);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
    out_buffer = new char[fileSize];
	file.seekg(0);
	file.read(out_buffer, fileSize);
	file.close();
}

void deallocate_file(char* out_buffer) {
    if (out_buffer == nullptr)
        throw std::runtime_error("failed to close file!");



}

b8 pko_file_read(const char* file_path, file_handle* file)
{

    if ((FILE*)file->f != nullptr) {
        pko_file_close(file);
    }

    file->f = fopen(file_path, "rb");
    
    if (file->f == 0) {
        printf("cant open file %s", file_path);
        return false;
    }

#ifdef _DEBUG
    printf("Loading : %s\n", file_path);
#endif  //_DEBUG

    fseek((FILE*)file->f, 0, SEEK_END);
    file->size = ftell((FILE*)file->f);
    fseek((FILE*)file->f, 0, SEEK_SET);

    file->str = (char*)malloc(file->size + 1);

    if (file->str == nullptr) return false;

    fread(file->str, 1, file->size, (FILE*)file->f);

    file->str[file->size - 1] = 0;

    return true;
}

void pko_file_close(file_handle* file)
{
    if ((FILE*)file->f != nullptr) {
        fclose((FILE*)file->f);
        free(file->str);
        file->str = nullptr;
    }
}

/*
1. read file line by line
2. find layout with set, binding or push_constant
    if set == 0
    - set to global_ubo
    else if set == 1
    - instance ubo
    else
    
*/
