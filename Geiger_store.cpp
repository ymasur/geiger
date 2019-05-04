/*
 Geiger_store.cpp
 ----------------
 09.04.2019 - ymasur@microclub.ch

 Module store can write datas onto SD/USB storage file
*/
#include <Arduino.h>
#include <FileIO.h>
#include <string.h>
#include "Geiger.hpp"
#include "Geiger_counter.hpp"

#define RETRY 3 //number of retry in write to file (0: mean 1)
#define RETRY_DELAY 1 // nb of millisecond between write try

/*  StoreCounts(char *fname, char *dateTimeStr, char mode='\0')
    ----------------------------------------------------------
    Store the actual counts in a file on SD card
    Global vars used:
    - char[] fname, contain the full path
    - char[] dateTimeStr, that contain the actual date and time
                          01234567890123456789
      format should be : "yyyy-mm-dd hh:mm:ss"

    Modified global vars:
    - fname, the filename in a 8.3 format. 4 first char are modified
    - errFile: false if OK; then true if an error occures
*/

void storeCounts(char *fname, char *dateTimeStr, char mode)
{
  short i = 0;
  // open the file.
  // The FileSystem card is mounted at the following "/mnt/SD" and
  // create the name with year and month on 4 digits
  fname[OFFSET_YYMM + 0] = dateTimeStr[2];
  fname[OFFSET_YYMM + 1] = dateTimeStr[3];
  fname[OFFSET_YYMM + 2] = dateTimeStr[5];
  fname[OFFSET_YYMM + 3] = dateTimeStr[6];

  if (mode) // FAST: add the day & "FA"
  {
    fname[OFFSET_YYMM + 4] = dateTimeStr[8];
    fname[OFFSET_YYMM + 5] = dateTimeStr[9];
    fname[OFFSET_YYMM + 6] = 'F';
    fname[OFFSET_YYMM + 7] = 'A';
  }
  else
  {
    fname[OFFSET_YYMM + 4] = 'D';
    fname[OFFSET_YYMM + 5] = 'A';
    fname[OFFSET_YYMM + 6] = 'T';
    fname[OFFSET_YYMM + 7] = 'A';   
  }
  

  // open the 'fname' file, 3 try...
  do 
  {
    errFile = true;
    File filept = FileSystem.open(fname, FILE_APPEND);
    if (filept)  // if the file is available, write to it:
    {
      filept.print(dateTimeStr); filept.print("\t");
      if (mode)  // Q: fast record enabled?
      { // A: yes, get it to record
        filept.println(counter.get_last_fast());
      }
      else  // A: no, get summed minute value (= each hour)
      {
        filept.println(counter.get_min());
      }
      filept.close();
      errFile = false;
    } // file pt OK
    else // A: file not availble, retry
    {
      delay(RETRY_DELAY);
    }
  } while(++i < RETRY && errFile == true);

  if (errFile == true) // Q: is the file isn't open?
  {                    // A: yes, pop up an error:
    String err_msg("Error writing file ");

    Serial.print(dateTimeStr);
    Serial.print("\t");  Serial.print(err_msg);
    Serial.println(fname);
    errFile = true;   // mark it
    err_msg = err_msg + String(fname);
    log_msg(err_msg); // try to log (can be in error, too)
  }
}

/*  log_msg()
    ---------
    Store the message in a logfile on SD card
    The format is: time + TAB + String given + CRLF
*/
void log_msg(String msg)
{
  const char *flog = "/mnt/sd/arduino/www/cntgeig.log";
  // open the file, 3 try...
  short i = 0;

  display_info(msg);
  dateTime_up_ascii();

  do 
  {
    errFile = true;
    File filept = FileSystem.open(flog, FILE_APPEND);
    if (filept)  // if the file is available, write to it:
    {
      errFile = false;      
      filept.print(dateTimeStr);
      filept.print("\t");
      filept.println(msg);
      filept.close();
    }
    else
    {
      delay(RETRY_DELAY);
    }
  } while(++i < RETRY && errFile == true);

  if (errFile == true) // if the file isn't open, pop up an error on serial:
  {
    Serial.print(dateTimeStr);
    Serial.print("\tError writing file ");
    Serial.println(flog);
  }
  // send msg onto serial too
  Serial.println(msg);
} // log_msg()
