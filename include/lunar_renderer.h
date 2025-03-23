#ifndef LUNAR_RENDERER_H
#define LUNAR_RENDERER_H

#include "lunar_calendar.h"

/* Text color definitions */
#define COLOR_RESET   "\x1B[0m"
#define COLOR_RED     "\x1B[31m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_BLUE    "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN    "\x1B[36m"
#define COLOR_WHITE   "\x1B[37m"
#define COLOR_BOLD    "\x1B[1m"

/* Special day types */
typedef enum {
    NORMAL_DAY,
    TODAY,
    NEW_MOON_DAY,
    FULL_MOON_DAY,
    GERMANIC_NEW_YEAR_DAY,
    WINTER_SOLSTICE_DAY,
    SPRING_EQUINOX_DAY,
    SUMMER_SOLSTICE_DAY,
    FALL_EQUINOX_DAY,
    FESTIVAL_DAY
} SpecialDayType;

/* Structure to store cell rendering options */
typedef struct {
    bool show_gregorian_date;  /* Show gregorian date in cell */
    bool show_moon_phase;      /* Show moon phase indicator */
    bool show_weekday;         /* Show weekday name */
    bool use_colors;           /* Use ANSI colors in output */
    bool highlight_today;      /* Highlight the current day */
    bool highlight_special_days; /* Highlight special days (full moon, new moon, etc.) */
} RenderOptions;

/* Structure to represent a rendered month layout */
typedef struct {
    char *buffer;          /* Text buffer for the rendered month */
    int buffer_size;       /* Size of the buffer */
    int width;             /* Width of the rendered month in characters */
    int height;            /* Height of the rendered month in characters */
} RenderedMonth;

/* Structure to represent a rendered year layout */
typedef struct {
    char *buffer;          /* Text buffer for the rendered year */
    int buffer_size;       /* Size of the buffer */
    int width;             /* Width of the rendered year in characters */
    int height;            /* Height of the rendered year in characters */
    int months_per_row;    /* Number of months per row in the layout */
} RenderedYear;

/* Default render options */
RenderOptions default_render_options(void);

/* Formatting utilities */
char *format_month_header(int year, int month, int width);
char *format_day_cell(LunarDay day, RenderOptions options);
char *format_special_day(SpecialDayType type, RenderOptions options, const char *text);
SpecialDayType get_special_day_type(LunarDay day);

/* Rendering functions */
RenderedMonth render_lunar_month(int year, int month, RenderOptions options);
RenderedYear render_lunar_year(int year, RenderOptions options);
char *render_metonic_cycle_position(int year, RenderOptions options);

/* Free memory allocated for rendered output */
void free_rendered_month(RenderedMonth *month);
void free_rendered_year(RenderedYear *year);

/* Display rendered output */
void display_rendered_month(RenderedMonth month);
void display_rendered_year(RenderedYear year);
void display_metonic_cycle_position(const char *position_text);

/* Helper function to calculate cell width based on options */
int calculate_cell_width(RenderOptions options);

#endif /* LUNAR_RENDERER_H */ 