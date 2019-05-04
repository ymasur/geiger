/*
 Geiger.hpp
 ----------
 31.03.2019 - ymasur@microclub.ch

*/
#include <Arduino.h>
#ifndef GEIGER_HPP
#define GEIGER_HPP

#ifdef MAIN
  #define CLASS
#else
  #define CLASS extern
#endif

// IO pin are defined here
#define LED13 13    // LED red must be connected to pin 1
#define LED_Y 8     // IRQ toggle LED, by each pulse gived on interrupt pin
#define PULSE_IRQ 7 // Micro Leonardo as 32u4-based 0, 1, 2, 3, 7 available
#define FAST_IN 6   // Input: 1 = FAST, 0 = SLOW

// time given by Unix cmd              1         2
//                           012345678901234567890
// asci format, as          "2019-08-29 22:10:42"
#define DT_LENGHT 20
CLASS char dateTimeStr[DT_LENGHT+1]; 

#define NAME_LENGHT 32  // sufficient for path/name
CLASS char fname[NAME_LENGHT+1];
//the four chars in fname 'cntg' are replaced by year and month, as 1902
#define OFFSET_YYMM 20 //offset used to modify the filename

// maximum offset of RTC to be corrected with Unix time (NTP)
#define MAX_CORRECTION (3600*2)
// minimum offset of time in second, for RTC adjust
#define MIN_CORRECTION 3

// global vars are defined here

CLASS bool stored;
CLASS bool fl_fast; // true = record each 10 sec.
CLASS bool min_upd;
CLASS bool hour_upd;
CLASS bool day_upd;
CLASS byte errFile;

// prototypes (needed for VSCode/PlateformIO)
void setup();
void poll_loop1();
void poll_loop_5();
bool IsSyncTime_ss();
bool IsSyncTime_mm();
bool IsSyncTime_hh();
void display_info(String info);
void geiger_display_counts();
void geiger_print_counts();
void irq_func();
void storeCounts(char *fname, char *dateTimeStr, char mode = '\0');
void dateTime_up_ascii();
void log_msg(String msg);
void webArduino();
void timeSync();

#endif // GEIGER_HPP