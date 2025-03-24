#ifndef CALENDAR_EVENTS_H
#define CALENDAR_EVENTS_H

#include <gtk/gtk.h>
#include <stdbool.h>

// Calendar event structure
typedef struct {
    int year;           // Gregorian year
    int month;          // Gregorian month
    int day;            // Gregorian day
    char* title;        // Event title
    char* description;  // Event description
    bool has_custom_color;  // Whether the event has a custom color
    GdkRGBA color;      // Custom color for the event
} CalendarEvent;

// Structure to hold a list of events
typedef struct {
    CalendarEvent** events;  // Array of event pointers
    int count;               // Number of events
    int capacity;            // Capacity of the events array
} EventList;

// Initialize the events system with a file path
bool events_init(const char* events_file_path);

// Clean up the events system
void events_cleanup(void);

// Save events to the events file
bool events_save(const char* events_file_path);

// Add an event
bool event_add(int year, int month, int day, const char* title, const char* description, GdkRGBA* color);

// Update an event
bool event_update(int year, int month, int day, int event_index, const char* title, const char* description, GdkRGBA* color);

// Delete an event
bool event_delete(int year, int month, int day, int event_index);

// Get events for a specific date
EventList* event_get_for_date(int year, int month, int day);

// Check if a date has any events
bool event_date_has_events(int year, int month, int day);

// Get the color for a date (if any)
bool event_get_date_color(int year, int month, int day, GdkRGBA* color);

// Free an event list
void event_list_free(EventList* list);

#endif /* CALENDAR_EVENTS_H */ 