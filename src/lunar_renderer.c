#define _POSIX_C_SOURCE 200809L  /* For strdup() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/lunar_calendar.h"
#include "../include/lunar_renderer.h"

/* Lunar cycle constants copied from lunar_calendar.c to avoid exposing in header */
#define LEAP_YEARS_COUNT 7
static const int LEAP_YEARS_IN_CYCLE[] = {3, 6, 8, 11, 14, 17, 19};

/* Month names array */
static const char *MONTH_NAMES[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December", "Thirteenth"
};

/* Default render options */
RenderOptions default_render_options(void) {
    RenderOptions options = {
        .show_gregorian_date = true,
        .show_moon_phase = true,
        .show_weekday = true,
        .use_colors = true,
        .highlight_today = true,
        .highlight_special_days = true
    };
    return options;
}

/* Determine if a date is a special day */
SpecialDayType get_special_day_type(LunarDay day) {
    /* Check if it's today */
    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    
    if (day.greg_year == today->tm_year + 1900 &&
        day.greg_month == today->tm_mon + 1 &&
        day.greg_day == today->tm_mday) {
        return TODAY;
    }
    
    /* Check moon phases */
    if (day.moon_phase == NEW_MOON) {
        return NEW_MOON_DAY;
    }
    
    if (day.moon_phase == FULL_MOON) {
        return FULL_MOON_DAY;
    }
    
    /* Check if Germanic New Year */
    int new_year_month, new_year_day;
    if (calculate_germanic_new_year(day.greg_year, &new_year_month, &new_year_day)) {
        if (day.greg_month == new_year_month && day.greg_day == new_year_day) {
            return GERMANIC_NEW_YEAR_DAY;
        }
    }
    
    /* Check if winter solstice */
    int ws_month, ws_day;
    if (calculate_winter_solstice(day.greg_year, &ws_month, &ws_day)) {
        if (day.greg_month == ws_month && day.greg_day == ws_day) {
            return WINTER_SOLSTICE_DAY;
        }
    }
    
    /* Check if spring equinox */
    int se_month, se_day;
    if (calculate_spring_equinox(day.greg_year, &se_month, &se_day)) {
        if (day.greg_month == se_month && day.greg_day == se_day) {
            return SPRING_EQUINOX_DAY;
        }
    }
    
    /* Check if summer solstice */
    int ss_month, ss_day;
    if (calculate_summer_solstice(day.greg_year, &ss_month, &ss_day)) {
        if (day.greg_month == ss_month && day.greg_day == ss_day) {
            return SUMMER_SOLSTICE_DAY;
        }
    }
    
    /* Check if fall equinox */
    int fe_month, fe_day;
    if (calculate_fall_equinox(day.greg_year, &fe_month, &fe_day)) {
        if (day.greg_month == fe_month && day.greg_day == fe_day) {
            return FALL_EQUINOX_DAY;
        }
    }
    
    return NORMAL_DAY;
}

/* Format a cell for a special day with appropriate coloring */
char *format_special_day(SpecialDayType type, RenderOptions options, const char *text) {
    if (!options.use_colors || !options.highlight_special_days) {
        return strdup(text);
    }
    
    char *result = NULL;
    const char *color_code = COLOR_RESET;
    
    switch (type) {
        case TODAY:
            color_code = COLOR_BOLD COLOR_BLUE;
            break;
        case NEW_MOON_DAY:
            color_code = COLOR_BOLD COLOR_WHITE;
            break;
        case FULL_MOON_DAY:
            color_code = COLOR_BOLD COLOR_YELLOW;
            break;
        case GERMANIC_NEW_YEAR_DAY:
            color_code = COLOR_BOLD COLOR_RED;
            break;
        case WINTER_SOLSTICE_DAY:
            color_code = COLOR_BOLD COLOR_CYAN;
            break;
        case SPRING_EQUINOX_DAY:
            color_code = COLOR_BOLD COLOR_GREEN;
            break;
        case SUMMER_SOLSTICE_DAY:
            color_code = COLOR_BOLD COLOR_RED;
            break;
        case FALL_EQUINOX_DAY:
            color_code = COLOR_BOLD COLOR_MAGENTA;
            break;
        case FESTIVAL_DAY:
            color_code = COLOR_BOLD COLOR_MAGENTA;
            break;
        case NORMAL_DAY:
        default:
            return strdup(text);
    }
    
    result = malloc(strlen(text) + strlen(color_code) + strlen(COLOR_RESET) + 1);
    if (result) {
        sprintf(result, "%s%s%s", color_code, text, COLOR_RESET);
    }
    
    return result;
}

/* Calculate cell width based on options */
int calculate_cell_width(RenderOptions options) {
    int width = 3; /* Minimum width for day number */
    
    if (options.show_gregorian_date) {
        width += 5; /* Space for G-DD */
    }
    
    if (options.show_moon_phase) {
        width += 2; /* Space for symbol */
    }
    
    return width;
}

/* Simple month rendering function */
RenderedMonth render_lunar_month(int year, int month, RenderOptions options __attribute__((unused))) {
    RenderedMonth result = {0};
    
    /* Allocate a reasonable-sized buffer */
    result.buffer_size = 4096;
    result.buffer = malloc(result.buffer_size);
    
    if (!result.buffer) {
        return result;
    }
    
    /* Start with a clean buffer */
    result.buffer[0] = '\0';
    
    /* Build a simple text representation */
    char temp[4096];
    const char *month_name = (month <= 12) ? MONTH_NAMES[month - 1] : MONTH_NAMES[12];
    
    /* Format basic info */
    sprintf(temp, "Lunar Month: %s %d\n", month_name, year);
    strcat(result.buffer, temp);
    
    strcat(result.buffer, "--------------------\n");
    
    /* Add days in simple format */
    int days_in_month = calculate_lunar_month_length(year, month);
    sprintf(temp, "Days in month: %d\n\n", days_in_month);
    strcat(result.buffer, temp);
    
    /* Create a basic calendar grid */
    strcat(result.buffer, "Su Mo Tu We Th Fr Sa\n");
    strcat(result.buffer, "--------------------\n");
    
    /* Calculate the weekday of the first day of the month */
    int first_day_of_month = 1;
    
    /* Get the Gregorian date for the 1st of the month */
    int greg_year, greg_month, greg_day;
    
    /* Try to convert lunar to Gregorian */
    if (!lunar_to_gregorian(year, month, first_day_of_month, 
                          &greg_year, &greg_month, &greg_day)) {
        /* If conversion fails, approximate */
        greg_year = year;
        greg_month = month;
        greg_day = 1;
    }
    
    /* Calculate the weekday for the first day */
    Weekday first_day_weekday = calculate_weekday(greg_year, greg_month, greg_day);
    
    /* Add leading spaces for the first week */
    for (int i = 0; i < first_day_weekday; i++) {
        strcat(result.buffer, "   ");
    }
    
    /* Current weekday position (0-6) */
    int current_weekday = first_day_weekday;
    
    /* Add all days of the month */
    for (int day = 1; day <= days_in_month; day++) {
        sprintf(temp, "%2d ", day);
        strcat(result.buffer, temp);
        
        /* Increment weekday and add line break if it's end of week */
        current_weekday = (current_weekday + 1) % 7;
        if (current_weekday == 0 && day < days_in_month) {
            strcat(result.buffer, "\n");
        }
    }
    
    /* Add final newline */
    strcat(result.buffer, "\n");
    
    /* Set dimensions */
    result.width = 20;
    result.height = 10;  /* Rough estimate */
    
    return result;
}

/* Render a lunar year */
RenderedYear render_lunar_year(int year, RenderOptions options __attribute__((unused))) {
    RenderedYear result = {0};
    
    /* Allocate buffer */
    result.buffer_size = 16384;
    result.buffer = malloc(result.buffer_size);
    
    if (!result.buffer) {
        return result;
    }
    
    /* Start with a clean buffer */
    result.buffer[0] = '\0';
    
    /* Header */
    char temp[4096];
    int eld_year = calculate_eld_year(year);
    bool is_leap = is_lunar_leap_year(year);
    
    sprintf(temp, "Lunar Calendar for Year %d (Eld Year %d)\n", year, eld_year);
    strcat(result.buffer, temp);
    
    if (is_leap) {
        strcat(result.buffer, "This is a leap year with 13 lunar months\n");
    } else {
        strcat(result.buffer, "This is a regular year with 12 lunar months\n");
    }
    
    strcat(result.buffer, "====================================\n\n");
    
    /* Get Metonic position */
    int metonic_year, metonic_cycle;
    get_metonic_position(year, 1, 1, &metonic_year, &metonic_cycle);
    
    sprintf(temp, "Metonic Cycle: Year %d of Cycle %d\n\n", metonic_year, metonic_cycle);
    strcat(result.buffer, temp);
    
    /* List months */
    int month_count = is_leap ? 13 : 12;
    
    for (int m = 1; m <= month_count; m++) {
        const char *month_name = (m <= 12) ? MONTH_NAMES[m - 1] : MONTH_NAMES[12];
        int days = calculate_lunar_month_length(year, m);
        
        sprintf(temp, "Month %2d: %s - %d days\n", m, month_name, days);
        strcat(result.buffer, temp);
    }
    
    /* Set dimensions */
    result.width = 50;
    result.height = month_count + 10;
    result.months_per_row = 1;
    
    return result;
}

/* Render the position within the Metonic cycle */
char *render_metonic_cycle_position(int year, RenderOptions options) {
    (void)options; /* Suppress unused parameter warning */
    
    /* Allocate a generous buffer */
    char *buffer = malloc(2048);
    if (!buffer) {
        return NULL;
    }
    
    /* Start with an empty buffer */
    buffer[0] = '\0';
    
    /* Get Metonic cycle info */
    int metonic_year, metonic_cycle;
    get_metonic_position(year, 1, 1, &metonic_year, &metonic_cycle);
    
    bool is_leap = is_lunar_leap_year(year);
    
    /* Format header */
    char temp[2048];
    sprintf(temp, "Metonic Cycle Position for Year %d\n", year);
    strcat(buffer, temp);
    strcat(buffer, "--------------------------------\n\n");
    
    /* Position info */
    sprintf(temp, "Year %d is in position %d of the 19-year Metonic cycle\n", 
           year, metonic_year);
    strcat(buffer, temp);
    
    sprintf(temp, "This is Metonic cycle number: %d\n", metonic_cycle);
    strcat(buffer, temp);
    
    sprintf(temp, "This year is a %s lunar year\n\n", 
           is_leap ? "leap (13 months)" : "regular (12 months)");
    strcat(buffer, temp);
    
    /* Visual representation */
    strcat(buffer, "Cycle visualization (years marked with * are leap years):\n");
    strcat(buffer, "======================================================\n");
    
    /* Create a visual representation of the cycle */
    for (int i = 1; i <= YEARS_PER_METONIC_CYCLE; i++) {
        /* Check if this is a leap year */
        bool is_year_leap = false;
        for (int j = 0; j < LEAP_YEARS_COUNT; j++) {
            if (LEAP_YEARS_IN_CYCLE[j] == i) {
                is_year_leap = true;
                break;
            }
        }
        
        /* Highlight current position */
        if (i == metonic_year) {
            sprintf(temp, "[%2d%s] << Current ", i, is_year_leap ? "*" : " ");
        } else {
            sprintf(temp, "[%2d%s] ", i, is_year_leap ? "*" : " ");
        }
        strcat(buffer, temp);
        
        /* Line break for readability */
        if (i % 5 == 0) {
            strcat(buffer, "\n");
        }
    }
    
    return buffer;
}

/* Free memory allocated for rendered month */
void free_rendered_month(RenderedMonth *month) {
    if (month && month->buffer) {
        free(month->buffer);
        month->buffer = NULL;
        month->buffer_size = 0;
        month->width = 0;
        month->height = 0;
    }
}

/* Free memory allocated for rendered year */
void free_rendered_year(RenderedYear *year) {
    if (year && year->buffer) {
        free(year->buffer);
        year->buffer = NULL;
        year->buffer_size = 0;
        year->width = 0;
        year->height = 0;
    }
}

/* Display rendered month */
void display_rendered_month(RenderedMonth month) {
    if (month.buffer) {
        printf("%s", month.buffer);
    }
}

/* Display rendered year */
void display_rendered_year(RenderedYear year) {
    if (year.buffer) {
        printf("%s", year.buffer);
    }
}

/* Display metonic cycle position */
void display_metonic_cycle_position(const char *position_text) {
    if (position_text) {
        printf("%s", position_text);
    }
} 