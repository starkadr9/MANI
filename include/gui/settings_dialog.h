#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "../gui/config.h"
#include "gui_app.h"

// Function pointer type for the main UI update function
typedef void (*MainUIUpdateFunc)(LunarCalendarApp* app);

/**
 * @brief Display the settings dialog
 * 
 * Creates and shows the settings dialog, allowing the user to modify application
 * settings including appearance, display options, custom names, and advanced settings.
 * 
 * @param app Pointer to the LunarCalendarApp structure
 * @param parent The parent window for the dialog
 * @param main_update_func Pointer to the function in gui_main.c that updates the main UI.
 * @return TRUE if settings were changed and saved, FALSE otherwise
 */
gboolean settings_dialog_show(LunarCalendarApp* app, GtkWindow* parent, MainUIUpdateFunc main_update_func);

#endif /* SETTINGS_DIALOG_H */ 