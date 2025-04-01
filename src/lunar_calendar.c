#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/lunar_calendar.h"

// --- Constants ---
#define GERMANIC_EPOCH_BC 750 
#define PI 3.14159265358979323846
#define LUNAR_CYCLE_DAYS 29.530588861 // Average synodic month length
#define DAYS_PER_JULIAN_CENTURY 36525.0
#define J2000_EPOCH 2451545.0
#ifndef YEARS_PER_METONIC_CYCLE // Define if not in header
#define YEARS_PER_METONIC_CYCLE 19
#endif

// --- Macros ---
#define RAD_TO_DEG(rad) ((rad) * 180.0 / PI)
#define DEG_TO_RAD(deg) ((deg) * PI / 180.0)

// --- Internal Helper Function Declarations ---
double calculate_mean_phase_jd(double k, int phase_type); 
double calculate_true_phase_jd(double k, int phase_type);

// --- Julian Day Conversion ---

/**
 * @brief Convert Gregorian date to Julian day (UT)
 * Algorithm from Astronomical Algorithms by Jean Meeus, Chapter 7
 */
double gregorian_to_julian_day(int year, int month, int day, double hour) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    
    int a = floor(year / 100.0);
    int b = 2 - a + floor(a / 4.0); // Correction for Gregorian calendar reform
    
    // Calculate JD at 0h UT
    double jd0h = floor(365.25 * (year + 4716)) + 
                floor(30.6001 * (month + 1)) + 
                 day + b - 1524.5;
                 
    return jd0h + (hour / 24.0);
}

/**
 * @brief Convert Julian day (UT) to Gregorian date
 * Algorithm from Astronomical Algorithms by Jean Meeus, Chapter 7
 */
void julian_day_to_gregorian(double julian_day, int *year, int *month, int *day, double *hour) {
    // Meeus algorithm is based on 0h UT JD
    double jd_for_calc = julian_day;

    double Z = floor(jd_for_calc + 0.5);
    double F = jd_for_calc + 0.5 - Z;

    double A;
    if (Z < 2299161) {
        A = Z;
    } else {
        double alpha = floor((Z - 1867216.25) / 36524.25);
        A = Z + 1 + alpha - floor(alpha / 4.0);
    }

    double B = A + 1524;
    double C = floor((B - 122.1) / 365.25);
    double D = floor(365.25 * C);
    double E = floor((B - D) / 30.6001);

    // Calculate day first, handling fractional part F
    *day = (int)floor(B - D - floor(30.6001 * E) + F);
    *month = (int)(E < 14 ? E - 1 : E - 13);
    *year = (int)(*month > 2 ? C - 4716 : C - 4715);
    
    // Calculate fractional part of the day as hour
    *hour = (jd_for_calc - floor(jd_for_calc)) * 24.0; // Correct hour calculation
    if (*hour < 0) *hour += 24.0; // Ensure hour is positive
    // Handle potential edge case where F is extremely close to 1 due to rounding
    if (F < 1e-9 && F >= 0) *hour = 0.0;
}


// --- Gregorian Calendar Helpers ---

/**
 * @brief Calculate if a given Gregorian year is a leap year
 */
bool is_gregorian_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * @brief Calculate the weekday for a given Gregorian date (Zeller's congruence)
 */
Weekday calculate_weekday(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int K = year % 100;
    int J = year / 100;
    // Using Meeus' version of Zeller's congruence for Gregorian calendar
    int h = (day + (int)floor(13.0 * (month + 1.0) / 5.0) + K + K/4 + J/4 + 5*J) % 7;
    
    // Convert result (0=Sat, 1=Sun, ...) to 0=Sun, 1=Mon, ...
    return (Weekday)((h + 1) % 7); 
}

// --- Solstice/Equinox Calculation ---

/**
 * @brief Calculate Julian day for solstices and equinoxes (UT)
 * Based on Jean Meeus' Astronomical Algorithms, Chapter 27 (simplified)
 * season: 0=winter solstice (Dec), 1=spring equinox (Mar), 2=summer solstice (Jun), 3=fall equinox (Sep)
 */
double calculate_solstice_equinox_jde(int year, int season) {
    double y = ((double)year - 2000.0) / 1000.0; // Years since 2000, in millennia
    double JDE0;

    switch(season) {
        case 0: // Winter Solstice
            JDE0 = 2451900.05952 + 365242.74049 * y + 0.00278 * y * y; 
            break;
        case 1: // Spring Equinox
            JDE0 = 2451623.80984 + 365242.37404 * y + 0.05169 * y * y;
            break;
        case 2: // Summer Solstice
             JDE0 = 2451716.56767 + 365241.62603 * y + 0.00325 * y * y;
           break;
        case 3: // Fall Equinox
             JDE0 = 2451810.21715 + 365242.01767 * y - 0.11575 * y * y;
            break;
        default:
            fprintf(stderr, "Error: Invalid season %d in calculate_solstice_equinox_jde\\n", season);
            return 0; // Invalid season
    }
    // Note: Neglects periodic terms and Delta T correction for simplicity.
    return JDE0; 
}

/**
 * @brief Calculate the winter solstice date for a given year
 */
bool calculate_winter_solstice(int year, int *month, int *day) {
    double jde = calculate_solstice_equinox_jde(year, 0); 
    if (jde == 0) return false;
    double hour_unused; int result_year;
    julian_day_to_gregorian(jde, &result_year, month, day, &hour_unused); 
    return true;
}

/**
 * @brief Calculate the spring equinox date for a given year
 */
bool calculate_spring_equinox(int year, int *month, int *day) {
    double jde = calculate_solstice_equinox_jde(year, 1); 
    if (jde == 0) return false;
    double hour_unused; int result_year;
    julian_day_to_gregorian(jde, &result_year, month, day, &hour_unused);
    return true;
}

/**
 * @brief Calculate the summer solstice date for a given year
 */
bool calculate_summer_solstice(int year, int *month, int *day) {
    double jde = calculate_solstice_equinox_jde(year, 2);
    if (jde == 0) return false;
    double hour_unused; int result_year;
    julian_day_to_gregorian(jde, &result_year, month, day, &hour_unused);
    return true;
}

/**
 * @brief Calculate the fall equinox date for a given year
 */
bool calculate_fall_equinox(int year, int *month, int *day) {
    double jde = calculate_solstice_equinox_jde(year, 3);
    if (jde == 0) return false;
    double hour_unused; int result_year;
    julian_day_to_gregorian(jde, &result_year, month, day, &hour_unused);
    return true;
}


// --- Moon Phase Calculation (Based on Meeus, Chapter 49) ---

/**
 * @brief Calculate the approximate Julian Day (UT) for the k-th phase since J2000.0
 * phase_type: 0=NM, 1=FQ, 2=FM, 3=LQ (determines the 0.0, 0.25, 0.5, 0.75 offset)
 */
double calculate_mean_phase_jd(double k, int phase_type) {
    double k_adjusted = k + (double)phase_type / 4.0;
    double T = k_adjusted / 1236.85; // 1236.85 lunations per century
    double jde = 2451550.09766 + LUNAR_CYCLE_DAYS * k_adjusted
                + (0.00015437 * T * T)
                - (0.000000150 * T * T * T)
                + (0.00000000073 * T * T * T * T);
    return jde;
}

/**
 * @brief Calculate the true Julian Day (UT) for the k-th phase, including corrections.
 * phase_type: 0=NM, 1=FQ, 2=FM, 3=LQ
 */
double calculate_true_phase_jd(double k, int phase_type) {
    double jde_mean = calculate_mean_phase_jd(k, phase_type);
    double T = (jde_mean - J2000_EPOCH) / DAYS_PER_JULIAN_CENTURY;
    
    double M_sun = DEG_TO_RAD(fmod(357.5291 + 35999.0503 * T, 360.0)); 
    double M_moon = DEG_TO_RAD(fmod(134.9634 + 477198.8675 * T, 360.0));
    double F_moon = DEG_TO_RAD(fmod(93.2721 + 483202.0175 * T, 360.0));

    double E = 1.0 - 0.002516 * T - 0.0000074 * T * T; 
    double E2 = E * E;
    double corrections = 0;

    if (phase_type == 0) { // New Moon
        corrections += -0.40720 * sin(M_moon) + 0.17241 * E * sin(M_sun);
        corrections += +0.01608 * sin(2.0 * M_moon) + 0.01039 * sin(2.0 * F_moon);
        corrections += +0.00739 * E * sin(M_moon - M_sun) - 0.00514 * E * sin(M_moon + M_sun);
        corrections += +0.00208 * E2 * sin(2.0 * M_sun);
    } else if (phase_type == 1) { // First Quarter
        corrections += -0.62801 * sin(M_moon) + 0.17172 * E * sin(M_sun);
        corrections += -0.01183 * E * sin(M_sun + M_moon) + 0.00871 * sin(2 * M_moon);
        corrections += +0.00800 * E * sin(M_moon - M_sun) + 0.00690 * sin(2 * F_moon);
    } else if (phase_type == 2) { // Full Moon
        corrections += -0.40614 * sin(M_moon) + 0.17302 * E * sin(M_sun);
        corrections += +0.01614 * sin(2.0 * M_moon) + 0.01043 * sin(2.0 * F_moon);
        corrections += +0.00734 * E * sin(M_moon - M_sun) - 0.00515 * E * sin(M_moon + M_sun);
        corrections += +0.00209 * E2 * sin(2.0 * M_sun);
   } else { // Last Quarter (phase_type == 3)
        corrections += -0.62581 * sin(M_moon) + 0.17226 * E * sin(M_sun);
        corrections += -0.01186 * E * sin(M_sun + M_moon) + 0.00867 * sin(2 * M_moon);
        corrections += +0.00797 * E * sin(M_moon - M_sun) + 0.00691 * sin(2 * F_moon);
    }
    // Add more periodic terms for higher accuracy if needed
    return jde_mean + corrections;
}

/**
 * @brief Find the Julian Day (UT) of the first occurrence of a specific phase 
 * (0=NM, 1=FQ, 2=FM, 3=LQ) *after* a given Julian Day (start_jd).
 */
double find_next_phase_jd(double start_jd, int phase_type) {
    double k_approx = (start_jd - 2451550.09766) / LUNAR_CYCLE_DAYS; 
    k_approx -= (double)phase_type / 4.0; 
    double k = floor(k_approx); 
    
    double phase_jd = 0;
    int iterations = 0; 
    const int MAX_ITERATIONS = 5; 
    double epsilon = 1e-5; // Tolerance for comparison
    
    while (iterations < MAX_ITERATIONS) { 
        phase_jd = calculate_true_phase_jd(k, phase_type);
        if (phase_jd >= start_jd - epsilon) { 
            // If it's very close or slightly before, try next k to ensure it's strictly *after*
            if (phase_jd < start_jd + epsilon) { 
                k += 1.0;
                phase_jd = calculate_true_phase_jd(k, phase_type);
            }
            // Check for calculation error (returned 0?)
             if (phase_jd == 0 && k > 0) { 
                 fprintf(stderr, "Error: calculate_true_phase_jd returned 0 unexpectedly for k=%.1f, phase=%d\n", k, phase_type);
                 // Attempt recovery? Maybe try k+1?
                 k += 1.0;
                 phase_jd = calculate_true_phase_jd(k, phase_type);
                 if (phase_jd == 0) return 0; // Still failed
             }
            return phase_jd;
        }
        k += 1.0; 
        iterations++;
    }
    
    fprintf(stderr, "Warning: find_next_phase_jd failed to converge for start_jd=%.4f, phase=%d. Returning estimate.\n", start_jd, phase_type);
    return calculate_true_phase_jd(floor(k_approx) + 1.0, phase_type); 
}

/**
 * @brief Calculate the moon phase for a given Julian Day (UT)
 */
MoonPhase calculate_moon_phase_from_jd(double jd) {
    double k_approx = (jd - 2451550.09766) / LUNAR_CYCLE_DAYS;
    double k_base = floor(k_approx);
    double epsilon = 1e-5;

    double nm0_jd = calculate_true_phase_jd(k_base, 0); 
    double fq0_jd = find_next_phase_jd(nm0_jd - epsilon, 1); 
    double fm0_jd = find_next_phase_jd(nm0_jd - epsilon, 2); 
    double lq0_jd = find_next_phase_jd(nm0_jd - epsilon, 3); 
    double nm1_jd = find_next_phase_jd(nm0_jd + epsilon, 0); 

    if (jd < nm0_jd + epsilon) { // Check if before the calculated NM0
       nm1_jd = nm0_jd;
       // Find phases for the previous lunation
       lq0_jd = calculate_true_phase_jd(k_base - 1.0, 3);
       fm0_jd = calculate_true_phase_jd(k_base - 1.0, 2);
       fq0_jd = calculate_true_phase_jd(k_base - 1.0, 1);
       nm0_jd = calculate_true_phase_jd(k_base - 1.0, 0);
    } else if (!(nm0_jd < fq0_jd && fq0_jd < fm0_jd && fm0_jd < lq0_jd && lq0_jd < nm1_jd)) {
        // Fallback if phases are out of order
        fprintf(stderr, "Warning: Moon phase boundaries disordered for JD %.4f. Recalculating sequentially.\n", jd);
        fq0_jd = find_next_phase_jd(nm0_jd, 1);
        fm0_jd = find_next_phase_jd(fq0_jd, 2);
        lq0_jd = find_next_phase_jd(fm0_jd, 3);
        nm1_jd = find_next_phase_jd(lq0_jd, 0);
        if (!(nm0_jd < fq0_jd && fq0_jd < fm0_jd && fm0_jd < lq0_jd && lq0_jd < nm1_jd)) {
            fprintf(stderr, "Error: Sequential recalculation failed. Cannot determine phase for JD %.4f.\n", jd);
            return NEW_MOON; 
        }
    }
    
    double tolerance = 0.75; // Tolerance for primary phase names

    if (fabs(jd - nm0_jd) < tolerance || fabs(jd - nm1_jd) < tolerance) return NEW_MOON;
    if (fabs(jd - fq0_jd) < tolerance) return FIRST_QUARTER;
    if (fabs(jd - fm0_jd) < tolerance) return FULL_MOON;
    if (fabs(jd - lq0_jd) < tolerance) return LAST_QUARTER;

    if (jd > nm0_jd && jd < fq0_jd) return WAXING_CRESCENT;
    if (jd > fq0_jd && jd < fm0_jd) return WAXING_GIBBOUS;
    if (jd > fm0_jd && jd < lq0_jd) return WANING_GIBBOUS;
    if (jd > lq0_jd && jd < nm1_jd) return WANING_CRESCENT;

    fprintf(stderr, "Warning: Could not classify moon phase for JD %.4f. Boundaries: NM0=%.2f, FQ=%.2f, FM=%.2f, LQ=%.2f, NM1=%.2f. Returning New Moon.\n", 
            jd, nm0_jd, fq0_jd, fm0_jd, lq0_jd, nm1_jd);
    return NEW_MOON; 
}

/**
 * @brief Calculate the moon phase for a given Gregorian date
 */
MoonPhase calculate_moon_phase(int year, int month, int day) {
    double jd = gregorian_to_julian_day(year, month, day, 12.0); // Use noon UT
    return calculate_moon_phase_from_jd(jd);
}


// --- Core Lunar Calendar Logic (Based on New Rules) ---

/**
 * @brief Calculate the Julian Day (UT) of the start of the specified lunar year.
 */
double calculate_lunar_new_year_jd(int gregorian_year_of_start) {
    int ws_year = gregorian_year_of_start - 1;
    double ws_jd = calculate_solstice_equinox_jde(ws_year, 0);
    if (ws_jd == 0) {
        ws_jd = gregorian_to_julian_day(ws_year, 12, 21, 12.0); 
        fprintf(stderr, "Warning: Using approximate WS JD for year %d in New Year calc.\n", ws_year);
    }
    double first_nm_jd = find_next_phase_jd(ws_jd, 0);
    if (first_nm_jd == 0) {
         fprintf(stderr, "Error: Could not find New Moon after WS JD %.4f\\n", ws_jd);
         return 0; 
    }
    double first_fm_jd = find_next_phase_jd(first_nm_jd, 2);
     if (first_fm_jd == 0) {
         fprintf(stderr, "Error: Could not find Full Moon after NM JD %.4f\\n", first_nm_jd);
         return 0; 
    }
    return first_fm_jd;
}

/**
 * @brief Calculate the number of lunar months in a given lunar year.
 */
int get_lunar_months_in_year(int lunar_year_identifier) {
    double year_start_jd = calculate_lunar_new_year_jd(lunar_year_identifier);
    double next_year_start_jd = calculate_lunar_new_year_jd(lunar_year_identifier + 1);
    if (year_start_jd == 0 || next_year_start_jd == 0) {
        fprintf(stderr, "Error: Could not calculate New Year JDs for year id %d. Falling back to 12 months.\n", lunar_year_identifier);
        return 12; 
    }

    int full_moon_count = 0;
    double current_fm_jd = year_start_jd; 
    double epsilon = 1e-5;

    while (1) {
        current_fm_jd = find_next_phase_jd(current_fm_jd, 2);
        if (current_fm_jd == 0) { 
             fprintf(stderr, "Error: Failed finding next FM during month count for year %d.\n", lunar_year_identifier);
             return 12; // Fallback
        }
        // Check if the found FM is strictly before the next year starts
        if (current_fm_jd < next_year_start_jd - epsilon) {
            full_moon_count++;
        } else {
            break; // Found FM is on or after next year start
        }
    }
    return full_moon_count + 1; // Add 1 for the first month
}

/**
 * @brief Calculate if a given lunar year is a leap year (13 months)
 */
bool is_lunar_leap_year(int lunar_year_identifier) {
    return get_lunar_months_in_year(lunar_year_identifier) == 13;
}

/**
 * @brief Calculate the Eld Year based on the *Gregorian* year
 */
int calculate_eld_year_from_gregorian(int gregorian_year) {
    return gregorian_year + GERMANIC_EPOCH_BC;
}

/**
 * @brief Get the position of a *Lunar Year* (identified by its Gregorian start year) within the conceptual Metonic cycle
 */
void get_metonic_position(int lunar_year_identifier, int *metonic_year_pos, int *metonic_cycle_num) {
    int reference_year = lunar_year_identifier; 
    int year_since_1AD = reference_year - 1;
    if (year_since_1AD < 0) year_since_1AD = 0; // Basic handling for BC years
    
    *metonic_cycle_num = year_since_1AD / YEARS_PER_METONIC_CYCLE + 1;
    *metonic_year_pos = (year_since_1AD % YEARS_PER_METONIC_CYCLE) + 1;
}

// --- Main Conversion Functions ---

/**
 * @brief Convert a Gregorian date to its corresponding lunar date based on the new rules.
 */
LunarDay gregorian_to_lunar(int year, int month, int day) {
    LunarDay result = {0}; 
    result.greg_year = year;
    result.greg_month = month;
    result.greg_day = day;
    result.weekday = calculate_weekday(year, month, day);
    double target_jd = gregorian_to_julian_day(year, month, day, 12.0); 
    result.moon_phase = calculate_moon_phase_from_jd(target_jd);
    result.eld_year = calculate_eld_year_from_gregorian(year); 

    int lunar_year_id = year; // Initialize to default
    double year_start_jd_guess = calculate_lunar_new_year_jd(year);
    if (year_start_jd_guess == 0) goto conversion_error;
    double epsilon = 1e-5;

    if (target_jd < year_start_jd_guess - epsilon) { 
        lunar_year_id = year - 1;
    } else {
        double next_year_start_jd = calculate_lunar_new_year_jd(year + 1);
        if (next_year_start_jd == 0) goto conversion_error; 
        if (target_jd >= next_year_start_jd - epsilon) { 
            lunar_year_id = year + 1;
        } else {
            lunar_year_id = year; 
        }
    }
    result.lunar_year = lunar_year_id;

    double year_start_jd = calculate_lunar_new_year_jd(lunar_year_id);
    if (year_start_jd == 0) goto conversion_error;

    int lunar_month_num = 1;
    double month_start_jd = year_start_jd; 
    double next_month_start_jd;
    int months_in_this_year = get_lunar_months_in_year(lunar_year_id);

    while (lunar_month_num <= months_in_this_year) {
        next_month_start_jd = find_next_phase_jd(month_start_jd, 2); 
        if (next_month_start_jd == 0) goto conversion_error;
        
        if (target_jd >= month_start_jd - epsilon && target_jd < next_month_start_jd - epsilon) {
            result.lunar_month = lunar_month_num;
            result.lunar_day = (int)floor(target_jd - month_start_jd) + 1; 
            goto conversion_success; 
        }
        
        month_start_jd = next_month_start_jd;
        lunar_month_num++;
    }
    
    fprintf(stderr, "Error in gregorian_to_lunar: Could not place JD %.4f within year %d.\n", 
            target_jd, lunar_year_id);

conversion_error:
    // Indicate error by setting lunar day/month to 0 or similar?
    result.lunar_day = 0; 
    result.lunar_month = 0;
    fprintf(stderr, "Returning invalid LunarDay due to conversion error.\n");
conversion_success:
    get_metonic_position(lunar_year_id, &result.metonic_year, &result.metonic_cycle);
    return result;
}

/**
 * @brief Convert a lunar date (year, month, day based on new rules) to a Gregorian date.
 */
bool lunar_to_gregorian(int lunar_year_id, int lunar_month, int lunar_day, 
                        int *greg_year, int *greg_month, int *greg_day) {

    int months_in_year = get_lunar_months_in_year(lunar_year_id);
    if (lunar_month < 1 || lunar_month > months_in_year || lunar_day < 1) {
        fprintf(stderr, "Error: Invalid lunar date input %d/%d/%d (year has %d months).\n", 
                lunar_year_id, lunar_month, lunar_day, months_in_year);
        return false;
    }
    
    double year_start_jd = calculate_lunar_new_year_jd(lunar_year_id);
    if (year_start_jd == 0) return false;
    
    double month_start_jd = year_start_jd;
    for (int m = 1; m < lunar_month; m++) {
        month_start_jd = find_next_phase_jd(month_start_jd, 2); 
        if (month_start_jd == 0) return false;
    }

    double target_jd = month_start_jd + (double)(lunar_day - 1);
    double epsilon = 1e-5;

    // Validate Day Number within Month
    double next_month_start_jd = find_next_phase_jd(month_start_jd, 2);
    if (next_month_start_jd == 0) return false;
    int month_length = (int)floor(next_month_start_jd - month_start_jd + 0.5); // Round
    if (lunar_day > month_length) {
         fprintf(stderr, "Error: Lunar day %d invalid for month %d/%d (length %d days).\n",
                 lunar_day, lunar_month, lunar_year_id, month_length);
         return false; 
    }
    if (target_jd >= next_month_start_jd - epsilon) { 
         fprintf(stderr, "Error: Calculated target JD %.4f >= next month start JD %.4f.\n",
                 target_jd, next_month_start_jd);
         return false;
    }

    double hour_unused;
    julian_day_to_gregorian(target_jd, greg_year, greg_month, greg_day, &hour_unused);
    
    return true;
}

// --- Utility Functions ---
/**
 * @brief Get the lunar date for today
 */
LunarDay get_today_lunar_date(void) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    return gregorian_to_lunar(tm_now->tm_year + 1900, 
                             tm_now->tm_mon + 1, 
                             tm_now->tm_mday);
}

// --- Removed / Obsolete Code Stubs ---
/* Stubs from previous logic are removed */

// --- End of File --- 