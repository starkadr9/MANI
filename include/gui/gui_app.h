#ifndef GUI_APP_H
#define GUI_APP_H

#include <gtk/gtk.h>
#include "../lunar_calendar.h"
#include "../lunar_renderer.h"
#include "config.h"

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
    char *events_file_path;
    LunarCalendarConfig *config;
    
    // Today's date (for highlighting)
    int today_year;
    int today_month;
    int today_day;
    
    // Selected day (for event editing)
    int selected_day_year;
    int selected_day_month;
    int selected_day_day;
    
    // Event editor widgets
    GtkWidget *event_editor;
    GtkWidget *event_list;
    GtkWidget *event_title_entry;
    GtkWidget *event_desc_text;
    GtkWidget *event_color_button;
} LunarCalendarApp;

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