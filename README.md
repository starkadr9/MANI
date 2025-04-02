# MANI - METONIC ALGORITHMIC NIGHTCYCLE INTERFACE 

A GTK-based desktop application displaying a lunisolar calendar based on reconstructed Germanic traditions, calculated using the Metonic cycle and astronomical events.

This application provides a visual calendar interface showing traditional lunar months alongside Gregorian dates, moon phases, and significant astronomical markers like solstices and equinoxes.

## Features

- Displays a lunisolar calendar aligned with astronomical events (Winter Solstice, New/Full Moons).
- Calculates and displays lunar dates corresponding to Gregorian dates.
- Shows moon phases for each day.
- Indicates special days: New Moons, Full Moons (month boundaries), Solstices, Equinoxes.
- Calculates Eld Year based on Germanic epoch.
- Displays the current year's position within the 19-year Metonic cycle.
- Allows navigation through months and years.
- Customizable month/weekday names and display options via Settings dialog.
- Loads and saves user preferences.

## Building

### Prerequisites

- GCC or a compatible C compiler
- Make
- GTK 3.0 development libraries (`libgtk-3-dev` or `gtk3-devel`)
- JSON-GLib development libraries (`libjson-glib-dev` or `json-glib-devel`)

**On Debian/Ubuntu:**
```bash
sudo apt-get install build-essential libgtk-3-dev libjson-glib-dev
```

**On Fedora/RHEL:**
```bash
sudo dnf install gcc make gtk3-devel json-glib-devel
```

### Compilation (Linux)

To build the GUI application on Linux:
```bash
make gui
```
This will create the executable at `./bin/lunar_calendar_gui`.

(Note: The `make core` target builds a separate command-line version, `bin/lunar_calendar`, which is less feature-rich than the GUI.)

### Compilation (Windows - Hypothetical)

Building a native Windows executable (`.exe`) requires a cross-compilation setup or building directly on Windows with the right tools. The general steps involve:

1.  **Install MinGW-w64:** Provides the GCC compiler and tools for Windows.
2.  **Install MSYS2 (Recommended):** A build environment providing a Unix-like shell and package management on Windows. Use its package manager (`pacman`) to install MinGW-w64 toolchains and GTK3/JSON-GLib libraries for Windows:
    ```bash
    # Within MSYS2 terminal
    pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-gtk3 mingw-w64-x86_64-json-glib
    ```
3.  **Modify Makefile:** Adjust the `Makefile` to use the MinGW-w64 compiler (`x86_64-w64-mingw32-gcc`) and link against the Windows versions of the GTK3 and JSON-GLib libraries.
4.  **Build:** Run `make gui` within the correctly configured MSYS2/MinGW environment.
5.  **Packaging:** The resulting `.exe` will require corresponding GTK and JSON-GLib `.dll` files to be distributed alongside it.

*This process is more complex and may require specific adjustments to the Makefile and build process.* 

## Running

After building, run the GUI application from the project's root directory:
```bash
./bin/lunar_calendar_gui
```

## Configuration

The application saves user preferences (like display options and custom names) to a configuration file. By default, this is typically stored in:
`~/.config/mani/config.json` 
(or a similar path depending on your system's XDG Base Directory Specification).

## License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**. See the `LICENSE` file for details.

## Acknowledgments

- Core astronomical calculations are based on algorithms presented in "Astronomical Algorithms" by Jean Meeus.
- Uses the GTK toolkit for the graphical user interface.
- Utilizes the JSON-GLib library for configuration and event handling. 