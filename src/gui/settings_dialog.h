#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <gtk/gtk.h>
#include "gui_app.h"
#include "config.h"

/**
 * Create and show a settings dialog.
 * 
 * @param app The main application structure
 * @param parent The parent window
 * @return TRUE if settings were changed and saved, FALSE otherwise
 */
gboolean settings_dialog_show(LunarCalendarApp* app, GtkWindow* parent);

#endif /* SETTINGS_DIALOG_H */ 