#ifndef LUNAR_CALENDAR_H
#define LUNAR_CALENDAR_H

#include <stdbool.h>
#include <time.h>

/* Metonic cycle constants */
#define YEARS_PER_METONIC_CYCLE 19
#define LUNATIONS_PER_METONIC_CYCLE 235

/* Astronomical constants */
#define LUNAR_MONTH_AVERAGE_DAYS 29.53058868  /* Average synodic month length */
#define SOLAR_YEAR_DAYS 365.242189  /* Average solar year in days */
#define WINTER_SOLSTICE_MONTH 12
#define DEFAULT_WINTER_SOLSTICE_DAY 21

/* Moon phase enumeration */
typedef enum {
    NEW_MOON,
    WAXING_CRESCENT,
    FIRST_QUARTER,
    WAXING_GIBBOUS,
    FULL_MOON,
    WANING_GIBBOUS,
    LAST_QUARTER,
    WANING_CRESCENT
} MoonPhase;

/* Days of the week enumeration */
typedef enum {
    SUNDAY,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY
} Weekday;

/* Structure to represent a day with both lunar and Gregorian information */
typedef struct {
    /* Gregorian date components */
    int greg_year;
    int greg_month;
    int greg_day;
    
    /* Lunar date components */
    int lunar_year;
    int lunar_month;
    int lunar_day;
    
    /* Moon phase for this day */
    MoonPhase moon_phase;
    
    /* Germanic Eld year */
    int eld_year;
    
    /* Weekday */
    Weekday weekday;
    
    /* Position in Metonic cycle */
    int metonic_year;   /* 1-19 */
    int metonic_cycle;  /* Which cycle this date is in */
} LunarDay;

/* Structure to represent a lunar month */
typedef struct {
    int year;
    int month_number;  /* 1-13, as lunar years can have 13 months */
    bool is_leap_month; /* Whether this is an intercalary month */
    int days_count;    /* 29 or 30 days */
    LunarDay days[30]; /* Array of days in this month */
    
    /* First day of month in Julian days */
    double julian_start;
} LunarMonth;

/* Structure to represent a lunar year */
typedef struct {
    int year;
    int months_count;  /* 12 or 13 */
    int days_count;    /* Total days in the year */
    LunarMonth months[13]; /* Array of months, max 13 for leap years */
    int metonic_year;  /* Position in the Metonic cycle (1-19) */
    
    /* Germanic new year (first full moon after first new moon after winter solstice) */
    int germanic_start_greg_month;
    int germanic_start_greg_day;
} LunarYear;

/* Structure to represent a complete Metonic cycle (19 years) */
typedef struct {
    int cycle_number;  /* Which Metonic cycle this is */
    LunarYear years[19]; /* Array of 19 years in the cycle */
    
    /* Start and end Julian days */
    double start_julian_day;
    double end_julian_day;
} MetonicCycle;

/* Function prototypes for date conversions and calculations */

/* Convert a Gregorian date to a lunar date */
LunarDay gregorian_to_lunar(int year, int month, int day);

/* Convert a lunar date to a Gregorian date */
bool lunar_to_gregorian(int lunar_year, int lunar_month, int lunar_day, 
                        int *greg_year, int *greg_month, int *greg_day);

/* Calculate the Germanic Eld year from a Gregorian year */
int calculate_eld_year(int gregorian_year);

/* Calculate the phase of the moon for a given date */
MoonPhase calculate_moon_phase(int year, int month, int day);

/* Calculate the exact time of the full moon for a given month/year */
bool calculate_full_moon(int year, int month, int *full_moon_day, double *full_moon_hour);

/* Calculate the exact time of the new moon for a given month/year */
bool calculate_new_moon(int year, int month, int *new_moon_day, double *new_moon_hour);

/* Calculate the winter solstice date for a given year */
bool calculate_winter_solstice(int year, int *month, int *day);

/* Calculate equinoxes and solstices for a given year */
bool calculate_spring_equinox(int year, int *month, int *day);
bool calculate_summer_solstice(int year, int *month, int *day);
bool calculate_fall_equinox(int year, int *month, int *day);

/* Helper functions for astronomical calculations */
double calculate_solstice_equinox_jde(int year, int season);
double periodic_terms_for_solstice_equinox(double T, int season);

/* Calculate the Germanic new year date for a given Gregorian year */
int calculate_germanic_new_year(int year, int *month, int *day);

/* Count the number of lunar months in a given year */
int count_lunar_months_in_year(int year);

/* Calculate the weekday for a given date */
Weekday calculate_weekday(int year, int month, int day);

/* Get the lunar date for today */
LunarDay get_today_lunar_date(void);

/* Get the position of a date within the Metonic cycle */
void get_metonic_position(int year, int month, int day, int *metonic_year, int *metonic_cycle);

/* Initialize a Metonic cycle starting from a given Gregorian year */
MetonicCycle initialize_metonic_cycle(int start_year);

/* Calculate if a given lunar month has 29 or 30 days */
int calculate_lunar_month_length(int year, int month);

/* Calculate if a given lunar year is a leap year (13 months) */
bool is_lunar_leap_year(int year);

/* Helper function to check if a given Gregorian year is a leap year */
bool is_gregorian_leap_year(int year);

/* Convert Julian day to Gregorian date */
void julian_day_to_gregorian(double julian_day, int *year, int *month, int *day, double *hour);

/* Convert Gregorian date to Julian day */
double gregorian_to_julian_day(int year, int month, int day, double hour);

#endif /* LUNAR_CALENDAR_H */ 