#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>
#include "../../include/gui/calendar_adapter.h"
#include "../../include/lunar_calendar.h"
#include "../../include/lunar_renderer.h"
#include "../../include/gui/gui_app.h"
#include "../../include/gui/calendar_events.h"

// Define M_PI if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function (unused, removed)
/* static SpecialDayType get_special_day_for_lunar(LunarDay lunar_day) {
    // Call the actual implementation which accepts a LunarDay parameter
    return get_special_day_type(lunar_day);
} */

// Moon phase icon names for GTK
static const char* MOON_PHASE_ICONS[] = {
    "lunar-calendar-new-moon-symbolic",         // NEW_MOON
    "lunar-calendar-waxing-crescent-symbolic",  // WAXING_CRESCENT
    "lunar-calendar-first-quarter-symbolic",    // FIRST_QUARTER
    "lunar-calendar-waxing-gibbous-symbolic",   // WAXING_GIBBOUS
    "lunar-calendar-full-moon-symbolic",        // FULL_MOON
    "lunar-calendar-waning-gibbous-symbolic",   // WANING_GIBBOUS
    "lunar-calendar-last-quarter-symbolic",     // LAST_QUARTER
    "lunar-calendar-waning-crescent-symbolic"   // WANING_CRESCENT
};

// Fallback icon names (standard GTK icons)
static const char* FALLBACK_MOON_PHASE_ICONS[] = {
    "weather-clear-night-symbolic",   // NEW_MOON
    "weather-few-clouds-night-symbolic", // WAXING_CRESCENT
    "weather-overcast-symbolic",      // FIRST_QUARTER
    "weather-few-clouds-night-symbolic", // WAXING_GIBBOUS
    "weather-clear-night-symbolic",   // FULL_MOON
    "weather-few-clouds-night-symbolic", // WANING_GIBBOUS
    "weather-overcast-symbolic",      // LAST_QUARTER
    "weather-few-clouds-night-symbolic"  // WANING_CRESCENT
};

// Default month names (fallback if config fails)
static const char* default_month_names[] = {
    "Month 1", "Month 2", "Month 3", "Month 4", "Month 5", "Month 6",
    "Month 7", "Month 8", "Month 9", "Month 10", "Month 11", "Month 12", "Month 13"
};

// Get all necessary display information for a specific Gregorian date cell.
// This relies entirely on the backend gregorian_to_lunar function.
CalendarDayCell* calendar_adapter_get_day_info(int year, int month, int day) {
    CalendarDayCell* cell = g_malloc0(sizeof(CalendarDayCell));
    if (!cell) {
        perror("Failed to allocate CalendarDayCell");
        return NULL;
    }
    
    // Set Gregorian date
    cell->greg_year = year;
    cell->greg_month = month;
    cell->greg_day = day;
    
    // Get today's date for comparison
    cell->is_today = calendar_adapter_is_today(year, month, day);
    
    // Get *all* lunar date info from the refactored backend function
    LunarDay lunar_day_info = gregorian_to_lunar(year, month, day);
    
    // Populate cell from the LunarDay struct returned by the backend
    cell->lunar_day = lunar_day_info.lunar_day;
    cell->lunar_month = lunar_day_info.lunar_month;
    cell->lunar_year = lunar_day_info.lunar_year; // This is the lunar year identifier
    cell->moon_phase = lunar_day_info.moon_phase;
    cell->weekday = lunar_day_info.weekday;
    
    // Check for special days using the function from lunar_renderer
    // (which should now use correct backend checks)
    cell->special_day_type = get_special_day_type(lunar_day_info); 
    cell->is_special_day = (cell->special_day_type != NORMAL_DAY);

    // Check for events associated with this Gregorian date (unused here, checked in GUI)
    // gboolean has_event = event_date_has_events(year, month, day);

    // Tooltip: Generate dynamically when needed, don't store in cell struct
    cell->tooltip_text = NULL; // Tooltip generated on demand by GUI code

    // Validity check - lunar_day 0 might indicate error from backend
    // The CalendarDayCell struct doesn't have an is_valid field either.
    // The calling code (create_month_model) should handle potential errors from gregorian_to_lunar.

    return cell;
}

// Get the name for a moon phase
const char* calendar_adapter_get_moon_phase_name(MoonPhase phase) {
    switch (phase) {
        case NEW_MOON:        return "New Moon";
        case WAXING_CRESCENT: return "Waxing Crescent";
        case FIRST_QUARTER:   return "First Quarter";
        case WAXING_GIBBOUS:  return "Waxing Gibbous";
        case FULL_MOON:       return "Full Moon";
        case WANING_GIBBOUS:  return "Waning Gibbous";
        case LAST_QUARTER:    return "Last Quarter";
        case WANING_CRESCENT: return "Waning Crescent";
        default:              return "Unknown Phase";
    }
}

// Get the icon name for a moon phase
GtkWidget* calendar_adapter_get_moon_phase_icon(MoonPhase phase) {
    if (phase < NEW_MOON || phase > WANING_CRESCENT) phase = NEW_MOON;
    GtkIconTheme* icon_theme = gtk_icon_theme_get_default();
    GtkWidget* image = NULL;
    if (gtk_icon_theme_has_icon(icon_theme, MOON_PHASE_ICONS[phase])) {
        image = gtk_image_new_from_icon_name(MOON_PHASE_ICONS[phase], GTK_ICON_SIZE_BUTTON);
    } else if (gtk_icon_theme_has_icon(icon_theme, FALLBACK_MOON_PHASE_ICONS[phase])) {
        image = gtk_image_new_from_icon_name(FALLBACK_MOON_PHASE_ICONS[phase], GTK_ICON_SIZE_BUTTON);
    } else {
        image = gtk_image_new_from_icon_name("image-missing-symbolic", GTK_ICON_SIZE_BUTTON);
        fprintf(stderr, "Warning: Missing moon phase icon: %s and fallback %s\n", 
                MOON_PHASE_ICONS[phase], FALLBACK_MOON_PHASE_ICONS[phase]);
    }
    if (image) gtk_widget_set_tooltip_text(image, calendar_adapter_get_moon_phase_name(phase));
    return image;
}

// Create a GdkPixbuf for the moon phase icon
GdkPixbuf* create_moon_phase_icon(MoonPhase phase, int size) {
    // Check bounds
    if (phase < NEW_MOON || phase > WANING_CRESCENT) {
        phase = NEW_MOON; // Default
    }

    GtkIconTheme* icon_theme = gtk_icon_theme_get_default();
    GError* error = NULL;
    GdkPixbuf* pixbuf = NULL;

    // Try themed icon first
    if (gtk_icon_theme_has_icon(icon_theme, MOON_PHASE_ICONS[phase])) {
        pixbuf = gtk_icon_theme_load_icon(icon_theme, MOON_PHASE_ICONS[phase], size, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
    } 
    
    // Try fallback icon if themed failed or doesn't exist
    if (!pixbuf && gtk_icon_theme_has_icon(icon_theme, FALLBACK_MOON_PHASE_ICONS[phase])) {
        g_clear_error(&error);
        pixbuf = gtk_icon_theme_load_icon(icon_theme, FALLBACK_MOON_PHASE_ICONS[phase], size, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
    }
    
    // If all else fails, use a placeholder
    if (!pixbuf) {
        g_clear_error(&error);
        pixbuf = gtk_icon_theme_load_icon(icon_theme, "image-missing-symbolic", size, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
        g_warning("Missing moon phase icon: %s and fallback %s. Using placeholder.", 
                  MOON_PHASE_ICONS[phase], FALLBACK_MOON_PHASE_ICONS[phase]);
    }

    if (error) {
        g_warning("Error loading moon phase icon: %s", error->message);
        g_error_free(error);
        // Return the placeholder if an error occurred during loading
        if (pixbuf) g_object_unref(pixbuf);
        pixbuf = gtk_icon_theme_load_icon(icon_theme, "image-missing-symbolic", size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
    }

    return pixbuf;
}

// Get the color for a special day type
void calendar_adapter_get_special_day_color(SpecialDayType type, GdkRGBA* color) {
    // Colors defined as constants for clarity
    const GdkRGBA C_TODAY = {0.8, 0.9, 1.0, 0.8}; // Light Blue, less transparent
    const GdkRGBA C_NEW_MOON = {0.9, 0.9, 0.9, 0.6}; // Light Grey
    const GdkRGBA C_FULL_MOON = {1.0, 1.0, 0.8, 0.7}; // Light Yellow
    const GdkRGBA C_WINTER = {0.8, 1.0, 1.0, 0.6}; // Light Cyan
    const GdkRGBA C_SPRING = {0.8, 1.0, 0.8, 0.6}; // Light Green
    const GdkRGBA C_SUMMER = {1.0, 0.9, 0.8, 0.6}; // Light Orange/Red
    const GdkRGBA C_FALL = {1.0, 0.85, 0.8, 0.6}; // Light Magenta/Orange
    const GdkRGBA C_NEW_YEAR = {1.0, 0.8, 0.8, 0.7}; // Light Red
    const GdkRGBA C_FESTIVAL = {0.9, 0.8, 1.0, 0.6}; // Light Purple
    const GdkRGBA C_DEFAULT = {1.0, 1.0, 1.0, 0.0}; // Default (transparent)

    switch (type) {
        case TODAY:                 *color = C_TODAY; break;
        case NEW_MOON_DAY:          *color = C_NEW_MOON; break;
        case FULL_MOON_DAY:         *color = C_FULL_MOON; break;
        case GERMANIC_NEW_YEAR_DAY: *color = C_NEW_YEAR; break;
        case WINTER_SOLSTICE_DAY:   *color = C_WINTER; break;
        case SPRING_EQUINOX_DAY:    *color = C_SPRING; break;
        case SUMMER_SOLSTICE_DAY:   *color = C_SUMMER; break;
        case FALL_EQUINOX_DAY:      *color = C_FALL; break;
        case FESTIVAL_DAY:          *color = C_FESTIVAL; break;
        case NORMAL_DAY:
        default:                    *color = C_DEFAULT; break;
    }
}

// Generate tooltip text for a calendar day
char* calendar_adapter_get_tooltip_for_day(CalendarDayCell* cell) {
    if (!cell) return NULL;
    
    // Get lunar date info (this might re-run conversion if not cached)
    LunarDay lunar_day = gregorian_to_lunar(cell->greg_year, cell->greg_month, cell->greg_day);
    
    // Build the tooltip string
    GString* tooltip = g_string_new(NULL);
    g_string_append_printf(tooltip, "Gregorian: %04d-%02d-%02d\n", 
                           cell->greg_year, cell->greg_month, cell->greg_day);
    g_string_append_printf(tooltip, "Lunar: Yr %d, M %d, D %d\n", 
                           cell->lunar_year, cell->lunar_month, cell->lunar_day);
    g_string_append_printf(tooltip, "Eld Year: %d\n", lunar_day.eld_year); // Use from re-fetched data
    g_string_append_printf(tooltip, "Phase: %s\n", calendar_adapter_get_moon_phase_name(cell->moon_phase));
    g_string_append_printf(tooltip, "Weekday: %s",
                           cell->weekday == SUNDAY ? "Sunday" :
                           cell->weekday == MONDAY ? "Monday" :
                           cell->weekday == TUESDAY ? "Tuesday" :
                           cell->weekday == WEDNESDAY ? "Wednesday" :
                           cell->weekday == THURSDAY ? "Thursday" :
                           cell->weekday == FRIDAY ? "Friday" :
                           cell->weekday == SATURDAY ? "Saturday" : "Unknown");
                           
    // Add special day info if applicable
    if (cell->special_day_type != NORMAL_DAY) {
        const char* special_name = "Special Day"; // Placeholder
        switch(cell->special_day_type) {
            case TODAY: special_name = "Today"; break;
            case NEW_MOON_DAY: special_name = "New Moon"; break;
            case FULL_MOON_DAY: special_name = "Full Moon"; break;
            case GERMANIC_NEW_YEAR_DAY: special_name = "Lunar New Year"; break;
            case WINTER_SOLSTICE_DAY: special_name = "Winter Solstice"; break;
            case SPRING_EQUINOX_DAY: special_name = "Spring Equinox"; break;
            case SUMMER_SOLSTICE_DAY: special_name = "Summer Solstice"; break;
            case FALL_EQUINOX_DAY: special_name = "Fall Equinox"; break;
            default: break;
        }
        g_string_append_printf(tooltip, "\n(%s)", special_name);
    }
    
    // Add event info if applicable
    EventList* events = event_get_for_date(cell->greg_year, cell->greg_month, cell->greg_day);
    if (events && events->count > 0) {
        g_string_append(tooltip, "\nEvents:");
        for (int i = 0; i < events->count; i++) {
            g_string_append_printf(tooltip, "\n- %s", events->events[i]->title);
        }
    }

    return g_string_free(tooltip, FALSE);
}

// Check if a given date is today
gboolean calendar_adapter_is_today(int year, int month, int day) {
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    return (year == tm_now->tm_year + 1900 &&
            month == tm_now->tm_mon + 1 &&
            day == tm_now->tm_mday);
}

// Check if a lunar year is a leap year
int calendar_adapter_is_lunar_leap_year(int year_identifier) {
    return is_lunar_leap_year(year_identifier);
}

// Get number of months in a lunar year
int calendar_adapter_get_months_in_year(int year_identifier) {
    return get_lunar_months_in_year(year_identifier);
}

// Get Unicode character for a moon phase
const char* calendar_adapter_get_unicode_moon(MoonPhase phase) {
    switch (phase) {
        case NEW_MOON:        return "🌑";
        case WAXING_CRESCENT: return "🌒";
        case FIRST_QUARTER:   return "🌓";
        case WAXING_GIBBOUS:  return "🌔";
        case FULL_MOON:       return "🌕";
        case WANING_GIBBOUS:  return "🌖";
        case LAST_QUARTER:    return "🌗";
        case WANING_CRESCENT: return "🌘";
        default:              return "⚫"; // Fallback: black circle
    }
}

// Helper to get month name from config or default
static const char* get_display_month_name(int month_num) {
    if (month_num < 1 || month_num > 13) return "Invalid Month";
    // Access config via global app data if needed (see gui_main.c)
    // LunarCalendarApp* app = g_object_get_data(G_OBJECT(g_application_get_default()), "app_data");
    // if (app && app->config && app->config->custom_month_names[month_num - 1] && strlen(app->config->custom_month_names[month_num - 1]) > 0)
    //    return app->config->custom_month_names[month_num - 1];
    // else
         return default_month_names[month_num - 1]; // Use static default for now
}

// Create the data model for a specific lunar month
CalendarGridModel* calendar_adapter_create_month_model(int year_identifier, int lunar_month) {
    CalendarGridModel* model = g_malloc0(sizeof(CalendarGridModel));
    if (!model) {
        perror("Failed to allocate CalendarGridModel");
        return NULL;
    }

    model->display_year = year_identifier;
    model->display_month = lunar_month;
    model->rows = 6; // Standard grid size
    model->cols = 7;
    model->days_in_month = 0; // Calculated below
    model->first_day_weekday = SUNDAY; // Calculated below
    model->cells = NULL;
    model->month_name = NULL;
    model->year_str = NULL;

    // --- Calculate Month Boundaries and Length ---
    double year_start_jd = calculate_lunar_new_year_jd(year_identifier);
    if (year_start_jd == 0) goto model_error;
    
    double month_start_jd = year_start_jd;
    for (int m = 1; m < lunar_month; m++) {
        month_start_jd = find_next_phase_jd(month_start_jd, 2); // Find start of month m+1
        if (month_start_jd == 0) goto model_error;
    }
    
    double next_month_start_jd = find_next_phase_jd(month_start_jd, 2);
    if (next_month_start_jd == 0) goto model_error;
    
    // Calculate exact length and clamp
    model->days_in_month = (int)floor(next_month_start_jd - month_start_jd + 0.5); // Round to nearest day
    if (model->days_in_month < 29) model->days_in_month = 29;
    if (model->days_in_month > 30) model->days_in_month = 30;

    // --- Determine Gregorian Date and Weekday of the First Day ---
    int greg_y, greg_m, greg_d;
    // Use the calculated month_start_jd (add 0.1 to avoid landing exactly on midnight?) 
    // The JD should represent the start of the day.
    double hour_unused;
    julian_day_to_gregorian(month_start_jd, &greg_y, &greg_m, &greg_d, &hour_unused);
    model->first_day_weekday = calculate_weekday(greg_y, greg_m, greg_d);

    // --- Set Month and Year Strings ---
    model->month_name = g_strdup(get_display_month_name(lunar_month));
    model->year_str = g_strdup_printf("%d", year_identifier); // Use the identifier as the year string
   
    // --- Allocate and Populate Day Cells --- 
    model->cells = g_malloc0(sizeof(CalendarDayCell*) * model->rows * model->cols);
    if (!model->cells) {
        perror("Failed to allocate cells array in model");
        goto model_error;
    }
    // Initialize all cells to NULL
    for (int i = 0; i < model->rows * model->cols; i++) model->cells[i] = NULL;

    int current_greg_y = greg_y; 
    int current_greg_m = greg_m;
    int current_greg_d = greg_d;
    int cell_row = 0;
    int cell_col = model->first_day_weekday;
    
    for (int i = 0; i < model->days_in_month; i++) {
        int index = cell_row * model->cols + cell_col;
        if (index < 0 || index >= model->rows * model->cols) {
             fprintf(stderr, "Error: Cell index out of bounds (%d) for day %d\n", index, i + 1);
             continue; // Skip this day if index is bad
        }

        // Get info for the current Gregorian date
        model->cells[index] = calendar_adapter_get_day_info(current_greg_y, current_greg_m, current_greg_d);
        if (!model->cells[index]) {
            fprintf(stderr, "Error getting day info for %d-%d-%d (Lunar %d/%d/%d)\n", 
                    current_greg_y, current_greg_m, current_greg_d, 
                    year_identifier, lunar_month, i + 1);
            // Create a placeholder? For now, leave NULL which GUI should handle
        } else if (model->cells[index]->lunar_day == 0) {
             fprintf(stderr, "Warning: Backend indicated error for %d-%d-%d (Lunar %d/%d/%d)\n", 
                    current_greg_y, current_greg_m, current_greg_d, 
                    year_identifier, lunar_month, i + 1);
             // Mark cell as invalid? The struct doesn't have is_valid.
             // GUI needs to handle potentially incomplete cells from backend errors.
        }
        
        // Advance Gregorian date by one day using JD
        double current_jd = gregorian_to_julian_day(current_greg_y, current_greg_m, current_greg_d, 12.0);
        double next_jd = current_jd + 1.0;
        julian_day_to_gregorian(next_jd, &current_greg_y, &current_greg_m, &current_greg_d, &hour_unused);
        
        // Advance grid position
        cell_col++;
        if (cell_col >= model->cols) {
            cell_col = 0;
            cell_row++;
        }
    }
    
    return model;

model_error:
    fprintf(stderr, "Error creating calendar model for %d/%d\n", lunar_month, year_identifier);
    // Clean up partially created model
    if (model) {
        if (model->cells) { // No need to free individual cells if allocation failed
             g_free(model->cells);
        }
        if (model->month_name) g_free(model->month_name);
        if (model->year_str) g_free(model->year_str);
        g_free(model);
    }
    return NULL;
}

// Free the memory used by the grid model
void calendar_adapter_free_model(CalendarGridModel* model) {
    if (model) {
        if (model->cells) {
            for (int i = 0; i < model->rows * model->cols; i++) {
                if (model->cells[i]) {
                    // tooltip_text is not stored, no need to free
                    g_free(model->cells[i]);
                }
            }
            g_free(model->cells);
        }
        g_free(model->month_name);
        g_free(model->year_str);
        g_free(model);
    }
} 