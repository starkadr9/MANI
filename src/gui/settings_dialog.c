#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/gui/settings_dialog.h"
#include "../../include/gui/config.h"
#include "../../include/gui/gui_app.h"

// Structure to hold pointers to widgets used in the settings dialog
typedef struct {
    // Appearance widgets
    GtkWidget* theme_combo;
    GtkWidget* primary_color_button;
    GtkWidget* secondary_color_button;
    GtkWidget* text_color_button;
    GtkWidget* font_button;
    GtkWidget* cell_size_spin;
    GtkWidget* preview_area;
    
    // Display widgets
    GtkWidget* show_moon_phases_check;
    GtkWidget* highlight_special_days_check;
    GtkWidget* show_gregorian_dates_check;
    GtkWidget* show_weekday_names_check;
    GtkWidget* show_metonic_cycle_check;
    GtkWidget* show_event_indicators_check;
    GtkWidget* week_start_day_combo;
    
    // Names widgets
    GtkWidget* month_name_entries[13]; // 1-12 months + header
    GtkWidget* weekday_name_entries[8]; // 0-6 days + header
    GtkWidget* full_moon_name_entries[12]; // Names for full moons, one per month
    
    // Advanced widgets
    GtkWidget* events_file_path_entry;
    GtkWidget* cache_dir_entry;
    GtkWidget* log_file_path_entry;
    GtkWidget* debug_logging_check;
} SettingsWidgets;

// Forward declarations for tab creation functions
// static GtkWidget* create_appearance_tab(LunarCalendarApp* app, SettingsWidgets* widgets);
static GtkWidget* create_display_tab(LunarCalendarApp* app, SettingsWidgets* widgets);
static GtkWidget* create_names_tab(LunarCalendarApp* app, SettingsWidgets* widgets);
static GtkWidget* create_moon_names_tab(LunarCalendarApp* app, SettingsWidgets* widgets);
static GtkWidget* create_advanced_tab(LunarCalendarApp* app, SettingsWidgets* widgets);

// Forward declarations for callback functions
static void apply_settings(LunarCalendarApp* app);
static void on_import_clicked(GtkButton* button, gpointer user_data);
static void on_export_clicked(GtkButton* button, gpointer user_data);
static void on_help_clicked(GtkButton* button, gpointer user_data);
static void on_reset_month_name(GtkButton* button, gpointer user_data);
static void on_reset_weekday_name(GtkButton* button, gpointer user_data);
static void on_reset_moon_name(GtkButton* button, gpointer user_data);
static void on_browse_events_file(GtkButton* button, gpointer user_data);
static void on_browse_cache_dir(GtkButton* button, gpointer user_data);
static void on_browse_log_file(GtkButton* button, gpointer user_data);
static void on_clear_cache(GtkButton* button, gpointer user_data);
static void on_reset_all_settings(GtkButton* button, gpointer user_data);

// Function to store widget pointers associated with the main app window
static void store_widget_pointer(LunarCalendarApp* app, const char* name, GtkWidget* widget) {
    if (app && app->window) {
        g_object_set_data(G_OBJECT(app->window), name, widget);
    }
}

/**
 * Create and show a settings dialog.
 */
gboolean settings_dialog_show(LunarCalendarApp* app, GtkWindow* parent, MainUIUpdateFunc main_update_func) {
    SettingsWidgets* widgets = g_malloc0(sizeof(SettingsWidgets));
    
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "MANI Settings",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Apply", GTK_RESPONSE_APPLY,
        "OK", GTK_RESPONSE_OK,
        NULL
    );
    
    // Set dialog size
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 500);
    
    // Get content area
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_set_spacing(GTK_BOX(content_area), 10);
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
    
    // Create notebook (tabbed interface)
    GtkWidget* notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(content_area), notebook, TRUE, TRUE, 0);
    
    // Create tabs
    // GtkWidget* appearance_tab = create_appearance_tab(app, widgets);
    // Not showing appearance tab since it doesn't work properly
    // gtk_notebook_append_page(GTK_NOTEBOOK(notebook), appearance_tab, 
    //                        gtk_label_new("Appearance"));
    
    GtkWidget* display_tab = create_display_tab(app, widgets);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), display_tab, 
                            gtk_label_new("Display"));
    
    GtkWidget* names_tab = create_names_tab(app, widgets);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), names_tab, 
                            gtk_label_new("Month Names"));
    
    // Add the Moon Names tab
    GtkWidget* moon_names_tab = create_moon_names_tab(app, widgets);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), moon_names_tab,
                            gtk_label_new("Moon Names"));
    
    GtkWidget* advanced_tab = create_advanced_tab(app, widgets);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), advanced_tab, 
                            gtk_label_new("Advanced"));
    
    // Add bottom action area with import/export/help buttons
    GtkWidget* bottom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(content_area), bottom_box, FALSE, FALSE, 0);
    
    // Import button
    GtkWidget* import_button = gtk_button_new_with_label("Import Config");
    g_signal_connect(import_button, "clicked", G_CALLBACK(on_import_clicked), app);
    gtk_box_pack_start(GTK_BOX(bottom_box), import_button, FALSE, FALSE, 0);
    
    // Export button
    GtkWidget* export_button = gtk_button_new_with_label("Export Config");
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_clicked), app);
    gtk_box_pack_start(GTK_BOX(bottom_box), export_button, FALSE, FALSE, 0);
    
    // Help button
    GtkWidget* help_button = gtk_button_new_with_label("Help");
    g_signal_connect(help_button, "clicked", G_CALLBACK(on_help_clicked), notebook);
    gtk_box_pack_end(GTK_BOX(bottom_box), help_button, FALSE, FALSE, 0);
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run the dialog
    gboolean settings_changed = FALSE;
    gint response;
    
    while ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_APPLY ||
           response == GTK_RESPONSE_OK) {
        // 1. Apply settings to the config struct
        apply_settings(app);
        settings_changed = TRUE;
        // g_print("Settings dialog: changes applied (response: %d)\n", response); // DEBUG REMOVED

        // 2. Save the configuration immediately
        if (app->config_file_path) {
            config_save(app->config_file_path, app->config);
            // g_print("Config saved within loop.\n"); // DEBUG REMOVED
        }

        // 3. Call the main UI update function
        if (main_update_func) {
            main_update_func(app);
            // g_print("Main UI update function called.\n"); // DEBUG REMOVED
        }

        // 4. Force immediate GUI update by processing pending events
        while (gtk_events_pending())
            gtk_main_iteration_do(FALSE);  // FALSE = don't block

        // 5. If OK, now we can break the loop
        if (response == GTK_RESPONSE_OK) {
            // g_print("Breaking loop after OK.\n"); // DEBUG REMOVED
            break;
        }
        
        // Actions specific to Apply (like status bar update)
        if (response == GTK_RESPONSE_APPLY) {
            if (app->status_bar) {
                gtk_statusbar_push(GTK_STATUSBAR(app->status_bar), 0, 
                               "Settings have been applied.");
            }
            // g_print("Continuing loop after Apply.\n"); // DEBUG REMOVED
        }
    }
    
    // Save settings again if they were changed (safety net)
    if (settings_changed) {
        // g_print("Final config save after loop exit (may be redundant).\n"); // DEBUG REMOVED
        config_save(app->config_file_path, app->config);
    }
    
    // Free the widgets structure
    g_free(widgets);
    
    // Destroy the dialog
    gtk_widget_destroy(dialog);
    
    return settings_changed;
}

// Display tab creation
static GtkWidget* create_display_tab(LunarCalendarApp* app, SettingsWidgets* widgets) {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    
    int row = 0;
    
    // Week start day
    GtkWidget* start_day_label = gtk_label_new("Week Starts On:");
    gtk_widget_set_halign(start_day_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), start_day_label, 0, row, 1, 1);
    
    widgets->week_start_day_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->week_start_day_combo), "Sunday");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->week_start_day_combo), "Monday");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->week_start_day_combo), "Saturday");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->week_start_day_combo), 
                            app->config->week_start_day);
    gtk_grid_attach(GTK_GRID(grid), widgets->week_start_day_combo, 1, row, 1, 1);
    
    // Store the widget for later access
    store_widget_pointer(app, "week_start_day_combo", widgets->week_start_day_combo);
    
    row++;
    
    // Show Gregorian dates
    GtkWidget* greg_dates_label = gtk_label_new("Show Gregorian Dates:");
    gtk_widget_set_halign(greg_dates_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), greg_dates_label, 0, row, 1, 1);
    
    widgets->show_gregorian_dates_check = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->show_gregorian_dates_check), 
                         app->config->show_gregorian_dates);
    gtk_widget_set_halign(widgets->show_gregorian_dates_check, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), widgets->show_gregorian_dates_check, 1, row, 1, 1);
    
    // Store the widget for later access
    store_widget_pointer(app, "show_gregorian_dates_check", widgets->show_gregorian_dates_check);
    
    row++;
    
    // Show moon phases
    GtkWidget* moon_phases_label = gtk_label_new("Show Moon Phases:");
    gtk_widget_set_halign(moon_phases_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), moon_phases_label, 0, row, 1, 1);
    
    widgets->show_moon_phases_check = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->show_moon_phases_check), 
                         app->config->show_moon_phases);
    gtk_widget_set_halign(widgets->show_moon_phases_check, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), widgets->show_moon_phases_check, 1, row, 1, 1);
    
    // Store the widget for later access
    store_widget_pointer(app, "show_moon_phases_check", widgets->show_moon_phases_check);
    
    row++;
    
    // Highlight special days
    GtkWidget* special_days_label = gtk_label_new("Highlight Special Days:");
    gtk_widget_set_halign(special_days_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), special_days_label, 0, row, 1, 1);
    
    widgets->highlight_special_days_check = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->highlight_special_days_check), 
                         app->config->highlight_special_days);
    gtk_widget_set_halign(widgets->highlight_special_days_check, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), widgets->highlight_special_days_check, 1, row, 1, 1);
    
    // Store the widget for later access
    store_widget_pointer(app, "highlight_special_days_check", widgets->highlight_special_days_check);
    
    row++;
    
    // Show weekday names
    GtkWidget* weekday_names_label = gtk_label_new("Show Weekday Names:");
    gtk_widget_set_halign(weekday_names_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), weekday_names_label, 0, row, 1, 1);
    
    widgets->show_weekday_names_check = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->show_weekday_names_check), 
                         app->config->show_weekday_names);
    gtk_widget_set_halign(widgets->show_weekday_names_check, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), widgets->show_weekday_names_check, 1, row, 1, 1);
    
    // Store the widget for later access
    store_widget_pointer(app, "show_weekday_names_check", widgets->show_weekday_names_check);
    
    row++;
    
    // Show event indicators
    GtkWidget* event_indicators_label = gtk_label_new("Show Event Indicators:");
    gtk_widget_set_halign(event_indicators_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), event_indicators_label, 0, row, 1, 1);
    
    widgets->show_event_indicators_check = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->show_event_indicators_check), 
                         app->config->show_event_indicators);
    gtk_widget_set_halign(widgets->show_event_indicators_check, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), widgets->show_event_indicators_check, 1, row, 1, 1);
    
    // Store the widget for later access
    store_widget_pointer(app, "show_event_indicators_check", widgets->show_event_indicators_check);
    
    row++;
    
    // Show metonic cycle
    GtkWidget* metonic_cycle_label = gtk_label_new("Show Metonic Cycle Bar:");
    gtk_widget_set_halign(metonic_cycle_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), metonic_cycle_label, 0, row, 1, 1);
    
    widgets->show_metonic_cycle_check = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->show_metonic_cycle_check), 
                         app->config->show_metonic_cycle);
    gtk_widget_set_halign(widgets->show_metonic_cycle_check, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), widgets->show_metonic_cycle_check, 1, row, 1, 1);
    
    // Store the widget for later access
    store_widget_pointer(app, "show_metonic_cycle_check", widgets->show_metonic_cycle_check);
    
    return grid;
}

// Names tab creation
static GtkWidget* create_names_tab(LunarCalendarApp* app, SettingsWidgets* widgets) {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    
    // Create a scrolled window to contain everything
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(grid), scrolled);
    
    // Make the scrolled window expand to fill available space
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    
    // Create a vertical box to hold the month and weekday sections
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_add(GTK_CONTAINER(scrolled), content_box);
    
    // Month names section with a frame
    GtkWidget* month_frame = gtk_frame_new("Lunar Month Names");
    gtk_box_pack_start(GTK_BOX(content_box), month_frame, FALSE, FALSE, 0);
    
    GtkWidget* month_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(month_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(month_grid), 10);
    gtk_widget_set_margin_start(month_grid, 10);
    gtk_widget_set_margin_end(month_grid, 10);
    gtk_widget_set_margin_top(month_grid, 10);
    gtk_widget_set_margin_bottom(month_grid, 10);
    gtk_container_add(GTK_CONTAINER(month_frame), month_grid);
    
    // Column headers for month names
    gtk_grid_attach(GTK_GRID(month_grid), gtk_label_new("#"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(month_grid), gtk_label_new("Default Name"), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(month_grid), gtk_label_new("Custom Name"), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(month_grid), gtk_label_new("Reset"), 3, 0, 1, 1);
    
    // Default month names
    const char* default_month_names[] = {
        "Month 1", "Month 2", "Month 3", "Month 4", "Month 5",
        "Month 6", "Month 7", "Month 8", "Month 9", "Month 10", 
        "Month 11", "Month 12", "Month 13"
    };
    
    // Create entries for month names
    for (int i = 0; i < 13; i++) {
        // Month number
        char month_num[4];
        snprintf(month_num, sizeof(month_num), "%d", i + 1);
        GtkWidget* number = gtk_label_new(month_num);
        gtk_grid_attach(GTK_GRID(month_grid), number, 0, i + 1, 1, 1);
        
        // Default name
        GtkWidget* default_name = gtk_label_new(default_month_names[i]);
        gtk_widget_set_halign(default_name, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(month_grid), default_name, 1, i + 1, 1, 1);
        
        // Custom name entry
        widgets->month_name_entries[i] = gtk_entry_new();
        if (app->config->custom_month_names[i]) {
            gtk_entry_set_text(GTK_ENTRY(widgets->month_name_entries[i]), 
                             app->config->custom_month_names[i]);
        } else {
            gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->month_name_entries[i]), 
                                        default_month_names[i]);
        }
        gtk_grid_attach(GTK_GRID(month_grid), widgets->month_name_entries[i], 2, i + 1, 1, 1);
        
        // Store the widget for later access
        char widget_name[32];
        snprintf(widget_name, sizeof(widget_name), "month_entry_%d", i);
        store_widget_pointer(app, widget_name, widgets->month_name_entries[i]);
        
        // Reset button
        GtkWidget* reset_btn = gtk_button_new_with_label("Reset");
        gtk_widget_set_name(reset_btn, g_strdup_printf("%d", i + 1));
        gtk_grid_attach(GTK_GRID(month_grid), reset_btn, 3, i + 1, 1, 1);
        g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_month_name), widgets->month_name_entries[i]);
    }
    
    // Weekday names section with a frame
    GtkWidget* weekday_frame = gtk_frame_new("Weekday Names");
    gtk_box_pack_start(GTK_BOX(content_box), weekday_frame, FALSE, FALSE, 0);
    
    GtkWidget* weekday_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(weekday_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(weekday_grid), 10);
    gtk_widget_set_margin_start(weekday_grid, 10);
    gtk_widget_set_margin_end(weekday_grid, 10);
    gtk_widget_set_margin_top(weekday_grid, 10);
    gtk_widget_set_margin_bottom(weekday_grid, 10);
    gtk_container_add(GTK_CONTAINER(weekday_frame), weekday_grid);
    
    // Column headers for weekday names
    gtk_grid_attach(GTK_GRID(weekday_grid), gtk_label_new("#"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(weekday_grid), gtk_label_new("Default Name"), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(weekday_grid), gtk_label_new("Custom Name"), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(weekday_grid), gtk_label_new("Reset"), 3, 0, 1, 1);
    
    // Default weekday names
    const char* default_weekday_names[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", 
        "Thursday", "Friday", "Saturday"
    };
    
    // Create entries for weekday names
    for (int i = 0; i < 7; i++) {
        // Weekday number
        char day_num[4];
        snprintf(day_num, sizeof(day_num), "%d", i);
        GtkWidget* number = gtk_label_new(day_num);
        gtk_grid_attach(GTK_GRID(weekday_grid), number, 0, i + 1, 1, 1);
        
        // Default name
        GtkWidget* default_name = gtk_label_new(default_weekday_names[i]);
        gtk_widget_set_halign(default_name, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(weekday_grid), default_name, 1, i + 1, 1, 1);
        
        // Custom name entry
        widgets->weekday_name_entries[i] = gtk_entry_new();
        if (app->config->custom_weekday_names[i]) {
            gtk_entry_set_text(GTK_ENTRY(widgets->weekday_name_entries[i]), 
                             app->config->custom_weekday_names[i]);
        } else {
            gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->weekday_name_entries[i]), 
                                        default_weekday_names[i]);
        }
        gtk_grid_attach(GTK_GRID(weekday_grid), widgets->weekday_name_entries[i], 2, i + 1, 1, 1);
        
        // Store the widget for later access
        char widget_name[32];
        snprintf(widget_name, sizeof(widget_name), "weekday_entry_%d", i);
        store_widget_pointer(app, widget_name, widgets->weekday_name_entries[i]);
        
        // Reset button
        GtkWidget* reset_btn = gtk_button_new_with_label("Reset");
        gtk_widget_set_name(reset_btn, g_strdup_printf("%d", i));
        gtk_grid_attach(GTK_GRID(weekday_grid), reset_btn, 3, i + 1, 1, 1);
        g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_weekday_name), widgets->weekday_name_entries[i]);
    }
    
    return grid;
}

// Moon Names tab creation
static GtkWidget* create_moon_names_tab(LunarCalendarApp* app, SettingsWidgets* widgets) {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    
    // Create a scrolled window to contain everything
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(grid), scrolled);
    
    // Make the scrolled window expand to fill available space
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    
    // Create a vertical box for the content
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_add(GTK_CONTAINER(scrolled), content_box);
    
    // Add a description label
    GtkWidget* description = gtk_label_new(
        "Customize the names of the full moons for each month of the year.\n"
        "These names will be displayed when viewing full moons in the calendar.");
    gtk_widget_set_halign(description, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content_box), description, FALSE, FALSE, 0);
    
    // Full moon names section with a frame
    GtkWidget* moon_frame = gtk_frame_new("Full Moon Names");
    gtk_box_pack_start(GTK_BOX(content_box), moon_frame, FALSE, FALSE, 0);
    
    GtkWidget* moon_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(moon_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(moon_grid), 10);
    gtk_widget_set_margin_start(moon_grid, 10);
    gtk_widget_set_margin_end(moon_grid, 10);
    gtk_widget_set_margin_top(moon_grid, 10);
    gtk_widget_set_margin_bottom(moon_grid, 10);
    gtk_container_add(GTK_CONTAINER(moon_frame), moon_grid);
    
    // Column headers
    gtk_grid_attach(GTK_GRID(moon_grid), gtk_label_new("Month"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(moon_grid), gtk_label_new("Default Name"), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(moon_grid), gtk_label_new("Custom Name"), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(moon_grid), gtk_label_new("Reset"), 3, 0, 1, 1);
    
    // Default full moon names
    const char* default_moon_names[] = {
        "1 Moon", "2 Moon", "3 Moon", "4 Moon",
        "5 Moon", "6 Moon", "7 Moon", "8 Moon",
        "9 Moon", "10 Moon", "11 Moon", "12 Moon"
    };
    
    // Month names for display
    const char* month_names[] = {
        "January", "February", "March", "April",
        "May", "June", "July", "August",
        "September", "October", "November", "December"
    };
    
    // Create entries for moon names
    for (int i = 0; i < 12; i++) {
        // Month name
        GtkWidget* month = gtk_label_new(month_names[i]);
        gtk_widget_set_halign(month, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(moon_grid), month, 0, i + 1, 1, 1);
        
        // Default name
        GtkWidget* default_name = gtk_label_new(default_moon_names[i]);
        gtk_widget_set_halign(default_name, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(moon_grid), default_name, 1, i + 1, 1, 1);
        
        // Custom name entry
        widgets->full_moon_name_entries[i] = gtk_entry_new();
        if (app->config->custom_full_moon_names[i]) {
            gtk_entry_set_text(GTK_ENTRY(widgets->full_moon_name_entries[i]), 
                             app->config->custom_full_moon_names[i]);
        } else {
            gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->full_moon_name_entries[i]), 
                                        default_moon_names[i]);
        }
        gtk_grid_attach(GTK_GRID(moon_grid), widgets->full_moon_name_entries[i], 2, i + 1, 1, 1);
        
        // Store the widget for later access
        char widget_name[32];
        snprintf(widget_name, sizeof(widget_name), "moon_entry_%d", i);
        store_widget_pointer(app, widget_name, widgets->full_moon_name_entries[i]);
        
        // Reset button
        GtkWidget* reset_btn = gtk_button_new_with_label("Reset");
        gtk_widget_set_name(reset_btn, g_strdup_printf("%d", i + 1));
        gtk_grid_attach(GTK_GRID(moon_grid), reset_btn, 3, i + 1, 1, 1);
        g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_moon_name), widgets->full_moon_name_entries[i]);
    }
    
    return grid;
}

// Advanced tab creation
static GtkWidget* create_advanced_tab(LunarCalendarApp* app, SettingsWidgets* widgets) {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    
    // Events file path
    GtkWidget* events_file_label = gtk_label_new("Events File:");
    gtk_widget_set_halign(events_file_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), events_file_label, 0, 0, 1, 1);
    
    GtkWidget* events_file_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    widgets->events_file_path_entry = gtk_entry_new();
    if (app->config->events_file_path) {
        gtk_entry_set_text(GTK_ENTRY(widgets->events_file_path_entry), app->config->events_file_path);
    }
    gtk_box_pack_start(GTK_BOX(events_file_box), widgets->events_file_path_entry, TRUE, TRUE, 0);
    
    GtkWidget* events_file_button = gtk_button_new_with_label("Browse...");
    gtk_box_pack_start(GTK_BOX(events_file_box), events_file_button, FALSE, FALSE, 0);
    g_signal_connect(events_file_button, "clicked", G_CALLBACK(on_browse_events_file), widgets->events_file_path_entry);
    
    gtk_grid_attach(GTK_GRID(grid), events_file_box, 1, 0, 1, 1);
    
    // Store widget for later access
    store_widget_pointer(app, "events_file_path_entry", widgets->events_file_path_entry);
    
    // Cache directory
    GtkWidget* cache_dir_label = gtk_label_new("Cache Directory:");
    gtk_widget_set_halign(cache_dir_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), cache_dir_label, 0, 1, 1, 1);
    
    GtkWidget* cache_dir_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    widgets->cache_dir_entry = gtk_entry_new();
    if (app->config->cache_dir) {
        gtk_entry_set_text(GTK_ENTRY(widgets->cache_dir_entry), app->config->cache_dir);
    }
    gtk_box_pack_start(GTK_BOX(cache_dir_box), widgets->cache_dir_entry, TRUE, TRUE, 0);
    
    GtkWidget* cache_dir_button = gtk_button_new_with_label("Browse...");
    gtk_box_pack_start(GTK_BOX(cache_dir_box), cache_dir_button, FALSE, FALSE, 0);
    g_signal_connect(cache_dir_button, "clicked", G_CALLBACK(on_browse_cache_dir), widgets->cache_dir_entry);
    
    gtk_grid_attach(GTK_GRID(grid), cache_dir_box, 1, 1, 1, 1);
    
    // Store widget for later access
    store_widget_pointer(app, "cache_dir_entry", widgets->cache_dir_entry);
    
    // Enable debug logging
    GtkWidget* debug_logging_label = gtk_label_new("Enable Debug Logging:");
    gtk_widget_set_halign(debug_logging_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), debug_logging_label, 0, 2, 1, 1);
    
    widgets->debug_logging_check = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->debug_logging_check), 
                          app->config->debug_logging);
    gtk_widget_set_halign(widgets->debug_logging_check, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), widgets->debug_logging_check, 1, 2, 1, 1);
    
    // Store widget for later access
    store_widget_pointer(app, "debug_logging_check", widgets->debug_logging_check);
    
    // Log file path
    GtkWidget* log_file_label = gtk_label_new("Log File:");
    gtk_widget_set_halign(log_file_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), log_file_label, 0, 3, 1, 1);
    
    GtkWidget* log_file_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    widgets->log_file_path_entry = gtk_entry_new();
    if (app->config->log_file_path) {
        gtk_entry_set_text(GTK_ENTRY(widgets->log_file_path_entry), app->config->log_file_path);
    }
    gtk_box_pack_start(GTK_BOX(log_file_box), widgets->log_file_path_entry, TRUE, TRUE, 0);
    
    GtkWidget* log_file_button = gtk_button_new_with_label("Browse...");
    gtk_box_pack_start(GTK_BOX(log_file_box), log_file_button, FALSE, FALSE, 0);
    g_signal_connect(log_file_button, "clicked", G_CALLBACK(on_browse_log_file), widgets->log_file_path_entry);
    
    gtk_grid_attach(GTK_GRID(grid), log_file_box, 1, 3, 1, 1);
    
    // Store widget for later access
    store_widget_pointer(app, "log_file_path_entry", widgets->log_file_path_entry);
    
    // Buttons section
    GtkWidget* buttons_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_top(buttons_box, 20);
    
    // Clear cache button
    GtkWidget* clear_cache_button = gtk_button_new_with_label("Clear Cache");
    gtk_box_pack_start(GTK_BOX(buttons_box), clear_cache_button, FALSE, FALSE, 0);
    g_signal_connect(clear_cache_button, "clicked", G_CALLBACK(on_clear_cache), app);
    
    // Reset all settings button
    GtkWidget* reset_all_button = gtk_button_new_with_label("Reset All Settings");
    gtk_box_pack_start(GTK_BOX(buttons_box), reset_all_button, FALSE, FALSE, 0);
    g_signal_connect(reset_all_button, "clicked", G_CALLBACK(on_reset_all_settings), app);
    
    gtk_grid_attach(GTK_GRID(grid), buttons_box, 0, 4, 2, 1);
    
    return grid;
}

// Helper functions for buttons and actions

// Reset month name callback
static void on_reset_month_name(GtkButton* button, gpointer user_data) {
    GtkWidget* entry = GTK_WIDGET(user_data);
    if (!entry) return;
    
    // Clear the entry text
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

// Reset moon name callback
static void on_reset_moon_name(GtkButton* button, gpointer user_data) {
    GtkWidget* entry = GTK_WIDGET(user_data);
    if (!entry) return;
    
    // Clear the entry text
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

// Reset weekday name callback
static void on_reset_weekday_name(GtkButton* button, gpointer user_data) {
    GtkWidget* entry = GTK_WIDGET(user_data);
    if (!entry) return;
    
    // Clear the entry text
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

// Browse for events file
static void on_browse_events_file(GtkButton* button, gpointer user_data) {
    GtkWidget* entry = GTK_WIDGET(user_data);
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Events File",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Add filter for JSON files
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "JSON Files");
    gtk_file_filter_add_pattern(filter, "*.json");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(entry), filename);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

// Browse for cache directory
static void on_browse_cache_dir(GtkButton* button, gpointer user_data) {
    GtkWidget* entry = GTK_WIDGET(user_data);
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Cache Directory",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Select", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* dirname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(entry), dirname);
        g_free(dirname);
    }
    
    gtk_widget_destroy(dialog);
}

// Browse for log file
static void on_browse_log_file(GtkButton* button, gpointer user_data) {
    GtkWidget* entry = GTK_WIDGET(user_data);
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Log File",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Save", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Add filter for log files
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Log Files");
    gtk_file_filter_add_pattern(filter, "*.log");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Set default name if entry is empty
    const char* current_text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (current_text == NULL || strlen(current_text) == 0) {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "mani.log");
    }
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(entry), filename);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

// Clear cache button handler
static void on_clear_cache(GtkButton* button, gpointer user_data) {
    // LunarCalendarApp* app = (LunarCalendarApp*)user_data; // Unused variable
    // Implement cache clearing logic here
    g_print("Cache clearing not yet implemented.\n");
}

// Reset all settings button handler
static void on_reset_all_settings(GtkButton* button, gpointer user_data) {
    // LunarCalendarApp* app = (LunarCalendarApp*)user_data; // Unused variable
    // Implement reset logic here
    g_print("Reset all settings not yet implemented.\n");
}

/**
 * Apply settings from the dialog to the app->config structure
 * This function reads values from all settings widgets and updates the app config
 */
static void apply_settings(LunarCalendarApp* app) {
    if (!app || !app->config || !app->window) return;
    g_print("--- apply_settings called ---\n"); // Debug start

    // Retrieve widget pointers (using g_object_get_data)
    GtkWidget* theme_combo = g_object_get_data(G_OBJECT(app->window), "theme_combo");
    GtkWidget* primary_color_button = g_object_get_data(G_OBJECT(app->window), "primary_color_button");
    GtkWidget* secondary_color_button = g_object_get_data(G_OBJECT(app->window), "secondary_color_button");
    GtkWidget* text_color_button = g_object_get_data(G_OBJECT(app->window), "text_color_button");
    GtkWidget* font_button = g_object_get_data(G_OBJECT(app->window), "font_button");
    GtkWidget* cell_size_spin = g_object_get_data(G_OBJECT(app->window), "cell_size_spin");
    GtkWidget* show_moon_phases_check = g_object_get_data(G_OBJECT(app->window), "show_moon_phases_check");
    GtkWidget* highlight_special_days_check = g_object_get_data(G_OBJECT(app->window), "highlight_special_days_check");
    GtkWidget* show_gregorian_dates_check = g_object_get_data(G_OBJECT(app->window), "show_gregorian_dates_check");
    GtkWidget* show_weekday_names_check = g_object_get_data(G_OBJECT(app->window), "show_weekday_names_check");
    GtkWidget* show_metonic_cycle_check = g_object_get_data(G_OBJECT(app->window), "show_metonic_cycle_check");
    GtkWidget* show_event_indicators_check = g_object_get_data(G_OBJECT(app->window), "show_event_indicators_check");
    GtkWidget* week_start_day_combo = g_object_get_data(G_OBJECT(app->window), "week_start_day_combo");
    GtkWidget* events_file_path_entry = g_object_get_data(G_OBJECT(app->window), "events_file_path_entry");
    GtkWidget* cache_dir_entry = g_object_get_data(G_OBJECT(app->window), "cache_dir_entry");
    GtkWidget* log_file_path_entry = g_object_get_data(G_OBJECT(app->window), "log_file_path_entry");
    GtkWidget* debug_logging_check = g_object_get_data(G_OBJECT(app->window), "debug_logging_check");

    g_print("Widget pointers retrieved (sample: theme_combo=%p)\n", theme_combo); // Debug retrieval

    // Apply settings from Appearance tab (if widgets exist)
    if (theme_combo) {
        app->config->theme_type = gtk_combo_box_get_active(GTK_COMBO_BOX(theme_combo));
        app->config->use_dark_theme = (app->config->theme_type == 1); // Assuming 1 is Dark
        // g_print("Applied theme_type: %d\n", app->config->theme_type); // DEBUG REMOVED
    } // else { g_print("WARN: theme_combo not found\n"); } // DEBUG REMOVED
    if (primary_color_button) {
         gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(primary_color_button), &app->config->primary_color);
         // g_print("Applied primary_color\n"); // DEBUG REMOVED
    } // else { g_print("WARN: primary_color_button not found\n"); } // DEBUG REMOVED
    if (secondary_color_button) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(secondary_color_button), &app->config->secondary_color);
        // g_print("Applied secondary_color\n"); // DEBUG REMOVED
     } // else { g_print("WARN: secondary_color_button not found\n"); } // DEBUG REMOVED
    if (text_color_button) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(text_color_button), &app->config->text_color);
         // g_print("Applied text_color\n"); // DEBUG REMOVED
     } // else { g_print("WARN: text_color_button not found\n"); } // DEBUG REMOVED
    if (font_button) {
        const char* font_name = gtk_font_button_get_font_name(GTK_FONT_BUTTON(font_button));
        g_free(app->config->font_name);
        app->config->font_name = g_strdup(font_name);
        // g_print("Applied font_name: %s\n", app->config->font_name ? app->config->font_name : "(null)"); // DEBUG REMOVED
    } // else { g_print("WARN: font_button not found\n"); } // DEBUG REMOVED
     if (cell_size_spin) {
        app->config->cell_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cell_size_spin));
        // g_print("Applied cell_size: %d\n", app->config->cell_size); // DEBUG REMOVED
    } // else { g_print("WARN: cell_size_spin not found\n"); } // DEBUG REMOVED

    // Apply settings from Display tab
    if (show_moon_phases_check) {
        // Use gtk_switch_get_active for GtkSwitch
        app->config->show_moon_phases = gtk_switch_get_active(GTK_SWITCH(show_moon_phases_check));
        // g_print("Applied show_moon_phases: %d\n", app->config->show_moon_phases); // DEBUG REMOVED
    } // else { g_print("WARN: show_moon_phases_check not found\n"); } // DEBUG REMOVED
    if (highlight_special_days_check) {
        // Use gtk_switch_get_active for GtkSwitch
        app->config->highlight_special_days = gtk_switch_get_active(GTK_SWITCH(highlight_special_days_check));
         // g_print("Applied highlight_special_days: %d\n", app->config->highlight_special_days); // DEBUG REMOVED
    } // else { g_print("WARN: highlight_special_days_check not found\n"); } // DEBUG REMOVED
    if (show_gregorian_dates_check) {
        // Use gtk_switch_get_active for GtkSwitch
        app->config->show_gregorian_dates = gtk_switch_get_active(GTK_SWITCH(show_gregorian_dates_check));
        // g_print("Applied show_gregorian_dates: %d\n", app->config->show_gregorian_dates); // DEBUG REMOVED
    } // else { g_print("WARN: show_gregorian_dates_check not found\n"); } // DEBUG REMOVED
    if (show_weekday_names_check) {
         // Use gtk_switch_get_active for GtkSwitch
        app->config->show_weekday_names = gtk_switch_get_active(GTK_SWITCH(show_weekday_names_check));
         // g_print("Applied show_weekday_names: %d\n", app->config->show_weekday_names); // DEBUG REMOVED
    } // else { g_print("WARN: show_weekday_names_check not found\n"); } // DEBUG REMOVED
    if (show_metonic_cycle_check) {
         // Use gtk_switch_get_active for GtkSwitch
        app->config->show_metonic_cycle = gtk_switch_get_active(GTK_SWITCH(show_metonic_cycle_check));
        // g_print("Applied show_metonic_cycle: %d\n", app->config->show_metonic_cycle); // DEBUG REMOVED
    } // else { g_print("WARN: show_metonic_cycle_check not found\n"); } // DEBUG REMOVED
    if (show_event_indicators_check) {
        // Use gtk_switch_get_active for GtkSwitch
        app->config->show_event_indicators = gtk_switch_get_active(GTK_SWITCH(show_event_indicators_check));
        // g_print("Applied show_event_indicators: %d\n", app->config->show_event_indicators); // DEBUG REMOVED
    } // else { g_print("WARN: show_event_indicators_check not found\n"); } // DEBUG REMOVED
     if (week_start_day_combo) {
        app->config->week_start_day = gtk_combo_box_get_active(GTK_COMBO_BOX(week_start_day_combo));
         // g_print("Applied week_start_day: %d\n", app->config->week_start_day); // DEBUG REMOVED
    } // else { g_print("WARN: week_start_day_combo not found\n"); } // DEBUG REMOVED

    // Apply settings from Names tabs
    // g_print("Applying names settings...\n"); // DEBUG REMOVED
    for (int i = 0; i < 13; i++) {
        char key[20];
        snprintf(key, sizeof(key), "month_entry_%d", i);
        GtkWidget* entry = g_object_get_data(G_OBJECT(app->window), key);
        if (entry) {
            const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
            g_free(app->config->custom_month_names[i]);
            app->config->custom_month_names[i] = g_strdup(text);
            // g_print("  Month %d: %s\n", i, app->config->custom_month_names[i] ? app->config->custom_month_names[i] : "(null/empty)");
        } else {
            // g_print("  WARN: Month entry %d not found\n", i);
            // If entry not found, maybe we should preserve the existing value?
        }
    }
    for (int i = 0; i < 7; i++) {
         char key[20];
        snprintf(key, sizeof(key), "weekday_entry_%d", i);
        GtkWidget* entry = g_object_get_data(G_OBJECT(app->window), key);
        if (entry) {
            const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
            g_free(app->config->custom_weekday_names[i]);
            app->config->custom_weekday_names[i] = g_strdup(text);
            // g_print("  Weekday %d: %s\n", i, app->config->custom_weekday_names[i] ? app->config->custom_weekday_names[i] : "(null/empty)");
        } // else { g_print("  WARN: Weekday entry %d not found\n", i); }
    }
     for (int i = 0; i < 12; i++) {
        char key[20];
        snprintf(key, sizeof(key), "moon_entry_%d", i);
        GtkWidget* entry = g_object_get_data(G_OBJECT(app->window), key);
         if (entry) {
            const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
            g_free(app->config->custom_full_moon_names[i]);
            app->config->custom_full_moon_names[i] = g_strdup(text);
            // g_print("  Moon %d: %s\n", i, app->config->custom_full_moon_names[i] ? app->config->custom_full_moon_names[i] : "(null/empty)");
        } // else { g_print("  WARN: Moon entry %d not found\n", i); }
    }

    // Apply settings from Advanced tab
     // g_print("Applying advanced settings...\n"); // DEBUG REMOVED
    if (events_file_path_entry) {
        const char* text = gtk_entry_get_text(GTK_ENTRY(events_file_path_entry));
        g_free(app->config->events_file_path);
        app->config->events_file_path = g_strdup(text);
        // Update the app's path as well for immediate effect if needed?
        // Be careful: Modifying app->events_file_path here might be unexpected.
        // Let's only update the config path for now.
        // g_free(app->events_file_path);
        // app->events_file_path = g_strdup(text);
         // g_print("Applied events_file_path: %s\n", app->config->events_file_path ? app->config->events_file_path : "(null)"); // DEBUG REMOVED
    } else { g_print("WARN: events_file_path_entry not found\n"); } // DEBUG REMOVED
    if (cache_dir_entry) {
        const char* text = gtk_entry_get_text(GTK_ENTRY(cache_dir_entry));
        g_free(app->config->cache_dir);
        app->config->cache_dir = g_strdup(text);
        // g_print("Applied cache_dir: %s\n", app->config->cache_dir ? app->config->cache_dir : "(null)"); // DEBUG REMOVED
    } else { g_print("WARN: cache_dir_entry not found\n"); } // DEBUG REMOVED
    if (log_file_path_entry) {
        const char* text = gtk_entry_get_text(GTK_ENTRY(log_file_path_entry));
        g_free(app->config->log_file_path);
        app->config->log_file_path = g_strdup(text);
         // g_print("Applied log_file_path: %s\n", app->config->log_file_path ? app->config->log_file_path : "(null)"); // DEBUG REMOVED
    } else { g_print("WARN: log_file_path_entry not found\n"); } // DEBUG REMOVED
    if (debug_logging_check) {
        // Use gtk_switch_get_active for GtkSwitch
        app->config->debug_logging = gtk_switch_get_active(GTK_SWITCH(debug_logging_check));
         // g_print("Applied debug_logging: %d\n", app->config->debug_logging); // DEBUG REMOVED
    } else { g_print("WARN: debug_logging_check not found\n"); } // DEBUG REMOVED

    // Indicate that settings have been applied
    // g_print("Configuration update attempted from settings dialog.\n"); // DEBUG REMOVED
    g_print("--- apply_settings finished ---\n"); // Debug end

    // Remove call to local update function - this should be handled externally
    // update_ui_from_config(app);
}

/**
 * Handle import settings button click.
 * Opens a file chooser dialog and imports settings from the selected file.
 */
static void on_import_clicked(GtkButton* button, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Import Configuration",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Import", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Add filter for config files
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Configuration Files");
    gtk_file_filter_add_pattern(filter, "*.conf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        // Import logic would go here
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

/**
 * Handle export settings button click.
 * Opens a file chooser dialog and exports settings to the selected file.
 */
static void on_export_clicked(GtkButton* button, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Export Settings",
                                                  GTK_WINDOW(app->window),
                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                  "_Cancel", GTK_RESPONSE_CANCEL,
                                                  "_Save", GTK_RESPONSE_ACCEPT,
                                                  NULL);
    
    // Suggest a filename
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "mani_settings.ini");
    
    // Add filters
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.ini");
    gtk_file_filter_set_name(filter, "Configuration Files (*.ini)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Set overwrite confirmation
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // Make sure the filename ends with .ini
        if (!g_str_has_suffix(filename, ".ini")) {
            char* new_filename = g_strconcat(filename, ".ini", NULL);
            g_free(filename);
            filename = new_filename;
        }
        
        // Save the config to the selected file
        if (config_save(filename, app->config)) {
            // Show a success message
            GtkWidget* info = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO,
                                                  GTK_BUTTONS_OK,
                                                  "Settings exported successfully");
            gtk_dialog_run(GTK_DIALOG(info));
            gtk_widget_destroy(info);
        } else {
            // Show an error message
            GtkWidget* error = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Failed to export settings");
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error),
                                                   "The settings could not be saved to the selected file.");
            gtk_dialog_run(GTK_DIALOG(error));
            gtk_widget_destroy(error);
        }
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

/**
 * Handle help button click.
 * Shows context-sensitive help based on the currently selected tab.
 */
static void on_help_clicked(GtkButton* button, gpointer user_data) {
    GtkWidget* notebook = GTK_WIDGET(user_data);
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    
    // Define help text for each tab
    const char* help_titles[] = {
        "Appearance Settings Help",
        "Display Settings Help",
        "Month Names Settings Help",
        "Advanced Settings Help"
    };
    
    const char* help_texts[] = {
        "The Appearance tab allows you to customize the look and feel of the application.\n\n"
        "• Theme: Choose between light, dark, or system default theme.\n"
        "• Primary Color: The main color used for UI elements.\n"
        "• Secondary Color: Used for highlights and accents.\n"
        "• Text Color: Used for all text elements.\n"
        "• Font: Select the font used throughout the application.\n"
        "• Calendar Cell Size: Adjust the size of calendar day cells.",
        
        "The Display tab controls what information is shown in the calendar view.\n\n"
        "• Week Start Day: Choose which day of the week appears first.\n"
        "• Show Gregorian Dates: Display standard calendar dates alongside lunar dates.\n"
        "• Show Moon Phases: Display moon phase icons in the calendar.\n"
        "• Highlight Special Days: Mark important Germanic lunar calendar dates.\n"
        "• Show Event Indicators: Display markers for days with events.",
        
        "The Month Names tab allows you to customize the names of lunar months and weekdays.\n\n"
        "• Custom Month Names: Enter custom names for the 12 standard lunar months and the intercalary (13th) month.\n"
        "• Custom Weekday Names: Enter custom names for the 7 days of the week.\n\n"
        "Leave any field blank to use the default name.",
        
        "The Advanced tab provides additional configuration options.\n\n"
        "• Events File Location: Specify where event data is stored.\n"
        "• Cache Directory: Location for cached lunar calculations.\n"
        "• Enable Debug Logging: Turn on detailed logging for troubleshooting.\n"
        "• Log File Location: Where log files are stored.\n"
        "• Show Metonic Cycle Tracker: Display the current position in the 19-year metonic cycle.\n"
        "• Clear Cache: Remove cached calculations (may require recalculation).\n"
        "• Reset All Settings: Restore all settings to default values."
    };
    
    // Create help dialog with the appropriate text
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "%s", help_titles[current_page]);
    
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                           "%s", help_texts[current_page]);
    
    gtk_window_set_title(GTK_WINDOW(dialog), "MANI Help");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
} 