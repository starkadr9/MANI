# Lunar Calendar Application

A GTK-based lunar calendar application that displays traditional lunar calendar data alongside Gregorian dates.

## Features

- Display lunar dates alongside Gregorian calendar
- Show moon phases for each day
- Navigate through months and years
- Track special days (new moons, full moons, solstices, equinoxes)
- Display detailed information for each day
- Save and load user preferences

## Building

### Prerequisites

- GCC or compatible C compiler
- GTK 3.0 development libraries
- Make

On Debian/Ubuntu:
```bash
sudo apt-get install build-essential libgtk-3-dev
```

On Fedora/RHEL:
```bash
sudo dnf install gcc make gtk3-devel
```

### Compilation

1. To build the command-line version:
```bash
make
```

2. To build the GUI version:
```bash
make gui
```

### Running

1. Command-line version:
```bash
./bin/lunar_calendar
```

2. GUI version:
```bash
./bin/lunar_calendar_gui
```

## Configuration

The application stores its configuration in `~/.lunar_calendar/config.ini`. This file is automatically created with default values if it doesn't exist.

## Command Line Usage

The command-line version supports various commands:

- `today` - Display lunar date for today
- `g2l YYYY MM DD` - Convert Gregorian date to lunar date
- `l2g YYYY MM DD` - Convert lunar date to Gregorian date
- `phase YYYY MM DD` - Show moon phase for Gregorian date
- `render_month YYYY MM` - Render a lunar month calendar
- `seasons YYYY` - Display solstices and equinoxes for a year

## License

This project is open source software.

## Acknowledgments

- Based on astronomical algorithms from Jean Meeus
- Uses GTK for the graphical user interface 