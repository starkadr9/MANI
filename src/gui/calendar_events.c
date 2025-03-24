#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "../../include/gui/calendar_events.h"
#include <time.h>
#include <json-glib/json-glib.h>

// Default event capacity
#define DEFAULT_CAPACITY 10
#define EVENTS_FILE_VERSION 1

// Global event storage
static EventList* g_all_events = NULL;
static char* g_events_file_path = NULL;

// Helper function to compare events by date
static int compare_events_by_date(const void* a, const void* b) {
    CalendarEvent* event_a = *(CalendarEvent**)a;
    CalendarEvent* event_b = *(CalendarEvent**)b;
    
    // Compare year
    if (event_a->year != event_b->year) {
        return event_a->year - event_b->year;
    }
    
    // Compare month
    if (event_a->month != event_b->month) {
        return event_a->month - event_b->month;
    }
    
    // Compare day
    return event_a->day - event_b->day;
}

// Initialize the event system
bool events_init(const char* events_file_path) {
    // If already initialized, clean up first
    if (g_all_events != NULL) {
        events_cleanup();
    }
    
    // Create an empty event list
    g_all_events = (EventList*)malloc(sizeof(EventList));
    if (g_all_events == NULL) {
        return false;
    }
    
    g_all_events->events = NULL;
    g_all_events->count = 0;
    g_all_events->capacity = 0;
    
    // Store the events file path
    if (events_file_path != NULL) {
        g_events_file_path = strdup(events_file_path);
        
        // Try to load events from the file
        FILE* file = fopen(events_file_path, "r");
        if (file != NULL) {
            fclose(file);
            
            // Load the events from the file
            JsonParser* parser = json_parser_new();
            GError* error = NULL;
            
            if (json_parser_load_from_file(parser, events_file_path, &error)) {
                JsonNode* root = json_parser_get_root(parser);
                if (JSON_NODE_HOLDS_ARRAY(root)) {
                    JsonArray* array = json_node_get_array(root);
                    guint length = json_array_get_length(array);
                    
                    for (guint i = 0; i < length; i++) {
                        JsonObject* obj = json_array_get_object_element(array, i);
                        
                        // Extract event data
                        int year = json_object_get_int_member(obj, "year");
                        int month = json_object_get_int_member(obj, "month");
                        int day = json_object_get_int_member(obj, "day");
                        const char* title = json_object_get_string_member(obj, "title");
                        const char* description = "";
                        if (json_object_has_member(obj, "description")) {
                            description = json_object_get_string_member(obj, "description");
                        }
                        
                        // Get color if present
                        GdkRGBA color = {0.8, 0.9, 0.8, 0.3};  // Default color
                        bool has_custom_color = false;
                        
                        if (json_object_has_member(obj, "color")) {
                            JsonObject* color_obj = json_object_get_object_member(obj, "color");
                            color.red = json_object_get_double_member(color_obj, "red");
                            color.green = json_object_get_double_member(color_obj, "green");
                            color.blue = json_object_get_double_member(color_obj, "blue");
                            color.alpha = json_object_get_double_member(color_obj, "alpha");
                            has_custom_color = true;
                        }
                        
                        // Add the event
                        event_add(year, month, day, title, description, has_custom_color ? &color : NULL);
                    }
                }
            }
            
            if (error != NULL) {
                g_error_free(error);
            }
            
            g_object_unref(parser);
        }
    }
    
    return true;
}

// Clean up the event system
void events_cleanup(void) {
    if (g_all_events != NULL) {
        // Free all events
        for (int i = 0; i < g_all_events->count; i++) {
            CalendarEvent* event = g_all_events->events[i];
            free(event->title);
            free(event->description);
            free(event);
        }
        
        // Free the events array
        free(g_all_events->events);
        free(g_all_events);
        g_all_events = NULL;
    }
    
    // Free the events file path
    if (g_events_file_path != NULL) {
        free(g_events_file_path);
        g_events_file_path = NULL;
    }
}

// Add a new event
bool event_add(int year, int month, int day, const char* title, const char* description, GdkRGBA* color) {
    if (g_all_events == NULL) {
        return false;
    }
    
    // Create a new event
    CalendarEvent* event = (CalendarEvent*)malloc(sizeof(CalendarEvent));
    if (event == NULL) {
        return false;
    }
    
    // Fill in the event data
    event->year = year;
    event->month = month;
    event->day = day;
    event->title = strdup(title);
    event->description = description != NULL ? strdup(description) : strdup("");
    event->has_custom_color = color != NULL;
    
    if (color != NULL) {
        event->color = *color;
    } else {
        // Default color (transparent light green)
        event->color.red = 0.8;
        event->color.green = 0.9;
        event->color.blue = 0.8;
        event->color.alpha = 0.3;
    }
    
    // Ensure we have enough capacity
    if (g_all_events->count >= g_all_events->capacity) {
        int new_capacity = g_all_events->capacity == 0 ? 10 : g_all_events->capacity * 2;
        CalendarEvent** new_events = (CalendarEvent**)realloc(g_all_events->events, 
                                                            new_capacity * sizeof(CalendarEvent*));
        if (new_events == NULL) {
            free(event->title);
            free(event->description);
            free(event);
            return false;
        }
        
        g_all_events->events = new_events;
        g_all_events->capacity = new_capacity;
    }
    
    // Add the event to the list
    g_all_events->events[g_all_events->count++] = event;
    
    return true;
}

// Delete an event
bool event_delete(int year, int month, int day, int event_index) {
    if (g_all_events == NULL) {
        return false;
    }
    
    // Find the event in the global list
    int global_index = -1;
    int date_index = 0;
    
    for (int i = 0; i < g_all_events->count; i++) {
        CalendarEvent* event = g_all_events->events[i];
        
        if (event->year == year && event->month == month && event->day == day) {
            if (date_index == event_index) {
                global_index = i;
                break;
            }
            date_index++;
        }
    }
    
    if (global_index == -1) {
        return false;
    }
    
    // Free the event
    CalendarEvent* event = g_all_events->events[global_index];
    free(event->title);
    free(event->description);
    free(event);
    
    // Remove the event from the list by shifting remaining events
    for (int i = global_index; i < g_all_events->count - 1; i++) {
        g_all_events->events[i] = g_all_events->events[i + 1];
    }
    
    // Decrease the count
    g_all_events->count--;
    
    return true;
}

// Update an event
bool event_update(int year, int month, int day, int event_index, 
                 const char* title, const char* description, GdkRGBA* color) {
    // Get the events for this date
    EventList* events = event_get_for_date(year, month, day);
    if (events == NULL || event_index >= events->count) {
        if (events != NULL) {
            event_list_free(events);
        }
        return false;
    }
    
    // Get the event to update
    CalendarEvent* event = events->events[event_index];
    
    // Update the event
    char* new_title = strdup(title);
    char* new_description = description != NULL ? strdup(description) : strdup("");
    
    if (new_title == NULL || new_description == NULL) {
        free(new_title);
        free(new_description);
        event_list_free(events);
        return false;
    }
    
    // Free the old strings
    free(event->title);
    free(event->description);
    
    // Update the event
    event->title = new_title;
    event->description = new_description;
    
    if (color != NULL) {
        event->color = *color;
        event->has_custom_color = true;
    }
    
    // Free the event list (but not the events themselves)
    event_list_free(events);
    
    return true;
}

// Get events for a specific date
EventList* event_get_for_date(int year, int month, int day) {
    if (g_all_events == NULL) {
        return NULL;
    }
    
    // Count events for this date
    int count = 0;
    for (int i = 0; i < g_all_events->count; i++) {
        CalendarEvent* event = g_all_events->events[i];
        if (event->year == year && event->month == month && event->day == day) {
            count++;
        }
    }
    
    if (count == 0) {
        return NULL;
    }
    
    // Create a new event list
    EventList* list = (EventList*)malloc(sizeof(EventList));
    if (list == NULL) {
        return NULL;
    }
    
    list->events = (CalendarEvent**)malloc(count * sizeof(CalendarEvent*));
    if (list->events == NULL) {
        free(list);
        return NULL;
    }
    
    list->count = count;
    list->capacity = count;
    
    // Fill in the list
    int index = 0;
    for (int i = 0; i < g_all_events->count; i++) {
        CalendarEvent* event = g_all_events->events[i];
        if (event->year == year && event->month == month && event->day == day) {
            list->events[index++] = event;
        }
    }
    
    return list;
}

// Check if a date has events
bool event_date_has_events(int year, int month, int day) {
    if (g_all_events == NULL) {
        return false;
    }
    
    for (int i = 0; i < g_all_events->count; i++) {
        CalendarEvent* event = g_all_events->events[i];
        if (event->year == year && event->month == month && event->day == day) {
            return true;
        }
    }
    
    return false;
}

// Get color for date (if it has a custom color event)
bool event_get_date_color(int year, int month, int day, GdkRGBA* color) {
    if (g_all_events == NULL || color == NULL) {
        return false;
    }
    
    // Look for the first event with a custom color
    for (int i = 0; i < g_all_events->count; i++) {
        CalendarEvent* event = g_all_events->events[i];
        if (event->year == year && event->month == month && event->day == day && event->has_custom_color) {
            *color = event->color;
            return true;
        }
    }
    
    return false;
}

// Free event list (but not the events themselves)
void event_list_free(EventList* list) {
    if (list != NULL) {
        free(list->events);
        free(list);
    }
}

// Save all events to file
bool events_save(const char* filename) {
    if (g_all_events == NULL) {
        return false;
    }
    
    // Use the provided file path or the stored one
    const char* file_path = filename != NULL ? filename : g_events_file_path;
    if (file_path == NULL) {
        return false;
    }
    
    // Create a JSON array
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_array(builder);
    
    // Add each event to the array
    for (int i = 0; i < g_all_events->count; i++) {
        CalendarEvent* event = g_all_events->events[i];
        
        // Create an object for this event
        json_builder_begin_object(builder);
        
        // Add event properties
        json_builder_set_member_name(builder, "year");
        json_builder_add_int_value(builder, event->year);
        
        json_builder_set_member_name(builder, "month");
        json_builder_add_int_value(builder, event->month);
        
        json_builder_set_member_name(builder, "day");
        json_builder_add_int_value(builder, event->day);
        
        json_builder_set_member_name(builder, "title");
        json_builder_add_string_value(builder, event->title);
        
        if (event->description != NULL && strlen(event->description) > 0) {
            json_builder_set_member_name(builder, "description");
            json_builder_add_string_value(builder, event->description);
        }
        
        if (event->has_custom_color) {
            json_builder_set_member_name(builder, "color");
            json_builder_begin_object(builder);
            
            json_builder_set_member_name(builder, "red");
            json_builder_add_double_value(builder, event->color.red);
            
            json_builder_set_member_name(builder, "green");
            json_builder_add_double_value(builder, event->color.green);
            
            json_builder_set_member_name(builder, "blue");
            json_builder_add_double_value(builder, event->color.blue);
            
            json_builder_set_member_name(builder, "alpha");
            json_builder_add_double_value(builder, event->color.alpha);
            
            json_builder_end_object(builder);
        }
        
        // End this event object
        json_builder_end_object(builder);
    }
    
    // End the array
    json_builder_end_array(builder);
    
    // Get the root node
    JsonNode* root = json_builder_get_root(builder);
    
    // Create a generator to write to a file
    JsonGenerator* generator = json_generator_new();
    json_generator_set_root(generator, root);
    json_generator_set_pretty(generator, TRUE);
    
    // Write to the file
    GError* error = NULL;
    bool success = json_generator_to_file(generator, file_path, &error);
    
    if (error != NULL) {
        g_error_free(error);
    }
    
    // Clean up
    json_node_free(root);
    g_object_unref(generator);
    g_object_unref(builder);
    
    return success;
}

// Load events from file
bool events_load(const char* filename) {
    // Clean up existing events
    events_cleanup();
    events_init(filename);
    
    return true;
} 