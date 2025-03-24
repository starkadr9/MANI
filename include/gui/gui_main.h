#ifndef GUI_MAIN_H
#define GUI_MAIN_H

#include <gtk/gtk.h>
#include "../lunar_calendar.h"
#include "config.h"
#include "calendar_adapter.h"

/* Function prototypes for the wireframe moon phase functions */
GdkPixbuf* create_moon_phase_icon(MoonPhase phase, int size);
GdkPixbuf* get_moon_phase_icon(LunarDay lunar_day, int size);

#endif /* GUI_MAIN_H */ 