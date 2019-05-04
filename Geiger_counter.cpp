/*
 Geiger_counter.cpp
 ------------------
 31.03.2019 - ymasur@microclub.ch
 
 Counter object

*/
#include <Arduino.h>
#include "Geiger_counter.hpp"

unsigned short Counter::get_live()
{
  register short c;
  noInterrupts();
  c = c_live;
  interrupts();
  return c;
}

unsigned short Counter::get_live_fast()
{
  register short c;
  noInterrupts();
  c = c_10s;
  interrupts();
  return c;
}

unsigned short Counter::get_last_fast()
{
  register short c;
  noInterrupts();
  c = c_last_10s;
  interrupts();
  return c;
}

void Counter::clear_fast()
{
  noInterrupts();
  c_last_10s = c_10s;
  c_10s = 0;
  interrupts();
}

void Counter::clear_all()
{
  noInterrupts();
  c_live = c_10s = c_min = 0;
  c_hour = 0L;
  c_day = 0L;
  interrupts();
};

void Counter::upd_live_to_min()
{
  noInterrupts();
  c_last_min = c_live; c_live = 0;
  c_min += c_last_min;
  interrupts();
}

void Counter::upd_min_to_hour()
{
  noInterrupts();
  c_hour += c_min; c_min = 0;
  interrupts();
}

void Counter::upd_hour_to_day()
{
  interrupts();
  c_day += c_hour; c_hour = 0L;
  interrupts();
}
