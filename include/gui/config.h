#ifndef CONFIG_H
#define CONFIG_H

#include <gtk/gtk.h>
#include "gui_app.h"

// Config file section names
#define CONFIG_SECTION_DISPLAY "Display"
#define CONFIG_SECTION_CALENDAR "Calendar"
#define CONFIG_SECTION_UI "UI"

// Default configuration values
#define DEFAULT_SHOW_GREGORIAN TRUE
#define DEFAULT_SHOW_MOON_PHASES TRUE
#define DEFAULT_SHOW_WEEKDAYS TRUE
#define DEFAULT_HIGHLIGHT_SPECIAL_DAYS TRUE
#define DEFAULT_USE_DARK_THEME FALSE
#define DEFAULT_START_DAY 0  // Sunday
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600
#define DEFAULT_UI_SCALE 1.0

// Configuration file path
#define CONFIG_DIR_NAME ".lunar_calendar"
#define CONFIG_FILE_NAME "config.ini"

// Get the path to the configuration file
char* config_get_file_path(void);

// Load configuration from file
LunarCalendarConfig* config_load(const char* file_path);

// Save configuration to file
gboolean config_save(const char* file_path, LunarCalendarConfig* config);

// Get default configuration
LunarCalendarConfig* config_get_defaults(void);

// Apply configuration to the application
void config_apply(LunarCalendarApp* app, LunarCalendarConfig* config);

// Free configuration structure
void config_free(LunarCalendarConfig* config);

#endif /* CONFIG_H */ 