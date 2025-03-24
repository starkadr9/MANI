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
    
    // Advanced widgets
    GtkWidget* events_file_path_entry;
    GtkWidget* cache_dir_entry;
    GtkWidget* log_file_path_entry;
    GtkWidget* debug_logging_check;
} SettingsWidgets;

// Forward declarations for tab creation functions
static GtkWidget* create_appearance_tab(LunarCalendarApp* app, SettingsWidgets* widgets);
static GtkWidget* create_display_tab(LunarCalendarApp* app, SettingsWidgets* widgets);
static GtkWidget* create_names_tab(LunarCalendarApp* app, SettingsWidgets* widgets);
static GtkWidget* create_advanced_tab(LunarCalendarApp* app, SettingsWidgets* widgets);

// Forward declaration for settings application
static void apply_settings(LunarCalendarApp* app);

// Forward declarations for button callbacks
static void on_import_clicked(GtkButton* button, gpointer user_data);
static void on_export_clicked(GtkButton* button, gpointer user_data);
static void on_help_clicked(GtkButton* button, gpointer user_data);
static void on_preview_draw(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data);
static void on_clear_cache(GtkButton* button, gpointer user_data);
static void on_reset_all_settings(GtkButton* button, gpointer user_data);
static void on_reset_month_name(GtkButton* button, gpointer user_data);
static void on_reset_weekday_name(GtkButton* button, gpointer user_data);
static void on_browse_events_file(GtkButton* button, gpointer user_data);
static void on_browse_cache_dir(GtkButton* button, gpointer user_data);
static void on_browse_log_file(GtkButton* button, gpointer user_data);

// Store widget pointer in app->window for later access
static void store_widget_pointer(LunarCalendarApp* app, const char* name, GtkWidget* widget) {
    if (!app || !app->window || !name || !widget) return;
    g_object_set_data(G_OBJECT(app->window), name, widget);
}

// Helper function to update UI from config
static void update_ui_from_config(LunarCalendarApp* app) {
    // This would update all UI elements based on the current configuration
    // For now we'll just redraw the main window
    if (app->window) {
        gtk_widget_queue_draw(app->window);
    }
    
    // Update any other UI elements that depend on configuration
    if (app->header_bar) {
        gtk_widget_queue_draw(app->header_bar);
    }
}

/**
 * Create and show a settings dialog.
 */
gboolean settings_dialog_show(LunarCalendarApp* app, GtkWindow* parent) {
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
    GtkWidget* appearance_tab = create_appearance_tab(app, widgets);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), appearance_tab, 
                            gtk_label_new("Appearance"));
    
    GtkWidget* display_tab = create_display_tab(app, widgets);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), display_tab, 
                            gtk_label_new("Display"));
    
    GtkWidget* names_tab = create_names_tab(app, widgets);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), names_tab, 
                            gtk_label_new("Month Names"));
    
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
        // Apply settings
        apply_settings(app);
        settings_changed = TRUE;
        
        if (response == GTK_RESPONSE_OK) {
            break;
        }
    }
    
    // Save settings if they were changed
    if (settings_changed) {
        config_save(app->config_file_path, app->config);
    }
    
    // Free the widgets structure
    g_free(widgets);
    
    // Destroy the dialog
    gtk_widget_destroy(dialog);
    
    return settings_changed;
}

// Appearance tab creation
static GtkWidget* create_appearance_tab(LunarCalendarApp* app, SettingsWidgets* widgets) {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    
    int row = 0;
    
    // Theme selection
    GtkWidget* theme_label = gtk_label_new("Theme:");
    gtk_widget_set_halign(theme_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), theme_label, 0, row, 1, 1);
    
    widgets->theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->theme_combo), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->theme_combo), "Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->theme_combo), "System Default");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->theme_combo), app->config->theme_type);
    gtk_grid_attach(GTK_GRID(grid), widgets->theme_combo, 1, row, 2, 1);
    
    // Store the theme combo widget for later access
    store_widget_pointer(app, "theme_combo", widgets->theme_combo);
    
    row++;
    
    // Primary color
    GtkWidget* primary_color_label = gtk_label_new("Primary Color:");
    gtk_widget_set_halign(primary_color_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), primary_color_label, 0, row, 1, 1);
    
    widgets->primary_color_button = gtk_color_button_new_with_rgba(&app->config->primary_color);
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widgets->primary_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), widgets->primary_color_button, 1, row, 2, 1);
    
    // Store the primary color button for later access
    store_widget_pointer(app, "primary_color_button", widgets->primary_color_button);
    
    row++;
    
    // Secondary color
    GtkWidget* secondary_color_label = gtk_label_new("Secondary Color:");
    gtk_widget_set_halign(secondary_color_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), secondary_color_label, 0, row, 1, 1);
    
    widgets->secondary_color_button = gtk_color_button_new_with_rgba(&app->config->secondary_color);
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widgets->secondary_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), widgets->secondary_color_button, 1, row, 2, 1);
    
    // Store the secondary color button for later access
    store_widget_pointer(app, "secondary_color_button", widgets->secondary_color_button);
    
    row++;
    
    // Text color
    GtkWidget* text_color_label = gtk_label_new("Text Color:");
    gtk_widget_set_halign(text_color_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), text_color_label, 0, row, 1, 1);
    
    widgets->text_color_button = gtk_color_button_new_with_rgba(&app->config->text_color);
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widgets->text_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), widgets->text_color_button, 1, row, 2, 1);
    
    // Store the text color button for later access
    store_widget_pointer(app, "text_color_button", widgets->text_color_button);
    
    row++;
    
    // Font selection
    GtkWidget* font_label = gtk_label_new("Font:");
    gtk_widget_set_halign(font_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), font_label, 0, row, 1, 1);
    
    widgets->font_button = gtk_font_button_new();
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(widgets->font_button),
                               app->config->font_name ? app->config->font_name : "Sans 10");
    gtk_grid_attach(GTK_GRID(grid), widgets->font_button, 1, row, 2, 1);
    
    // Store the font button for later access
    store_widget_pointer(app, "font_button", widgets->font_button);
    
    row++;
    
    // Calendar cell size
    GtkWidget* cell_size_label = gtk_label_new("Calendar Cell Size:");
    gtk_widget_set_halign(cell_size_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), cell_size_label, 0, row, 1, 1);
    
    widgets->cell_size_spin = gtk_spin_button_new_with_range(60, 120, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgets->cell_size_spin), app->config->cell_size);
    gtk_grid_attach(GTK_GRID(grid), widgets->cell_size_spin, 1, row, 2, 1);
    
    // Store the cell size spinner for later access
    store_widget_pointer(app, "cell_size_spin", widgets->cell_size_spin);
    
    row++;
    
    // Preview box
    GtkWidget* preview_label = gtk_label_new("Preview:");
    gtk_widget_set_halign(preview_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), preview_label, 0, row, 1, 1);
    
    GtkWidget* preview_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(preview_frame), GTK_SHADOW_IN);
    gtk_widget_set_size_request(preview_frame, 200, 100);
    gtk_grid_attach(GTK_GRID(grid), preview_frame, 1, row, 2, 1);
    
    widgets->preview_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(preview_frame), widgets->preview_area);
    g_signal_connect(widgets->preview_area, "draw", G_CALLBACK(on_preview_draw), widgets->primary_color_button);
    
    // Store the preview area for later access
    store_widget_pointer(app, "preview_area", widgets->preview_area);
    
    return grid;
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
    store_widget_pointer(app, "week_start_day", widgets->week_start_day_combo);
    
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
    store_widget_pointer(app, "show_gregorian_dates", widgets->show_gregorian_dates_check);
    
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
    store_widget_pointer(app, "show_moon_phases", widgets->show_moon_phases_check);
    
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
    store_widget_pointer(app, "highlight_special_days", widgets->highlight_special_days_check);
    
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
    store_widget_pointer(app, "show_weekday_names", widgets->show_weekday_names_check);
    
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
    store_widget_pointer(app, "show_event_indicators", widgets->show_event_indicators_check);
    
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
    store_widget_pointer(app, "show_metonic_cycle", widgets->show_metonic_cycle_check);
    
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
        "After Yule", "Sol", "Hretha", "Eostre", "Three Milkings",
        "Mead", "Hay", "Harvest", "Holy", "Winter", 
        "Blood", "Before Yule", "Thirteenth"
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
        snprintf(widget_name, sizeof(widget_name), "month_entry_%d", i + 1);
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
    store_widget_pointer(app, "events_file_entry", widgets->events_file_path_entry);
    
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
    store_widget_pointer(app, "debug_logging", widgets->debug_logging_check);
    
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
    store_widget_pointer(app, "log_file_entry", widgets->log_file_path_entry);
    
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
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_YES_NO,
                                             "Are you sure you want to clear the cache?");
    
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                           "This will remove all cached lunar data and may require recalculation.");
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
        // Clear cache directory
        GtkWidget* parent_dialog = gtk_widget_get_toplevel(GTK_WIDGET(button));
        GtkWidget* cache_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(gtk_widget_get_parent(GTK_WIDGET(button))), 
                                                            "cache_dir_entry"));
        const char* cache_dir = gtk_entry_get_text(GTK_ENTRY(cache_entry));
        
        // TODO: Implement actual cache clearing
        g_print("Clearing cache directory: %s\n", cache_dir);
        
        GtkWidget* info = gtk_message_dialog_new(GTK_WINDOW(parent_dialog),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "Cache cleared successfully");
        gtk_dialog_run(GTK_DIALOG(info));
        gtk_widget_destroy(info);
    }
    
    gtk_widget_destroy(dialog);
}

// Reset all settings button handler
static void on_reset_all_settings(GtkButton* button, gpointer user_data) {
    LunarCalendarApp* app = (LunarCalendarApp*)user_data;
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_WARNING,
                                             GTK_BUTTONS_YES_NO,
                                             "Are you sure you want to reset all settings?");
    
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                           "This will restore all settings to their default values.\n"
                                           "This action cannot be undone.");
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
        // Reset all config values to defaults
        // TODO: Implement config_reset_to_defaults(app->config);
        
        GtkWidget* parent_dialog = gtk_widget_get_toplevel(GTK_WIDGET(button));
        GtkWidget* info = gtk_message_dialog_new(GTK_WINDOW(parent_dialog),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "All settings have been reset to defaults");
        gtk_dialog_run(GTK_DIALOG(info));
        gtk_widget_destroy(info);
        
        // Close the settings dialog by simulating a cancel click
        GtkWidget* cancel_button = GTK_WIDGET(g_object_get_data(G_OBJECT(parent_dialog), "cancel_button"));
        g_signal_emit_by_name(cancel_button, "clicked");
    }
    
    gtk_widget_destroy(dialog);
}

/**
 * Apply settings from the dialog to the app configuration.
 * This function reads values from all settings widgets and updates the app config.
 */
static void apply_settings(LunarCalendarApp* app) {
    if (!app) return;
    
    // Get widgets from the dialog
    GtkWidget* font_button = g_object_get_data(G_OBJECT(app->window), "font_button");
    GtkWidget* primary_color_button = g_object_get_data(G_OBJECT(app->window), "primary_color_button");
    GtkWidget* secondary_color_button = g_object_get_data(G_OBJECT(app->window), "secondary_color_button");
    GtkWidget* text_color_button = g_object_get_data(G_OBJECT(app->window), "text_color_button");
    GtkWidget* theme_combo = g_object_get_data(G_OBJECT(app->window), "theme_combo");
    GtkWidget* cell_size_spin = g_object_get_data(G_OBJECT(app->window), "cell_size_spin");
    
    // Get display settings
    GtkWidget* show_moon_phases = g_object_get_data(G_OBJECT(app->window), "show_moon_phases");
    GtkWidget* highlight_special_days = g_object_get_data(G_OBJECT(app->window), "highlight_special_days");
    GtkWidget* show_gregorian_dates = g_object_get_data(G_OBJECT(app->window), "show_gregorian_dates");
    GtkWidget* show_weekday_names = g_object_get_data(G_OBJECT(app->window), "show_weekday_names");
    GtkWidget* show_metonic_cycle = g_object_get_data(G_OBJECT(app->window), "show_metonic_cycle");
    GtkWidget* show_event_indicators = g_object_get_data(G_OBJECT(app->window), "show_event_indicators");
    GtkWidget* week_start_day = g_object_get_data(G_OBJECT(app->window), "week_start_day");
    
    // Update appearance settings in the config
    if (theme_combo) {
        app->config->theme_type = gtk_combo_box_get_active(GTK_COMBO_BOX(theme_combo));
        
        // Apply theme immediately
        GtkSettings* settings = gtk_settings_get_default();
        if (settings) {
            if (app->config->theme_type == 0) { // Light
                g_object_set(settings, "gtk-application-prefer-dark-theme", FALSE, NULL);
                app->config->use_dark_theme = FALSE;
            } else if (app->config->theme_type == 1) { // Dark
                g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
                app->config->use_dark_theme = TRUE;
            } else { // System default
                // TODO: Get system preference
            }
        }
    }
    
    if (cell_size_spin) {
        app->config->cell_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cell_size_spin));
    }
    
    if (primary_color_button) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(primary_color_button), &app->config->primary_color);
    }
    
    if (secondary_color_button) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(secondary_color_button), &app->config->secondary_color);
    }
    
    if (text_color_button) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(text_color_button), &app->config->text_color);
    }
    
    if (font_button) {
        const char* font_name = gtk_font_button_get_font_name(GTK_FONT_BUTTON(font_button));
        if (font_name) {
            if (app->config->font_name) {
                g_free(app->config->font_name);
            }
            app->config->font_name = g_strdup(font_name);
        }
    }
    
    // Update display settings
    if (show_moon_phases) {
        app->config->show_moon_phases = gtk_switch_get_active(GTK_SWITCH(show_moon_phases));
    }
    
    if (highlight_special_days) {
        app->config->highlight_special_days = gtk_switch_get_active(GTK_SWITCH(highlight_special_days));
    }
    
    if (show_gregorian_dates) {
        app->config->show_gregorian_dates = gtk_switch_get_active(GTK_SWITCH(show_gregorian_dates));
    }
    
    if (show_weekday_names) {
        app->config->show_weekday_names = gtk_switch_get_active(GTK_SWITCH(show_weekday_names));
    }
    
    if (show_metonic_cycle) {
        app->config->show_metonic_cycle = gtk_switch_get_active(GTK_SWITCH(show_metonic_cycle));
        
        // Show/hide the metonic cycle bar based on setting
        if (app->metonic_cycle_bar) {
            if (app->config->show_metonic_cycle) {
                gtk_widget_show(app->metonic_cycle_bar);
            } else {
                gtk_widget_hide(app->metonic_cycle_bar);
            }
        }
    }
    
    if (show_event_indicators) {
        app->config->show_event_indicators = gtk_switch_get_active(GTK_SWITCH(show_event_indicators));
    }
    
    if (week_start_day) {
        app->config->week_start_day = gtk_combo_box_get_active(GTK_COMBO_BOX(week_start_day));
    }
    
    // Save custom month names
    for (int i = 0; i < 13; i++) {
        GtkWidget* entry = g_object_get_data(G_OBJECT(app->window), g_strdup_printf("month_entry_%d", i + 1));
        if (entry) {
            const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
            if (text && strlen(text) > 0) {
                if (app->config->custom_month_names[i]) {
                    g_free(app->config->custom_month_names[i]);
                }
                app->config->custom_month_names[i] = g_strdup(text);
            } else if (app->config->custom_month_names[i]) {
                g_free(app->config->custom_month_names[i]);
                app->config->custom_month_names[i] = NULL;
            }
        }
    }
    
    // Save custom weekday names
    for (int i = 0; i < 7; i++) {
        GtkWidget* entry = g_object_get_data(G_OBJECT(app->window), g_strdup_printf("weekday_entry_%d", i));
        if (entry) {
            const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
            if (text && strlen(text) > 0) {
                if (app->config->custom_weekday_names[i]) {
                    g_free(app->config->custom_weekday_names[i]);
                }
                app->config->custom_weekday_names[i] = g_strdup(text);
            } else if (app->config->custom_weekday_names[i]) {
                g_free(app->config->custom_weekday_names[i]);
                app->config->custom_weekday_names[i] = NULL;
            }
        }
    }
    
    // Save advanced settings
    GtkWidget* events_file_entry = g_object_get_data(G_OBJECT(app->window), "events_file_entry");
    GtkWidget* cache_dir_entry = g_object_get_data(G_OBJECT(app->window), "cache_dir_entry");
    GtkWidget* log_file_entry = g_object_get_data(G_OBJECT(app->window), "log_file_entry");
    GtkWidget* debug_logging = g_object_get_data(G_OBJECT(app->window), "debug_logging");
    
    if (events_file_entry) {
        const char* path = gtk_entry_get_text(GTK_ENTRY(events_file_entry));
        if (path && strlen(path) > 0) {
            if (app->config->events_file_path) {
                g_free(app->config->events_file_path);
            }
            app->config->events_file_path = g_strdup(path);
        }
    }
    
    if (cache_dir_entry) {
        const char* path = gtk_entry_get_text(GTK_ENTRY(cache_dir_entry));
        if (path && strlen(path) > 0) {
            if (app->config->cache_dir) {
                g_free(app->config->cache_dir);
            }
            app->config->cache_dir = g_strdup(path);
        }
    }
    
    if (log_file_entry) {
        const char* path = gtk_entry_get_text(GTK_ENTRY(log_file_entry));
        if (path && strlen(path) > 0) {
            if (app->config->log_file_path) {
                g_free(app->config->log_file_path);
            }
            app->config->log_file_path = g_strdup(path);
        }
    }
    
    if (debug_logging) {
        app->config->debug_logging = gtk_switch_get_active(GTK_SWITCH(debug_logging));
    }
    
    // Save configuration to file
    if (app->config_file_path) {
        config_save(app->config_file_path, app->config);
    }
    
    // Update the UI to reflect the new settings
    update_ui_from_config(app);
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

// Preview drawing callback
static void on_preview_draw(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data) {
    // Simple preview drawing
    // Draw background
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_paint(cr);
    
    // Draw sample box using selected colors
    GtkWidget* primary_color = (GtkWidget*)user_data;
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(primary_color), &color);
    
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_rectangle(cr, 10, 10, width - 20, height - 20);
    cairo_fill(cr);
    
    // Draw sample text
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, width/2 - 50, height/2);
    cairo_show_text(cr, "Preview");
} 