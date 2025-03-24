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

// Load configuration from file
LunarCalendarConfig* config_load(const char* file_path) {
    if (!file_path) {
        return config_get_defaults();
    }
    
    // Check if the file exists
    if (!g_file_test(file_path, G_FILE_TEST_EXISTS)) {
        return config_get_defaults();
    }
    
    // Create a new config with default values
    LunarCalendarConfig* config = config_get_defaults();
    
    // Create a key file object
    GKeyFile* key_file = g_key_file_new();
    
    // Load the file
    GError* error = NULL;
    if (!g_key_file_load_from_file(key_file, file_path, G_KEY_FILE_NONE, &error)) {
        g_error_free(error);
        g_key_file_free(key_file);
        return config;
    }
    
    // Read display options
    if (g_key_file_has_group(key_file, CONFIG_SECTION_DISPLAY)) {
        if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_gregorian_dates", NULL)) {
            config->show_gregorian_dates = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_gregorian_dates", NULL);
        }
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_moon_phases", NULL)) {
            config->show_moon_phases = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_moon_phases", NULL);
        }
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "show_weekday_names", NULL)) {
            config->show_weekday_names = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_weekday_names", NULL);
        }
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "highlight_special_days", NULL)) {
            config->highlight_special_days = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "highlight_special_days", NULL);
        }
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_DISPLAY, "use_dark_theme", NULL)) {
            config->use_dark_theme = g_key_file_get_boolean(key_file, CONFIG_SECTION_DISPLAY, "use_dark_theme", NULL);
        }
    }
    
    // Read calendar options
    if (g_key_file_has_group(key_file, CONFIG_SECTION_CALENDAR)) {
        if (g_key_file_has_key(key_file, CONFIG_SECTION_CALENDAR, "start_day_of_week", NULL)) {
            config->start_day_of_week = g_key_file_get_integer(key_file, CONFIG_SECTION_CALENDAR, "start_day_of_week", NULL);
            // Validate the value
            if (config->start_day_of_week < 0 || config->start_day_of_week > 6) {
                config->start_day_of_week = DEFAULT_START_DAY;
            }
        }
    }
    
    // Read UI options
    if (g_key_file_has_group(key_file, CONFIG_SECTION_UI)) {
        if (g_key_file_has_key(key_file, CONFIG_SECTION_UI, "window_width", NULL)) {
            config->window_width = g_key_file_get_integer(key_file, CONFIG_SECTION_UI, "window_width", NULL);
            // Validate the value
            if (config->window_width < 400) {
                config->window_width = DEFAULT_WINDOW_WIDTH;
            }
        }
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_UI, "window_height", NULL)) {
            config->window_height = g_key_file_get_integer(key_file, CONFIG_SECTION_UI, "window_height", NULL);
            // Validate the value
            if (config->window_height < 300) {
                config->window_height = DEFAULT_WINDOW_HEIGHT;
            }
        }
        
        if (g_key_file_has_key(key_file, CONFIG_SECTION_UI, "ui_scale", NULL)) {
            config->ui_scale = g_key_file_get_double(key_file, CONFIG_SECTION_UI, "ui_scale", NULL);
            // Validate the value
            if (config->ui_scale < 0.5 || config->ui_scale > 2.0) {
                config->ui_scale = DEFAULT_UI_SCALE;
            }
        }
    }
    
    // Free the key file
    g_key_file_free(key_file);
    
    return config;
}

// Save configuration to file
gboolean config_save(const char* file_path, LunarCalendarConfig* config) {
    if (!file_path || !config) {
        return FALSE;
    }
    
    // Create a key file object
    GKeyFile* key_file = g_key_file_new();
    
    // Add display options
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_gregorian_dates", config->show_gregorian_dates);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_moon_phases", config->show_moon_phases);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "show_weekday_names", config->show_weekday_names);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "highlight_special_days", config->highlight_special_days);
    g_key_file_set_boolean(key_file, CONFIG_SECTION_DISPLAY, "use_dark_theme", config->use_dark_theme);
    
    // Add calendar options
    g_key_file_set_integer(key_file, CONFIG_SECTION_CALENDAR, "start_day_of_week", config->start_day_of_week);
    
    // Add UI options
    g_key_file_set_integer(key_file, CONFIG_SECTION_UI, "window_width", config->window_width);
    g_key_file_set_integer(key_file, CONFIG_SECTION_UI, "window_height", config->window_height);
    g_key_file_set_double(key_file, CONFIG_SECTION_UI, "ui_scale", config->ui_scale);
    
    // Save the file
    GError* error = NULL;
    gboolean success = g_key_file_save_to_file(key_file, file_path, &error);
    if (!success) {
        g_error_free(error);
    }
    
    // Free the key file
    g_key_file_free(key_file);
    
    return success;
}

// Get default configuration
LunarCalendarConfig* config_get_defaults(void) {
    LunarCalendarConfig* config = g_malloc0(sizeof(LunarCalendarConfig));
    if (!config) {
        return NULL;
    }
    
    // Set default values
    config->show_gregorian_dates = DEFAULT_SHOW_GREGORIAN;
    config->show_moon_phases = DEFAULT_SHOW_MOON_PHASES;
    config->show_weekday_names = DEFAULT_SHOW_WEEKDAYS;
    config->highlight_special_days = DEFAULT_HIGHLIGHT_SPECIAL_DAYS;
    config->use_dark_theme = DEFAULT_USE_DARK_THEME;
    config->start_day_of_week = DEFAULT_START_DAY;
    config->window_width = DEFAULT_WINDOW_WIDTH;
    config->window_height = DEFAULT_WINDOW_HEIGHT;
    config->ui_scale = DEFAULT_UI_SCALE;
    
    return config;
}

// Apply configuration to the application
void config_apply(LunarCalendarApp* app, LunarCalendarConfig* config) {
    if (!app || !config) {
        return;
    }
    
    // Apply window size
    if (app->window) {
        gtk_window_set_default_size(GTK_WINDOW(app->window), 
                                   config->window_width, 
                                   config->window_height);
    }
    
    // Apply dark theme
    GtkSettings* settings = gtk_settings_get_default();
    if (settings) {
        g_object_set(settings, "gtk-application-prefer-dark-theme", config->use_dark_theme, NULL);
    }
    
    // Other config options would be applied to relevant widgets
    // For now, we just have window size and dark theme
}

// Free configuration structure
void config_free(LunarCalendarConfig* config) {
    g_free(config);
} 