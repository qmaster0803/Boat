#include "power.h"

void PowerModule::update()
{
  float raw_voltage = (analogRead(_vol_pin)+6)*5.0/1023*10;
  float raw_current = (analogRead(A14)+2)*5.0/1023*10;
  _voltage_filter(raw_voltage);
  _current_filter(raw_current);
  voltage = _voltage_filtered;
  current = _current_filtered;
}

void PowerModule::_voltage_filter(float value)
{
  float k = 0;
  if(abs(value-_voltage_filtered) < 1) {k = 0.1;}
  else {k = 0.8;}
  _voltage_filtered += (value - _voltage_filtered)*k;
}

void PowerModule::_current_filter(float value)
{
  float k = 0;
  if(abs(value-_current_filtered) < 1) {k = 0.1;}
  else {k = 1;}
  _current_filtered += (value - _current_filtered)*k;
}
