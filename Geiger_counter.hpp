/*
 Geiger_counter.hpp
 ------------------
 31.03.2019 - ymasur@microclub.ch

*/

#ifndef GEIGER_COUNTER_HPP
#define GEIGER_COUNTER_HPP

#ifdef MAIN
  #define CLASS
#else
  #define CLASS extern
#endif

class Counter
{
  public:
  void clear_all();
  unsigned short get_live(); //{return c_live;}
  unsigned short get_live_fast();
  unsigned short get_last_fast();
  void clear_fast();
  unsigned short get_last_min() {return c_last_min;}
  unsigned short get_min(){return c_min;}
  unsigned long get_hour(){return c_hour;}
  unsigned long get_day(){return c_day;}
  void add_live() {c_live++; c_10s++;}
  void upd_live_to_min();// {c_min += c_live; c_live = 0;}
  void upd_min_to_hour();// {c_hour += c_min; c_min = 0;}
  void upd_hour_to_day();// {c_day += c_hour; c_hour = 0;}

  private:
  unsigned short volatile c_live; // the running value
  unsigned short c_10s; // fast count: 10 seconds (reset by write on file)
  unsigned short c_last_10s;
  unsigned short c_last_min;  // the last running value until a minute passed
  unsigned short c_min; // total accumulated 0..59 minutes
  unsigned long c_hour; // total accumulated 0..23 hours
  unsigned long c_day;  // grand total each day passed until start
};

// global vars are defined here
CLASS Counter counter; // nb of Geiger pulses are stored here
#endif // GEIGER_COUNTER_HPP
