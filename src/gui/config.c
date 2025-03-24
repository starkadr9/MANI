#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include "../../include/gui/config.h"
#include "../../include/gui/gui_app.h"

// Get path to user's home directory
static char* get_home_dir(void) {
    return g_strdup(g_get_home_dir());
}

// Ensure the config directory exists
static gboolean ensure_config_dir(const char* dir_path) {
    // Check if directory exists
    if (g_file_test(dir_path, G_FILE_TEST_IS_DIR)) {
        return TRUE;
    }
    
    // Create the directory
    return g_mkdir_with_parents(dir_path, 0755) == 0;
}

// Get the path to the configuration file
char* config_get_file_path(void) {
    char* home_dir = get_home_dir();
    if (!home_dir) {
        return NULL;
    }
    
    // Create the config directory path
    char* config_dir = g_build_filename(home_dir, CONFIG_DIR_NAME, NULL);
    g_free(home_dir);
    
    // Ensure the directory exists
    if (!ensure_config_dir(config_dir)) {
        g_free(config_dir);
        return NULL;
    }
    
    // Build the config file path
    char* config_file = g_build_filename(config_dir, CONFIG_FILE_NAME, NULL);
    g_free(config_dir);
    
    return config_file;
}

// Get the default events file path
char* events_get_file_path(void) {
    const char* home_dir = g_get_home_dir();
    if (!home_dir) {
        return NULL;
    }
    
    // Create the config directory path
    char* config_dir = g_build_filename(home_dir, CONFIG_DIR_NAME, NULL);
    
    // Make sure the directory exists
    if (g_mkdir_with_parents(config_dir, 0700) != 0) {
        g_free(config_dir);
        return NULL;
    }
    
    // Create the events file path
    char* events_file = g_build_filename(config_dir, "events.dat", NULL);
    g_free(config_dir);
    
    return events_file;
}

/**
 * Load configuration from file.
 */
LunarCalendarConfig* config_load(const char* filename) {
    if (!filename) {
        return config_get_defaults();
    }
    
    GKeyFile* key_file = g_key_file_new();
    GError* error = NULL;
    
    if (!g_key_file_load_from_file(key_file, filename, G_KEY_FILE_NONE, &error)) {
        if (error) {
            g_error_free(error);
        }
        g_key_file_free(key_file);
        return config_get_defaults();
    }
    
    // Get default configuration as a starting point
    LunarCalendarConfig* config = config_get_defaults();
    
    // Display section
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "window_width", NULL)) {
        config->window_width = g_key_file_get_integer(key_file, CONFIG_SECTION_DISPLAY, "window_width", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "window_height", NULL)) {
        config->window_height = g_key_file_get_integer(key_file, CONFIG_SECTION_DISPLAY, "window_height", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_moon_phases", NULL)) {
        config->show_moon_phases = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_moon_phases", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "highlight_special_days", NULL)) {
        config->highlight_special_days = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "highlight_special_days", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_gregorian_dates", NULL)) {
        config->show_gregorian_dates = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_gregorian_dates", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_weekday_names", NULL)) {
        config->show_weekday_names = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_weekday_names", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_event_indicators", NULL)) {
        config->show_event_indicators = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_event_indicators", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "week_start_day", NULL)) {
        config->week_start_day = g_key_file_get_integer(key_file, CONFIG_SECTION_DISPLAY, "week_start_day", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_metonic_cycle", NULL)) {
        config->show_metonic_cycle = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_metonic_cycle", NULL);
    }
    
    // Calendar section
    if (g_key_file_has_key(key_file, CONFIG_SECTION_CALENDAR, "calendar_type", NULL)) {
        config->calendar_type = g_key_file_get_integer(key_file, CONFIG_SECTION_CALENDAR, "calendar_type", NULL);
    }
    
    // Handle UI section (for backward compatibility)
    if (g_key_file_has_key(key_file, CONFIG_SECTION_UI, "window_width", NULL)) {
        config->window_width = g_key_file_get_integer(key_file, CONFIG_SECTION_UI, "window_width", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_UI, "window_height", NULL)) {
        config->window_height = g_key_file_get_integer(key_file, CONFIG_SECTION_UI, "window_height", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_UI, "ui_scale", NULL)) {
        config->ui_scale = g_key_file_get_double(key_file, CONFIG_SECTION_UI, "ui_scale", NULL);
    }
    
    // Appearance section
    if (g_key_file_has_key(key_file, CONFIG_SECTION_APPEARANCE, "use_dark_theme", NULL)) {
        config->use_dark_theme = g_key_file_get_boolean(key_file, CONFIG_SECTION_APPEARANCE, "use_dark_theme", NULL);
    } else if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "use_dark_theme", NULL)) {
        // Backward compatibility
        config->use_dark_theme = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "use_dark_theme", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_APPEARANCE, "theme_type", NULL)) {
        config->theme_type = g_key_file_get_integer(key_file, CONFIG_SECTION_APPEARANCE, "theme_type", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_APPEARANCE, "cell_size", NULL)) {
        config->cell_size = g_key_file_get_integer(key_file, CONFIG_SECTION_APPEARANCE, "cell_size", NULL);
    }
    
    // Load colors
    if (g_key_file_has_key(key_file, CONFIG_SECTION_APPEARANCE, "primary_color", NULL)) {
        char* color_str = g_key_file_get_string(key_file, CONFIG_SECTION_APPEARANCE, "primary_color", NULL);
        if (color_str) {
            sscanf(color_str, "%lf,%lf,%lf,%lf", 
                  &config->primary_color.red, 
                  &config->primary_color.green, 
                  &config->primary_color.blue, 
                  &config->primary_color.alpha);
            g_free(color_str);
        }
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_APPEARANCE, "secondary_color", NULL)) {
        char* color_str = g_key_file_get_string(key_file, CONFIG_SECTION_APPEARANCE, "secondary_color", NULL);
        if (color_str) {
            sscanf(color_str, "%lf,%lf,%lf,%lf", 
                  &config->secondary_color.red, 
                  &config->secondary_color.green, 
                  &config->secondary_color.blue, 
                  &config->secondary_color.alpha);
            g_free(color_str);
        }
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_APPEARANCE, "text_color", NULL)) {
        char* color_str = g_key_file_get_string(key_file, CONFIG_SECTION_APPEARANCE, "text_color", NULL);
        if (color_str) {
            sscanf(color_str, "%lf,%lf,%lf,%lf", 
                  &config->text_color.red, 
                  &config->text_color.green, 
                  &config->text_color.blue, 
                  &config->text_color.alpha);
            g_free(color_str);
        }
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_APPEARANCE, "font_name", NULL)) {
        char* font_name = g_key_file_get_string(key_file, CONFIG_SECTION_APPEARANCE, "font_name", NULL);
        if (font_name) {
            if (config->font_name) {
                g_free(config->font_name);
            }
            config->font_name = font_name; // Ownership transferred
        }
    }
    
    // Names section - load custom month names
    for (int i = 0; i < 13; i++) {
        char key[32];
        snprintf(key, sizeof(key), "month_%d_name", i + 1);
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_NAMES, key, NULL)) {
            char* month_name = g_key_file_get_string(key_file, CONFIG_SECTION_NAMES, key, NULL);
            if (month_name && strlen(month_name) > 0) {
                if (config->custom_month_names[i]) {
                    g_free(config->custom_month_names[i]);
                }
                config->custom_month_names[i] = month_name; // Ownership transferred
            } else {
                g_free(month_name);
            }
        }
    }
    
    // Names section - load custom weekday names
    for (int i = 0; i < 7; i++) {
        char key[32];
        snprintf(key, sizeof(key), "weekday_%d_name", i + 1);
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_NAMES, key, NULL)) {
            char* weekday_name = g_key_file_get_string(key_file, CONFIG_SECTION_NAMES, key, NULL);
            if (weekday_name && strlen(weekday_name) > 0) {
                if (config->custom_weekday_names[i]) {
                    g_free(config->custom_weekday_names[i]);
                }
                config->custom_weekday_names[i] = weekday_name; // Ownership transferred
            } else {
                g_free(weekday_name);
            }
        }
    }
    
    // Advanced section
    if (g_key_file_has_key(key_file, CONFIG_SECTION_ADVANCED, "ui_scale", NULL)) {
        config->ui_scale = g_key_file_get_double(key_file, CONFIG_SECTION_ADVANCED, "ui_scale", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_ADVANCED, "events_file_path", NULL)) {
        char* events_file = g_key_file_get_string(key_file, CONFIG_SECTION_ADVANCED, "events_file_path", NULL);
        if (events_file) {
            if (config->events_file_path) {
                g_free(config->events_file_path);
            }
            config->events_file_path = events_file; // Ownership transferred
        }
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_ADVANCED, "cache_dir", NULL)) {
        char* cache_dir = g_key_file_get_string(key_file, CONFIG_SECTION_ADVANCED, "cache_dir", NULL);
        if (cache_dir) {
            if (config->cache_dir) {
                g_free(config->cache_dir);
            }
            config->cache_dir = cache_dir; // Ownership transferred
        }
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_ADVANCED, "debug_logging", NULL)) {
        config->debug_logging = g_key_file_get_boolean(key_file, CONFIG_SECTION_ADVANCED, "debug_logging", NULL);
    }
    
    if (g_key_file_has_key(key_file, CONFIG_SECTION_ADVANCED, "log_file_path", NULL)) {
        char* log_file = g_key_file_get_string(key_file, CONFIG_SECTION_ADVANCED, "log_file_path", NULL);
        if (log_file) {
            if (config->log_file_path) {
                g_free(config->log_file_path);
            }
            config->log_file_path = log_file; // Ownership transferred
        }
    }
    
    g_key_file_free(key_file);
    return config;
}

/**
 * Get default configuration values.
 */
LunarCalendarConfig* config_get_defaults(void) {
    LunarCalendarConfig* config = malloc(sizeof(LunarCalendarConfig));
    if (!config) {
        return NULL;
    }
    
    // Initialize fields to default values
    config->window_width = DEFAULT_WINDOW_WIDTH;
    config->window_height = DEFAULT_WINDOW_HEIGHT;
    config->show_moon_phases = DEFAULT_SHOW_MOON_PHASES;
    config->highlight_special_days = DEFAULT_HIGHLIGHT_SPECIAL_DAYS;
    config->calendar_type = DEFAULT_CALENDAR_TYPE;
    config->show_gregorian_dates = DEFAULT_SHOW_GREGORIAN;
    config->show_weekday_names = DEFAULT_SHOW_WEEKDAYS;
    config->use_dark_theme = DEFAULT_USE_DARK_THEME;
    config->week_start_day = DEFAULT_START_DAY;
    config->ui_scale = DEFAULT_UI_SCALE;
    config->show_event_indicators = DEFAULT_SHOW_EVENT_INDICATORS;
    config->show_metonic_cycle = DEFAULT_SHOW_METONIC_CYCLE;
    
    // Appearance defaults
    config->theme_type = DEFAULT_THEME_TYPE;
    config->cell_size = DEFAULT_CELL_SIZE;
    
    // Set default colors
    config->primary_color.red = 0.2;
    config->primary_color.green = 0.4;
    config->primary_color.blue = 0.6;
    config->primary_color.alpha = 1.0;
    
    config->secondary_color.red = 0.8;
    config->secondary_color.green = 0.3;
    config->secondary_color.blue = 0.2;
    config->secondary_color.alpha = 1.0;
    
    config->text_color.red = 0.0;
    config->text_color.green = 0.0;
    config->text_color.blue = 0.0;
    config->text_color.alpha = 1.0;
    
    // Default font
    config->font_name = g_strdup("Sans 10");
    
    // Initialize custom name arrays
    for (int i = 0; i < 13; i++) {
        config->custom_month_names[i] = NULL;
    }
    
    for (int i = 0; i < 7; i++) {
        config->custom_weekday_names[i] = NULL;
    }
    
    // Advanced settings
    config->events_file_path = events_get_file_path();
    
    // Default cache directory
    char* cache_dir = g_build_filename(g_get_user_cache_dir(), CONFIG_DIR_NAME, NULL);
    config->cache_dir = cache_dir;
    
    config->debug_logging = DEFAULT_DEBUG_LOGGING;
    
    // Default log file location
    char* log_dir = g_build_filename(g_get_user_data_dir(), CONFIG_DIR_NAME, NULL);
    g_mkdir_with_parents(log_dir, 0755);
    config->log_file_path = g_build_filename(log_dir, "mani.log", NULL);
    g_free(log_dir);
    
    return config;
}

/**
 * Free configuration structure.
 */
void config_free(LunarCalendarConfig* config) {
    if (config) {
        // Free dynamically allocated strings
        if (config->font_name) {
            g_free(config->font_name);
        }
        
        if (config->events_file_path) {
            g_free(config->events_file_path);
        }
        
        if (config->cache_dir) {
            g_free(config->cache_dir);
        }
        
        if (config->log_file_path) {
            g_free(config->log_file_path);
        }
        
        // Free custom names
        for (int i = 0; i < 13; i++) {
            if (config->custom_month_names[i]) {
                g_free(config->custom_month_names[i]);
            }
        }
        
        for (int i = 0; i < 7; i++) {
            if (config->custom_weekday_names[i]) {
                g_free(config->custom_weekday_names[i]);
            }
        }
        
        free(config);
    }
}

/**
 * Save configuration to file.
 */
bool config_save(const char* filename, LunarCalendarConfig* config) {
    if (!filename || !config) {
        return false;
    }
    
    GKeyFile* key_file = g_key_file_new();
    
    // Display section
    g_key_file_set_integer(key_file, CONFIG_SECTION_DISPLAY, "window_width", config->window_width);
    g_key_file_set_integer(key_file, CONFIG_SECTION_DISPLAY, "window_height", config->window_height);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_moon_phases", config->show_moon_phases);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "highlight_special_days", config->highlight_special_days);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_gregorian_dates", config->show_gregorian_dates);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_weekday_names", config->show_weekday_names);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_event_indicators", config->show_event_indicators);
    g_key_file_set_integer(key_file, CONFIG_SECTION_DISPLAY, "week_start_day", config->week_start_day);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_metonic_cycle", config->show_metonic_cycle);
    
    // Calendar section
    g_key_file_set_integer(key_file, CONFIG_SECTION_CALENDAR, "calendar_type", config->calendar_type);
    
    // Appearance section
    g_key_file_set_boolean(key_file, CONFIG_SECTION_APPEARANCE, "use_dark_theme", config->use_dark_theme);
    g_key_file_set_integer(key_file, CONFIG_SECTION_APPEARANCE, "theme_type", config->theme_type);
    g_key_file_set_integer(key_file, CONFIG_SECTION_APPEARANCE, "cell_size", config->cell_size);
    
    // Save colors as strings in format "r,g,b,a"
    char primary_color_str[64];
    snprintf(primary_color_str, sizeof(primary_color_str), "%.3lf,%.3lf,%.3lf,%.3lf",
            config->primary_color.red, config->primary_color.green,
            config->primary_color.blue, config->primary_color.alpha);
    g_key_file_set_string(key_file, CONFIG_SECTION_APPEARANCE, "primary_color", primary_color_str);
    
    char secondary_color_str[64];
    snprintf(secondary_color_str, sizeof(secondary_color_str), "%.3lf,%.3lf,%.3lf,%.3lf",
            config->secondary_color.red, config->secondary_color.green,
            config->secondary_color.blue, config->secondary_color.alpha);
    g_key_file_set_string(key_file, CONFIG_SECTION_APPEARANCE, "secondary_color", secondary_color_str);
    
    char text_color_str[64];
    snprintf(text_color_str, sizeof(text_color_str), "%.3lf,%.3lf,%.3lf,%.3lf",
            config->text_color.red, config->text_color.green,
            config->text_color.blue, config->text_color.alpha);
    g_key_file_set_string(key_file, CONFIG_SECTION_APPEARANCE, "text_color", text_color_str);
    
    if (config->font_name) {
        g_key_file_set_string(key_file, CONFIG_SECTION_APPEARANCE, "font_name", config->font_name);
    }
    
    // Names section - save custom month names
    for (int i = 0; i < 13; i++) {
        char key[32];
        snprintf(key, sizeof(key), "month_%d_name", i + 1);
        if (config->custom_month_names[i] && strlen(config->custom_month_names[i]) > 0) {
            g_key_file_set_string(key_file, CONFIG_SECTION_NAMES, key, config->custom_month_names[i]);
        }
    }
    
    // Names section - save custom weekday names
    for (int i = 0; i < 7; i++) {
        char key[32];
        snprintf(key, sizeof(key), "weekday_%d_name", i + 1);
        if (config->custom_weekday_names[i] && strlen(config->custom_weekday_names[i]) > 0) {
            g_key_file_set_string(key_file, CONFIG_SECTION_NAMES, key, config->custom_weekday_names[i]);
        }
    }
    
    // Advanced section
    g_key_file_set_double(key_file, CONFIG_SECTION_ADVANCED, "ui_scale", config->ui_scale);
    
    if (config->events_file_path) {
        g_key_file_set_string(key_file, CONFIG_SECTION_ADVANCED, "events_file_path", config->events_file_path);
    }
    
    if (config->cache_dir) {
        g_key_file_set_string(key_file, CONFIG_SECTION_ADVANCED, "cache_dir", config->cache_dir);
    }
    
    g_key_file_set_boolean(key_file, CONFIG_SECTION_ADVANCED, "debug_logging", config->debug_logging);
    
    if (config->log_file_path) {
        g_key_file_set_string(key_file, CONFIG_SECTION_ADVANCED, "log_file_path", config->log_file_path);
    }
    
    // Save to file
    gsize length;
    gchar* data = g_key_file_to_data(key_file, &length, NULL);
    
    // Make sure directory exists
    char* dir_path = g_path_get_dirname(filename);
    g_mkdir_with_parents(dir_path, 0755);
    g_free(dir_path);
    
    // Write to file
    GError* error = NULL;
    gboolean success = g_file_set_contents(filename, data, length, &error);
    
    if (!success) {
        g_warning("Failed to save configuration: %s", error->message);
        g_error_free(error);
    }
    
    g_free(data);
    g_key_file_free(key_file);
    
    return success;
}

// Apply configuration to the application
void config_apply(void* app_ptr, LunarCalendarConfig* config) {
    LunarCalendarApp* app = (LunarCalendarApp*)app_ptr;
    if (!app || !config) {
        return;
    }
    
    // Apply window size
    if (app->window) {
        gtk_window_set_default_size(GTK_WINDOW(app->window), 
                                   config->window_width, 
                                   config->window_height);
    }
    
    // Apply dark theme if requested
    if (config->use_dark_theme) {
        GtkSettings* settings = gtk_settings_get_default();
        if (settings) {
            g_object_set(settings, "gtk-application-prefer-dark-theme", config->use_dark_theme, NULL);
        }
    }
} 