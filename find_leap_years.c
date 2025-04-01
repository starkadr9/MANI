#include <stdio.h>
#include <stdlib.h>
#include "include/lunar_calendar.h"

int main(int argc, char **argv) {
    int start_year = 2020;
    int end_year = 2030;
    
    if (argc > 1) {
        start_year = atoi(argv[1]);
    }
    
    if (argc > 2) {
        end_year = atoi(argv[2]);
    }
    
    printf("Checking for leap years (13 months) between %d and %d:\n", start_year, end_year);
    printf("Year\tMetonic Year\tMonths\tLeap?\n");
    printf("----\t------------\t------\t-----\n");
    
    for (int year = start_year; year <= end_year; year++) {
        int metonic_year, metonic_cycle;
        get_metonic_position(year, 1, 1, &metonic_year, &metonic_cycle);
        
        int month_count = count_lunar_months_in_year(year);
        bool is_leap = is_lunar_leap_year(year);
        
        printf("%d\t%d\t\t%d\t%s\n", 
               year, 
               metonic_year, 
               month_count, 
               is_leap ? "YES" : "NO");
    }
    
    return 0;
} 