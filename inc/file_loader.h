#ifndef file_loader_h
#define file_loader_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

char *file_load(char *path , int *length);
char **list_files_in_folder(const char *folder_path, int *count);
unsigned char *load_png_rgba(const char *filename, int *out_width, int *out_height);
char **list_files_in_folder_recursive(const char *folder_path, int *count);
char **load_file_list(char *file_path, int *file_count);

#endif