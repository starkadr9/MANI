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
#include "../../include/gui/settings_dialog.h"
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
/*static const char* get_weekday_name(Weekday weekday) {
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
}*/

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
static void init_metonic_cycle_bar(LunarCalendarApp* app);
static void update_metonic_cycle_display(LunarCalendarApp* app);
static void on_metonic_help_clicked(GtkButton* button, gpointer user_data);
static void update_ui_from_config(LunarCalendarApp* app);
static void on_settings_clicked(GtkButton* button, gpointer user_data);
static gboolean draw_moon_phase_cairo(GtkWidget *widget, cairo_t *cr, gpointer data);

/**
 * Get the name of a lunar month.
 * 
 * @param month_num Month number (1-13)
 * @param buffer Buffer to store the name
 * @param buffer_size Size of the buffer
 */
static void lunar_get_month_name(int month_num, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0 || month_num < 1 || month_num > 13) {
        return;
    }
    
    // Default month names
    static const char* default_month_names[] = {
        "Month 1", "Month 2", "Month 3", "Month 4", "Month 5",
        "Month 6", "Month 7", "Month 8", "Month 9", "Month 10", "Month 11", "Month 12", "Month 13"
    };
    
    LunarCalendarApp* app = g_object_get_data(G_OBJECT(g_application_get_default()), "app_data");
    if (app && app->config && app->config->custom_month_names[month_num - 1] && 
        strlen(app->config->custom_month_names[month_num - 1]) > 0) {
        // Use custom name if available
        strncpy(buffer, app->config->custom_month_names[month_num - 1], buffer_size);
        buffer[buffer_size - 1] = '\0';
    } else {
        // Use default name
        strncpy(buffer, default_month_names[month_num - 1], buffer_size);
        buffer[buffer_size - 1] = '\0';
    }
}

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
    app->app = gtk_application_new("org.lunar.mani", G_APPLICATION_DEFAULT_FLAGS); // Fix deprecated flag
    
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
    // Store app data in the application for global access
    g_object_set_data(G_OBJECT(app->app), "app_data", app);
    
    // Hold/Release is not needed for a single-window application
    // g_application_hold(G_APPLICATION(app->app));
    
    // Run the application
    int status = g_application_run(G_APPLICATION(app->app), 0, NULL);
    
    // Hold/Release is not needed for a single-window application
    // g_application_release(G_APPLICATION(app->app));
    
    return status;
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

// Callback function when the application is activated
static void activate(GtkApplication* app, gpointer user_data) {
    // Get the application data
    LunarCalendarApp* lunar_app = (LunarCalendarApp*)user_data;
    
    // Create the main window if it doesn't exist
    if (!lunar_app->window) {
        lunar_app->window = gtk_application_window_new(app);
        
        // Set window title
        gtk_window_set_title(GTK_WINDOW(lunar_app->window), "MANI - Lunar Calendar");
        
        // Set window size from configuration
        if (lunar_app->config) {
            gtk_window_set_default_size(GTK_WINDOW(lunar_app->window), 
                                    lunar_app->config->window_width, 
                                    lunar_app->config->window_height);
        } else {
            // Default size if no config
            gtk_window_set_default_size(GTK_WINDOW(lunar_app->window), 800, 600);
        }
        
        // Connect the window destroy signal to gtk_main_quit directly
        g_signal_connect(lunar_app->window, "destroy", G_CALLBACK(gtk_window_close), NULL);
        
        // Connect the window destroy signal to our custom handler for cleaning up resources
        g_signal_connect(lunar_app->window, "destroy", G_CALLBACK(on_window_destroy), lunar_app);
    }
    
    // Save GTK app reference
    lunar_app->gtk_app = app;
    
    // Build the UI
    build_ui(lunar_app);
    
    // Show the window and all its contents
    gtk_widget_show_all(lunar_app->window);
    
    // Hide the metonic cycle status bar if not enabled in config
    if (lunar_app->metonic_cycle_bar && !lunar_app->config->show_metonic_cycle) {
        gtk_widget_hide(lunar_app->metonic_cycle_bar);
    }
    
    // Initialize GTK main loop (important for proper quit behavior)
    gtk_main();
}

/**
 * Build the application UI
 */
static void build_ui(LunarCalendarApp* app) {
    // Create header bar
    app->header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(app->header_bar), TRUE);
    
    // Calculate and show Eld Year in title
    GDate today;
    g_date_set_time_t(&today, time(NULL));
    int current_year = g_date_get_year(&today);
    int eld_year = calculate_eld_year_from_gregorian(current_year);
    
    char title[100];
    snprintf(title, sizeof(title), "MANI - Eld Year %d", eld_year);
    gtk_header_bar_set_title(GTK_HEADER_BAR(app->header_bar), title);
    
    // Add settings button to the header bar
    GtkWidget* settings_button = gtk_button_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(settings_button, "Settings");
    g_signal_connect(settings_button, "clicked", G_CALLBACK(on_settings_clicked), app);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(app->header_bar), settings_button);
    
    gtk_window_set_titlebar(GTK_WINDOW(app->window), app->header_bar);
    
    // Create main layout
    app->main_layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), app->main_layout);
    
    // Create horizontal layout for sidebar and calendar
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(app->main_layout), content_box, TRUE, TRUE, 0);
    
    // Create sidebar (fixed width 200px)
    app->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(app->sidebar, 200, -1);
    gtk_box_pack_start(GTK_BOX(content_box), app->sidebar, FALSE, FALSE, 0);
    
    // Create a scrolled window for the calendar
    GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
                                  GTK_POLICY_AUTOMATIC, 
                                  GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(content_box), scroll, TRUE, TRUE, 0);
    
    // Create a box for the calendar
    app->calendar_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(scroll), app->calendar_view);
    
    // Add navigation controls
    GtkWidget* nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(nav_box), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(nav_box), 5);
    gtk_box_pack_start(GTK_BOX(app->calendar_view), nav_box, FALSE, FALSE, 0);
    
    // Previous month button
    GtkWidget* prev_button = gtk_button_new_from_icon_name("go-previous-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(nav_box), prev_button, FALSE, FALSE, 0);
    g_signal_connect(prev_button, "clicked", G_CALLBACK(on_prev_month), app);
    
    // Month selector (combobox)
    GtkWidget* month_combo = gtk_combo_box_text_new();
    for (int i = 1; i <= 13; i++) {
        char month_text[50];
        lunar_get_month_name(i, month_text, sizeof(month_text));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(month_combo), month_text);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(month_combo), app->current_month - 1);
    gtk_box_pack_start(GTK_BOX(nav_box), month_combo, FALSE, FALSE, 5);
    g_signal_connect(month_combo, "changed", G_CALLBACK(on_month_changed), app);
    
    // Year selector (spinbutton)
    GtkWidget* year_spin = gtk_spin_button_new_with_range(1, 9999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(year_spin), app->current_year);
    gtk_box_pack_start(GTK_BOX(nav_box), year_spin, FALSE, FALSE, 5);
    g_signal_connect(year_spin, "value-changed", G_CALLBACK(on_year_changed), app);
    
    // Next month button
    GtkWidget* next_button = gtk_button_new_from_icon_name("go-next-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(nav_box), next_button, FALSE, FALSE, 0);
    g_signal_connect(next_button, "clicked", G_CALLBACK(on_next_month), app);
    
    // Add a status bar at the bottom
    app->status_bar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(app->main_layout), app->status_bar, FALSE, FALSE, 0);
    
    // Initialize the metonic cycle display
    init_metonic_cycle_bar(app);
    
    // Update UI
    update_ui(app);
}

// Update the calendar view to show the current LUNAR month using the CalendarGridModel
static void update_calendar_view(LunarCalendarApp* app) {
    // ---- Variable Declarations ----
    char status_msg[256]; // Single declaration for status message
    // ---- End Variable Declarations ----

    // Clear the calendar view, but keep the navigation controls
    GList* children = gtk_container_get_children(GTK_CONTAINER(app->calendar_view));
    // Skip the first child which is the navigation controls
    if (children != NULL) {
        GList* iter = children;
        if (iter->next != NULL) {  // Skip first child (navigation controls)
            iter = iter->next;
            while (iter != NULL) {
                gtk_widget_destroy(GTK_WIDGET(iter->data));
                iter = g_list_next(iter);
            }
        }
    }
    g_list_free(children);
    
    // Create a calendar grid
    GtkWidget* calendar_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(calendar_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(calendar_grid), 2);
    gtk_grid_set_row_homogeneous(GTK_GRID(calendar_grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(calendar_grid), TRUE);
    gtk_box_pack_start(GTK_BOX(app->calendar_view), calendar_grid, TRUE, TRUE, 0);
    
    // --- Get the data model from the adapter --- 
    CalendarGridModel* model = calendar_adapter_create_month_model(app->current_year, app->current_month);

    if (!model) {
        // Failed to create the model, show error
        GtkWidget* error_label = gtk_label_new("Error: Could not create calendar model.");
        gtk_grid_attach(GTK_GRID(calendar_grid), error_label, 0, 0, 7, 1);
            gtk_widget_show_all(app->calendar_view);
        return; // Exit early
    }
    
    // If we're asking for month 13 but it's not a leap year (model creation might still succeed but be incomplete)
    if (app->current_month == 13 && !calendar_adapter_is_lunar_leap_year(app->current_year)) {
            GtkWidget* error_label = gtk_label_new("No 13th month in this year - not a lunar leap year.");
            gtk_grid_attach(GTK_GRID(calendar_grid), error_label, 0, 1, 7, 1);
        calendar_adapter_free_model(model); // Free the potentially incomplete model
            gtk_widget_show_all(app->calendar_view);
            return;
    }

    // --- Update Header and Status Bar --- 
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(app->header_bar), model->month_name);
    // Update status bar with basic info from model
    snprintf(status_msg, sizeof(status_msg),
             "Displaying: %s, %s (%d days)", 
             model->month_name, model->year_str, model->days_in_month);
    gtk_statusbar_push(GTK_STATUSBAR(app->status_bar), 0, status_msg);

    // --- Add Day Name Headers --- 
    const char* day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    if (app->config && app->config->show_weekday_names) {
        int week_start = 0; // Default Sunday
        if (app->config && app->config->week_start_day >= 0 && app->config->week_start_day <= 2) {
            if (app->config->week_start_day == 1) week_start = 1; // Monday
            else if (app->config->week_start_day == 2) week_start = 6; // Saturday
        }
        
        for (int i = 0; i < 7; i++) {
            int day_index = (week_start + i) % 7;
            const char* day_name = day_names[day_index];
            if (app->config->custom_weekday_names[day_index] && 
                strlen(app->config->custom_weekday_names[day_index]) > 0) {
                day_name = app->config->custom_weekday_names[day_index];
            }
            GtkWidget* day_label = gtk_label_new(day_name);
            gtk_widget_set_hexpand(day_label, TRUE);
            gtk_grid_attach(GTK_GRID(calendar_grid), day_label, i, 0, 1, 1);
        }
    }
    
    // --- Fill Calendar Grid from Model --- 
    int grid_row = 1; // Start at row 1 if showing headers
    if (!app->config || !app->config->show_weekday_names) {
        grid_row = 0; // If not showing headers, start grid at row 0
    }

    for (int i = 0; i < model->rows * model->cols; i++) {
        CalendarDayCell* cell = model->cells[i];
        int col = i % model->cols;
        int row = grid_row + (i / model->cols);
        
        // Create a frame for the day cell
        GtkWidget* day_frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(day_frame), GTK_SHADOW_ETCHED_IN);
        gtk_widget_set_size_request(day_frame, 80, 80);
        
        // Create an event box to capture clicks
        GtkWidget* event_box = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(day_frame), event_box);
        GtkStyleContext* style_context = gtk_widget_get_style_context(day_frame);
        gtk_style_context_add_class(style_context, "day-cell");

        if (!cell) { // Empty cell (before 1st or after last day)
            gtk_style_context_add_class(style_context, "empty-cell");
             gtk_widget_set_opacity(day_frame, 0.1); // Make empty cells very faint
            // Optionally add a placeholder label or leave blank
            // GtkWidget* empty_label = gtk_label_new("-");
            // gtk_container_add(GTK_CONTAINER(event_box), empty_label);
        } else { // Valid day cell
        // Make it obvious that the day is clickable
        GtkCssProvider* day_provider = gtk_css_provider_new();
        const char* day_css = ".day-cell:hover { background-color: rgba(120, 120, 120, 0.2); }";
        gtk_css_provider_load_from_data(day_provider, day_css, -1, NULL);
            gtk_style_context_add_provider(style_context, 
                                    GTK_STYLE_PROVIDER(day_provider), 
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(day_provider);
        
        // Create a box for the day content
        GtkWidget* day_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_container_add(GTK_CONTAINER(event_box), day_box);
        
            // Lunar day number (primary display)
        char day_str[10];
            snprintf(day_str, sizeof(day_str), "%d", cell->lunar_day);
        GtkWidget* day_number = gtk_label_new(day_str);
        gtk_widget_set_halign(day_number, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(day_box), day_number, FALSE, FALSE, 0);
        
            // Show Gregorian date if enabled
        if (app->config && app->config->show_gregorian_dates) {
            char greg_str[32];
            snprintf(greg_str, sizeof(greg_str), "%04d-%02d-%02d", 
                         cell->greg_year, cell->greg_month, cell->greg_day);
                GtkWidget* greg_date = gtk_label_new(greg_str);
                 PangoAttrList* attrs = pango_attr_list_new();
                 pango_attr_list_insert(attrs, pango_attr_scale_new(PANGO_SCALE_SMALL));
                 gtk_label_set_attributes(GTK_LABEL(greg_date), attrs);
                 pango_attr_list_unref(attrs);
            gtk_widget_set_halign(greg_date, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(day_box), greg_date, FALSE, FALSE, 0);
        }
        
            // Add moon phase if enabled
        if (app->config && app->config->show_moon_phases) {
                 GtkWidget* moon_label = gtk_label_new(calendar_adapter_get_unicode_moon(cell->moon_phase));
                 gtk_widget_set_halign(moon_label, GTK_ALIGN_START);
                 gtk_box_pack_start(GTK_BOX(day_box), moon_label, FALSE, FALSE, 0);
            }

            // Add event indicator if needed
            if (event_date_has_events(cell->greg_year, cell->greg_month, cell->greg_day)) {
                GtkWidget* event_indicator = gtk_label_new("📅");
            gtk_widget_set_halign(event_indicator, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(day_box), event_indicator, FALSE, FALSE, 0);
        }
        
            // Make the event box clickable
        gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
        DayClickData* click_data = g_malloc(sizeof(DayClickData));
        click_data->app = app;
            click_data->year = cell->greg_year;
            click_data->month = cell->greg_month;
            click_data->day = cell->greg_day;
        g_object_set_data_full(G_OBJECT(event_box), "click-data", click_data, g_free);
        g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_day_clicked), NULL);
        
            // Set tooltip
            char* tooltip = calendar_adapter_get_tooltip_for_day(cell);
            gtk_widget_set_tooltip_text(day_frame, tooltip);
            g_free(tooltip); // Free the tooltip string returned by the adapter

            // Apply highlighting based on cell properties and config
            if (app->config && app->config->highlight_special_days && cell->is_special_day) {
                gtk_style_context_add_class(style_context, "special-day");
            GtkCssProvider* provider = gtk_css_provider_new();
            GdkRGBA color;
                calendar_adapter_get_special_day_color(cell->special_day_type, &color);
            char css[256];
            snprintf(css, sizeof(css), 
                    ".special-day { background-color: rgba(%d, %d, %d, %f); }",
                    (int)(color.red * 255), (int)(color.green * 255), 
                    (int)(color.blue * 255), color.alpha);
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
                gtk_style_context_add_provider(style_context, 
                                        GTK_STYLE_PROVIDER(provider), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
        }
        
            // Event color override
        GdkRGBA event_color;
            if (event_get_date_color(cell->greg_year, cell->greg_month, cell->greg_day, &event_color)) {
                gtk_style_context_add_class(style_context, "event-day");
            GtkCssProvider* provider = gtk_css_provider_new();
            char css[256];
            snprintf(css, sizeof(css), 
                    ".event-day { background-color: rgba(%d, %d, %d, %f); }",
                    (int)(event_color.red * 255), (int)(event_color.green * 255), 
                    (int)(event_color.blue * 255), event_color.alpha);
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
                gtk_style_context_add_provider(style_context, 
                                        GTK_STYLE_PROVIDER(provider), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
        }
        
            // Highlight today
            if (cell->is_today) {
            gtk_frame_set_shadow_type(GTK_FRAME(day_frame), GTK_SHADOW_IN);
                gtk_style_context_add_class(style_context, "today-cell"); // Add a class for CSS styling
                // GtkWidget* today_label = gtk_label_new("Today");
                // gtk_widget_set_halign(today_label, GTK_ALIGN_START);
                // gtk_box_pack_start(GTK_BOX(day_box), today_label, FALSE, FALSE, 0);
            }

            // Highlight selected day
            if (cell->greg_year == app->selected_day_year && 
                cell->greg_month == app->selected_day_month && 
                cell->greg_day == app->selected_day_day) {
                gtk_style_context_add_class(style_context, "selected-day");
            GtkCssProvider* provider = gtk_css_provider_new();
            const char* css = ".selected-day { border: 2px solid #3584e4; background-color: rgba(53, 132, 228, 0.3); }";
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
                gtk_style_context_add_provider(style_context, 
                                        GTK_STYLE_PROVIDER(provider), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
            gtk_frame_set_shadow_type(GTK_FRAME(day_frame), GTK_SHADOW_ETCHED_OUT);
            }
        }
        
        // Add the day cell to the grid
        gtk_grid_attach(GTK_GRID(calendar_grid), day_frame, col, row, 1, 1);
    }

    // Free the model now that the grid is built
    calendar_adapter_free_model(model);
    
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
        app->current_year--;
        /* Check number of months in the *new* previous year */
        // Use the corrected backend function
        int month_count = get_lunar_months_in_year(app->current_year); 
        app->current_month = month_count; // Wrap to 12 or 13
    }
    
    /* Update display */
    update_ui(app);
}

// Callback for next month button
static void on_next_month(GtkWidget* widget, gpointer data) {
    LunarCalendarApp* app = (LunarCalendarApp*)data;
    
    /* Check number of months in *current* year */
    // Use the corrected backend function
    int month_count = get_lunar_months_in_year(app->current_year);
    
    /* Go to next month */
    app->current_month++;
    if (app->current_month > month_count) {
        app->current_month = 1; // Wrap to month 1
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
        
        // Save configuration
        if (app->config_file_path) {
            config_save(app->config_file_path, app->config);
        }
    }
    
    // Save events
    if (app && app->events_file_path) {
        events_save(app->events_file_path);
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
    
    // Ensure we quit the main loop
    gtk_main_quit();
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
    LunarDay displayed_lunar_day_info = gregorian_to_lunar(app->current_year, app->current_month, 1);
    char displayed_eld_year_str[128];
    snprintf(displayed_eld_year_str, sizeof(displayed_eld_year_str), 
             "Displayed Eld Year: %d", displayed_lunar_day_info.eld_year);
    GtkWidget* displayed_eld_label = gtk_label_new(displayed_eld_year_str);
    gtk_box_pack_start(GTK_BOX(vbox), displayed_eld_label, FALSE, FALSE, 5);
    
    // Add a drawing area for the moon phase
    GtkWidget* moon_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(moon_drawing_area, 150, 150); // Set desired size
    g_object_set_data(G_OBJECT(moon_drawing_area), "moon-phase", GINT_TO_POINTER(lunar_day.moon_phase));
    g_signal_connect(G_OBJECT(moon_drawing_area), "draw", G_CALLBACK(draw_moon_phase_cairo), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), moon_drawing_area, FALSE, FALSE, 10);
    
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
        
        // Use custom month name if available
        const char* month_name = month_names[app->current_month - 1];
        if (app->config && app->config->custom_month_names[app->current_month - 1] && 
            strlen(app->config->custom_month_names[app->current_month - 1]) > 0) {
            month_name = app->config->custom_month_names[app->current_month - 1];
        }
        
        // Add the month count and check if this is a 13-month year
        int month_count = get_lunar_months_in_year(app->current_year);
        char year_info[50];
        sprintf(year_info, "(%d-Month Year)", month_count);
        
        // Full moon names for the 12 regular months
        const char* full_moon_names[] = {
            "1 Moon", "2 Moon", "3 Moon", "4 Moon", 
            "5 Moon", "6 Moon", "7 Moon", "8 Moon",
            "9 Moon", "10 Moon", "11 Moon", "12 Moon", 
            "13 Moon" // Name for the 13th month
        };
        
        // Use appropriate name for the full moon
        const char* moon_name = full_moon_names[app->current_month - 1];
        
        // Override with custom names if available
        if (app->config && app->current_month <= 12 && 
            app->config->custom_full_moon_names[app->current_month - 1] && 
            strlen(app->config->custom_full_moon_names[app->current_month - 1]) > 0) {
            moon_name = app->config->custom_full_moon_names[app->current_month - 1];
        }
        
        snprintf(month_text, sizeof(month_text), "Month %d: %s (%s) %s", 
                 app->current_month, month_name, moon_name, year_info);
        
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

/**
 * Initialize the metonic cycle status bar.
 * Creates a status bar at the bottom of the window showing the current position
 * in the 19-year metonic cycle.
 */
static void init_metonic_cycle_bar(LunarCalendarApp* app) {
    if (app->metonic_cycle_bar != NULL) {
        gtk_widget_destroy(app->metonic_cycle_bar);
    }
    
    // Only show if enabled in settings
    if (!app->config->show_metonic_cycle) {
        app->metonic_cycle_bar = NULL;
        return;
    }
    
    // Create a new status bar
    app->metonic_cycle_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(app->metonic_cycle_bar), 2);
    
    // Add to the main window layout
    gtk_box_pack_end(GTK_BOX(app->main_layout), app->metonic_cycle_bar, FALSE, FALSE, 0);
    
    // Create label for the metonic cycle info
    app->metonic_cycle_label = gtk_label_new("");
    gtk_widget_set_halign(app->metonic_cycle_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(app->metonic_cycle_bar), app->metonic_cycle_label, FALSE, FALSE, 5);
    
    // Create a progress bar for the visual indication
    app->metonic_cycle_progress = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(app->metonic_cycle_progress), TRUE);
    gtk_widget_set_size_request(app->metonic_cycle_progress, 200, -1);
    gtk_box_pack_start(GTK_BOX(app->metonic_cycle_bar), app->metonic_cycle_progress, FALSE, FALSE, 5);
    
    // Add help button
    GtkWidget* help_button = gtk_button_new_with_label("?");
    gtk_widget_set_tooltip_text(help_button, "What is the Metonic Cycle?");
    gtk_box_pack_end(GTK_BOX(app->metonic_cycle_bar), help_button, FALSE, FALSE, 5);
    g_signal_connect(help_button, "clicked", G_CALLBACK(on_metonic_help_clicked), app);
    
    // Show all widgets
    gtk_widget_show_all(app->metonic_cycle_bar);
    
    // Update the metonic cycle information
    update_metonic_cycle_display(app);
}

/**
 * Update the metonic cycle status bar.
 * Calculates the current year's position in the 19-year metonic cycle
 * and updates the display.
 */
static void update_metonic_cycle_display(LunarCalendarApp* app) {
    if (app->metonic_cycle_bar == NULL || !app->config->show_metonic_cycle) {
        return;
    }
    
    // Get the current year
    GDate date;
    g_date_set_time_t(&date, time(NULL));
    int current_year = g_date_get_year(&date);
    
    // Calculate position in metonic cycle (1-19)
    // The metonic cycle is 19 years long, with the years starting at 1
    int metonic_year = ((current_year + 1) % 19);
    if (metonic_year == 0) {
        metonic_year = 19;
    }
    
    // Update the label
    char cycle_text[128];
    snprintf(cycle_text, sizeof(cycle_text), "Metonic Cycle Year %d of 19", metonic_year);
    gtk_label_set_text(GTK_LABEL(app->metonic_cycle_label), cycle_text);
    
    // Update the progress bar
    double progress = (double)metonic_year / 19.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->metonic_cycle_progress), progress);
    
    // Set the progress bar text
    char progress_text[32];
    snprintf(progress_text, sizeof(progress_text), "%d/19", metonic_year);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->metonic_cycle_progress), progress_text);
}

/**
 * Handle the metonic cycle help button click.
 * Shows a dialog with information about the metonic cycle.
 */
static void on_metonic_help_clicked(GtkButton* button, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "The Metonic Cycle");
    
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
        "The Metonic Cycle is a period of 19 years in which the phases of the moon recur on the same day of the year.\n\n"
        "This cycle is the basis for many lunar calendars, including the Germanic lunar calendar. "
        "It was discovered by the ancient Greek astronomer Meton in the 5th century BCE.\n\n"
        "The cycle consists of 19 years, containing 235 lunar months. "
        "These 235 months are divided into 125 full months of 30 days and 110 hollow months of 29 days.\n\n"
        "In practical terms, 12 of the 19 years have 12 lunar months (ordinary years) while 7 years have 13 lunar months "
        "(intercalary years).\n\n"
        "The progress bar shows the current position in the 19-year cycle.");
    
    gtk_window_set_title(GTK_WINDOW(dialog), "Metonic Cycle Information");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/**
 * Update the UI components based on the current configuration.
 * This is called when settings are changed.
 */
static void update_ui_from_config(LunarCalendarApp* app) {
    if (!app) return;
    
    // Print debug info to confirm this is being called
    g_print("Applying settings changes...\n");
    
    // Update theme based on settings
    GtkSettings* settings = gtk_settings_get_default();
    if (settings) {
        g_object_set(settings, "gtk-application-prefer-dark-theme", app->config->use_dark_theme, NULL);
    }
    
    // Apply custom fonts, colors and cell size using CSS
    GtkCssProvider* provider = gtk_css_provider_new();
    GString* css_string = g_string_new("");
    
    // Apply font if set
    if (app->config->font_name && strlen(app->config->font_name) > 0) {
        g_string_append_printf(css_string, 
            "* { font-family: %s; }\n", 
            app->config->font_name);
    }
    
    // Apply colors
    g_string_append_printf(css_string,
        ".primary-color { color: rgba(%d,%d,%d,%.2f); }\n"
        ".secondary-color { color: rgba(%d,%d,%d,%.2f); }\n"
        ".cell-content { color: rgba(%d,%d,%d,%.2f); }\n",
        (int)(app->config->primary_color.red * 255),
        (int)(app->config->primary_color.green * 255),
        (int)(app->config->primary_color.blue * 255),
        app->config->primary_color.alpha,
        (int)(app->config->secondary_color.red * 255),
        (int)(app->config->secondary_color.green * 255),
        (int)(app->config->secondary_color.blue * 255),
        app->config->secondary_color.alpha,
        (int)(app->config->text_color.red * 255),
        (int)(app->config->text_color.green * 255),
        (int)(app->config->text_color.blue * 255),
        app->config->text_color.alpha);
    
    // Set cell size
    if (app->config->cell_size > 0) {
        g_string_append_printf(css_string,
            ".day-cell { min-width: %dpx; min-height: %dpx; }\n",
            app->config->cell_size, app->config->cell_size);
    }
    
    // Load the CSS if we have any
    if (css_string->len > 0) {
        gtk_css_provider_load_from_data(provider, css_string->str, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    
    g_string_free(css_string, TRUE);
    g_object_unref(provider);
    
    // Update metonic cycle bar visibility
    if (app->metonic_cycle_bar) {
        if (app->config->show_metonic_cycle) {
            gtk_widget_show(app->metonic_cycle_bar);
            update_metonic_cycle_display(app);
        } else {
            gtk_widget_hide(app->metonic_cycle_bar);
        }
    }
    
    // Clear and rebuild calendar view to reflect new settings
    update_calendar_view(app);
    
    // Update month label (to show custom month names)
    update_month_label(app);
    
    // Update sidebar if it exists
    if (app->sidebar) {
        update_sidebar(app);
    }
    
    // Update header
    update_header(app);
    
    // Redraw the entire window to reflect changes
    if (app->window) {
        gtk_widget_queue_draw(app->window);
    }
    
    // Show a status message to confirm changes were applied
    if (app->status_bar) {
        gtk_statusbar_push(GTK_STATUSBAR(app->status_bar), 0, 
                       "Settings have been applied successfully.");
    }
}

/**
 * Handle settings button click.
 * Opens the settings dialog.
 */
static void on_settings_clicked(GtkButton* button, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    
    // Show the settings dialog, passing the update function
    if (settings_dialog_show(app, GTK_WINDOW(app->window), update_ui_from_config)) {
        // Settings were changed and saved (OK was clicked)
        // No need to call update_ui_from_config again here as it's handled by settings_dialog_show
        g_print("Settings updated after dialog closed with OK.\n");
    }
}

// Cairo drawing function for the moon phase
static gboolean draw_moon_phase_cairo(GtkWidget *widget, cairo_t *cr, gpointer data) {
    MoonPhase phase = (MoonPhase)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "moon-phase"));
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);
    double radius = MIN(width, height) / 2.0 - 5.0; // Radius with padding
    double center_x = width / 2.0;
    double center_y = height / 2.0;

    // Draw background (optional, useful for transparent themes)
    // cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.8);
    // cairo_paint(cr);

    // --- Draw Moon Outline --- 
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White
    cairo_set_line_width(cr, 2.0);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke_preserve(cr); // Keep path for potential fill

    // --- Calculate Illumination --- 
    double illumination_fraction = 0.0;
    gboolean waxing = TRUE; // True if waxing (right side lit), False if waning (left side lit)

    switch (phase) {
        case NEW_MOON:        illumination_fraction = 0.0; break;
        case WAXING_CRESCENT: illumination_fraction = 0.25; waxing = TRUE; break;
        case FIRST_QUARTER:   illumination_fraction = 0.5;  waxing = TRUE; break;
        case WAXING_GIBBOUS:  illumination_fraction = 0.75; waxing = TRUE; break;
        case FULL_MOON:       illumination_fraction = 1.0; break;
        case WANING_GIBBOUS:  illumination_fraction = 0.75; waxing = FALSE; break;
        case LAST_QUARTER:    illumination_fraction = 0.5;  waxing = FALSE; break;
        case WANING_CRESCENT: illumination_fraction = 0.25; waxing = FALSE; break;
        default:              illumination_fraction = 0.0; break;
    }

    // --- Draw Illuminated Portion --- 
    if (illumination_fraction == 1.0) { // Full Moon
        cairo_fill(cr); // Fill the preserved outline path
    } else if (illumination_fraction > 0.0) { // Partially illuminated
        // Use clipping to draw illuminated part
        cairo_save(cr);

        if (illumination_fraction == 0.5) { // Quarters
            if (waxing) { // First Quarter (right half lit)
                cairo_rectangle(cr, center_x, center_y - radius - 2, radius + 2, 2 * radius + 4);
            } else { // Last Quarter (left half lit)
                cairo_rectangle(cr, center_x - radius - 2, center_y - radius - 2, radius + 2, 2 * radius + 4);
            }
            cairo_clip(cr);
            cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
            cairo_fill(cr);

        } else { // Crescent or Gibbous
            // Calculate the horizontal shift of the terminator ellipse center
            // x = radius * cos(angle), angle depends on phase fraction
            // Simplified: map fraction (0..1) to angle (PI..0 for waxing, 0..PI for waning)
            double angle = acos(1.0 - 2.0 * illumination_fraction);
            double x_offset = radius * cos(angle);
            if (!waxing) x_offset = -x_offset;

            // Clip to the main circle boundary first
            cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
            cairo_clip(cr);

            if ( (waxing && illumination_fraction < 0.5) || (!waxing && illumination_fraction < 0.5) ) { // Crescent
                // Draw the lit part (overlap of two circles)
                if (waxing) { // Right side lit
                     cairo_arc(cr, center_x + x_offset, center_y, radius, 0, 2 * M_PI);
                } else { // Left side lit
                    cairo_arc(cr, center_x + x_offset, center_y, radius, 0, 2 * M_PI);
                }
                cairo_fill(cr);
            } else { // Gibbous
                 // Fill the main circle first
                 cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
                 cairo_fill(cr);
                 
                 // Then draw the dark part (overlap)
                 cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black
                 if (waxing) { // Dark on left
                     cairo_arc(cr, center_x - x_offset, center_y, radius, 0, 2 * M_PI);
                 } else { // Dark on right
                    cairo_arc(cr, center_x - x_offset, center_y, radius, 0, 2 * M_PI);
                 }
                 cairo_fill(cr);
            }
        }
        cairo_restore(cr); // Remove clipping
    }
    // If illumination is 0.0 (New Moon), only the outline is drawn.

    return FALSE; // Indicate redraw not needed unless requested
} 