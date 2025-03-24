#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <time.h>
#include "../../include/gui/gui_app.h"
#include "../../include/gui/config.h"
#include "../../include/gui/calendar_adapter.h"
#include "../../include/lunar_calendar.h"
#include "../../include/lunar_renderer.h"

// Get weekday name
static const char* get_weekday_name(Weekday weekday) {
    switch (weekday) {
        case SUNDAY: return "Sunday";
        case MONDAY: return "Monday";
        case TUESDAY: return "Tuesday";
        case WEDNESDAY: return "Wednesday";
        case THURSDAY: return "Thursday";
        case FRIDAY: return "Friday";
        case SATURDAY: return "Saturday";
        default: return "Unknown";
    }
}

// Forward declarations
static void activate(GtkApplication* app, gpointer user_data);
static void build_ui(LunarCalendarApp* app);
static void on_window_destroy(GtkWidget* widget, gpointer data);
static void update_calendar_view(LunarCalendarApp* app);
static void on_month_changed(GtkWidget* widget, gpointer data);
static void on_year_changed(GtkWidget* widget, gpointer data);
static void on_prev_month(GtkWidget* widget, gpointer data);
static void on_next_month(GtkWidget* widget, gpointer data);
static void update_header(LunarCalendarApp* app);
static void update_sidebar(LunarCalendarApp* app);
static void update_ui(LunarCalendarApp* app);
static void update_month_label(LunarCalendarApp* app);

int main(int argc, char** argv) {
    // Initialize the GUI application
    LunarCalendarApp* app = gui_app_init(&argc, &argv);
    if (!app) {
        fprintf(stderr, "Failed to initialize application\n");
        return 1;
    }
    
    // Run the application
    int status = gui_app_run(app);
    
    // Cleanup
    gui_app_cleanup(app);
    
    return status;
}

// Initialize the GUI application
LunarCalendarApp* gui_app_init(int* argc, char*** argv) {
    // Initialize GTK
    gtk_init(argc, argv);
    
    // Allocate the application structure
    LunarCalendarApp* app = g_malloc0(sizeof(LunarCalendarApp));
    if (!app) {
        return NULL;
    }
    
    // Create the GTK application
    app->app = gtk_application_new("org.lunar.mani", G_APPLICATION_FLAGS_NONE);
    if (!app->app) {
        g_free(app);
        return NULL;
    }
    
    // Set up signals
    g_signal_connect(app->app, "activate", G_CALLBACK(activate), app);
    
    // Get current date for initial view
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    app->current_year = tm_now->tm_year + 1900;
    app->current_month = tm_now->tm_mon + 1; // GTK months are 1-12
    
    // Load configuration
    app->config_file_path = config_get_file_path();
    LunarCalendarConfig* config = NULL;
    
    if (app->config_file_path) {
        config = config_load(app->config_file_path);
    }
    
    // If config couldn't be loaded, use defaults
    if (!config) {
        config = config_get_defaults();
    }
    
    // Apply configuration (this will be fully applied when the window is created)
    app->config = config;
    
    return app;
}

// Run the application main loop
int gui_app_run(LunarCalendarApp* app) {
    return g_application_run(G_APPLICATION(app->app), 0, NULL);
}

// Clean up resources
void gui_app_cleanup(LunarCalendarApp* app) {
    if (!app) {
        return;
    }
    
    // Save configuration before exiting
    if (app->config && app->config_file_path) {
        config_save(app->config_file_path, app->config);
        config_free(app->config);
    }
    
    // Free the application object
    g_object_unref(app->app);
    
    // Free configuration path
    g_free(app->config_file_path);
    
    // Free the application structure
    g_free(app);
}

// Callback when application is activated
static void activate(GtkApplication* app, gpointer user_data) {
    LunarCalendarApp* cal_app = (LunarCalendarApp*)user_data;
    
    // Create the main window
    cal_app->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(cal_app->window), "MANI - Germanic Lunar Calendar");
    
    // Apply configuration
    if (cal_app->config) {
        config_apply(cal_app, cal_app->config);
    } else {
        // Default window size if no config
        gtk_window_set_default_size(GTK_WINDOW(cal_app->window), 800, 600);
    }
    
    // Connect destroy signal
    g_signal_connect(cal_app->window, "destroy", G_CALLBACK(on_window_destroy), cal_app);
    
    // Build the UI
    build_ui(cal_app);
    
    // Show the window and all of its contents
    gtk_widget_show_all(cal_app->window);
}

// Build the user interface
static void build_ui(LunarCalendarApp* app) {
    // Create a vertical box for the main layout
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), main_box);
    
    // Get current date info
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int current_year = tm_now->tm_year + 1900;
    int eld_year = calculate_eld_year(current_year);
    
    // Create header bar with controls
    app->header_bar = gtk_header_bar_new();
    char title[100];
    snprintf(title, sizeof(title), "MANI - Eld Year %d", eld_year);
    gtk_header_bar_set_title(GTK_HEADER_BAR(app->header_bar), title);
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(app->header_bar), "Germanic Lunar Calendar");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(app->header_bar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(app->window), app->header_bar);
    
    // Create navigation controls
    GtkWidget* nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    // Previous month button
    GtkWidget* prev_button = gtk_button_new_from_icon_name("go-previous-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(prev_button, "Previous Month");
    g_signal_connect(prev_button, "clicked", G_CALLBACK(on_prev_month), app);
    gtk_container_add(GTK_CONTAINER(nav_box), prev_button);
    
    // Month selector
    GtkWidget* month_combo = gtk_combo_box_text_new();
    for (int i = 0; i < 12; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(month_combo), 
                                       (i < 12) ? 
                                       (i == 0 ? "January" : 
                                        i == 1 ? "February" : 
                                        i == 2 ? "March" : 
                                        i == 3 ? "April" : 
                                        i == 4 ? "May" : 
                                        i == 5 ? "June" : 
                                        i == 6 ? "July" : 
                                        i == 7 ? "August" : 
                                        i == 8 ? "September" : 
                                        i == 9 ? "October" : 
                                        i == 10 ? "November" : "December") 
                                       : "Thirteenth");
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(month_combo), app->current_month - 1);
    g_signal_connect(month_combo, "changed", G_CALLBACK(on_month_changed), app);
    gtk_container_add(GTK_CONTAINER(nav_box), month_combo);
    
    // Year selector
    GtkWidget* year_spin = gtk_spin_button_new_with_range(1, 9999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(year_spin), app->current_year);
    g_signal_connect(year_spin, "value-changed", G_CALLBACK(on_year_changed), app);
    gtk_container_add(GTK_CONTAINER(nav_box), year_spin);
    
    // Next month button
    GtkWidget* next_button = gtk_button_new_from_icon_name("go-next-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(next_button, "Next Month");
    g_signal_connect(next_button, "clicked", G_CALLBACK(on_next_month), app);
    gtk_container_add(GTK_CONTAINER(nav_box), next_button);
    
    // Add navigation controls to header bar
    gtk_header_bar_pack_start(GTK_HEADER_BAR(app->header_bar), nav_box);
    
    // Create menu button for settings
    GtkWidget* menu_button = gtk_menu_button_new();
    gtk_button_set_image(GTK_BUTTON(menu_button), 
                         gtk_image_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_BUTTON));
    gtk_header_bar_pack_end(GTK_HEADER_BAR(app->header_bar), menu_button);
    
    // Create horizontal box for main content
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);
    
    // Create sidebar for additional info
    app->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(app->sidebar, 200, -1);
    gtk_box_pack_start(GTK_BOX(content_box), app->sidebar, FALSE, FALSE, 0);
    
    // Create a scrolled window for the calendar view
    GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
                                   GTK_POLICY_AUTOMATIC, 
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(content_box), scroll, TRUE, TRUE, 0);
    
    // Create an empty calendar view (will be populated later)
    app->calendar_view = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(app->calendar_view), 1);
    gtk_grid_set_column_spacing(GTK_GRID(app->calendar_view), 1);
    gtk_container_set_border_width(GTK_CONTAINER(app->calendar_view), 10);
    gtk_container_add(GTK_CONTAINER(scroll), app->calendar_view);
    
    // Create status bar
    app->status_bar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(main_box), app->status_bar, FALSE, FALSE, 0);
    
    // Create year label for top header (this will be populated in update_header)
    app->year_label = gtk_label_new("");
    PangoAttrList* year_attrs = pango_attr_list_new();
    pango_attr_list_insert(year_attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(app->year_label), year_attrs);
    pango_attr_list_unref(year_attrs);
    
    // We will populate the sidebar in update_sidebar
    // Update everything
    update_ui(app);
    
    // Push a status message
    gtk_statusbar_push(GTK_STATUSBAR(app->status_bar), 0, 
                      "Ready - Displaying MANI Germanic lunar calendar");
}

// Update the calendar view to show the current LUNAR month
static void update_calendar_view(LunarCalendarApp* app) {
    // Clear existing calendar view
    GList* children = gtk_container_get_children(GTK_CONTAINER(app->calendar_view));
    for (GList* child = children; child != NULL; child = child->next) {
        gtk_widget_destroy(GTK_WIDGET(child->data));
    }
    g_list_free(children);
    
    // Find full moons for the current year and a bit of prev/next year
    Date full_moons[20]; // Store up to 20 full moons
    int moon_count = get_year_full_moons(app->current_year, full_moons, 20);
    
    if (moon_count == 0) {
        // No full moons found (shouldn't happen), show error
        GtkWidget* error_label = gtk_label_new("Error: Could not find full moons for this year.");
        gtk_grid_attach(GTK_GRID(app->calendar_view), error_label, 0, 0, 7, 1);
        return;
    }
    
    // Find the full moon that corresponds to our current lunar month
    // Lunar months are 1-indexed, so month 1 is the first full moon after winter solstice
    Date winter_solstice = { app->current_year, 12, 21 };
    
    // If we're before winter solstice, use previous year's
    Date current_date = { app->current_year, app->current_month, 1 };
    if (current_date.month < 12 || (current_date.month == 12 && current_date.day < 21)) {
        winter_solstice.year--;
    }
    
    // Find the app->current_month'th full moon after winter solstice
    int target_moon_index = -1;
    int moon_counter = 0;
    for (int i = 0; i < moon_count; i++) {
        if (compare_dates(full_moons[i], winter_solstice) > 0) {
            moon_counter++;
            if (moon_counter == app->current_month) {
                target_moon_index = i;
                break;
            }
        }
    }
    
    if (target_moon_index == -1) {
        // Couldn't find the requested lunar month
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "Error: Could not find lunar month %d for year %d.", 
                app->current_month, app->current_year);
        GtkWidget* error_label = gtk_label_new(error_msg);
        gtk_grid_attach(GTK_GRID(app->calendar_view), error_label, 0, 0, 7, 1);
        return;
    }
    
    // This full moon is the first day of our lunar month
    Date month_start = full_moons[target_moon_index];
    
    // Find the next full moon (end of lunar month)
    Date month_end;
    Date next_month_start;
    if (target_moon_index + 1 < moon_count) {
        month_end = full_moons[target_moon_index + 1];
        next_month_start = month_end; // The next month starts at the next full moon
    } else {
        // If no next full moon found, estimate it as 29.5 days later (round to 29)
        month_end = month_start;
        // Add 29 days to month_end
        for (int i = 0; i < 29; i++) {
            if (month_end.day < 28) {
                month_end.day++;
            } else {
                // Handle month end
                int days_in_month = 31;
                if (month_end.month == 4 || month_end.month == 6 || 
                    month_end.month == 9 || month_end.month == 11) {
                    days_in_month = 30;
                } else if (month_end.month == 2) {
                    days_in_month = is_gregorian_leap_year(month_end.year) ? 29 : 28;
                }
                
                if (month_end.day < days_in_month) {
                    month_end.day++;
                } else {
                    month_end.day = 1;
                    if (month_end.month < 12) {
                        month_end.month++;
                    } else {
                        month_end.month = 1;
                        month_end.year++;
                    }
                }
            }
        }
        next_month_start = month_end;
    }
    
    // Calculate days in this lunar month
    int days_in_month = days_between(month_start, month_end);
    
    // Enforce proper lunar month length (29-30 days)
    if (days_in_month < 29) {
        // Month too short, extend to 29 days
        days_in_month = 29;
        // Adjust month_end
        month_end = month_start;
        for (int i = 0; i < 29; i++) {
            if (month_end.day < 28) {
                month_end.day++;
            } else {
                int days_in_greg_month = 31;
                if (month_end.month == 4 || month_end.month == 6 || 
                    month_end.month == 9 || month_end.month == 11) {
                    days_in_greg_month = 30;
                } else if (month_end.month == 2) {
                    days_in_greg_month = is_gregorian_leap_year(month_end.year) ? 29 : 28;
                }
                
                if (month_end.day < days_in_greg_month) {
                    month_end.day++;
                } else {
                    month_end.day = 1;
                    if (month_end.month < 12) {
                        month_end.month++;
                    } else {
                        month_end.month = 1;
                        month_end.year++;
                    }
                }
            }
        }
        next_month_start = month_end;
    } else if (days_in_month > 30) {
        // Month too long, truncate to 30 days
        days_in_month = 30;
        // Adjust month_end
        month_end = month_start;
        for (int i = 0; i < 30; i++) {
            if (month_end.day < 28) {
                month_end.day++;
            } else {
                int days_in_greg_month = 31;
                if (month_end.month == 4 || month_end.month == 6 || 
                    month_end.month == 9 || month_end.month == 11) {
                    days_in_greg_month = 30;
                } else if (month_end.month == 2) {
                    days_in_greg_month = is_gregorian_leap_year(month_end.year) ? 29 : 28;
                }
                
                if (month_end.day < days_in_greg_month) {
                    month_end.day++;
                } else {
                    month_end.day = 1;
                    if (month_end.month < 12) {
                        month_end.month++;
                    } else {
                        month_end.month = 1;
                        month_end.year++;
                    }
                }
            }
        }
        next_month_start = month_end;
    }
    
    // Calculate days needed to complete the calendar grid
    int additional_days = 6;  // Display the first 6 days of the next lunar month
    
    // Calculate the days in this lunar month plus additional days
    int total_days_to_display = days_in_month + additional_days;
    
    // Create month title with Germanic lunar month name and Gregorian reference
    char header[100];
    char* month_descriptions[] = {
        "Wolf Moon", "Snow Moon", "Worm Moon", "Pink Moon", 
        "Flower Moon", "Strawberry Moon", "Buck Moon", "Sturgeon Moon",
        "Harvest Moon", "Hunter's Moon", "Beaver Moon", "Cold Moon", "Blue Moon"
    };
    
    // Use the Germanic lunar month name with the Gregorian reference date
    snprintf(header, sizeof(header), "Lunar Month %d (%s) - %d",
            app->current_month, 
            month_descriptions[(app->current_month - 1) % 13],
            app->current_year);
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(app->header_bar), header);
    
    // Add day name headers
    const char* day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int col = 0; col < 7; col++) {
        GtkWidget* day_label = gtk_label_new(day_names[col]);
        gtk_widget_set_hexpand(day_label, TRUE);
        gtk_grid_attach(GTK_GRID(app->calendar_view), day_label, col, 0, 1, 1);
    }
    
    // Find the day of week for the full moon (start of lunar month)
    Weekday first_day_weekday = calculate_weekday(month_start.year, month_start.month, month_start.day);
    
    // Push a status message to indicate the calendar type
    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), 
             "Ready - Displaying Germanic lunar month %d (%s) - Full Moon: %04d-%02d-%02d",
             app->current_month, 
             month_descriptions[(app->current_month - 1) % 13],
             month_start.year, month_start.month, month_start.day);
    gtk_statusbar_push(GTK_STATUSBAR(app->status_bar), 0, status_msg);
    
    // Fill in the calendar grid
    int row = 1;
    int col = first_day_weekday;
    
    // Current date for tracking
    Date current_date_in_month = month_start;
    
    // For each day in the lunar month + additional days
    for (int lunar_day = 1; lunar_day <= total_days_to_display; lunar_day++) {
        // Check if this day is part of the next month (after the next full moon)
        gboolean is_next_month = (lunar_day > days_in_month);
        
        // Create a frame for the day cell
        GtkWidget* day_frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(day_frame), GTK_SHADOW_ETCHED_IN);
        gtk_widget_set_size_request(day_frame, 80, 80);
        
        // Create a box for the day content
        GtkWidget* day_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_container_add(GTK_CONTAINER(day_frame), day_box);
        
        // Lunar day number (this is the primary display)
        char day_str[10];
        
        // Adjust the day number for next month display
        int display_day = lunar_day;
        if (is_next_month) {
            display_day = lunar_day - days_in_month;
        }
        
        snprintf(day_str, sizeof(day_str), "%d", display_day);
        GtkWidget* day_number = gtk_label_new(day_str);
        gtk_widget_set_halign(day_number, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(day_box), day_number, FALSE, FALSE, 0);
        
        // Get lunar date info
        int greg_year = current_date_in_month.year;
        int greg_month = current_date_in_month.month;
        int greg_day = current_date_in_month.day;
        
        // Show Gregorian date for reference
        char greg_str[32];
        snprintf(greg_str, sizeof(greg_str), "%04d-%02d-%02d", 
                 greg_year, greg_month, greg_day);
        
        GtkWidget* greg_date = gtk_label_new(greg_str);
        gtk_widget_set_halign(greg_date, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(day_box), greg_date, FALSE, FALSE, 0);
        
        // Get full day info for special day highlighting and moon phase
        CalendarDayCell* day_cell = calendar_adapter_get_germanic_day_info(
            greg_year, greg_month, greg_day);
        
        // Add moon phase
        GtkWidget* moon_phase;
        
        // Use adapter to get icon
        if (app->config && app->config->show_moon_phases) {
            moon_phase = calendar_adapter_get_moon_phase_icon(day_cell->moon_phase);
        } else {
            // Fallback to text
            moon_phase = gtk_label_new(calendar_adapter_get_moon_phase_name(day_cell->moon_phase));
            gtk_widget_set_halign(moon_phase, GTK_ALIGN_START);
        }
        
        gtk_box_pack_start(GTK_BOX(day_box), moon_phase, FALSE, FALSE, 0);
        
        // Set tooltip with full information
        gtk_widget_set_tooltip_text(day_frame, day_cell->tooltip_text);
        
        // If this is a day from the next month, make it visually distinct (faded)
        if (is_next_month) {
            // Create a style context for faded appearance
            GtkStyleContext* context = gtk_widget_get_style_context(day_frame);
            
            // Add opacity to the frame
            gtk_widget_set_opacity(day_frame, 0.35);  // Make it much more faded (35% opacity)
            
            // Make the text lighter
            gtk_widget_set_opacity(day_number, 0.5);
            gtk_widget_set_opacity(greg_date, 0.5);
            gtk_widget_set_opacity(moon_phase, 0.5);
            
            // Add a subtle border
            gtk_style_context_add_class(context, "next-month-day");
        }
        
        // Highlight special days if configured
        if (app->config && app->config->highlight_special_days && day_cell->is_special_day) {
            GdkRGBA color;
            calendar_adapter_get_special_day_color(day_cell->special_day_type, &color);
            gtk_widget_override_background_color(day_frame, GTK_STATE_FLAG_NORMAL, &color);
        }
        
        // Highlight today's date
        if (day_cell->is_today) {
            gtk_frame_set_shadow_type(GTK_FRAME(day_frame), GTK_SHADOW_IN);
            GtkWidget* today_label = gtk_label_new("Today");
            gtk_widget_set_halign(today_label, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(day_box), today_label, FALSE, FALSE, 0);
        }
        
        // Add the day cell to the grid
        gtk_grid_attach(GTK_GRID(app->calendar_view), day_frame, col, row, 1, 1);
        
        // Free the day cell (but not the tooltip text which is used by GTK)
        day_cell->tooltip_text = NULL; // Will be freed by GTK
        g_free(day_cell);
        
        // Move to the next day
        // Update the Gregorian date
        int days_in_greg_month = 31;
        if (current_date_in_month.month == 4 || current_date_in_month.month == 6 || 
            current_date_in_month.month == 9 || current_date_in_month.month == 11) {
            days_in_greg_month = 30;
        } else if (current_date_in_month.month == 2) {
            days_in_greg_month = is_gregorian_leap_year(current_date_in_month.year) ? 29 : 28;
        }
        
        current_date_in_month.day++;
        if (current_date_in_month.day > days_in_greg_month) {
            current_date_in_month.day = 1;
            current_date_in_month.month++;
            if (current_date_in_month.month > 12) {
                current_date_in_month.month = 1;
                current_date_in_month.year++;
            }
        }
        
        // Move to the next cell
        col++;
        if (col > 6) {
            col = 0;
            row++;
        }
    }
    
    // Show all the widgets
    gtk_widget_show_all(app->calendar_view);
}

// Callback when month is changed
static void on_month_changed(GtkWidget* widget, gpointer data) {
    LunarCalendarApp* app = (LunarCalendarApp*)data;
    int month = gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) + 1;
    if (month != app->current_month) {
        app->current_month = month;
        update_ui(app);
    }
}

// Callback when year is changed
static void on_year_changed(GtkWidget* widget, gpointer data) {
    LunarCalendarApp* app = (LunarCalendarApp*)data;
    int year = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    if (year != app->current_year) {
        app->current_year = year;
        update_ui(app);
    }
}

// Callback for previous month button
static void on_prev_month(GtkWidget* widget, gpointer data) {
    LunarCalendarApp* app = (LunarCalendarApp*)data;
    
    /* Go to previous month */
    app->current_month--;
    if (app->current_month < 1) {
        app->current_month = 12;
        app->current_year--;
    }
    
    /* Update display */
    update_ui(app);
}

// Callback for next month button
static void on_next_month(GtkWidget* widget, gpointer data) {
    LunarCalendarApp* app = (LunarCalendarApp*)data;
    
    /* Go to next month */
    app->current_month++;
    if (app->current_month > 12) {
        app->current_month = 1;
        app->current_year++;
    }
    
    /* Update display */
    update_ui(app);
}

// Callback when window is destroyed
static void on_window_destroy(GtkWidget* widget, gpointer data) {
    (void)widget; // Unused parameter
    
    // Save window size before exit
    LunarCalendarApp* app = (LunarCalendarApp*)data;
    if (app && app->config && app->window) {
        int width, height;
        gtk_window_get_size(GTK_WINDOW(app->window), &width, &height);
        app->config->window_width = width;
        app->config->window_height = height;
    }
}

/* Update the sidebar with current date information including a wireframe moon phase */
static void update_sidebar(LunarCalendarApp* app) {
    // Clear existing widgets
    GList* children = gtk_container_get_children(GTK_CONTAINER(app->sidebar));
    for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Get today's date
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int today_year = tm_now->tm_year + 1900;
    int today_month = tm_now->tm_mon + 1;
    int today_day = tm_now->tm_mday;
    
    // Create a vertical box for sidebar content
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app->sidebar), vbox);
    
    // Create a label for the current date
    char date_str[128];
    snprintf(date_str, sizeof(date_str), "Today: %04d-%02d-%02d", 
             today_year, today_month, today_day);
    GtkWidget* date_label = gtk_label_new(date_str);
    gtk_box_pack_start(GTK_BOX(vbox), date_label, FALSE, FALSE, 5);
    
    // Get lunar information for today
    LunarDay lunar_day = gregorian_to_lunar(today_year, today_month, today_day);
    
    // Create a label for the Eld Year
    char eld_year_str[128];
    snprintf(eld_year_str, sizeof(eld_year_str), "Eld Year: %d", lunar_day.eld_year);
    GtkWidget* eld_year_label = gtk_label_new(eld_year_str);
    gtk_box_pack_start(GTK_BOX(vbox), eld_year_label, FALSE, FALSE, 5);
    
    // Add a label for the currently displayed Eld Year
    LunarDay displayed_lunar_day = gregorian_to_lunar(app->current_year, app->current_month, 1);
    char displayed_eld_year_str[128];
    snprintf(displayed_eld_year_str, sizeof(displayed_eld_year_str), 
             "Displayed Eld Year: %d", displayed_lunar_day.eld_year);
    GtkWidget* displayed_eld_label = gtk_label_new(displayed_eld_year_str);
    gtk_box_pack_start(GTK_BOX(vbox), displayed_eld_label, FALSE, FALSE, 5);
    
    // Add a large wireframe moon showing current phase
    GdkPixbuf* moon_pixbuf = create_moon_phase_icon(lunar_day.moon_phase, 150);
    GtkWidget* moon_image = gtk_image_new_from_pixbuf(moon_pixbuf);
    g_object_unref(moon_pixbuf);
    gtk_box_pack_start(GTK_BOX(vbox), moon_image, FALSE, FALSE, 10);
    
    // Add a label for the moon phase name
    GtkWidget* phase_label = gtk_label_new(calendar_adapter_get_moon_phase_name(lunar_day.moon_phase));
    gtk_box_pack_start(GTK_BOX(vbox), phase_label, FALSE, FALSE, 5);
    
    // Make all widgets visible
    gtk_widget_show_all(app->sidebar);
}

/* Update the header with year information */
static void update_header(LunarCalendarApp* app) {
    // Get lunar information for the current month
    LunarDay lunar_day = gregorian_to_lunar(app->current_year, app->current_month, 1);
    
    // Update the header bar title with the Eld Year
    char title[128];
    snprintf(title, sizeof(title), "MANI - Eld Year %d", lunar_day.eld_year);
    gtk_header_bar_set_title(GTK_HEADER_BAR(app->header_bar), title);
}

/* Update the main UI */
static void update_ui(LunarCalendarApp* app) {
    update_calendar_view(app);
    update_month_label(app);
    update_header(app);
    update_sidebar(app);
}

/* Update the month label */
static void update_month_label(LunarCalendarApp* app) {
    const char* month_names[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December", "Thirteenth"
    };
    
    if (app->current_month >= 1 && app->current_month <= 13) {
        char month_text[100];
        snprintf(month_text, sizeof(month_text), "Month %d: %s", 
                 app->current_month, month_names[app->current_month - 1]);
        
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(app->header_bar), month_text);
    }
} 