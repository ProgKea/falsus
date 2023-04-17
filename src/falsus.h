#ifndef FALSUS_H_
#define FALSUS_H_

#include "config.h"

#include <stddef.h>
#include <time.h>

typedef enum {
    SECTION_COMPANY = 0,
    SECTION_SCHOOL,
    COUNT_SECTIONS,
} Section_Kind;

#define REPORT_CAP 100
#define WEEK_COUNT 4
#define WEEK_ITEM_CAP 10
#define WEEK_CHAR_CAP 1024

typedef struct {
    char item[WEEK_ITEM_CAP][WEEK_CHAR_CAP];
} Week;

typedef struct {
    Section_Kind kind; // TODO: I think this is not needed
    Week weeks[WEEK_COUNT];
} Section;

typedef struct {
    char year[100];
    char month[100];
    int count;
    Section sections[COUNT_SECTIONS];
} Report;

typedef struct {
    Report reports[REPORT_CAP];
    const char *report_file;
    int month_offset;
    int month;
    int year;
    int report_count;
} Falsus;

void falsus_init(Falsus *f, char *content, size_t content_size, const char *report_file);
void falsus_write(Falsus *f, const char *output_directory);

#endif  // FALSUS_H_
