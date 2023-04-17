#include "falsus.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

static_assert(COUNT_SECTIONS == 2, "The number of sections have changed");
static const char *section_headings[COUNT_SECTIONS] = {
    [SECTION_COMPANY] = "Betriebliche Tätigkeit",
    [SECTION_SCHOOL] = "Themen des Berufsschulunterrichts",
};

static struct tm falsus_get_time(Falsus *f)
{
    f->month = (REPORTS_FIRST_MONTH+f->report_count+f->month_offset) % 13;
    if (f->month == 0) {
        f->year++;
        f->month_offset++;
        f->month = 1;
    }
    struct tm time = {0};
    time.tm_year = f->year - 1900;
    time.tm_mon = f->month - 1;
    time.tm_mday = 1;
    return time;
}

static void falsus_set_time(Falsus *f)
{
    struct tm time = falsus_get_time(f);
    Report *last = &f->reports[f->report_count];
    if (strftime(last->month, sizeof(last->month), "%B", &time) == 0 || strftime(last->year, sizeof(last->year), "%Y", &time) == 0) {
        fprintf(stderr, "ERROR: strftime exceeded may bytes.\n");
        exit(1);
    }
}

static void falsus_parse(Falsus *f, char *content, size_t content_size)
{
    Section_Kind current_section = SECTION_COMPANY;
    size_t current_week = 0;
    size_t current_item = 0;
    size_t newline_streak = 0;
    size_t line = 0;

    char line_content[WEEK_CHAR_CAP];
    memset(line_content, 0, sizeof(line_content));
    for (size_t i = 0; i < content_size; ++i) {
        char x = content[i];
        if (x == '\n') {
            newline_streak++;
            line++;
        }
        else newline_streak = 0;

        switch (newline_streak) {
        case 0:
            if (x == '&') strncat(line_content, "\\", 1);
            strncat(line_content, &x, 1);
            break;
        case 1:
        {
            Section *section = &f->reports[f->report_count].sections[current_section];
            section->kind = current_section;
            strncpy(section->weeks[current_week].item[current_item], line_content, WEEK_CHAR_CAP);
            current_item++;
            break;
        }
        case 2:
            current_week++;
            current_item = 0;
            break;
        case 3:
            if (current_week < 3) {
                fprintf(stderr, "%s:%zu: WARNING: less than 4 weeks were provided in the last section `%s`\n",
                        f->report_file, line, section_headings[current_section]);
            }
            if (current_week > 3) {
                fprintf(stderr, "%s:%zu: WARNING: more than 4 weeks were provided in the last section `%s`\n",
                        f->report_file, line, section_headings[current_section]);
            }
            current_section = SECTION_SCHOOL;
            current_week = 0;
            break;
        case 4:
            if (current_week < 3) {
                fprintf(stderr, "%s:%zu: WARNING: less than 4 weeks were provided in the last report\n",
                        f->report_file, line);
            }
            if (current_week > 3) {
                fprintf(stderr, "%s:%zu: WARNING: more than 4 weeks were provided in the last report\n",
                        f->report_file, line);
            }
            falsus_set_time(f);
            f->report_count++;
            current_section = SECTION_COMPANY;
            break;
        default:
            fprintf(stderr, "%s:%zu: Too many lines were provided.\n", f->report_file, line);
            exit(1);
        }

        if (newline_streak != 0) memset(line_content, 0, sizeof(line_content));
    }
    falsus_set_time(f);
}

static void concat(char *result, const char *a, const char *b)
{
    strcpy(result, a);
    strcat(result, b);
}

static void write_latex_command_no_newline(FILE *stream, const char *command, const char *format, ...)
{
    fprintf(stream, "\\%s{", command);
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    fprintf(stream, "}");
}

static void write_latex_command(FILE *stream, const char *command, const char *format, ...)
{
    fprintf(stream, "\\%s{", command);
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    fprintf(stream, "}\n");
}

static void write_latex_include(FILE *stream, const char *file_path)
{
    write_latex_command(stream, "input", file_path);
}

void falsus_write(Falsus *f, const char *output_directory)
{
    char include_file_path[300];
    char current_file_path[300];
    concat(include_file_path, output_directory, "include_reports.tex");

    FILE *include_file = fopen(include_file_path, "w");
    if (include_file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file `%s`\n", include_file_path);
        exit(1);
    }

    for (int report_i = 0; report_i < f->report_count+1; ++report_i) {
        char report_file_name[300];
        char report_include_name[300];
        if (snprintf(report_include_name, sizeof(report_file_name), "report-%d", report_i+1) < 0) {
            fprintf(stderr, "ERROR: sprintf failed to create file name\n");
            exit(1);
        }
        concat(report_file_name, report_include_name, ".tex");
        concat(current_file_path, output_directory, report_file_name);

        Report *report = &f->reports[report_i];
        FILE *current_report_file = fopen(current_file_path, "w");
        if (current_report_file == NULL) {
            fprintf(stderr, "ERROR: Failed to open file `%s`\n", current_file_path);
            exit(1);
        }

        write_latex_include(include_file, current_file_path);
        // write_latex_command(current_report_file, "Titelzeile", "%s %s, %d",
        //                     report->month, report->year, report_i+1);
        fprintf(current_report_file, "\\Titelzeile{%s}{%s}{%d}\n",
                report->month, report->year, report_i+1);

        for (Section_Kind section_i = 0; section_i < COUNT_SECTIONS; ++section_i) {
            write_latex_command_no_newline(current_report_file, "Monat", "%s", section_headings[section_i]);
            fprintf(current_report_file, "{\n");
            Section *section = &report->sections[section_i];
            for (int week_i = 0; week_i < WEEK_COUNT; ++week_i) {
                Week *week = &section->weeks[week_i];
                bool contains_items = strlen(week->item[0]) > 0;
                if (contains_items) fprintf(current_report_file, "  \\Woche{%d}{\n", week_i+1);

                int non_empty = 0;
                for (int item_i = 0; item_i < WEEK_ITEM_CAP; ++item_i) if (strlen(week->item[item_i]) > 0) non_empty++;
                for (int item_i = 0; item_i < WEEK_ITEM_CAP; ++item_i) {
                    char *item = week->item[item_i];
                    if (strlen(item) <= 0) continue;
                    fprintf(current_report_file, "    %s\n", item);
                    if (item_i < non_empty-1) fprintf(current_report_file, "\n");
                }
                if (contains_items) fprintf(current_report_file, "  }\n");
            }
            fprintf(current_report_file, "}\n");
        }

        write_latex_command_no_newline(current_report_file, "Unterschrift", "");
        write_latex_command_no_newline(current_report_file, "newpage", "");
        fclose(current_report_file);
    }
    fclose(include_file);
}

void falsus_init(Falsus *f, char *content, size_t content_size, const char *report_file)
{
    f->report_count = 0;
    f->report_file = report_file;
    f->month = REPORTS_FIRST_MONTH;
    f->month_offset = 0; // TODO: get rid of this variable (although it works)
    f->year = REPORTS_FIRST_YEAR;
    falsus_parse(f, content, content_size);
}
