CC = gcc
CFLAGS = -Wall -g -O2 `pkg-config --cflags gtk+-3.0 json-glib-1.0`
LDFLAGS = `pkg-config --libs gtk+-3.0 json-glib-1.0` -lm

OBJ_DIR = obj
BIN_DIR = bin

# Source files
SRCS_CORE = src/lunar_calendar.c src/lunar_renderer.c src/main.c
SRCS_GUI = src/gui/gui_main.c src/gui/calendar_adapter.c src/gui/config.c src/gui/calendar_events.c src/gui/settings_dialog.c
OBJS_CORE = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRCS_CORE))
OBJS_GUI = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRCS_GUI))

# Core objects without main.o for gui target
CORE_NO_MAIN = $(filter-out $(OBJ_DIR)/main.o, $(OBJS_CORE))

# Header files
INCLUDE_DIR = include

# Targets
all: core gui

core: $(BIN_DIR)/lunar_calendar

gui: $(BIN_DIR)/lunar_calendar_gui

# Rules
$(BIN_DIR)/lunar_calendar: $(OBJS_CORE)
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/lunar_calendar_gui: $(CORE_NO_MAIN) $(OBJS_GUI)
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) 