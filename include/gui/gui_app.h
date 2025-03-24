#ifndef GUI_APP_H
#define GUI_APP_H

#include <gtk/gtk.h>
#include "../lunar_calendar.h"
#include "../lunar_renderer.h"

// Forward declaration for config structure
typedef struct LunarCalendarConfig LunarCalendarConfig;

// GUI application structure
typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *calendar_view;
    GtkWidget *header_bar;
    GtkWidget *sidebar;
    GtkWidget *status_bar;
    GtkWidget *year_label;  // Added for Eld Year display
    
    // Current view state
    int current_year;
    int current_month;
    MoonPhase current_moon_phase;
    
    // Calendar data model
    LunarDay **calendar_data;
    int rows;
    int cols;
    
    // Configuration
    char *config_file_path;
    LunarCalendarConfig *config;
} LunarCalendarApp;

// Configuration structure
typedef struct LunarCalendarConfig {
    // Display options
    gboolean show_gregorian_dates;
    gboolean show_moon_phases;
    gboolean show_weekday_names;
    gboolean highlight_special_days;
    gboolean use_dark_theme;
    
    // Calendar options
    int start_day_of_week;  // 0=Sunday, 1=Monday, etc.
    
    // UI options
    int window_width;
    int window_height;
    double ui_scale;
} LunarCalendarConfig;

// Initialize the GUI application
LunarCalendarApp* gui_app_init(int* argc, char*** argv);

// Run the application main loop
int gui_app_run(LunarCalendarApp* app);

// Clean up resources
void gui_app_cleanup(LunarCalendarApp* app);

// Update calendar view to show a specific month
void gui_app_show_month(LunarCalendarApp* app, int year, int month);

// Create a new calendar widget for the given month/year
GtkWidget* create_calendar_widget(int year, int month);

#endif /* GUI_APP_H */ 