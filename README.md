# Lunar Calendar - Metonic Cycle Implementation

This is a C implementation of a lunar calendar based on the Metonic cycle, which is a 19-year period after which the phases of the moon recur on the same day of the year.

## Features

* Conversion between Gregorian and lunar dates
* Calculation of the Germanic Eld year (starting from 750 BC as the epoch)
* Moon phase determination for any given date
* Complete 19-year Metonic cycle calculation
* Command-line interface for testing the calculations

## Data Structures

The implementation provides the following key data structures:

* `LunarDay` - Represents a single day with both lunar and Gregorian date information
* `LunarMonth` - Represents a lunar month (29-30 days)
* `LunarYear` - Represents a lunar year (collection of lunar months)
* `MetonicCycle` - Represents the complete 19-year Metonic cycle

## Building the Project

Requirements:
* C compiler (GCC recommended)
* Make

To build the project:

```
make
```

This will create the executable in the `bin` directory.

## Usage

Run the program:

```
./bin/lunar_calendar
```

### Available Commands

* `today` - Display lunar date for today
* `g2l YYYY MM DD` - Convert Gregorian date to lunar date
* `l2g YYYY MM DD` - Convert lunar date to Gregorian date
* `phase YYYY MM DD` - Show moon phase for Gregorian date
* `eld YYYY` - Calculate Germanic Eld year for Gregorian year
* `cycle YYYY` - Display Metonic cycle starting from year YYYY
* `help` - Display help information
* `quit` - Exit the program

### Examples

Convert Gregorian date to lunar date:
```
> g2l 2025 1 1
```

Check the moon phase for a specific date:
```
> phase 2025 12 25
```

Calculate the Eld year:
```
> eld 2025
```

## Implementation Notes

* The lunar calendar calculations are based on approximations using the Metonic cycle.
* The moon phase calculation uses a simple mathematical model based on known new moon dates.
* The Germanic Eld year is calculated as the Gregorian year + 750 (using 750 BC as the epoch). 