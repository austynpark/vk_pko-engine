#define _CRT_SECURE_NO_WARNINGS
#include "file_handle.h"

#include <stdio.h>
#include <stdlib.h>

b8 pko_file_read(const char* file_path, file_handle* file)
{

    if ((FILE*)file->f != nullptr) {
        pko_file_close(file);
    }

    file->f = fopen(file_path, "rb");

#ifdef _DEBUG
    printf("Loading : %s\n", file_path);
#endif  //_DEBUG

    fseek((FILE*)file->f, 0, SEEK_END);
    file->size = ftell((FILE*)file->f);
    fseek((FILE*)file->f, 0, SEEK_SET);

    file->str = (char*)malloc(file->size + 1);

    if (file->str == nullptr) return false;

    fread(file->str, 1, file->size, (FILE*)file->f);

    file->str[file->size] = 0;

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
