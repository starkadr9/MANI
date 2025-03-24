#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <gtk/gtk.h>
#include "gui_app.h"

/**
 * @brief Display the settings dialog
 * 
 * Creates and shows the settings dialog, allowing the user to modify application
 * settings including appearance, display options, custom names, and advanced settings.
 * 
 * @param app Pointer to the LunarCalendarApp structure
 * @param parent The parent window for the dialog
 * @return TRUE if settings were changed and saved, FALSE otherwise
 */
gboolean settings_dialog_show(LunarCalendarApp *app, GtkWindow *parent);

#endif /* SETTINGS_DIALOG_H */ 