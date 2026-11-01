#include "file_loader.h"

typedef struct {
    int width;
    int height;
    unsigned char *data; // RGBA
} Image;


char *file_load(char *path , int *length){

    FILE *f = fopen(path , "rb");

    if(!f){
        printf("cannot find file: %s\n" , path);
        return 0;
    }

    fseek(f , 0 , SEEK_END);
    int size = ftell(f);
    fseek(f , SEEK_SET , 0);

    char * str = malloc(size + 1);
    
    if (!str){
        printf("FAILED TO MALLOC\n");
        fclose(f);
        return NULL;
    }

    str[size] = '\0';
    fread(str, size , 1 , f);

    fclose(f);

    if (length){
        *length = size;
    }

    return str;
}


char **list_files_in_folder(const char *folder_path, int *count) {
    DIR *dir;
    struct dirent *entry;

    char **files = NULL;
    int capacity = 10;
    int size = 0;

    *count = 0;

    dir = opendir(folder_path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }

    files = malloc(capacity * sizeof(char *));
    if (!files) {
        closedir(dir);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        // skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (size >= capacity) {
            capacity *= 2;
            char **tmp = realloc(files, capacity * sizeof(char *));
            if (!tmp) {
                // cleanup on failure
                for (int i = 0; i < size; i++) free(files[i]);
                free(files);
                closedir(dir);
                return NULL;
            }
            files = tmp;
        }

        files[size] = strdup(entry->d_name);
        if (!files[size]) {
            for (int i = 0; i < size; i++) free(files[i]);
            free(files);
            closedir(dir);
            return NULL;
        }

        size++;
    }

    closedir(dir);

    *count = size;
    return files;
}

char **list_files_in_folder_recursive(const char *folder_path, int *count)
{
    char **files = NULL;
    int file_count = 0;
    int file_capacity = 10;
    
    char **dirs = NULL;
    int dir_count = 0;
    int dir_capacity = 10;

    *count = 0;

    files = malloc(file_capacity * sizeof(char *));
    dirs = malloc(dir_capacity * sizeof(char *));

    if (!files || !dirs) {
        free(files);
        free(dirs);
        return NULL;
    }

    // Start with the requested directory
    dirs[dir_count++] = strdup(folder_path);

    if (!dirs[0]) {
        free(files);
        free(dirs);
        return NULL;
    }

    while (dir_count > 0) {
        char *current_dir = dirs[--dir_count];

        DIR *dir = opendir(current_dir);
        if (!dir) {
            perror("opendir");
            free(current_dir);
            continue;
        }

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Build full path
            size_t path_len =
                strlen(current_dir) + 1 + strlen(entry->d_name) + 1;

            char *full_path = malloc(path_len);
            if (!full_path) {
                closedir(dir);
                free(current_dir);

                for (int i = 0; i < file_count; i++)
                    free(files[i]);

                for (int i = 0; i < dir_count; i++)
                    free(dirs[i]);

                free(files);
                free(dirs);
                return NULL;
            }

            snprintf(
                full_path,
                path_len,
                "%s/%s",
                current_dir,
                entry->d_name
            );

            struct stat st;

            if (stat(full_path, &st) != 0) {
                perror("stat");
                free(full_path);
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                if (dir_count >= dir_capacity) {
                    dir_capacity *= 2;

                    char **tmp = realloc(
                        dirs,
                        dir_capacity * sizeof(char *)
                    );

                    if (!tmp) {
                        free(full_path);
                        closedir(dir);
                        free(current_dir);

                        for (int i = 0; i < file_count; i++)
                            free(files[i]);

                        for (int i = 0; i < dir_count; i++)
                            free(dirs[i]);

                        free(files);
                        free(dirs);
                        return NULL;
                    }

                    dirs = tmp;
                }

                dirs[dir_count++] = full_path;
            }
            else {
                // It's a file: add it to the results
                if (file_count >= file_capacity) {
                    file_capacity *= 2;

                    char **tmp = realloc(
                        files,
                        file_capacity * sizeof(char *)
                    );

                    if (!tmp) {
                        free(full_path);
                        closedir(dir);
                        free(current_dir);

                        for (int i = 0; i < file_count; i++)
                            free(files[i]);

                        for (int i = 0; i < dir_count; i++)
                            free(dirs[i]);

                        free(files);
                        free(dirs);
                        return NULL;
                    }

                    files = tmp;
                }

                files[file_count++] = full_path;
            }
        }

        closedir(dir);
        free(current_dir);
    }

    free(dirs);

    *count = file_count;
    return files;
}

unsigned char *load_png_rgba(const char *filename, int *out_width, int *out_height) {

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    unsigned char header[8];
    fread(header, 1, 8, fp);

    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        return NULL;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    if (!png || !info) {
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);

    png_read_info(png, info);

    int width  = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth  = png_get_bit_depth(png, info);

    // Normalize everything to 8-bit RGBA
    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    // Allocate output RGBA buffer
    unsigned char *data = (unsigned char *)malloc(width * height * 4);
    if (!data) {
        fclose(fp);
        return NULL;
    }

    png_bytep *rows = (png_bytep *)malloc(sizeof(png_bytep) * height);
    if (!rows) {
        free(data);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }
    for (int y = 0; y < height; y++) {
        rows[y] = data + y * width * 4;
    }

    png_read_image(png, rows);

    free(rows);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    *out_width = width;
    *out_height = height;

    return data;
}


char **get_lines(char *buffer, int *line_number) {
    if (!buffer || !line_number) {
        return NULL;
    }

    *line_number = 0;
    int capacity = 1024;
    char **lines = malloc(sizeof(char *) * capacity);
    if (!lines) {
        return NULL;
    }

    char *start = buffer;
    for (char *c = buffer;; c++) {
        if (*c == '\n' || (*c == '\0' && c > start)) {
            char *line_end = c;
            while (start < line_end && (*start == ' ' || *start == '\t')) {
                start++;
            }
            while (line_end > start &&
                   (line_end[-1] == ' ' || line_end[-1] == '\t' || line_end[-1] == '\r')) {
                line_end--;
            }
            size_t line_len = (size_t)(line_end - start);

            if (line_len == 0) {
                if (*c == '\0') {
                    break;
                }
                start = c + 1;
                continue;
            }

            if (*line_number >= capacity) {
                int new_capacity = capacity * 2;
                char **tmp = realloc(lines, sizeof(char *) * new_capacity);
                if (!tmp) {
                    for (int i = 0; i < *line_number; i++) {
                        free(lines[i]);
                    }
                    free(lines);
                    return NULL;
                }
                lines = tmp;
                capacity = new_capacity;
            }

            char *line_start = malloc(line_len + 1);
            if (!line_start) {
                for (int i = 0; i < *line_number; i++) {
                    free(lines[i]);
                }
                free(lines);
                return NULL;
            }

            memcpy(line_start, start, line_len);
            line_start[line_len] = '\0';
            lines[*line_number] = line_start;
            (*line_number)++;

            if (*c == '\0') {
                break;
            }

            start = c + 1;
        } else if (*c == '\0') {
            break;
        }
    }

    return lines;
}

char **load_file_list(char *file_path, int *file_count) {
    if (!file_path || !file_count) {
        return NULL;
    }

    int length = 0;
    char *file_list_buffer = file_load(file_path, &length);

    if (!file_list_buffer) {
        printf("Failed to load file list: %s\n", file_path);
        return NULL;
    }

    char **ret_value = get_lines(file_list_buffer, file_count);
    free(file_list_buffer);
    return ret_value;
}
