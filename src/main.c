#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <locale.h>

#include "falsus.h"

typedef int Errno;

#define return_defer(value) do { result = (value); goto defer; } while (0)

static Errno read_entire_file(const char *file_path, char **buffer, size_t *buffer_size)
{
    Errno result = 0;
    FILE *f = NULL;

    f = fopen(file_path, "rb");
    if (f == NULL) return_defer(errno);
    if (fseek(f, 0, SEEK_END) < 0) return_defer(errno);
    long m = ftell(f);
    if (m < 0) return_defer(errno);
    if (fseek(f, 0, SEEK_SET) < 0) return_defer(errno);
    *buffer_size = m;
    *buffer = malloc(*buffer_size + 1);
    if (fread(*buffer, *buffer_size, 1, f) != 1) return_defer(errno);
    (*buffer)[*buffer_size] = '\0';
    if (ferror(f)) return_defer(errno);

defer:
    if (f) fclose(f);
    return result;
}

static void parse_command_line_args(char ***args, char **input_file, char **output_directory)
{
    (*args)++;
    *input_file = **args;
    (*args)++;
    *output_directory = **args;

    if (access(*output_directory, F_OK) != 0) {
        fprintf(stderr, "ERROR: Directory `%s` does not exist\n", *output_directory);
        exit(1);
    }
    if (access(*input_file, F_OK) != 0) {
        fprintf(stderr, "ERROR: File `%s` does not exist\n", *input_file);
        exit(1);
    }

    if ((*output_directory)[strlen(*output_directory)-1] != '/') { // Ensure that the directory ends with /
        strncat(*output_directory, "/", 1);
    }
}

static void print_usage(const char *program)
{
    fprintf(stderr, "USAGE: %s <reports_file> <output directory>\n", program);
}

static Falsus falsus = {0};

int main(int argc, char **argv)
{
    const char *program = *argv;
    if (argc < 3) {
        fprintf(stderr, "ERROR: Not enough arguments were provided\n");
        print_usage(program);
        return 1;
    } else if (argc > 3) {
        fprintf(stderr, "ERROR: Too much arguments were provided\n");
        print_usage(program);
        return 1;
    }

    setlocale(LC_TIME, "de_DE.utf8");

    char *input_file;
    char *output_directory;
    parse_command_line_args(&argv, &input_file, &output_directory);

    char *content = NULL;
    size_t content_size = 0;
    Errno err = read_entire_file(input_file, &content, &content_size);
    if (err != 0) {
        fprintf(stderr, "ERROR: Failed to read file %s: %s\n", input_file, strerror(err));
        return 1;
    }

    falsus_init(&falsus, content, content_size, input_file);
    falsus_write(&falsus, output_directory);

    if (content) free(content);
    return 0;
}
