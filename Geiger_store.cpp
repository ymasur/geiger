/*
 Geiger_store.cpp
 ----------------
 18.03.2019 - ymasur@microclub.ch

 Module store can write datas onto SD/USB storage file
*/
#include <Arduino.h>
#include <FileIO.h>
#include <string.h>
#include "Geiger.hpp"
#include "Geiger_counter.hpp"

#define RETRY 3 //number of retry in write to file (0: mean 1)
#define RETRY_DELAY 1 // nb of millisecond between retry

/*  storeCounts()
    -------------
    Store the actual counts in a file on SD card
    Global vars used:
    - char[] fname, contain the full path
    - char[] dateTimeStr, that contain the actual date and time
      format should be : "yy-mm-dd hh:mm:ss"

    Modified global vars:
    - fname, the filename in a 8.3 format. 4 first char are modified
    - errFile: false if OK; then true if an error occures
*/
void storeCounts(char *fname, char *dateTimeStr)
{
  short i = 0;
  // open the file.
  // The FileSystem card is mounted at the following "/mnt/SD" and
  // create the name with year and month on 4 digits
  fname[OFFSET_YYMM + 0] = dateTimeStr[2];
  fname[OFFSET_YYMM + 1] = dateTimeStr[3];
  fname[OFFSET_YYMM + 2] = dateTimeStr[5];
  fname[OFFSET_YYMM + 3] = dateTimeStr[6];

  // open the 'fname' file, 3 try...
  do 
  {
    errFile = true;
    File filept = FileSystem.open(fname, FILE_APPEND);
    if (filept)  // if the file is available, write to it:
    {
      filept.print(dateTimeStr); filept.print("\t");
      if (fl_fast)  // Q: fast record enabled?
      { // A: yes, add "F" in a field
        filept.print("F\t");
        filept.println(counter.get_last_fast());
      }
      else  // A: no, get summed minute value (= each hour)
      {
        filept.println(counter.get_min());
      }
      filept.close();
      errFile = false;
    }
    else
    {
      delay(RETRY_DELAY);
    }
  } while(++i < RETRY && errFile == true);

  if (errFile == true) // if the file isn't open, pop up an error:
  {
    String err_msg("Error writing file ");

    Serial.print(dateTimeStr);
    Serial.print("\t");  Serial.print(err_msg);
    Serial.println(fname);
    errFile = true;
    err_msg = err_msg + String(fname);
    log_msg(err_msg);
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
