#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

#include "defines.h"
#include <string>

struct file_handle {
    void* f = nullptr;
    char* str = nullptr;
    u32 size = 0;
};

bool pko_file_read(const char* file_path, file_handle* file);
void pko_file_close(file_handle* file);

void read_file(char* out_buffer, const std::string& filename);
const char* get_file_extension(const char* filename);


#endif // !FILE_HANDLE_H
