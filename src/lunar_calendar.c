#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/lunar_calendar.h"

/* 
 * Constants for lunar calculations
 * The Metonic cycle contains 235 lunar months in 19 solar years.
 * In this cycle, 12 years have 12 months, and 7 years have 13 months (leap years).
 */
#define GERMANIC_EPOCH_BC 750  /* 750 BC as the epoch for Eld Year */

/* 
 * Array to track which years in the Metonic cycle have 13 months 
 * (intercalary/leap months). These are traditionally years 3, 6, 8, 11, 14, 17, 19
 * in the Metonic cycle (1-indexed).
 */
static const int LEAP_YEARS_IN_CYCLE[] = {3, 6, 8, 11, 14, 17, 19};
static const int LEAP_YEARS_COUNT = 7;

/* Constants for astronomical calculations */
#define PI 3.14159265358979323846
#define RAD_TO_DEG(rad) ((rad) * 180.0 / PI)
#define DEG_TO_RAD(deg) ((deg) * PI / 180.0)

/* 
 * Convert Gregorian date to Julian day
 * Algorithm from Astronomical Algorithms by Jean Meeus
 */
double gregorian_to_julian_day(int year, int month, int day, double hour) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    
    int a = floor(year / 100.0);
    int b = 2 - a + floor(a / 4.0);
    
    double jd = floor(365.25 * (year + 4716)) + 
                floor(30.6001 * (month + 1)) + 
                day + b - 1524.5 + (hour / 24.0);
    
    return jd;
}

/* Convert Julian day to Gregorian date */
void julian_day_to_gregorian(double julian_day, int *year, int *month, int *day, double *hour) {
    double z = floor(julian_day + 0.5);
    double f = julian_day + 0.5 - z;
    
    double alpha;
    if (z < 2299161) {
        alpha = z;
    } else {
        double a = floor((z - 1867216.25) / 36524.25);
        alpha = z + 1 + a - floor(a / 4);
    }
    
    double b = alpha + 1524;
    double c = floor((b - 122.1) / 365.25);
    double d = floor(365.25 * c);
    double e = floor((b - d) / 30.6001);
    
    *day = (int)(b - d - floor(30.6001 * e) + f);
    *month = (int)(e < 14 ? e - 1 : e - 13);
    *year = (int)(*month > 2 ? c - 4716 : c - 4715);
    *hour = f * 24.0;
}

/* Calculate if a given Gregorian year is a leap year */
bool is_gregorian_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/* Calculate if a given lunar year is a leap year (13 months) */
bool is_lunar_leap_year(int year) {
    int position_in_cycle = ((year - 1) % YEARS_PER_METONIC_CYCLE) + 1;
    
    for (int i = 0; i < LEAP_YEARS_COUNT; i++) {
        if (position_in_cycle == LEAP_YEARS_IN_CYCLE[i]) {
            return true;
        }
    }
    
    return false;
}

/* Calculate the weekday for a given date */
Weekday calculate_weekday(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year--;
    }
    
    int h = (day + (13 * (month + 1)) / 5 + year + year/4 - year/100 + year/400) % 7;
    
    /* Convert to 0-based index (0 = Sunday) */
    return (Weekday)((h + 6) % 7);
}

/*
 * Calculate the moon phase using a more accurate algorithm
 * Based on the Astronomical Algorithms by Jean Meeus
 */
MoonPhase calculate_moon_phase(int year, int month, int day) {
    int new_moon_day;
    double new_moon_hour;
    int full_moon_day;
    double full_moon_hour;
    
    /* Direct comparison with new moon and full moon dates */
    if (calculate_new_moon(year, month, &new_moon_day, &new_moon_hour) && day == new_moon_day) {
        return NEW_MOON;
    }
    
    if (calculate_full_moon(year, month, &full_moon_day, &full_moon_hour) && day == full_moon_day) {
        return FULL_MOON;
    }
    
    /* Get Julian day for the target date at noon */
    double jd = gregorian_to_julian_day(year, month, day, 12.0);
    double new_moon_jd = 0.0;
    double full_moon_jd = 0.0;
    
    /* Find the new moon before or on this date */
    int curr_month = month;
    int curr_year = year;
    int prev_month = month;
    int prev_year = year;
    
    /* Look in current month first */
    if (calculate_new_moon(curr_year, curr_month, &new_moon_day, &new_moon_hour)) {
        new_moon_jd = gregorian_to_julian_day(curr_year, curr_month, new_moon_day, new_moon_hour);
        
        /* If the new moon is after our date, we need to go back one month */
        if (new_moon_jd > jd) {
            prev_month = curr_month - 1;
            prev_year = curr_year;
            if (prev_month < 1) {
                prev_month = 12;
                prev_year--;
            }
            
            if (calculate_new_moon(prev_year, prev_month, &new_moon_day, &new_moon_hour)) {
                new_moon_jd = gregorian_to_julian_day(prev_year, prev_month, new_moon_day, new_moon_hour);
            }
        }
    }
    
    /* Find the full moon after or on this date */
    curr_month = month;
    curr_year = year;
    int next_month = month;
    int next_year = year;
    
    /* Look in current month first */
    if (calculate_full_moon(curr_year, curr_month, &full_moon_day, &full_moon_hour)) {
        full_moon_jd = gregorian_to_julian_day(curr_year, curr_month, full_moon_day, full_moon_hour);
        
        /* If the full moon is before our date, we need to go forward one month */
        if (full_moon_jd < jd) {
            next_month = curr_month + 1;
            next_year = curr_year;
            if (next_month > 12) {
                next_month = 1;
                next_year++;
            }
            
            if (calculate_full_moon(next_year, next_month, &full_moon_day, &full_moon_hour)) {
                full_moon_jd = gregorian_to_julian_day(next_year, next_month, full_moon_day, full_moon_hour);
            }
        }
    }
    
    /* Now determine where we are in the lunar cycle */
    if (new_moon_jd > 0 && full_moon_jd > 0) {
        /* Normalized position between new moon and full moon (0.0 to 1.0) */
        double lunar_cycle_length = 29.530588861; /* Average synodic month */
        double age = jd - new_moon_jd;
        double lunar_age = age / lunar_cycle_length;
        
        /* Determine the moon phase based on the normalized value */
        if (lunar_age < 0.03) return NEW_MOON;                 /* ~1 day */
        if (lunar_age < 0.25) return WAXING_CRESCENT;          /* ~7 days */
        if (lunar_age < 0.28) return FIRST_QUARTER;            /* ~1 day */
        if (lunar_age < 0.47) return WAXING_GIBBOUS;           /* ~6 days */
        if (lunar_age < 0.53) return FULL_MOON;                /* ~1-2 days */
        if (lunar_age < 0.72) return WANING_GIBBOUS;           /* ~6 days */
        if (lunar_age < 0.75) return LAST_QUARTER;             /* ~1 day */
        if (lunar_age < 0.97) return WANING_CRESCENT;          /* ~7 days */
        return NEW_MOON;                                       /* Next new moon */
    }
    
    /* Fallback to the simplified calculation if the above fails */
    /* Convert date to Julian day */
    double t = (jd - 2451545.0) / 36525.0;
    
    /* Sun's mean anomaly */
    double m_sun = 357.52911 + t * (35999.05029 - 0.0001537 * t);
    m_sun = fmod(m_sun, 360.0);
    if (m_sun < 0) m_sun += 360.0;
    
    /* Moon's mean anomaly */
    double m_moon = 134.96298 + t * (477198.867398 + t * (0.0086972 - t * 0.000001778));
    m_moon = fmod(m_moon, 360.0);
    if (m_moon < 0) m_moon += 360.0;
    
    /* Moon's mean elongation from the Sun */
    double d = 297.85036 + t * (445267.111480 + t * (-0.0019142 + t * 0.00000144));
    d = fmod(d, 360.0);
    if (d < 0) d += 360.0;
    
    /* Phase angle (0 = new, 180 = full) */
    double phase_angle = 180.0 - d - 6.289 * sin(DEG_TO_RAD(m_moon)) 
                       + 2.100 * sin(DEG_TO_RAD(m_sun)) 
                       - 1.274 * sin(DEG_TO_RAD(2*d - m_moon))
                       - 0.658 * sin(DEG_TO_RAD(2*d))
                       - 0.214 * sin(DEG_TO_RAD(2*m_moon));
    
    phase_angle = fmod(phase_angle, 360.0);
    if (phase_angle < 0) phase_angle += 360.0;
    
    /* Normalized phase (0.0 to 1.0) */
    double phase = phase_angle / 360.0;
    
    /* Determine the moon phase based on the normalized value */
    if (phase < 0.0625 || phase >= 0.9375) return NEW_MOON;
    if (phase < 0.1875) return WAXING_CRESCENT;
    if (phase < 0.3125) return FIRST_QUARTER;
    if (phase < 0.4375) return WAXING_GIBBOUS;
    if (phase < 0.5625) return FULL_MOON;
    if (phase < 0.6875) return WANING_GIBBOUS;
    if (phase < 0.8125) return LAST_QUARTER;
    return WANING_CRESCENT;
}

/*
 * Calculate the time of the new moon closest to the specified month/year
 * Returns true if successful, false otherwise
 */
bool calculate_new_moon(int year, int month, int *new_moon_day, double *new_moon_hour) {
    /* Convert to Julian day for the middle of the month */
    double jd = gregorian_to_julian_day(year, month, 15, 0.0);
    
    /* Time in Julian centuries since J2000.0 */
    double t = (jd - 2451545.0) / 36525.0;
    
    /* Approximate k value for the nearest new moon */
    double k = round((jd - 2451550.09766) / 29.530588861);
    
    /* Time of mean new moon */
    double jde = 2451550.09766 + 29.530588861 * k
                + 0.00015437 * t * t
                - 0.000000150 * t * t * t
                + 0.00000000073 * t * t * t * t;
    
    /* Sun's mean anomaly at time jde */
    double m_sun = 2.5534 + 29.10535670 * k
                 - 0.0000014 * t * t
                 - 0.00000011 * t * t * t;
    m_sun = fmod(m_sun, 2 * PI);
    
    /* Moon's mean anomaly at time jde */
    double m_moon = 201.5643 + 385.81693528 * k
                  + 0.0107582 * t * t
                  + 0.00001238 * t * t * t
                  - 0.000000058 * t * t * t * t;
    m_moon = fmod(m_moon, 2 * PI);
    
    /* Moon's argument of latitude */
    double f = 160.7108 + 390.67050284 * k
             - 0.0016118 * t * t
             - 0.00000227 * t * t * t
             + 0.000000011 * t * t * t * t;
    f = fmod(f, 2 * PI);
    
    /* Longitude of ascending node of lunar orbit */
    double omega = 124.7746 - 1.56375588 * k
                 + 0.0020672 * t * t
                 + 0.00000215 * t * t * t;
    omega = fmod(omega, 2 * PI);
    
    /* Corrections for periodic terms */
    double e = 1 - 0.002516 * t - 0.0000074 * t * t;
    
    /* Apply corrections to jde */
    jde = jde - 0.40720 * sin(m_moon)
             + 0.17241 * e * sin(m_sun)
             + 0.01608 * sin(2 * m_moon)
             + 0.01039 * sin(2 * f)
             + 0.00739 * e * sin(m_moon - m_sun)
             - 0.00514 * e * sin(m_moon + m_sun)
             + 0.00208 * e * e * sin(2 * m_sun)
             - 0.00111 * sin(m_moon - 2 * f)
             - 0.00057 * sin(m_moon + 2 * f)
             + 0.00056 * e * sin(2 * m_moon + m_sun)
             - 0.00042 * sin(3 * m_moon)
             + 0.00042 * e * sin(m_sun + 2 * f)
             + 0.00038 * e * sin(m_sun - 2 * f)
             - 0.00024 * e * sin(2 * m_moon - m_sun)
             - 0.00017 * sin(omega)
             - 0.00007 * sin(m_moon + 2 * m_sun)
             + 0.00004 * sin(2 * m_moon - 2 * f)
             + 0.00004 * sin(3 * m_sun)
             + 0.00003 * sin(m_moon + m_sun - 2 * f)
             + 0.00003 * sin(2 * m_moon + 2 * f)
             - 0.00003 * sin(m_moon + m_sun + 2 * f)
             + 0.00003 * sin(m_moon - m_sun + 2 * f)
             - 0.00002 * sin(m_moon - m_sun - 2 * f)
             - 0.00002 * sin(3 * m_moon + m_sun)
             + 0.00002 * sin(4 * m_moon);
    
    /* Convert Julian day to Gregorian date */
    int result_year, result_month;
    julian_day_to_gregorian(jde, &result_year, &result_month, new_moon_day, new_moon_hour);
    
    /* If new moon is not in the requested month, check one lunation before and after */
    if (result_month != month || result_year != year) {
        /* Try lunation before */
        double jde_before = jde - 29.530588861;
        julian_day_to_gregorian(jde_before, &result_year, &result_month, new_moon_day, new_moon_hour);
        if (result_month == month && result_year == year) {
            return true;
        }
        
        /* Try lunation after */
        double jde_after = jde + 29.530588861;
        julian_day_to_gregorian(jde_after, &result_year, &result_month, new_moon_day, new_moon_hour);
        if (result_month == month && result_year == year) {
            return true;
        }
        
        /* No new moon found in the requested month */
        return false;
    }
    
    return true;
}

/*
 * Calculate the time of the full moon closest to the specified month/year
 * Returns true if successful, false otherwise
 */
bool calculate_full_moon(int year, int month, int *full_moon_day, double *full_moon_hour) {
    /* Convert to Julian day for the middle of the month */
    double jd = gregorian_to_julian_day(year, month, 15, 0.0);
    
    /* Time in Julian centuries since J2000.0 */
    double t = (jd - 2451545.0) / 36525.0;
    
    /* Approximate k value for the nearest full moon (k + 0.5) */
    double k = round((jd - 2451550.09766) / 29.530588861 - 0.5) + 0.5;
    
    /* Time of mean full moon */
    double jde = 2451550.09766 + 29.530588861 * k
                + 0.00015437 * t * t
                - 0.000000150 * t * t * t
                + 0.00000000073 * t * t * t * t;
    
    /* Sun's mean anomaly at time jde */
    double m_sun = 2.5534 + 29.10535670 * k
                 - 0.0000014 * t * t
                 - 0.00000011 * t * t * t;
    m_sun = fmod(m_sun, 2 * PI);
    
    /* Moon's mean anomaly at time jde */
    double m_moon = 201.5643 + 385.81693528 * k
                  + 0.0107582 * t * t
                  + 0.00001238 * t * t * t
                  - 0.000000058 * t * t * t * t;
    m_moon = fmod(m_moon, 2 * PI);
    
    /* Moon's argument of latitude */
    double f = 160.7108 + 390.67050284 * k
             - 0.0016118 * t * t
             - 0.00000227 * t * t * t
             + 0.000000011 * t * t * t * t;
    f = fmod(f, 2 * PI);
    
    /* Longitude of ascending node of lunar orbit */
    double omega = 124.7746 - 1.56375588 * k
                 + 0.0020672 * t * t
                 + 0.00000215 * t * t * t;
    omega = fmod(omega, 2 * PI);
    
    /* Corrections for periodic terms */
    double e = 1 - 0.002516 * t - 0.0000074 * t * t;
    
    /* Apply corrections to jde - full moon has different corrections than new moon */
    jde = jde - 0.40614 * sin(m_moon)
             + 0.17302 * e * sin(m_sun)
             + 0.01614 * sin(2 * m_moon)
             + 0.01043 * sin(2 * f)
             + 0.00734 * e * sin(m_moon - m_sun)
             - 0.00515 * e * sin(m_moon + m_sun)
             + 0.00209 * e * e * sin(2 * m_sun)
             - 0.00111 * sin(m_moon - 2 * f)
             - 0.00057 * sin(m_moon + 2 * f)
             + 0.00056 * e * sin(2 * m_moon + m_sun)
             - 0.00042 * sin(3 * m_moon)
             + 0.00042 * e * sin(m_sun + 2 * f)
             + 0.00038 * e * sin(m_sun - 2 * f)
             - 0.00024 * e * sin(2 * m_moon - m_sun)
             - 0.00017 * sin(omega)
             - 0.00007 * sin(m_moon + 2 * m_sun)
             + 0.00004 * sin(2 * m_moon - 2 * f)
             + 0.00004 * sin(3 * m_sun)
             + 0.00003 * sin(m_moon + m_sun - 2 * f)
             + 0.00003 * sin(2 * m_moon + 2 * f)
             - 0.00003 * sin(m_moon + m_sun + 2 * f)
             + 0.00003 * sin(m_moon - m_sun + 2 * f)
             - 0.00002 * sin(m_moon - m_sun - 2 * f)
             - 0.00002 * sin(3 * m_moon + m_sun)
             + 0.00002 * sin(4 * m_moon);
    
    /* Convert Julian day to Gregorian date */
    int result_year, result_month;
    julian_day_to_gregorian(jde, &result_year, &result_month, full_moon_day, full_moon_hour);
    
    /* If full moon is not in the requested month, check one lunation before and after */
    if (result_month != month || result_year != year) {
        /* Try lunation before */
        double jde_before = jde - 29.530588861;
        julian_day_to_gregorian(jde_before, &result_year, &result_month, full_moon_day, full_moon_hour);
        if (result_month == month && result_year == year) {
            return true;
        }
        
        /* Try lunation after */
        double jde_after = jde + 29.530588861;
        julian_day_to_gregorian(jde_after, &result_year, &result_month, full_moon_day, full_moon_hour);
        if (result_month == month && result_year == year) {
            return true;
        }
        
        /* No full moon found in the requested month */
        return false;
    }
    
    return true;
}

/* Calculate the winter solstice date for a given year */
bool calculate_winter_solstice(int year, int *month, int *day) {
    /* More accurate astronomical calculation for winter solstice */
    /* Based on Jean Meeus' Astronomical Algorithms */
    
    double JDE0 = calculate_solstice_equinox_jde(year, 0); /* 0 = winter solstice */
    
    /* Convert to Gregorian date */
    double hour_unused;
    int result_year;
    julian_day_to_gregorian(JDE0, &result_year, month, day, &hour_unused);
    
    return true;
}

/* Calculate the spring equinox date for a given year */
bool calculate_spring_equinox(int year, int *month, int *day) {
    /* Astronomical calculation for spring equinox */
    double JDE0 = calculate_solstice_equinox_jde(year, 1); /* 1 = spring equinox */
    
    /* Convert to Gregorian date */
    double hour_unused;
    int result_year;
    julian_day_to_gregorian(JDE0, &result_year, month, day, &hour_unused);
    
    return true;
}

/* Calculate the summer solstice date for a given year */
bool calculate_summer_solstice(int year, int *month, int *day) {
    /* Astronomical calculation for summer solstice */
    double JDE0 = calculate_solstice_equinox_jde(year, 2); /* 2 = summer solstice */
    
    /* Convert to Gregorian date */
    double hour_unused;
    int result_year;
    julian_day_to_gregorian(JDE0, &result_year, month, day, &hour_unused);
    
    return true;
}

/* Calculate the fall equinox date for a given year */
bool calculate_fall_equinox(int year, int *month, int *day) {
    /* Astronomical calculation for fall equinox */
    double JDE0 = calculate_solstice_equinox_jde(year, 3); /* 3 = fall equinox */
    
    /* Convert to Gregorian date */
    double hour_unused;
    int result_year;
    julian_day_to_gregorian(JDE0, &result_year, month, day, &hour_unused);
    
    return true;
}

/* Helper function to calculate Julian day for solstices and equinoxes
 * Based on Jean Meeus' Astronomical Algorithms, Chapter 27
 * 
 * season: 0=winter solstice, 1=spring equinox, 2=summer solstice, 3=fall equinox
 */
double calculate_solstice_equinox_jde(int year, int season) {
    double y = (year - 2000) / 1000.0;
    double JDE0;
    
    /* Constants based on Astronomical Algorithms by Jean Meeus */
    switch(season) {
        case 0: /* Winter solstice */
            JDE0 = 2451900.05952 + 365242.74049 * y + 0.00278 * y * y;
            break;
        case 1: /* Spring equinox */
            JDE0 = 2451623.80984 + 365242.37404 * y + 0.05169 * y * y;
            break;
        case 2: /* Summer solstice */
            JDE0 = 2451716.56767 + 365241.62603 * y + 0.00325 * y * y;
            break;
        case 3: /* Fall equinox */
            JDE0 = 2451810.21715 + 365242.01767 * y - 0.11575 * y * y;
            break;
        default:
            return 0;
    }
    
    /* Apply periodic terms */
    double T = (JDE0 - 2451545.0) / 36525.0; /* Julian centuries since J2000.0 */
    double W = 35999.373 * T - 2.47;
    double dλ = 1 + 0.0334 * cos(DEG_TO_RAD(W)) + 0.0007 * cos(DEG_TO_RAD(2 * W));
    
    /* Add periodic term corrections from Meeus' tables */
    double S = periodic_terms_for_solstice_equinox(T, season);
    
    return JDE0 + 0.00001 * S / dλ;
}

/* Calculate periodic terms for solstices and equinoxes */
double periodic_terms_for_solstice_equinox(double T, int season) {
    /* Coefficients for periodic terms from Meeus' tables */
    /* This is a simplified implementation with the most significant terms */
    
    /* A, B, C terms from Meeus Chapter 27 Table 27.A */
    static const double terms[24][3] = {
        /* A, B, C for each term */
        {485, 324.96, 1934.136},
        {203, 337.23, 32964.467},
        {199, 342.08, 20.186},
        {182, 27.85, 445267.112},
        {156, 73.14, 45036.886},
        {136, 171.52, 22518.443},
        {77, 222.54, 65928.934},
        {74, 296.72, 3034.906},
        {70, 243.58, 9037.513},
        {58, 119.81, 33718.147},
        {52, 297.17, 150.678},
        {50, 21.02, 2281.226},
        {45, 247.54, 29929.562},
        {44, 325.15, 31555.956},
        {29, 60.93, 4443.417},
        {18, 155.12, 67555.328},
        {17, 288.79, 4562.452},
        {16, 198.04, 62894.029},
        {14, 199.76, 31436.921},
        {12, 95.39, 14577.848},
        {12, 287.11, 31931.756},
        {12, 320.81, 34777.259},
        {9, 227.73, 1222.114},
        {8, 15.45, 16859.074}
    };
    
    /* For full accuracy, we would need separate coefficients for each season */
    /* This is a simplification - for real implementations, use season-specific coefficients */
    (void)season; /* Acknowledge that we see the parameter but don't fully use it yet */
    
    double sum = 0.0;
    for (int i = 0; i < 24; i++) {
        double A = terms[i][0];
        double B = terms[i][1];
        double C = terms[i][2];
        
        sum += A * cos(DEG_TO_RAD(B + C * T));
    }
    
    return sum;
}

/*
 * Calculate the Germanic New Year for a given year
 * This is the first full moon after the first new moon after the winter solstice
 */
int calculate_germanic_new_year(int year, int *month, int *day) {
    /* Find the winter solstice of the previous year */
    int ws_month, ws_day;
    int prev_year = year - 1;
    if (!calculate_winter_solstice(prev_year, &ws_month, &ws_day)) {
        return 0; /* Failed to calculate winter solstice */
    }
    
    /* Convert the winter solstice to Julian days for comparison */
    double ws_jd = gregorian_to_julian_day(prev_year, ws_month, ws_day, 12.0);
    
    /* Use a more robust day-by-day search method starting from winter solstice */
    double search_jd = ws_jd;
    double new_moon_jd = 0;
    
    /* Search for up to 45 days to find the first new moon after winter solstice */
    for (int i = 0; i < 45; i++) {
        int search_year, search_month, search_day;
        double search_hour;
        
        /* Convert Julian day to Gregorian date */
        julian_day_to_gregorian(search_jd, &search_year, &search_month, &search_day, &search_hour);
        
        /* Get the moon phase for this day */
        MoonPhase phase = calculate_moon_phase(search_year, search_month, search_day);
        
        /* If we found a new moon, record it and stop searching */
        if (phase == NEW_MOON) {
            new_moon_jd = search_jd;
            break;
        }
        
        /* Move to the next day */
        search_jd += 1.0;
    }
    
    /* If we didn't find a new moon, return failure */
    if (new_moon_jd == 0) {
        return 0;
    }
    
    /* Now search for the first full moon after the new moon */
    search_jd = new_moon_jd + 1.0; /* Start from the day after new moon */
    
    /* Search for up to 30 days (one lunar month) to find the next full moon */
    for (int i = 0; i < 30; i++) {
        int search_year, search_month, search_day;
        double search_hour;
        
        /* Convert Julian day to Gregorian date */
        julian_day_to_gregorian(search_jd, &search_year, &search_month, &search_day, &search_hour);
        
        /* Get the moon phase for this day */
        MoonPhase phase = calculate_moon_phase(search_year, search_month, search_day);
        
        /* If we found a full moon, that's our Germanic New Year */
        if (phase == FULL_MOON) {
            *month = search_month;
            *day = search_day;
            return 1; /* Success */
        }
        
        /* Move to the next day */
        search_jd += 1.0;
    }
    
    return 0;
}

/* Calculate the Germanic Eld year from a Gregorian year */
int calculate_eld_year(int gregorian_year) {
    /* The Eld year 2775 corresponds to 2025 CE */
    return gregorian_year + GERMANIC_EPOCH_BC;
}

/* Get the position of a date within the Metonic cycle */
void get_metonic_position(int year, int month, int day, int *metonic_year, int *metonic_cycle) {
    /* For more accuracy we could use the exact date, but the calculations mainly depend on the year */
    /* We're including month and day for potential future improvements */
    (void)month; /* Prevent unused parameter warning */
    (void)day;   /* Prevent unused parameter warning */
    
    /* Calculate which Metonic cycle this is */
    *metonic_cycle = (year - 1) / YEARS_PER_METONIC_CYCLE + 1;
    
    /* Calculate which year in the cycle (1-19) */
    *metonic_year = ((year - 1) % YEARS_PER_METONIC_CYCLE) + 1;
}

/* Calculate if a given lunar month has 29 or 30 days */
int calculate_lunar_month_length(int year, int month) {
    /* In a real implementation, this would be calculated based on the actual moon cycles */
    /* For simplicity, we alternate 30 and 29 days, with adjustments for leap years */
    
    /* Find position in Metonic cycle */
    int metonic_year, metonic_cycle;
    get_metonic_position(year, month, 1, &metonic_year, &metonic_cycle);
    
    /* Basic pattern: odd months have 30 days, even months have 29 days */
    /* However, the exact lengths depend on astronomical calculations */
    int length = (month % 2 == 1) ? 30 : 29;
    
    /* Some special adjustments based on position in Metonic cycle */
    if (is_lunar_leap_year(year) && month == 6) {
        /* In leap years, make the 6th month 30 days */
        length = 30;
    }
    
    return length;
}

/* 
 * Convert a Gregorian date to a lunar date
 * This is an enhanced version with more astronomical accuracy.
 */
LunarDay gregorian_to_lunar(int year, int month, int day) {
    LunarDay result;
    
    /* Set the Gregorian date components */
    result.greg_year = year;
    result.greg_month = month;
    result.greg_day = day;
    
    /* Calculate the weekday */
    result.weekday = calculate_weekday(year, month, day);
    
    /* Calculate the moon phase for this date */
    result.moon_phase = calculate_moon_phase(year, month, day);
    
    /* Calculate position in Metonic cycle */
    get_metonic_position(year, month, day, &result.metonic_year, &result.metonic_cycle);
    
    /* Convert to Julian day */
    double julian_day = gregorian_to_julian_day(year, month, day, 12.0); /* Noon */
    
    /* 
     * Use a reference date that we know its lunar date
     * January 21, 2023 was the start of the Lunar Year 2023
     * (Chinese/East Asian New Year)
     */
    double reference_julian_day = gregorian_to_julian_day(2023, 1, 21, 12.0);
    int reference_lunar_year = 2023;
    int reference_lunar_month = 1;
    int reference_lunar_day = 1;
    
    /* Calculate the number of days between the reference date and target date */
    double days_diff = julian_day - reference_julian_day;
    
    /* 
     * Starting from the reference lunar date, we'll adjust the lunar date
     * based on the days difference.
     */
    int curr_lunar_year = reference_lunar_year;
    int curr_lunar_month = reference_lunar_month;
    int curr_lunar_day = reference_lunar_day;
    
    /* Handle dates before the reference date */
    if (days_diff < 0) {
        /* Implementation for earlier dates would be more complex in practice */
        /* For simplicity, we'll just set placeholder values */
        result.lunar_year = curr_lunar_year - 1;
        result.lunar_month = 12;
        result.lunar_day = 30 + (int)days_diff;
        result.eld_year = calculate_eld_year(year);
        return result;
    }
    
    /* Move forward day by day (this could be optimized) */
    while (days_diff > 0) {
        /* Get days in current lunar month */
        int days_in_month = calculate_lunar_month_length(curr_lunar_year, curr_lunar_month);
        
        /* If we can skip the entire month */
        if (days_diff >= days_in_month - curr_lunar_day + 1) {
            days_diff -= (days_in_month - curr_lunar_day + 1);
            curr_lunar_month++;
            curr_lunar_day = 1;
            
            /* Check if we need to move to next year */
            if (curr_lunar_month > 12 && !is_lunar_leap_year(curr_lunar_year)) {
                curr_lunar_year++;
                curr_lunar_month = 1;
            } else if (curr_lunar_month > 13) {
                curr_lunar_year++;
                curr_lunar_month = 1;
            }
        } else {
            /* Just advance the days */
            curr_lunar_day += (int)days_diff;
            days_diff = 0;
        }
    }
    
    result.lunar_year = curr_lunar_year;
    result.lunar_month = curr_lunar_month;
    result.lunar_day = curr_lunar_day;
    result.eld_year = calculate_eld_year(year);
    
    return result;
}

/* 
 * Convert a lunar date to a Gregorian date 
 * Enhanced with more accurate calculations
 */
bool lunar_to_gregorian(int lunar_year, int lunar_month, int lunar_day, 
                        int *greg_year, int *greg_month, int *greg_day) {
    /* Validate input */
    if (lunar_month < 1 || lunar_month > 13 || lunar_day < 1 || lunar_day > 30) {
        return false;
    }
    
    /* Additional validation: Check if this is a valid day for this lunar month */
    int days_in_month = calculate_lunar_month_length(lunar_year, lunar_month);
    if (lunar_day > days_in_month) {
        return false;
    }
    
    /* 
     * Use the same reference date as in gregorian_to_lunar 
     * January 21, 2023 = Lunar Year 2023, Month 1, Day 1
     */
    double reference_julian_day = gregorian_to_julian_day(2023, 1, 21, 12.0);
    int reference_lunar_year = 2023;
    int reference_lunar_month = 1;
    int reference_lunar_day = 1;
    
    /* Calculate the difference in days between our reference lunar date and the target lunar date */
    double days_diff = 0;
    
    /* Handle year difference */
    for (int y = reference_lunar_year; y < lunar_year; y++) {
        /* Add days for each year */
        if (is_lunar_leap_year(y)) {
            days_diff += 384; /* Approximate days in a leap lunar year (13 months) */
        } else {
            days_diff += 354; /* Approximate days in a regular lunar year (12 months) */
        }
    }
    
    /* Handle month difference */
    for (int m = reference_lunar_month; m < lunar_month; m++) {
        /* Add days for each month */
        days_diff += calculate_lunar_month_length(lunar_year, m);
    }
    
    /* Handle day difference */
    days_diff += (lunar_day - reference_lunar_day);
    
    /* Calculate the Julian day for the target date */
    double target_julian_day = reference_julian_day + days_diff;
    
    /* Convert Julian day to Gregorian date */
    double hour;
    julian_day_to_gregorian(target_julian_day, greg_year, greg_month, greg_day, &hour);
    
    return true;
}

/* Get the lunar date for today */
LunarDay get_today_lunar_date(void) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    
    return gregorian_to_lunar(tm_now->tm_year + 1900, 
                             tm_now->tm_mon + 1, 
                             tm_now->tm_mday);
}

/* Initialize a Metonic cycle starting from a given Gregorian year */
MetonicCycle initialize_metonic_cycle(int start_year) {
    MetonicCycle cycle;
    
    /* Calculate which cycle number this is (approximate) */
    cycle.cycle_number = (start_year - 1) / YEARS_PER_METONIC_CYCLE + 1;
    
    /* Set start and end Julian days */
    cycle.start_julian_day = gregorian_to_julian_day(start_year, 1, 1, 0.0);
    cycle.end_julian_day = gregorian_to_julian_day(start_year + YEARS_PER_METONIC_CYCLE, 1, 1, 0.0) - 1;
    
    /* Initialize each year in the cycle */
    for (int i = 0; i < YEARS_PER_METONIC_CYCLE; i++) {
        int current_year = start_year + i;
        int year_in_cycle = i + 1;  /* 1-indexed position in the cycle */
        
        cycle.years[i].year = current_year;
        cycle.years[i].metonic_year = year_in_cycle;
        
        /* Determine if this is a leap lunar year */
        bool is_leap = is_lunar_leap_year(current_year);
        
        cycle.years[i].months_count = is_leap ? 13 : 12;
        
        /* Calculate the days in this lunar year */
        int total_days = 0;
        for (int m = 1; m <= cycle.years[i].months_count; m++) {
            total_days += calculate_lunar_month_length(current_year, m);
        }
        cycle.years[i].days_count = total_days;
        
        /* Calculate Germanic new year */
        if (!calculate_germanic_new_year(current_year, 
                                       &cycle.years[i].germanic_start_greg_month,
                                       &cycle.years[i].germanic_start_greg_day)) {
            /* If calculation fails, use default values */
            cycle.years[i].germanic_start_greg_month = 1;
            cycle.years[i].germanic_start_greg_day = 1;
        }
        
        /* In a more complete implementation, we would initialize each month and day */
    }
    
    return cycle;
} 