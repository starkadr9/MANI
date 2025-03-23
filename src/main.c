#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/lunar_calendar.h"
#include "../include/lunar_renderer.h"

/* Function to print the moon phase name */
const char* get_moon_phase_name(MoonPhase phase) {
    switch (phase) {
        case NEW_MOON: return "New Moon";
        case WAXING_CRESCENT: return "Waxing Crescent";
        case FIRST_QUARTER: return "First Quarter";
        case WAXING_GIBBOUS: return "Waxing Gibbous";
        case FULL_MOON: return "Full Moon";
        case WANING_GIBBOUS: return "Waning Gibbous";
        case LAST_QUARTER: return "Last Quarter";
        case WANING_CRESCENT: return "Waning Crescent";
        default: return "Unknown";
    }
}

/* Function to print the weekday name */
const char* get_weekday_name(Weekday weekday) {
    switch (weekday) {
        case SUNDAY: return "Sunday";
        case MONDAY: return "Monday";
        case TUESDAY: return "Tuesday";
        case WEDNESDAY: return "Wednesday";
        case THURSDAY: return "Thursday";
        case FRIDAY: return "Friday";
        case SATURDAY: return "Saturday";
        default: return "Unknown";
    }
}

/* Display help information */
void display_help() {
    printf("Lunar Calendar - Metonic Cycle Calculator\n");
    printf("------------------------------------------\n");
    printf("Commands:\n");
    printf("  today              - Display lunar date for today\n");
    printf("  g2l YYYY MM DD     - Convert Gregorian date to lunar date\n");
    printf("  l2g YYYY MM DD     - Convert lunar date to Gregorian date\n");
    printf("  phase YYYY MM DD   - Show moon phase for Gregorian date\n");
    printf("  eld YYYY           - Calculate Germanic Eld year for Gregorian year\n");
    printf("  cycle YYYY         - Display Metonic cycle starting from year YYYY\n");
    printf("  weekday YYYY MM DD - Calculate weekday for given date\n");
    printf("  newmoon YYYY MM    - Find new moon in given month\n");
    printf("  fullmoon YYYY MM   - Find full moon in given month\n");
    printf("  germanic_new_year YYYY - Calculate Germanic New Year for given year\n");
    printf("  mpos YYYY MM DD    - Get Metonic position for given date\n");
    printf("  month_length YYYY MM - Calculate lunar month length\n");
    printf("  seasons YYYY       - Display solstices and equinoxes for given year\n");
    printf("  help               - Display this help information\n");
    printf("  quit               - Exit the program\n");
    printf("\n");
    printf("Rendering Commands:\n");
    printf("  render_month YYYY MM - Render a lunar month calendar\n");
    printf("  render_year YYYY     - Render a full lunar year calendar\n");
    printf("  render_cycle YYYY    - Render the Metonic cycle position\n");
}

/* Process a command and its arguments */
void process_command(const char* command) {
    int year, month, day;
    
    if (strcmp(command, "help") == 0) {
        display_help();
    } 
    else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
        exit(0);
    }
    else if (strcmp(command, "today") == 0) {
        LunarDay today = get_today_lunar_date();
        printf("Today's Gregorian date: %04d-%02d-%02d\n", 
               today.greg_year, today.greg_month, today.greg_day);
        printf("Today's Lunar date: Year %d, Month %d, Day %d\n", 
               today.lunar_year, today.lunar_month, today.lunar_day);
        printf("Weekday: %s\n", get_weekday_name(today.weekday));
        printf("Moon phase: %s\n", get_moon_phase_name(today.moon_phase));
        printf("Germanic Eld year: %d\n", today.eld_year);
        printf("Position in Metonic cycle: Year %d of Cycle %d\n", 
               today.metonic_year, today.metonic_cycle);
    }
    else if (strncmp(command, "g2l ", 4) == 0) {
        if (sscanf(command + 4, "%d %d %d", &year, &month, &day) == 3) {
            LunarDay result = gregorian_to_lunar(year, month, day);
            printf("Gregorian date: %04d-%02d-%02d\n", year, month, day);
            printf("Lunar date: Year %d, Month %d, Day %d\n", 
                   result.lunar_year, result.lunar_month, result.lunar_day);
            printf("Weekday: %s\n", get_weekday_name(result.weekday));
            printf("Moon phase: %s\n", get_moon_phase_name(result.moon_phase));
            printf("Germanic Eld year: %d\n", result.eld_year);
            printf("Position in Metonic cycle: Year %d of Cycle %d\n", 
                   result.metonic_year, result.metonic_cycle);
        } else {
            printf("Error: Invalid format. Use 'g2l YYYY MM DD'\n");
        }
    }
    else if (strncmp(command, "l2g ", 4) == 0) {
        if (sscanf(command + 4, "%d %d %d", &year, &month, &day) == 3) {
            int greg_year, greg_month, greg_day;
            if (lunar_to_gregorian(year, month, day, &greg_year, &greg_month, &greg_day)) {
                printf("Lunar date: Year %d, Month %d, Day %d\n", year, month, day);
                printf("Gregorian date: %04d-%02d-%02d\n", greg_year, greg_month, greg_day);
                Weekday weekday = calculate_weekday(greg_year, greg_month, greg_day);
                printf("Weekday: %s\n", get_weekday_name(weekday));
                printf("Moon phase: %s\n", 
                       get_moon_phase_name(calculate_moon_phase(greg_year, greg_month, greg_day)));
                printf("Germanic Eld year: %d\n", calculate_eld_year(greg_year));
                
                int metonic_year, metonic_cycle;
                get_metonic_position(greg_year, greg_month, greg_day, &metonic_year, &metonic_cycle);
                printf("Position in Metonic cycle: Year %d of Cycle %d\n", 
                       metonic_year, metonic_cycle);
            } else {
                printf("Error: Invalid lunar date\n");
            }
        } else {
            printf("Error: Invalid format. Use 'l2g YYYY MM DD'\n");
        }
    }
    else if (strncmp(command, "phase ", 6) == 0) {
        /* Try to parse multiple date formats */
        int result;
        /* Try parsing dash-separated format: YYYY-MM-DD */
        result = sscanf(command + 6, "%d-%d-%d", &year, &month, &day);
        if (result != 3) {
            /* Try parsing space-separated format: YYYY MM DD */
            result = sscanf(command + 6, "%d %d %d", &year, &month, &day);
        }
        
        if (result == 3) {
            MoonPhase phase = calculate_moon_phase(year, month, day);
            printf("Moon phase on %04d-%02d-%02d: %s\n", 
                   year, month, day, get_moon_phase_name(phase));
        } else {
            printf("Error: Invalid format. Use 'phase YYYY-MM-DD' or 'phase YYYY MM DD'\n");
        }
    }
    else if (strncmp(command, "eld ", 4) == 0) {
        if (sscanf(command + 4, "%d", &year) == 1) {
            int eld_year = calculate_eld_year(year);
            printf("Germanic Eld year for %d CE: %d\n", year, eld_year);
        } else {
            printf("Error: Invalid format. Use 'eld YYYY'\n");
        }
    }
    else if (strncmp(command, "cycle ", 6) == 0) {
        if (sscanf(command + 6, "%d", &year) == 1) {
            MetonicCycle cycle = initialize_metonic_cycle(year);
            printf("Metonic Cycle #%d starting from year %d:\n", 
                   cycle.cycle_number, year);
            printf("Year\tPosition\tMonths\tDays\tLeap?\tGermanic New Year\n");
            
            for (int i = 0; i < YEARS_PER_METONIC_CYCLE; i++) {
                LunarYear ly = cycle.years[i];
                printf("%d\t%d\t\t%d\t%d\t%s\t%02d-%02d\n", 
                       ly.year, ly.metonic_year, ly.months_count, ly.days_count,
                       (ly.months_count == 13) ? "Yes" : "No",
                       ly.germanic_start_greg_month, ly.germanic_start_greg_day);
            }
        } else {
            printf("Error: Invalid format. Use 'cycle YYYY'\n");
        }
    }
    else if (strncmp(command, "weekday ", 8) == 0) {
        if (sscanf(command + 8, "%d %d %d", &year, &month, &day) == 3) {
            Weekday weekday = calculate_weekday(year, month, day);
            printf("Weekday for %04d-%02d-%02d: %s\n", 
                   year, month, day, get_weekday_name(weekday));
        } else {
            printf("Error: Invalid format. Use 'weekday YYYY MM DD'\n");
        }
    }
    else if (strncmp(command, "newmoon ", 8) == 0) {
        if (sscanf(command + 8, "%d %d", &year, &month) == 2) {
            int new_moon_day;
            double new_moon_hour;
            if (calculate_new_moon(year, month, &new_moon_day, &new_moon_hour)) {
                int hour = (int)new_moon_hour;
                int minute = (int)((new_moon_hour - hour) * 60);
                printf("New moon in %04d-%02d: Day %d at ~%02d:%02d\n", 
                       year, month, new_moon_day, hour, minute);
            } else {
                printf("Error: Could not calculate new moon for %04d-%02d\n", year, month);
            }
        } else {
            printf("Error: Invalid format. Use 'newmoon YYYY MM'\n");
        }
    }
    else if (strncmp(command, "fullmoon ", 9) == 0) {
        if (sscanf(command + 9, "%d %d", &year, &month) == 2) {
            int full_moon_day;
            double full_moon_hour;
            if (calculate_full_moon(year, month, &full_moon_day, &full_moon_hour)) {
                int hour = (int)full_moon_hour;
                int minute = (int)((full_moon_hour - hour) * 60);
                printf("Full moon in %04d-%02d: Day %d at ~%02d:%02d\n", 
                       year, month, full_moon_day, hour, minute);
            } else {
                printf("Error: Could not calculate full moon for %04d-%02d\n", year, month);
            }
        } else {
            printf("Error: Invalid format. Use 'fullmoon YYYY MM'\n");
        }
    }
    else if (strncmp(command, "germanic_new_year ", 18) == 0) {
        if (sscanf(command + 18, "%d", &year) == 1) {
            int month, day;
            if (calculate_germanic_new_year(year, &month, &day)) {
                printf("Germanic New Year for %d: %04d-%02d-%02d\n", 
                       year, year, month, day);
                
                /* Show moon phase at Germanic New Year */
                MoonPhase phase = calculate_moon_phase(year, month, day);
                printf("Moon phase: %s\n", get_moon_phase_name(phase));
                
                /* Show weekday */
                Weekday weekday = calculate_weekday(year, month, day);
                printf("Weekday: %s\n", get_weekday_name(weekday));
                
                /* Show Eld year */
                printf("Germanic Eld year: %d\n", calculate_eld_year(year));
            } else {
                printf("Error: Could not calculate Germanic New Year for %d\n", year);
            }
        } else {
            printf("Error: Invalid format. Use 'germanic_new_year YYYY'\n");
        }
    }
    else if (strncmp(command, "mpos ", 5) == 0) {
        if (sscanf(command + 5, "%d %d %d", &year, &month, &day) == 3) {
            int metonic_year, metonic_cycle;
            get_metonic_position(year, month, day, &metonic_year, &metonic_cycle);
            printf("Date %04d-%02d-%02d is in:\n", year, month, day);
            printf("Metonic Year: %d\n", metonic_year);
            printf("Metonic Cycle: %d\n", metonic_cycle);
            printf("Lunar Leap Year: %s\n", is_lunar_leap_year(year) ? "Yes" : "No");
        } else {
            printf("Error: Invalid format. Use 'mpos YYYY MM DD'\n");
        }
    }
    else if (strncmp(command, "month_length ", 13) == 0) {
        if (sscanf(command + 13, "%d %d", &year, &month) == 2) {
            int length = calculate_lunar_month_length(year, month);
            printf("Lunar month %d in year %d has %d days\n", month, year, length);
        } else {
            printf("Error: Invalid format. Use 'month_length YYYY MM'\n");
        }
    }
    else if (strncmp(command, "seasons ", 8) == 0) {
        if (sscanf(command + 8, "%d", &year) == 1) {
            int month, day;
            
            printf("Astronomical seasons for year %d:\n", year);
            printf("-------------------------------\n");
            
            /* Winter Solstice */
            if (calculate_winter_solstice(year, &month, &day)) {
                printf("Winter Solstice: %04d-%02d-%02d\n", year, month, day);
                Weekday wd = calculate_weekday(year, month, day);
                printf("                 %s\n", get_weekday_name(wd));
            }
            
            /* Spring Equinox */
            if (calculate_spring_equinox(year, &month, &day)) {
                printf("Spring Equinox:  %04d-%02d-%02d\n", year, month, day);
                Weekday wd = calculate_weekday(year, month, day);
                printf("                 %s\n", get_weekday_name(wd));
            }
            
            /* Summer Solstice */
            if (calculate_summer_solstice(year, &month, &day)) {
                printf("Summer Solstice: %04d-%02d-%02d\n", year, month, day);
                Weekday wd = calculate_weekday(year, month, day);
                printf("                 %s\n", get_weekday_name(wd));
            }
            
            /* Fall Equinox */
            if (calculate_fall_equinox(year, &month, &day)) {
                printf("Fall Equinox:    %04d-%02d-%02d\n", year, month, day);
                Weekday wd = calculate_weekday(year, month, day);
                printf("                 %s\n", get_weekday_name(wd));
            }
        } else {
            printf("Error: Invalid format. Use 'seasons YYYY'\n");
        }
    }
    else if (strncmp(command, "render_month ", 13) == 0) {
        if (sscanf(command + 13, "%d %d", &year, &month) == 2) {
            RenderOptions options = default_render_options();
            RenderedMonth rendered = render_lunar_month(year, month, options);
            if (rendered.buffer) {
                display_rendered_month(rendered);
                free_rendered_month(&rendered);
            } else {
                printf("Error: Could not render lunar month\n");
            }
        } else {
            printf("Error: Invalid format. Use 'render_month YYYY MM'\n");
        }
    }
    else if (strncmp(command, "render_year ", 12) == 0) {
        if (sscanf(command + 12, "%d", &year) == 1) {
            RenderOptions options = default_render_options();
            RenderedYear rendered = render_lunar_year(year, options);
            if (rendered.buffer) {
                display_rendered_year(rendered);
                free_rendered_year(&rendered);
            } else {
                printf("Error: Could not render lunar year\n");
            }
        } else {
            printf("Error: Invalid format. Use 'render_year YYYY'\n");
        }
    }
    else if (strncmp(command, "render_cycle ", 13) == 0) {
        if (sscanf(command + 13, "%d", &year) == 1) {
            RenderOptions options = default_render_options();
            char *position_text = render_metonic_cycle_position(year, options);
            if (position_text) {
                display_metonic_cycle_position(position_text);
                free(position_text);
            } else {
                printf("Error: Could not render Metonic cycle position\n");
            }
        } else {
            printf("Error: Invalid format. Use 'render_cycle YYYY'\n");
        }
    }
    else {
        printf("Unknown command. Type 'help' for available commands.\n");
    }
}

int main(int argc, char *argv[]) {
    char command[256];
    
    printf("Lunar Calendar - Metonic Cycle Calculator\n");
    printf("Type 'help' for available commands\n\n");
    
    /* If arguments were provided, process them as a command */
    if (argc > 1) {
        /* First, check for single commands like "today" */
        if (argc == 2) {
            process_command(argv[1]);
            return 0;
        }
        
        /* Handle commands with arguments (g2l, l2g, etc.) */
        if (argc >= 3) {
            /* Create command string */
            command[0] = '\0';
            strcat(command, argv[1]);
            strcat(command, " ");
            
            for (int i = 2; i < argc; i++) {
                strcat(command, argv[i]);
                if (i < argc - 1) {
                    strcat(command, " ");
                }
            }
            
            process_command(command);
            return 0;
        }
    }
    
    /* Interactive mode */
    while (1) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        /* Remove trailing newline */
        command[strcspn(command, "\n")] = '\0';
        
        /* Process the command */
        process_command(command);
        
        printf("\n");
    }
    
    return 0;
} 