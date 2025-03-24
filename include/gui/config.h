#ifndef CONFIG_H
#define CONFIG_H

#include <gtk/gtk.h>
#include <stdbool.h>

// Config file section names
#define CONFIG_SECTION_DISPLAY "Display"
#define CONFIG_SECTION_CALENDAR "Calendar"
#define CONFIG_SECTION_UI "UI"
#define CONFIG_SECTION_APPEARANCE "Appearance"
#define CONFIG_SECTION_ADVANCED "Advanced"
#define CONFIG_SECTION_NAMES "Names"

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
#define DEFAULT_CELL_SIZE 80
#define DEFAULT_THEME_TYPE 2  // System default
#define DEFAULT_SHOW_EVENT_INDICATORS TRUE
#define DEFAULT_SHOW_METONIC_CYCLE FALSE
#define DEFAULT_DEBUG_LOGGING FALSE

// Configuration file path
#define CONFIG_DIR_NAME ".lunar_calendar"
#define CONFIG_FILE_NAME "config.ini"

// Default calendar type
#define DEFAULT_CALENDAR_TYPE 1  // 1 = Germanic

// Configuration structure
typedef struct {
    // Display section
    int window_width;
    int window_height;
    bool show_moon_phases;
    bool highlight_special_days;
    int calendar_type;    // 0 = Traditional, 1 = Germanic
    bool show_gregorian_dates;
    bool show_weekday_names;
    bool show_event_indicators;
    int week_start_day;  // 0=Sunday, 1=Monday, 2=Saturday
    bool show_metonic_cycle;
    
    // Appearance section
    bool use_dark_theme;
    int theme_type;      // 0=Light, 1=Dark, 2=System Default
    GdkRGBA primary_color;
    GdkRGBA secondary_color;
    GdkRGBA text_color;
    char* font_name;
    int cell_size;      // Size of calendar cells in pixels
    
    // Names section
    char* custom_month_names[13];
    char* custom_weekday_names[7];
    
    // Advanced section
    double ui_scale;
    char* events_file_path;
    char* cache_dir;
    bool debug_logging;
    char* log_file_path;
} LunarCalendarConfig;

// Get the path to the configuration file
char* config_get_file_path(void);

// Get the default events file path
char* events_get_file_path(void);

// Load configuration from file
LunarCalendarConfig* config_load(const char* filename);

// Save configuration to file
bool config_save(const char* filename, LunarCalendarConfig* config);

// Free configuration
void config_free(LunarCalendarConfig* config);

// Get default configuration
LunarCalendarConfig* config_get_defaults(void);

// Apply configuration to app
void config_apply(void* app, LunarCalendarConfig* config);

#endif /* CONFIG_H */ 