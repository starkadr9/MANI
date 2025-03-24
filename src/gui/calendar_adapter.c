#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>
#include "../../include/gui/calendar_adapter.h"
#include "../../include/lunar_calendar.h"
#include "../../include/lunar_renderer.h"

// Define M_PI if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to resolve function signature conflict
static SpecialDayType get_special_day_for_lunar(LunarDay lunar_day) {
    // Call the actual implementation which accepts a LunarDay parameter
    return get_special_day_type(lunar_day);
}

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

// Find full moon dates for a given year (fills an array with up to 15 full moon dates)
int get_year_full_moons(int year, Date* full_moons, int max_moons) {
    // Start searching from December of previous year to catch full moons near year boundary
    Date current = { year-1, 12, 1 };
    int moon_count = 0;
    int last_full_moon_day = 0;  // Track days since last full moon to ensure proper spacing
    
    // Continue until we reach next year or find all the moons we can store
    while (current.year <= year && moon_count < max_moons) {
        // Check each day of the year for full moons
        LunarDay lunar_day = gregorian_to_lunar(current.year, current.month, current.day);
        
        if (lunar_day.moon_phase == FULL_MOON) {
            // Ensure we don't add full moons too close together (must be at least 25 days apart)
            if (moon_count == 0 || last_full_moon_day >= 25) {
                full_moons[moon_count].year = current.year;
                full_moons[moon_count].month = current.month;
                full_moons[moon_count].day = current.day;
                moon_count++;
                last_full_moon_day = 0;
                
                // Skip ahead ~25 days to avoid finding the same full moon
                // but not too far to miss the next one
                for (int i = 0; i < 25 && current.year <= year; i++) {
                    if (current.day < 28) {
                        current.day++;
                    } else {
                        current.day = 1;
                        if (current.month < 12) {
                            current.month++;
                        } else {
                            current.month = 1;
                            current.year++;
                        }
                    }
                    last_full_moon_day++;
                }
            } else {
                // If this full moon is too close to previous, skip ahead just one day
                if (current.day < 28) {
                    current.day++;
                } else {
                    current.day = 1;
                    if (current.month < 12) {
                        current.month++;
                    } else {
                        current.month = 1;
                        current.year++;
                    }
                }
                last_full_moon_day++;
            }
        } else {
            // Move to next day
            if (current.day < 28) {
                current.day++;
            } else {
                // Handle month end more carefully
                int days_in_month = 31;
                if (current.month == 4 || current.month == 6 || 
                    current.month == 9 || current.month == 11) {
                    days_in_month = 30;
                } else if (current.month == 2) {
                    days_in_month = is_gregorian_leap_year(current.year) ? 29 : 28;
                }
                
                if (current.day < days_in_month) {
                    current.day++;
                } else {
                    current.day = 1;
                    if (current.month < 12) {
                        current.month++;
                    } else {
                        current.month = 1;
                        current.year++;
                    }
                }
            }
            last_full_moon_day++;
        }
    }
    
    // Verify the spacing between full moons (should be ~29.5 days)
    for (int i = 1; i < moon_count; i++) {
        int days = days_between(full_moons[i-1], full_moons[i]);
        // If the spacing is too short or too long, adjust
        if (days < 29 || days > 30) {
            // Estimate the correct date (29.5 days after previous)
            Date corrected = full_moons[i-1];
            
            // Add 29 days
            for (int j = 0; j < 29; j++) {
                if (corrected.day < 28) {
                    corrected.day++;
                } else {
                    // Check month length
                    int days_in_month = 31;
                    if (corrected.month == 4 || corrected.month == 6 || 
                        corrected.month == 9 || corrected.month == 11) {
                        days_in_month = 30;
                    } else if (corrected.month == 2) {
                        days_in_month = is_gregorian_leap_year(corrected.year) ? 29 : 28;
                    }
                    
                    if (corrected.day < days_in_month) {
                        corrected.day++;
                    } else {
                        corrected.day = 1;
                        if (corrected.month < 12) {
                            corrected.month++;
                        } else {
                            corrected.month = 1;
                            corrected.year++;
                        }
                    }
                }
            }
            
            // Use the corrected date
            full_moons[i] = corrected;
        }
    }
    
    return moon_count;
}

// Compare two dates, returns:
// -1 if a < b
// 0 if a == b
// 1 if a > b
int compare_dates(Date a, Date b) {
    if (a.year < b.year) return -1;
    if (a.year > b.year) return 1;
    if (a.month < b.month) return -1;
    if (a.month > b.month) return 1;
    if (a.day < b.day) return -1;
    if (a.day > b.day) return 1;
    return 0;
}

// Calculate days between two dates
int days_between(Date start, Date end) {
    // Use a simple Julian day calculation for this purpose
    int a = (14 - start.month) / 12;
    int y = start.year + 4800 - a;
    int m = start.month + 12 * a - 3;
    int jd_start = start.day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    
    a = (14 - end.month) / 12;
    y = end.year + 4800 - a;
    m = end.month + 12 * a - 3;
    int jd_end = end.day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    
    return jd_end - jd_start;
}

// Get Germanic lunar day info for a specific date (starting with full moons)
CalendarDayCell* calendar_adapter_get_germanic_day_info(int year, int month, int day) {
    CalendarDayCell* cell = g_malloc0(sizeof(CalendarDayCell));
    if (!cell) {
        return NULL;
    }
    
    // Set Gregorian date
    cell->greg_year = year;
    cell->greg_month = month;
    cell->greg_day = day;
    
    // Get today's date
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    
    // Check if this is today
    cell->is_today = (year == tm_now->tm_year + 1900 &&
                     month == tm_now->tm_mon + 1 &&
                     day == tm_now->tm_mday);
    
    // Get lunar date info
    LunarDay lunar_day = gregorian_to_lunar(year, month, day);
    
    // Set lunar date
    cell->lunar_day = lunar_day.lunar_day;
    cell->lunar_month = lunar_day.lunar_month;
    cell->lunar_year = lunar_day.lunar_year;
    
    // Set other properties
    cell->moon_phase = lunar_day.moon_phase;
    cell->weekday = lunar_day.weekday;
    
    // Create a Date structure for the current date
    Date current_date = { year, month, day };
    
    // Get full moons for current year (and maybe some from previous/next year)
    Date full_moons[15]; // Store up to 15 full moons
    int moon_count = get_year_full_moons(year, full_moons, 15);
    
    // Find the full moon that starts the current lunar month
    // (the last full moon before or on the current date)
    int current_moon_index = -1;
    for (int i = 0; i < moon_count; i++) {
        if (compare_dates(full_moons[i], current_date) <= 0) {
            current_moon_index = i;
        } else {
            break; // Found the first full moon after our date
        }
    }
    
    if (current_moon_index == -1 && moon_count > 0) {
        // If we didn't find a full moon before our date, but we have full moons,
        // then our date is before the first full moon of the year.
        // Use the last full moon of the previous year (if available)
        Date prev_full_moons[5];
        int prev_moon_count = get_year_full_moons(year-1, prev_full_moons, 5);
        if (prev_moon_count > 0) {
            // Use the last full moon of the previous year
            Date start_moon = prev_full_moons[prev_moon_count-1];
            int days = days_between(start_moon, current_date);
            cell->lunar_day = days + 1;  // Day 1 is the full moon day
            cell->lunar_month = 12;  // Last month of previous year
        } else {
            // Fallback (shouldn't happen often)
            cell->lunar_day = 1;
            cell->lunar_month = 1;
        }
    } else if (current_moon_index >= 0 && current_moon_index < moon_count) {
        // We found the full moon that starts our current lunar month
        Date start_moon = full_moons[current_moon_index];
        int days = days_between(start_moon, current_date);
        cell->lunar_day = days + 1;  // Day 1 is the full moon day
        
        // Find winter solstice for current year
        Date winter_solstice = { year, 12, 21 };
        
        // If we're before winter solstice, use last year's
        if (month < 12 || (month == 12 && day < 21)) {
            winter_solstice.year--;
        }
        
        // Count how many full moons have occurred since the winter solstice
        int month_number = 1;
        for (int i = 0; i < moon_count; i++) {
            if (compare_dates(full_moons[i], winter_solstice) > 0 && 
                compare_dates(full_moons[i], current_date) <= 0) {
                month_number++;
            }
        }
        
        cell->lunar_month = month_number;
    } else {
        // Fallback for edge cases
        cell->lunar_day = 1;
        cell->lunar_month = 1;
    }
    
    // Set the lunar year
    cell->lunar_year = year;
    
    // Check for special day
    SpecialDayType special_day = get_special_day_for_lunar(lunar_day);
    cell->is_special_day = (special_day != NORMAL_DAY);
    cell->special_day_type = special_day;
    
    // Create tooltip text with Germanic lunar calendar information
    char* tooltip = g_strdup_printf(
        "Gregorian: %04d-%02d-%02d\n"
        "Germanic Lunar: Month %d, Day %d\n"
        "Moon Phase: %s\n"
        "Weekday: %s\n"
        "Eld Year: %d",
        year, month, day,
        cell->lunar_month, cell->lunar_day,
        calendar_adapter_get_moon_phase_name(cell->moon_phase),
        cell->weekday == SUNDAY ? "Sunday" :
        cell->weekday == MONDAY ? "Monday" :
        cell->weekday == TUESDAY ? "Tuesday" :
        cell->weekday == WEDNESDAY ? "Wednesday" :
        cell->weekday == THURSDAY ? "Thursday" :
        cell->weekday == FRIDAY ? "Friday" :
        cell->weekday == SATURDAY ? "Saturday" : "Unknown",
        lunar_day.eld_year);
    
    cell->tooltip_text = tooltip;
    
    return cell;
}

// Get the name for a moon phase
const char* calendar_adapter_get_moon_phase_name(MoonPhase phase) {
    switch (phase) {
        case NEW_MOON: return "New Moon";
        case WAXING_CRESCENT: return "Waxing Crescent";
        case FIRST_QUARTER: return "First Quarter";
        case WAXING_GIBBOUS: return "Waxing Gibbous";
        case FULL_MOON: return "Full Moon";
        case WANING_GIBBOUS: return "Waning Gibbous";
        case LAST_QUARTER: return "Last Quarter";
        case WANING_CRESCENT: return "Waning Crescent";
        default: return "Unknown";
    }
}

/* Get a text label for the specified moon phase */
const char* get_moon_phase_name(MoonPhase phase) {
    switch (phase) {
        case NEW_MOON:
            return "New Moon";
        case WAXING_CRESCENT:
            return "Waxing Crescent";
        case FIRST_QUARTER:
            return "First Quarter";
        case WAXING_GIBBOUS:
            return "Waxing Gibbous";
        case FULL_MOON:
            return "Full Moon";
        case WANING_GIBBOUS:
            return "Waning Gibbous";
        case LAST_QUARTER:
            return "Last Quarter";
        case WANING_CRESCENT:
            return "Waning Crescent";
        default:
            return "Unknown";
    }
}

// Get an icon for a moon phase (simple text version)
GtkWidget* calendar_adapter_get_moon_phase_icon(MoonPhase phase) {
    // Create a text representation instead of icon
    const char* phase_symbol = "●"; // Default full circle
    
    switch (phase) {
        case NEW_MOON:
            phase_symbol = "○";  // Empty circle
            break;
        case WAXING_CRESCENT:
            phase_symbol = "◑";  // Half circle
            break;
        case FIRST_QUARTER:
            phase_symbol = "◑";  // Half circle
            break;
        case WAXING_GIBBOUS:
            phase_symbol = "◕";  // Almost full circle
            break;
        case FULL_MOON:
            phase_symbol = "●";  // Full circle
            break;
        case WANING_GIBBOUS:
            phase_symbol = "◕";  // Almost full circle
            break;
        case LAST_QUARTER:
            phase_symbol = "◐";  // Half circle
            break;
        case WANING_CRESCENT:
            phase_symbol = "◐";  // Half circle
            break;
        default:
            phase_symbol = "?";
            break;
    }
    
    // Create a label with the phase symbol in a large font
    GtkWidget* label = gtk_label_new(phase_symbol);
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(2.0));  // Make it larger
    gtk_label_set_attributes(GTK_LABEL(label), attrs);
    pango_attr_list_unref(attrs);
    
    return label;
}

/* Create a simple wireframe moon phase icon - specifically for the sidebar */
GdkPixbuf* create_moon_phase_icon(MoonPhase phase, int size) {
    // Create a new Cairo surface
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);
    
    // Clear the background
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    
    // Center and radius
    double center_x = size / 2.0;
    double center_y = size / 2.0;
    double radius = (size / 2.0) * 0.8; // Slightly smaller than half the size
    
    // Set line width based on size
    cairo_set_line_width(cr, size / 40.0);
    
    // Draw the dark part of the moon based on phase
    cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.6); // Dark gray
    
    switch (phase) {
        case NEW_MOON:
            // New moon - entire circle is dark
            cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
            cairo_fill(cr);
            break;
            
        case WAXING_CRESCENT:
            // Waxing crescent - right side illuminated (25%)
            cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
            cairo_fill(cr);
            
            // Cut out the illuminated part
            cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
            cairo_move_to(cr, center_x, center_y);
            cairo_arc(cr, center_x, center_y, radius, -M_PI/4, M_PI/4);
            cairo_close_path(cr);
            cairo_fill(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            break;
            
        case FIRST_QUARTER:
            // First quarter - left half dark
            cairo_move_to(cr, center_x, center_y - radius);
            cairo_arc(cr, center_x, center_y, radius, 3*M_PI/2, M_PI/2);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case WAXING_GIBBOUS:
            // Waxing gibbous - left side dark (25%)
            cairo_move_to(cr, center_x, center_y - radius);
            cairo_arc(cr, center_x, center_y, radius, 3*M_PI/4, 5*M_PI/4);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case FULL_MOON:
            // Full moon - no dark part
            break;
            
        case WANING_GIBBOUS:
            // Waning gibbous - right side dark (25%)
            cairo_move_to(cr, center_x, center_y - radius);
            cairo_arc(cr, center_x, center_y, radius, -M_PI/4, M_PI/4);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case LAST_QUARTER:
            // Last quarter - right half dark
            cairo_move_to(cr, center_x, center_y - radius);
            cairo_arc(cr, center_x, center_y, radius, -M_PI/2, 3*M_PI/2);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case WANING_CRESCENT:
            // Waning crescent - left side illuminated (25%)
            cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
            cairo_fill(cr);
            
            // Cut out the illuminated part
            cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
            cairo_move_to(cr, center_x, center_y);
            cairo_arc(cr, center_x, center_y, radius, 3*M_PI/4, 5*M_PI/4);
            cairo_close_path(cr);
            cairo_fill(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            break;
    }
    
    // Draw the wireframe outline and grid
    cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, 0.9); // Light gray
    
    // Draw the main circle outline
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    // Vertical line
    cairo_move_to(cr, center_x, center_y - radius);
    cairo_line_to(cr, center_x, center_y + radius);
    cairo_stroke(cr);
    
    // Horizontal line
    cairo_move_to(cr, center_x - radius, center_y);
    cairo_line_to(cr, center_x + radius, center_y);
    cairo_stroke(cr);
    
    // Convert to pixbuf
    GdkPixbuf* pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    
    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    
    return pixbuf;
}

// Get the color for a special day type
void calendar_adapter_get_special_day_color(SpecialDayType type, GdkRGBA* color) {
    // Default to transparent (no color)
    color->red = 1.0;
    color->green = 1.0;
    color->blue = 1.0;
    color->alpha = 0.0;
    
    switch (type) {
        case TODAY:
            color->red = 0.6;
            color->green = 0.8;
            color->blue = 1.0;  // Light blue
            color->alpha = 0.3;  // More transparent
            break;
        case NEW_MOON_DAY:
            color->red = 0.7;
            color->green = 0.7;
            color->blue = 0.7;  // Light gray
            color->alpha = 0.3;
            break;
        case FULL_MOON_DAY:
            color->red = 1.0;
            color->green = 1.0;
            color->blue = 0.7;  // Light yellow
            color->alpha = 0.4;
            break;
        case GERMANIC_NEW_YEAR_DAY:
            color->red = 1.0;
            color->green = 0.8;
            color->blue = 0.8;  // Light red/pink
            color->alpha = 0.4;
            break;
        case WINTER_SOLSTICE_DAY:
            color->red = 0.8;
            color->green = 1.0;
            color->blue = 1.0;  // Light cyan
            color->alpha = 0.3;
            break;
        case SPRING_EQUINOX_DAY:
            color->red = 0.8;
            color->green = 1.0;
            color->blue = 0.8;  // Light green
            color->alpha = 0.3;
            break;
        case SUMMER_SOLSTICE_DAY:
            color->red = 1.0;
            color->green = 0.9;
            color->blue = 0.7;  // Light orange
            color->alpha = 0.3;
            break;
        case FALL_EQUINOX_DAY:
            color->red = 1.0;
            color->green = 0.8;
            color->blue = 1.0;  // Light purple
            color->alpha = 0.3;
            break;
        case FESTIVAL_DAY:
            color->red = 1.0;
            color->green = 0.8;
            color->blue = 1.0;  // Light purple
            color->alpha = 0.3;
            break;
        case NORMAL_DAY:
        default:
            // Already set to transparent
            break;
    }
}

// Generate tooltip text for a calendar day
char* calendar_adapter_get_tooltip_for_day(CalendarDayCell* day) {
    if (!day) {
        return NULL;
    }
    
    // Build a detailed tooltip with all the day's information
    char* tooltip = g_strdup_printf(
        "Gregorian: %04d-%02d-%02d\n"
        "Lunar: Year %d, Month %d, Day %d\n"
        "Moon Phase: %s\n"
        "Weekday: %s",
        day->greg_year, day->greg_month, day->greg_day,
        day->lunar_year, day->lunar_month, day->lunar_day,
        calendar_adapter_get_moon_phase_name(day->moon_phase),
        day->weekday == SUNDAY ? "Sunday" :
        day->weekday == MONDAY ? "Monday" :
        day->weekday == TUESDAY ? "Tuesday" :
        day->weekday == WEDNESDAY ? "Wednesday" :
        day->weekday == THURSDAY ? "Thursday" :
        day->weekday == FRIDAY ? "Friday" :
        day->weekday == SATURDAY ? "Saturday" : "Unknown"
    );
    
    return tooltip;
}

// Check if the given date is today
gboolean calendar_adapter_is_today(int year, int month, int day) {
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    
    return (year == tm_now->tm_year + 1900 &&
            month == tm_now->tm_mon + 1 &&
            day == tm_now->tm_mday);
}

// Month names
static const char* MONTH_NAMES[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December", "Thirteenth"
};

// Create a calendar model for a specific month/year
CalendarGridModel* calendar_adapter_create_month_model(int year, int month) {
    CalendarGridModel* model = g_malloc0(sizeof(CalendarGridModel));
    if (!model) {
        return NULL;
    }
    
    // Set basic properties
    model->display_year = year;
    model->display_month = month;
    
    // Determine if we're showing a Gregorian or lunar month
    // For now, we'll just show Gregorian months
    
    // Get first day of month weekday
    model->first_day_weekday = calculate_weekday(year, month, 1);
    
    // Calculate days in month (simplified for Gregorian calendar)
    model->days_in_month = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        model->days_in_month = 30;
    } else if (month == 2) {
        model->days_in_month = is_gregorian_leap_year(year) ? 29 : 28;
    }
    
    // Set up the grid dimensions
    model->rows = 6;  // Maximum possible rows in a month view
    model->cols = 7;  // Days of the week
    
    // Allocate the grid cells
    model->cells = g_malloc0(sizeof(CalendarDayCell*) * model->rows * model->cols);
    if (!model->cells) {
        g_free(model);
        return NULL;
    }
    
    // Initialize all cells to NULL
    for (int i = 0; i < model->rows * model->cols; i++) {
        model->cells[i] = NULL;
    }
    
    // Fill in the grid with actual day data
    int row = 0;
    int col = model->first_day_weekday;
    
    for (int day = 1; day <= model->days_in_month; day++) {
        // Calculate the index in the flat array
        int index = row * model->cols + col;
        
        // Create the cell
        model->cells[index] = calendar_adapter_get_day_info(year, month, day);
        
        // Move to the next cell
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }
    
    // Set month and year strings
    model->month_name = g_strdup(MONTH_NAMES[month - 1]);
    model->year_str = g_strdup_printf("%d", year);
    
    return model;
}

// Free the calendar model
void calendar_adapter_free_model(CalendarGridModel* model) {
    if (!model) {
        return;
    }
    
    // Free all the cells
    for (int i = 0; i < model->rows * model->cols; i++) {
        if (model->cells[i]) {
            g_free((void*)model->cells[i]->tooltip_text);
            g_free(model->cells[i]);
        }
    }
    
    // Free the cell array
    g_free(model->cells);
    
    // Free strings
    g_free(model->month_name);
    g_free(model->year_str);
    
    // Free the model itself
    g_free(model);
}

// Get lunar day info for a specific date
CalendarDayCell* calendar_adapter_get_day_info(int year, int month, int day) {
    CalendarDayCell* cell = g_malloc0(sizeof(CalendarDayCell));
    if (!cell) {
        return NULL;
    }
    
    // Set Gregorian date
    cell->greg_year = year;
    cell->greg_month = month;
    cell->greg_day = day;
    
    // Get lunar date info
    LunarDay lunar_day = gregorian_to_lunar(year, month, day);
    
    // Set lunar date
    cell->lunar_day = lunar_day.lunar_day;
    cell->lunar_month = lunar_day.lunar_month;
    cell->lunar_year = lunar_day.lunar_year;
    
    // Set other properties
    cell->moon_phase = lunar_day.moon_phase;
    cell->weekday = lunar_day.weekday;
    
    // Check if this is today
    cell->is_today = calendar_adapter_is_today(year, month, day);
    
    // Check for special day
    SpecialDayType special_day = get_special_day_for_lunar(lunar_day);
    cell->is_special_day = (special_day != NORMAL_DAY);
    cell->special_day_type = special_day;
    
    // Create tooltip text
    cell->tooltip_text = calendar_adapter_get_tooltip_for_day(cell);
    
    return cell;
} 