#ifndef GUI_MAIN_H
#define GUI_MAIN_H

#include <gtk/gtk.h>
#include "../lunar_calendar.h"
#include "config.h"
#include "calendar_adapter.h"

typedef struct {
    GtkApplication* app;
    GtkWidget* window;
    GtkWidget* calendar_view;
    GtkWidget* month_label;
    GtkWidget* year_label;  // Added year label
    GtkWidget* status_label;
    
    int current_year;
    int current_month;
    
    LunarCalendarConfig config;
} LunarCalendarApp;

/* Function prototypes for the wireframe moon phase functions */
GdkPixbuf* create_moon_phase_icon(MoonPhase phase, int size);
GdkPixbuf* get_moon_phase_icon(LunarDay lunar_day, int size);

#endif /* GUI_MAIN_H */ 