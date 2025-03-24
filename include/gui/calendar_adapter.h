#ifndef CALENDAR_ADAPTER_H
#define CALENDAR_ADAPTER_H

#include <gtk/gtk.h>
#include "../lunar_calendar.h"
#include "../lunar_renderer.h"

// Date structure for cleaner date handling
typedef struct {
    int year;
    int month;
    int day;
} Date;

// Calendar data model for a day cell
typedef struct {
    int lunar_day;
    int lunar_month;
    int lunar_year;
    
    int greg_day;
    int greg_month;
    int greg_year;
    
    MoonPhase moon_phase;
    Weekday weekday;
    
    gboolean is_today;
    gboolean is_special_day;
    SpecialDayType special_day_type;
    
    const char* tooltip_text;
} CalendarDayCell;

// Calendar grid model
typedef struct {
    CalendarDayCell** cells;
    int rows;
    int cols;
    
    int display_year;
    int display_month;
    
    int first_day_weekday;  // Weekday of the 1st of the month
    int days_in_month;
    
    char* month_name;
    char* year_str;
} CalendarGridModel;

// Date-related utility functions
int get_year_full_moons(int year, Date* full_moons, int max_moons);
int compare_dates(Date a, Date b);
int days_between(Date start, Date end);

// Create a calendar model for a specific month/year
CalendarGridModel* calendar_adapter_create_month_model(int year, int month);

// Free the calendar model
void calendar_adapter_free_model(CalendarGridModel* model);

// Get lunar day info for a specific date
CalendarDayCell* calendar_adapter_get_day_info(int year, int month, int day);

// Get Germanic lunar day info (with full moon as the start of months)
CalendarDayCell* calendar_adapter_get_germanic_day_info(int year, int month, int day);

// Get the name for a moon phase
const char* calendar_adapter_get_moon_phase_name(MoonPhase phase);

// Get an icon for a moon phase (returns a GtkImage widget)
GtkWidget* calendar_adapter_get_moon_phase_icon(MoonPhase phase);

// Get the color for a special day type
void calendar_adapter_get_special_day_color(SpecialDayType type, GdkRGBA* color);

// Generate tooltip text for a calendar day
char* calendar_adapter_get_tooltip_for_day(CalendarDayCell* day);

// Check if the given date is today
gboolean calendar_adapter_is_today(int year, int month, int day);

/* Get a text label for the specified moon phase */
const char* get_moon_phase_name(MoonPhase phase);

/* Create a wireframe moon phase icon */
GdkPixbuf* create_moon_phase_icon(MoonPhase phase, int size);

/* Get the moon phase icon for a day */
GdkPixbuf* get_moon_phase_icon(LunarDay lunar_day, int size);

/* These functions are already declared in lunar_renderer.h - don't redeclare with different signatures */
/* SpecialDayType get_special_day_type(int year, int month, int day); */
/* const char* format_special_day(SpecialDayType type); */
/* LunarDay calendar_adapter_get_germanic_day_info(int year, int month, int day); */

#endif /* CALENDAR_ADAPTER_H */ 