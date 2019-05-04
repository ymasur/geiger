/*
 Geiger.cpp
 ----------
 02.04.2019 - ymasur@microclub.ch
  
 Geiger WEB interface and record.

 This program serves datas from Geiger detector  
 via the Arduino Yun's built-in webserver using the Bridge library.

 The circuit:
 * Input pin 6 selects the FAST (open) or SLOW (low) mode
 * INput pin 7 receives pulses of Geiger detectors platine
 * Output pin 8 give pulse for a LED
 * Display I2C attached to pin SCL/SDA of Yun
 * RTC DS3231 attached to I2C
 * USB stick attached to USB slot of the Arduino Yun
 
 The USB stick must have an empty folder in the root 
 named "arduino" and a subfolder of that named "www". 
 Ex. on a PC with USB: f:\arduino\www
 This will ensure that the Yun will create a link 
 to the USB to the "/mnt/sd" path.

 The system is writing to it.

 Pages are obtainable with the URL: http://geiger.home/sd/index.php

 */
#ifndef MAIN  // this is the main module
#define MAIN

#define __YUN   // useful to isolate YUN specific functions 
#define __FILE  // same, for files operations
#define DEBUG 1 // level 0 - 1 - 2

#define __PROG__ "Geiger_Yun_LCD"
#define VERSION "0.71d" // Module version
#ifdef __YUN
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>
#endif
#ifdef __FILE
#include <FileIO.h>
#endif
#include <string.h>
#include <Wire.h>
#include <time.h>
#include <RTClib.h>
#include <jm_Scheduler.h>
#include <jm_LCM2004_I2C.h>
#include <jm_LCM2004_I2C_Plus.h>

#include "Geiger.hpp"
#include "Geiger_counter.hpp"

// instance of LCD
jm_LCM2004_I2C_Plus lcd(0x3F);


// list of scheduler used
jm_Scheduler geiger_LCD;
jm_Scheduler clock_scheduler;
jm_Scheduler pulse_500ms;
jm_Scheduler pulse_1s;
jm_Scheduler led_monostable;

//only without hardware RTC
//#define RTC_MILLIS
//RTC_Millis rtc; //only without hardware RTC 
RTC_DS3231 rtc;
DateTime myTime;
bool timeFirstSync = false;

#ifdef __YUN
//YunServer server;
BridgeServer server;
//YunClient client;
BridgeClient client;
#endif

/* blink(short n=1, short t=1)
   ---------------------------
   Blink n times, with impuls cycle t (1/10 sec)
   I/O used: LED13
   return: -
*/
void blink(short n=1, short t=1)
{
  for (short i=0; i<n; i++)
  {
    digitalWrite(LED13, LOW);
    delay(50 * t);
    digitalWrite(LED13, HIGH);
    delay(50 * t);
  }
}

void setup()
{
  // initialize the digital pin for LEDs as an output.
  pinMode(LED13, OUTPUT);
  digitalWrite(LED13, LOW);
  pinMode(LED_Y, OUTPUT);
  digitalWrite(LED_Y, LOW);
  pinMode(FAST_IN, INPUT_PULLUP);

  // I2C
  Wire.begin();
  // LCD
  lcd.begin();  lcd.clear_display();
  display_info("LCD init done..."); delay(1000);
  counter.clear_all();
  //Init Geiger counter and IRQ input. From this point, it counts!
  attachInterrupt(digitalPinToInterrupt(PULSE_IRQ), irq_func, FALLING);
  fl_fast = true;   // set default mode of record to fast (pin open = 1)
  display_info("IRQ started...");
  blink(10,2);  // blink 10x LED 13 200 ms
    
  // setup path/filename for store
  // char position in the path string
  //                       1         2         3
  //             012345678901234567890123456789012
  strncpy(fname, "/mnt/sd/arduino/www/cntgdata.txt", NAME_LENGHT);

 // we use serial for log messages
  Serial.begin(9600);
  display_info("Serial started...");
//  while (!Serial); // wait for USB Serial ready
  blink(8,10); //LED13 blink 8 pulses, 1 sec
 
#ifdef __YUN
  display_info("Yun Bridge started  ");
  //Bridge startup
  Bridge.begin();

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
  display_info("Yun SD card access  ");
  // SD card access
  FileSystem.begin();
#endif    //   012345678901234567890
  display_info("Start polling loops ");
  // start polling loops
  pulse_500ms.start(poll_loop_5, 1000L * 500);

  // RTC, start
  display_info(  "Start RTC, read.... ");
  timeFirstSync = true;
  if (! rtc.begin()) 
  {
    log_msg("Couldn't find RTC");
    blink(30,1);  // pulse LED13 6.0 sec at 0.2 Hz 
  }

  if (rtc.lostPower()) 
  {       //012345678901234567890
    log_msg("RTC lost power!     ");
  }

  myTime = rtc.now(); // take the RTC time  
  dateTime_up_ascii();  
  log_msg(__PROG__ " " VERSION "\n");

} // End setup()

/*  main loop
 *  ---------
 */
void loop()
{
  jm_Scheduler::cycle();
  webArduino();
} // end loop()

/*  dateTime_up_ascii(void)
    -----------------------
    Write the value of date & time
    Global var modified:
    - dateTimeStr, with the new value
    Global var used:
    - myTime
*/
void dateTime_up_ascii(void)
{
  snprintf(dateTimeStr, 32, "%04d-%02d-%02d %02d:%02d:%02d",
           myTime.year(),
           myTime.month(),
           myTime.day(),
           myTime.hour(),
           myTime.minute(),
           myTime.second()
           );
}

// Check speed of record
void check_speed()
{
  if (fl_fast != digitalRead(FAST_IN))
  {
    const char *msg;

    fl_fast = digitalRead(FAST_IN);// 
    msg = fl_fast ? "Record mode is FAST " : "Record mode is SLOW ";
    log_msg(msg);
  }
}

// Running by schedule every 500 ms
static byte reentry0 = 0;

void poll_loop_5()
{
  static byte old_sec;

  if (++reentry0 > 1) // check against reentry1 of the 
  {
    log_msg("RENTRY Alarm - poll_loop");
    return;
  };
  
  check_speed();
  myTime = rtc.now(); // take the actual time  
  if (myTime.second() == old_sec)
  {
    digitalWrite(LED13, 0); // phase OFF
  }
  else
  {
    old_sec = myTime.second(); // take las value
    digitalWrite(LED13, 1); // phase ON
    poll_loop1();
  }
  reentry0--;
} //end poll loop 0.5 sec

// running by phase of poll_loop_5(), each second
void poll_loop1()
{

  dateTime_up_ascii();  // update the ASCII version of time
  geiger_display_counts();  // On LCD
  geiger_print_counts();    // On serial
  if (fl_fast == true && (myTime.second() % 10) == 0) // fast to be stored?
  {
    storeCounts(fname, dateTimeStr, 'F'); // in FAST recipent
    counter.clear_fast();
  }

  if (myTime.second() == 0)  //Q: is the time at hh:mm:00 ?
  { //A: yes, 
    counter.upd_live_to_min();//update live counter to minute counter

    if (myTime.minute() == 0) //Q: is the time at hh:00:00
    { //A: yes, 
      storeCounts(fname, dateTimeStr);  // store the hh:00:00 value
      counter.upd_min_to_hour();  // add min counter to hour counter

      if (myTime.hour() == 0)//Q: is the time at 00:00:00
      { //A: yes update hour counter to day counter
        counter.upd_hour_to_day();
      } // 00:00:00  

      if (myTime.hour() == 3)//Q: is the time at 03:00:00
      { //A: yes synchronyse RTC to NTP
        timeSync();
      }
    } // hh:00:00
      // timeSync(); // test: each minute
  } // HH:mm:00
}// end poll loop 1.0 sec

// LCD display info (line 1)
void display_info(String info)
{
  lcd.set_cursor(0, 0); // column, line
  if (info.length() == 0)
    lcd.print("Geiger counter " VERSION);
  else
  {
    lcd.print(info);
  }
}

// Display date, time and counts (lines 2, 3, 4)
void geiger_display_counts()
{
  lcd.set_cursor(0, 1); // column, line
  lcd.print(dateTimeStr);
  lcd.set_cursor(0, 2);

  if (fl_fast)
  {
    lcd.print_u16(counter.get_live_fast(), 5);
    lcd.print_u16(counter.get_live(), 5);
    lcd.print_u16(counter.get_last_min(), 10);
    lcd.set_cursor(0, 3);
    lcd.print_u32(counter.get_min());
    lcd.print_u32(counter.get_hour());
  }
  else
  {
    lcd.print_u32(counter.get_live());
    lcd.print_u32(counter.get_min());
    lcd.set_cursor(0, 3);
    lcd.print_u32(counter.get_hour());
    lcd.print_u32(counter.get_day());
  }

} // geiger_display_counts()

void geiger_print_counts()
{
  Serial.print(dateTimeStr);
  if (fl_fast)
    Serial.print("\t (live, last fast, hour, day)\t");
  else
    Serial.print("\t (live, last min, hour, day)\t");
  Serial.print(counter.get_live());
  Serial.print(", ");

  if (fl_fast)
    Serial.print(counter.get_last_fast());
  else
    Serial.print(counter.get_last_min());
  
  Serial.print(", ");
  Serial.print(counter.get_min());  // so, running hour
  Serial.print(", ");
  Serial.print(counter.get_hour()); // so, running day
  Serial.println();
} // geiger_print_counts()


/*
 * webArduino()
 * ------------
 * servicing WEB page if needed
 */
void webArduino()
{
#ifdef __YUN  
  // Is there a new client?
  client = server.accept();
  if (client)
  {
    // read the command
    String command = client.readString();
    command.trim(); //kill whitespace
                    //    Serial.println(command);
    // is "counts" command?
    if (command == "counts")
    {
      // print the counter:
      client.print("Date:\t");
      client.println(dateTimeStr);
      client.print("Counts live:\t");
      client.print(counter.get_live());
      client.print(" last hour:");
      client.print(counter.get_hour());
      // Add general infos
      client.print(__PROG__ " V " VERSION);
      // Show if recording is ready, or not:
      if (errFile)
        client.print("Fichier SD en erreur: ");
      else
        client.print("Fichier SD alimente: ");
      client.println(fname);
    }
    // Close connection and free resources.
    client.stop();
  }
#endif  
}

/*  getTimeStamp(char *p, short len)
    --------------------------------
    This function fill a string with the time stamp
    Vars used:
    - *p : pointer to the destination str
    - len: max number of char (with final 0)
    
    returned value:
    - number of chr written
*/
static byte reentry1 = 0;

int getTimeStamp(char *p, short len)
{
  if (reentry1)
  {
    log_msg("ALERT reentry getTimeStamp()");
    return 0;
  }
    
  ++reentry1;
  short i = 0;
  char c;
  #ifdef __YUN
  log_msg("Get NTP time... ");
  Process time;
  // date is a command line utility to get the date and the time
  // in different formats depending on the additional parameter
  time.begin("date");
  // parameters: for the complete date yy-mm-dd hh:mm:ss
  time.addParameter("+%y-%m-%d %T");
  time.run(); // run the command

  // read the output of the command
  while (time.available() > 0 && i < len)
  {
    c = time.read();
    if (c != '\n')
      p[i] = c;
    i++;
  }
  p[i] = '\0'; // end of the collected string
  #endif

  reentry1--;  
  return i;
}

// take the Unix time of the Yun
#define TIME_MSG_LEN 20

/*  void timeSync()
    ---------------
    The RTC can be (re)synchronised by the Unix ntp.
    First, call the time stamp of the Yun
    The first call take caution of the Unix time: it must be in the futur.
    This is not the case if the Yun start ans get no Internet connection.
    Var used:
    - timeFirstSync, as flag for first call
    - rtc, corrected with the ntp source of time

    return value: -
*/
void timeSync()
{
  char ntpStr[TIME_MSG_LEN+2];  //string to store the ntp time
  long sec_correction = 0L;     // sign + is to advance our time
  //                         0123456789012345678901234567890
  char str_correction[32] = "T Correction: 000000000      \n";

  getTimeStamp(ntpStr, TIME_MSG_LEN);
  // convert ascii time to a a DateTime
  // 01234567890123456
  // yy-mm-dd hh:mm:ss
  
  DateTime ntp_time(
    atoi(ntpStr),         // yy
    atoi(ntpStr + 3),     // mm
    atoi(ntpStr + 6),     // dd
    atoi(ntpStr + 9),     // hh
    atoi(ntpStr + 12),    // mm
    atoi(ntpStr + 15) );  // ss

#if DEBUG && DEBUG > 0
  Serial.print("ntp time:"); Serial.println(ntp_time.unixtime());
  Serial.print("my  time:"); Serial.println(myTime.unixtime());
#endif
  sec_correction = ntp_time.unixtime() - myTime.unixtime();
  // set text for log with the correction value 
#if DEBUG && DEBUG > 0
  ltoa(sec_correction, str_correction+14, 10);
  log_msg(str_correction);
#endif  
  // then, use only positive value 
  if (sec_correction < 0L) 
      sec_correction = -sec_correction;

  if (ntp_time.year() < 2019 )
    {
#if DEBUG && DEBUG > 0
      log_msg("NTP oldest as compile time, no time correction done");
#endif  
      return;      
    }
   
  timeFirstSync = false;

  if (sec_correction < MIN_CORRECTION)
  {
#if DEBUG > 1   
    log_msg("Too little, no time correction done");
#endif    
    return;
  }
  
  rtc.adjust(ntp_time);   // use ntp time to adjust RTC
  getTimeStamp(ntpStr, TIME_MSG_LEN);  // reload ASCII version
  log_msg("NTP used");
}

// Arrête le témoin LED_Y
void Led_Y_mono_stop()
{
  digitalWrite(LED_Y, LOW);
}

// Démarre un monostable sur témoin LED_Y
void Led_mono_start()
{
  digitalWrite(LED_Y, HIGH);
  led_monostable.start(Led_Y_mono_stop, timestamp_read() + 10000L, 0);
}

// called by IRQ
void irq_func()
{
  if (!digitalRead(LED_Y))
    Led_mono_start();

  counter.add_live();
}

#endif  // MAIN process
