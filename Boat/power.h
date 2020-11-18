#ifndef BOAT_POWER
#define BOAT_POWER

#include <Arduino.h>

class PowerModule
{
  public:
    inline PowerModule(int vol_pin, int cur_pin) {_vol_pin = vol_pin; _cur_pin = cur_pin;}
    void update();
    float voltage = 0;
    float current = 0;
  private:
    int _vol_pin;
    int _cur_pin;
    float _voltage_filtered = 0.0;
    float _current_filtered = 0.0;
    void _voltage_filter(float value);
    void _current_filter(float value);
};

#endif
