#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

#include "defines.h"

struct file_handle {
    void* f = nullptr;
    char* str = nullptr;
    u32 size = 0;
};

bool read(const char* file_path, file_handle* file);
void close(file_handle* file);


#endif // !FILE_HANDLE_H
