#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <time.h>
#include "../../include/gui/gui_app.h"
#include "../../include/gui/config.h"
#include "../../include/gui/calendar_adapter.h"
#include "../../include/gui/calendar_events.h"
#include "../../include/lunar_calendar.h"
#include "../../include/lunar_renderer.h"

// Data structure for day click event
typedef struct {
    LunarCalendarApp* app;
    int year;
    int month;
    int day;
} DayClickData;

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
static gboolean on_day_clicked(GtkWidget* widget, GdkEventButton* event, gpointer user_data);
static void update_event_editor(LunarCalendarApp* app);
static void on_add_event(GtkWidget* widget, gpointer user_data);
static void on_edit_event(GtkWidget* widget, gpointer user_data);
static void on_delete_event(GtkWidget* widget, gpointer user_data);

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
    gtk_init(argc, argv);
    
    // Initialize the application
    LunarCalendarApp* app = g_malloc0(sizeof(LunarCalendarApp));
    app->app = gtk_application_new("org.lunar.mani", G_APPLICATION_FLAGS_NONE);
    
    // Connect the activate signal
    g_signal_connect(app->app, "activate", G_CALLBACK(activate), app);
    
    // Create a config structure with default values
    app->config = config_get_defaults();
    
    // Get the user's home directory for config
    const char* home_dir = g_get_home_dir();
    char* config_dir = g_build_filename(home_dir, ".config", "lunar_calendar", NULL);
    
    // Create the config directory if it doesn't exist
    g_mkdir_with_parents(config_dir, 0755);
    
    // Set config file path
    app->config_file_path = g_build_filename(config_dir, "config.json", NULL);
    
    // Set events file path
    app->events_file_path = g_build_filename(config_dir, "events.json", NULL);
    
    // Try to load config file
    LunarCalendarConfig* loaded_config = config_load(app->config_file_path);
    if (loaded_config) {
        // Replace default config with loaded config
        config_free(app->config);
        app->config = loaded_config;
    } else {
        // Save default config
        config_save(app->config_file_path, app->config);
    }
    
    // Initialize the events system
    events_init(app->events_file_path);
    
    // Get the current date for initializing the view
    time_t now = time(NULL);
    struct tm* current_time = localtime(&now);
    
    app->current_year = current_time->tm_year + 1900;
    app->current_month = current_time->tm_mon + 1;
    
    // Store today's date for highlighting
    app->today_year = app->current_year;
    app->today_month = app->current_month;
    app->today_day = current_time->tm_mday;
    
    // Initialize selected day to today
    app->selected_day_year = app->today_year;
    app->selected_day_month = app->today_month;
    app->selected_day_day = app->today_day;
    
    // Clean up
    g_free(config_dir);
    
    return app;
}

// Run the application main loop
int gui_app_run(LunarCalendarApp* app) {
    return g_application_run(G_APPLICATION(app->app), 0, NULL);
}

// Clean up resources
void gui_app_cleanup(LunarCalendarApp* app) {
    if (app) {
        // Save configuration
        if (app->config && app->config_file_path) {
            config_save(app->config_file_path, app->config);
        }
        
        // Save events
        if (app->events_file_path) {
            events_save(app->events_file_path);
        }
        
        // Free configuration
        if (app->config) {
            config_free(app->config);
        }
        
        // Free paths
        if (app->config_file_path) {
            g_free(app->config_file_path);
        }
        
        if (app->events_file_path) {
            g_free(app->events_file_path);
        }
        
        // Clean up events system
        events_cleanup();
        
        // Unreference GTK application
        if (app->app) {
            g_object_unref(app->app);
        }
        
        // Free the structure
        g_free(app);
    }
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
    // Clear the calendar view
    GList* children = gtk_container_get_children(GTK_CONTAINER(app->calendar_view));
    for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
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
        
        // Create an event box to properly capture events
        GtkWidget* event_box = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(day_frame), event_box);
        
        // Make it obvious that the day is clickable
        GtkStyleContext* context = gtk_widget_get_style_context(day_frame);
        gtk_style_context_add_class(context, "day-cell");
        
        // Add CSS to make it look interactive
        GtkCssProvider* day_provider = gtk_css_provider_new();
        const char* day_css = ".day-cell:hover { background-color: rgba(120, 120, 120, 0.2); }";
        gtk_css_provider_load_from_data(day_provider, day_css, -1, NULL);
        gtk_style_context_add_provider(context, 
                                    GTK_STYLE_PROVIDER(day_provider), 
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(day_provider);
        
        // Create a box for the day content
        GtkWidget* day_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_container_add(GTK_CONTAINER(event_box), day_box);
        
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
        
        // Get Gregorian date info
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
        
        // If this day has events, add an indicator
        if (event_date_has_events(greg_year, greg_month, greg_day)) {
            GtkWidget* event_indicator = gtk_label_new("📅");  // Calendar emoji
            gtk_widget_set_halign(event_indicator, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(day_box), event_indicator, FALSE, FALSE, 0);
        }
        
        // Make sure the event box can receive events
        gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
        
        // Create the click data - store directly on the widget 
        DayClickData* click_data = g_malloc(sizeof(DayClickData));
        click_data->app = app;
        click_data->year = greg_year;
        click_data->month = greg_month;
        click_data->day = greg_day;
        
        // Store the click data as object data on the event box so it will be freed when the widget is destroyed
        g_object_set_data_full(G_OBJECT(event_box), "click-data", click_data, g_free);
        
        // Connect to the button-press-event signal with the click data
        g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_day_clicked), NULL);
        
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
            GtkStyleContext* context = gtk_widget_get_style_context(day_frame);
            gtk_style_context_add_class(context, "special-day");
            
            // Add custom CSS for the special day
            GtkCssProvider* provider = gtk_css_provider_new();
            GdkRGBA color;
            calendar_adapter_get_special_day_color(day_cell->special_day_type, &color);
            
            char css[256];
            snprintf(css, sizeof(css), 
                    ".special-day { background-color: rgba(%d, %d, %d, %f); }",
                    (int)(color.red * 255), (int)(color.green * 255), 
                    (int)(color.blue * 255), color.alpha);
                    
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
            gtk_style_context_add_provider(context, 
                                        GTK_STYLE_PROVIDER(provider), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
        }
        
        // Check for custom event color
        GdkRGBA event_color;
        if (event_get_date_color(greg_year, greg_month, greg_day, &event_color)) {
            // Apply custom event color using CSS
            GtkStyleContext* context = gtk_widget_get_style_context(day_frame);
            gtk_style_context_add_class(context, "event-day");
            
            GtkCssProvider* provider = gtk_css_provider_new();
            char css[256];
            snprintf(css, sizeof(css), 
                    ".event-day { background-color: rgba(%d, %d, %d, %f); }",
                    (int)(event_color.red * 255), (int)(event_color.green * 255), 
                    (int)(event_color.blue * 255), event_color.alpha);
                    
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
            gtk_style_context_add_provider(context, 
                                        GTK_STYLE_PROVIDER(provider), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
        }
        
        // Highlight today's date
        if (day_cell->is_today) {
            gtk_frame_set_shadow_type(GTK_FRAME(day_frame), GTK_SHADOW_IN);
            GtkWidget* today_label = gtk_label_new("Today");
            gtk_widget_set_halign(today_label, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(day_box), today_label, FALSE, FALSE, 0);
        }
        
        // Highlight selected day (if any)
        if (greg_year == app->selected_day_year && 
            greg_month == app->selected_day_month && 
            greg_day == app->selected_day_day) {
            GtkStyleContext* context = gtk_widget_get_style_context(day_frame);
            gtk_style_context_add_class(context, "selected-day");
            
            // Apply bold styling to selected day using CSS
            GtkCssProvider* provider = gtk_css_provider_new();
            const char* css = ".selected-day { border: 2px solid #3584e4; background-color: rgba(53, 132, 228, 0.3); }";
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
            gtk_style_context_add_provider(context, 
                                        GTK_STYLE_PROVIDER(provider), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
            
            gtk_frame_set_shadow_type(GTK_FRAME(day_frame), GTK_SHADOW_ETCHED_OUT);
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
    
    // Clean up any other resources that need to be freed
    if (app && app->calendar_view) {
        // Clear calendar view (which also cleans up signal handlers and their data)
        GList* children = gtk_container_get_children(GTK_CONTAINER(app->calendar_view));
        for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);
    }
}

/* Update the sidebar with current date information */
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
    
    // Add a separator before the event editor
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 10);
    
    // Add event editor section
    GtkWidget* event_frame = gtk_frame_new("Event Editor");
    gtk_box_pack_start(GTK_BOX(vbox), event_frame, TRUE, TRUE, 0);
    
    // Create event editor box
    app->event_editor = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(event_frame), app->event_editor);
    
    // Make all widgets visible
    gtk_widget_show_all(app->sidebar);
    
    // Update the event editor with the selected date's events
    update_event_editor(app);
}

// Implement the update_event_editor function
static void update_event_editor(LunarCalendarApp* app) {
    if (!app->event_editor) {
        return;
    }
    
    // Clear the existing event editor content
    GList* children = gtk_container_get_children(GTK_CONTAINER(app->event_editor));
    for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Add a label showing the selected date
    char date_str[128];
    snprintf(date_str, sizeof(date_str), "Date: %04d-%02d-%02d", 
             app->selected_day_year, app->selected_day_month, app->selected_day_day);
    GtkWidget* date_label = gtk_label_new(date_str);
    gtk_widget_set_halign(date_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(app->event_editor), date_label, FALSE, FALSE, 5);
    
    // Get events for the selected date
    EventList* events = event_get_for_date(
        app->selected_day_year, app->selected_day_month, app->selected_day_day);
    
    // Create a scrolled window for the event list
    GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 100);
    gtk_box_pack_start(GTK_BOX(app->event_editor), scroll, TRUE, TRUE, 5);
    
    // Create a list box for events
    app->event_list = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scroll), app->event_list);
    
    // Add events to the list
    if (events && events->count > 0) {
        for (int i = 0; i < events->count; i++) {
            CalendarEvent* event = events->events[i];
            
            // Create a box for each event
            GtkWidget* event_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            
            // Color indicator
            if (event->has_custom_color) {
                GtkWidget* color_box = gtk_frame_new(NULL);
                gtk_widget_set_size_request(color_box, 16, 16);
                
                // Apply the event color using CSS
                GtkStyleContext* context = gtk_widget_get_style_context(color_box);
                gtk_style_context_add_class(context, "event-color");
                
                GtkCssProvider* provider = gtk_css_provider_new();
                char css[256];
                snprintf(css, sizeof(css), 
                        ".event-color { background-color: rgba(%d, %d, %d, %f); }",
                        (int)(event->color.red * 255), (int)(event->color.green * 255), 
                        (int)(event->color.blue * 255), event->color.alpha);
                        
                gtk_css_provider_load_from_data(provider, css, -1, NULL);
                gtk_style_context_add_provider(context, 
                                            GTK_STYLE_PROVIDER(provider), 
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                g_object_unref(provider);
            }
            
            // Event title (with edit button)
            GtkWidget* title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
            GtkWidget* title_label = gtk_label_new(event->title);
            gtk_widget_set_halign(title_label, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(title_box), title_label, FALSE, FALSE, 0);
            
            // Short preview of the description
            if (event->description && strlen(event->description) > 0) {
                char preview[40];
                strncpy(preview, event->description, sizeof(preview) - 4);
                preview[sizeof(preview) - 4] = '\0';
                if (strlen(event->description) > sizeof(preview) - 4) {
                    strcat(preview, "...");
                }
                
                GtkWidget* desc_label = gtk_label_new(preview);
                gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
                gtk_label_set_line_wrap(GTK_LABEL(desc_label), TRUE);
                PangoAttrList* attrs = pango_attr_list_new();
                pango_attr_list_insert(attrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
                gtk_label_set_attributes(GTK_LABEL(desc_label), attrs);
                pango_attr_list_unref(attrs);
                gtk_box_pack_start(GTK_BOX(title_box), desc_label, FALSE, FALSE, 0);
            }
            
            gtk_box_pack_start(GTK_BOX(event_box), title_box, TRUE, TRUE, 2);
            
            // Edit button
            GtkWidget* edit_button = gtk_button_new_from_icon_name("document-edit-symbolic", GTK_ICON_SIZE_BUTTON);
            gtk_widget_set_tooltip_text(edit_button, "Edit event");
            g_object_set_data(G_OBJECT(edit_button), "event-index", GINT_TO_POINTER(i));
            g_signal_connect(edit_button, "clicked", G_CALLBACK(on_edit_event), app);
            gtk_box_pack_start(GTK_BOX(event_box), edit_button, FALSE, FALSE, 2);
            
            // Delete button
            GtkWidget* delete_button = gtk_button_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON);
            gtk_widget_set_tooltip_text(delete_button, "Delete event");
            g_object_set_data(G_OBJECT(delete_button), "event-index", GINT_TO_POINTER(i));
            g_signal_connect(delete_button, "clicked", G_CALLBACK(on_delete_event), app);
            gtk_box_pack_start(GTK_BOX(event_box), delete_button, FALSE, FALSE, 2);
            
            // Add the event box to the list
            GtkWidget* list_item = gtk_list_box_row_new();
            gtk_container_add(GTK_CONTAINER(list_item), event_box);
            gtk_list_box_insert(GTK_LIST_BOX(app->event_list), list_item, -1);
        }
    } else {
        // No events message
        GtkWidget* no_events = gtk_label_new("No events for this date");
        gtk_widget_set_sensitive(no_events, FALSE);
        gtk_list_box_insert(GTK_LIST_BOX(app->event_list), no_events, -1);
    }
    
    // Add a separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(app->event_editor), separator, FALSE, FALSE, 5);
    
    // Add the event creation form
    GtkWidget* form_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(app->event_editor), form_box, FALSE, FALSE, 0);
    
    // Title field
    GtkWidget* title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* title_label = gtk_label_new("Title:");
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(title_box), title_label, FALSE, FALSE, 5);
    
    app->event_title_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(title_box), app->event_title_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(form_box), title_box, FALSE, FALSE, 5);
    
    // Description field
    GtkWidget* desc_label = gtk_label_new("Description:");
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(form_box), desc_label, FALSE, FALSE, 5);
    
    GtkWidget* desc_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(desc_scroll), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(desc_scroll, -1, 80);
    gtk_box_pack_start(GTK_BOX(form_box), desc_scroll, TRUE, TRUE, 0);
    
    app->event_desc_text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->event_desc_text), GTK_WRAP_WORD);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->event_desc_text));
    gtk_text_buffer_set_text(buffer, "", -1);  // Initialize with empty text
    gtk_container_add(GTK_CONTAINER(desc_scroll), app->event_desc_text);
    
    // Color selection
    GtkWidget* color_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* color_label = gtk_label_new("Custom Color:");
    gtk_widget_set_halign(color_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(color_box), color_label, FALSE, FALSE, 5);
    
    app->event_color_button = gtk_color_button_new();
    gtk_widget_set_tooltip_text(app->event_color_button, "Choose custom color for this event");
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(app->event_color_button), TRUE);
    
    // Set a default color
    GdkRGBA default_color = {0.8, 0.9, 0.8, 0.3};
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(app->event_color_button), &default_color);
    gtk_box_pack_start(GTK_BOX(color_box), app->event_color_button, TRUE, TRUE, 5);
    
    gtk_box_pack_start(GTK_BOX(form_box), color_box, FALSE, FALSE, 5);
    
    // Add button
    GtkWidget* add_button = gtk_button_new_with_label("Add Event");
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_event), app);
    gtk_box_pack_start(GTK_BOX(form_box), add_button, FALSE, FALSE, 5);
    
    // Free event list
    if (events) {
        event_list_free(events);
    }
    
    // Show all widgets
    gtk_widget_show_all(app->event_editor);
}

// Add event handler
static void on_add_event(GtkWidget* widget, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    
    // Get the event title
    const char* title = gtk_entry_get_text(GTK_ENTRY(app->event_title_entry));
    if (!title || strlen(title) == 0) {
        // Show an error message if the title is empty
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Event title cannot be empty");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Get the event description
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->event_desc_text));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char* description = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    // Get the event color
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(app->event_color_button), &color);
    
    // Add the event
    bool success = event_add(app->selected_day_year, app->selected_day_month, 
                            app->selected_day_day, title, description, &color);
    
    // Free the description
    g_free(description);
    
    if (success) {
        // Clear the form
        gtk_entry_set_text(GTK_ENTRY(app->event_title_entry), "");
        gtk_text_buffer_set_text(buffer, "", 0);
        
        // Save events
        if (app->events_file_path) {
            events_save(app->events_file_path);
        }
        
        // Update the UI
        update_event_editor(app);
        update_calendar_view(app);
    } else {
        // Show an error message
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to add event");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// Edit event handler
static void on_edit_event(GtkWidget* widget, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    
    // Get the event index
    int event_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "event-index"));
    
    // Get the event
    EventList* events = event_get_for_date(
        app->selected_day_year, app->selected_day_month, app->selected_day_day);
    
    if (!events || event_index >= events->count) {
        if (events) {
            event_list_free(events);
        }
        return;
    }
    
    CalendarEvent* event = events->events[event_index];
    
    // Create a dialog for editing
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Edit Event",
                                                   GTK_WINDOW(app->window),
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   "Cancel", GTK_RESPONSE_CANCEL,
                                                   "Save", GTK_RESPONSE_ACCEPT,
                                                   NULL);
    
    // Create the content area
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
    gtk_box_set_spacing(GTK_BOX(content_area), 10);
    
    // Title field
    GtkWidget* title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* title_label = gtk_label_new("Title:");
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(title_box), title_label, FALSE, FALSE, 5);
    
    GtkWidget* title_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(title_entry), event->title);
    gtk_box_pack_start(GTK_BOX(title_box), title_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), title_box, FALSE, FALSE, 5);
    
    // Description field
    GtkWidget* desc_label = gtk_label_new("Description:");
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content_area), desc_label, FALSE, FALSE, 5);
    
    GtkWidget* desc_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(desc_scroll), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(desc_scroll, 300, 100);
    gtk_box_pack_start(GTK_BOX(content_area), desc_scroll, TRUE, TRUE, 0);
    
    GtkWidget* desc_text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(desc_text), GTK_WRAP_WORD);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(desc_text));
    gtk_text_buffer_set_text(buffer, event->description, -1);
    gtk_container_add(GTK_CONTAINER(desc_scroll), desc_text);
    
    // Color selection
    GtkWidget* color_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* color_label = gtk_label_new("Custom Color:");
    gtk_widget_set_halign(color_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(color_box), color_label, FALSE, FALSE, 5);
    
    GtkWidget* color_button = gtk_color_button_new();
    gtk_widget_set_tooltip_text(color_button, "Choose custom color for this event");
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(color_button), TRUE);
    
    // Set the current color
    if (event->has_custom_color) {
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_button), &event->color);
    } else {
        GdkRGBA default_color = {0.8, 0.9, 0.8, 0.3};
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_button), &default_color);
    }
    gtk_box_pack_start(GTK_BOX(color_box), color_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), color_box, FALSE, FALSE, 5);
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (result == GTK_RESPONSE_ACCEPT) {
        // Get the updated values
        const char* new_title = gtk_entry_get_text(GTK_ENTRY(title_entry));
        
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        char* new_description = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        GdkRGBA new_color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(color_button), &new_color);
        
        // Update the event
        bool success = event_update(app->selected_day_year, app->selected_day_month, 
                                   app->selected_day_day, event_index, 
                                   new_title, new_description, &new_color);
        
        // Free the description
        g_free(new_description);
        
        if (success) {
            // Save events
            if (app->events_file_path) {
                events_save(app->events_file_path);
            }
            
            // Update the UI
            update_event_editor(app);
            update_calendar_view(app);
        } else {
            // Show an error message
            GtkWidget* error_dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                           GTK_DIALOG_MODAL,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_OK,
                                                           "Failed to update event");
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
    }
    
    // Free events
    event_list_free(events);
    
    // Destroy the dialog
    gtk_widget_destroy(dialog);
}

// Delete event handler
static void on_delete_event(GtkWidget* widget, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    
    // Get the event index
    int event_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "event-index"));
    
    // Confirm the deletion
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Are you sure you want to delete this event?");
    
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (result != GTK_RESPONSE_YES) {
        return;
    }
    
    // Delete the event
    bool success = event_delete(app->selected_day_year, app->selected_day_month, 
                               app->selected_day_day, event_index);
    
    if (success) {
        // Save events
        if (app->events_file_path) {
            events_save(app->events_file_path);
        }
        
        // Update the UI
        update_event_editor(app);
        update_calendar_view(app);
    } else {
        // Show an error message
        GtkWidget* error_dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                        GTK_DIALOG_MODAL,
                                                        GTK_MESSAGE_ERROR,
                                                        GTK_BUTTONS_OK,
                                                        "Failed to delete event");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
    }
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

// Day click handler
static gboolean on_day_clicked(GtkWidget* widget, GdkEventButton* event, gpointer user_data) {
    // Get the click data from the widget
    DayClickData* data = g_object_get_data(G_OBJECT(widget), "click-data");
    if (!data) {
        g_print("ERROR: No click data found on widget\n");
        return FALSE;
    }
    
    g_print("Day clicked: %04d-%02d-%02d\n", data->year, data->month, data->day);
    
    LunarCalendarApp* app = data->app;
    
    // Update the selected day
    app->selected_day_year = data->year;
    app->selected_day_month = data->month;
    app->selected_day_day = data->day;
    
    // Update status message
    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), 
            "Selected day: %04d-%02d-%02d", 
            app->selected_day_year, app->selected_day_month, app->selected_day_day);
    gtk_statusbar_push(GTK_STATUSBAR(app->status_bar), 0, status_msg);
    
    // Update the calendar to highlight the selected day
    update_calendar_view(app);
    
    // Update the event editor in the sidebar
    update_event_editor(app);
    
    // Return TRUE to indicate the event was handled and stop propagation
    return TRUE;
} 