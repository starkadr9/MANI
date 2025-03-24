CC=gcc
CFLAGS=-Wall -Wextra -g -std=c99 -I./include
LDFLAGS=-lm

# GTK flags
GTK_CFLAGS=$(shell pkg-config --cflags gtk+-3.0)
GTK_LDFLAGS=$(shell pkg-config --libs gtk+-3.0)

SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
GUI_SRC_DIR=$(SRC_DIR)/gui

SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TARGET=$(BIN_DIR)/lunar_calendar

# GUI application
GUI_SRCS=$(wildcard $(GUI_SRC_DIR)/*.c)
GUI_OBJS=$(patsubst $(GUI_SRC_DIR)/%.c,$(OBJ_DIR)/gui/%.o,$(GUI_SRCS))
CALENDAR_OBJS=$(filter-out $(OBJ_DIR)/main.o, $(OBJS))
GUI_TARGET=$(BIN_DIR)/lunar_calendar_gui

.PHONY: all gui clean

all: directories $(TARGET)

gui: directories $(GUI_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(GUI_TARGET): $(GUI_OBJS) $(CALENDAR_OBJS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $^ $(LDFLAGS) $(GTK_LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/gui/%.o: $(GUI_SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)/gui
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: all
	$(TARGET)

run-gui: gui
	$(GUI_TARGET) 