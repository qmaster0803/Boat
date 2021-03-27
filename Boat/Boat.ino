#include "ESC.h"
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <iarduino_GPS_NMEA.h>
#include "power.h"

#define HEARTRATE_PERIOD 500

#define RUDDER_COMMAND           0x00
#define SIDE_COMMAND             0x01
#define FORWARD_COMMAND          0x02
#define RUDDER_POSITION_COMMAND  0x03
#define RUDDER_ENDPOINTS_COMMAND 0x04
#define FRONT_LIGHT_COMMAND      0x05
#define CIRCLE_LIGHTS_COMMAND    0x06

#define TELEMETRY_SEND_COMMAND 0x12

ESC rudder (A0, 1000, 2000, 1500);
ESC right  (5, 1000, 2000, 500);
ESC left   (4, 1000, 2000, 500);
ESC front  (2, 1000, 2000, 500);
ESC back   (3, 1000, 2000, 500);
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
PowerModule pm(A15, A14);
iarduino_GPS_NMEA gps;

byte radio_buffer[10];
byte radio_bytes_count = 0;
unsigned long radio_command_time = 0;
unsigned int engine_left_value = 1000;
unsigned int engine_right_value = 1000;
unsigned int engine_front_value = 1000;
unsigned int engine_back_value = 1000;
unsigned int rudder_value = 1500;
unsigned long last_signal_received = 0;

byte rudder_position = 127;
byte rudder_endpoints = 127;

bool signal_active = false;
bool telemetry_send = false;

union send_float_union
{
  float union_float;
  int union_int;
  byte union_bytes[4];
};

void setup() {
  Serial.begin(9600);
  Serial1.begin(57600);
  Serial2.begin(9600);
  rudder.arm();
  right.arm();
  left.arm();
  front.arm();
  back.arm();
  gps.begin(Serial2);
  gps.timeZone(GPS_AutoDetectZone);
  pinMode(A13, OUTPUT);
  pinMode(A12, OUTPUT);
}

void loop() {
  if (Serial1.available() > 0)
  {
    radio_buffer[radio_bytes_count] = Serial1.read();
    radio_bytes_count++;
    radio_command_time = millis();
    signal_active = true;
  }
  if(radio_bytes_count > 0 && millis()-radio_command_time > HEARTRATE_PERIOD)
  {
    radio_bytes_count = 0;
  }
  if(radio_buffer[radio_bytes_count-1] == 0xFF && radio_bytes_count >= 3)
  {
    handle_incoming_command();
    update_engines();
    radio_bytes_count = 0;
  }
  if(millis() - radio_command_time > HEARTRATE_PERIOD && signal_active)
  {
    do_failsafe();
    signal_active = false;
  }
  if(telemetry_send)
  {
    send_telemetry();
    telemetry_send = false;
  }
}

void send_telemetry()
{
  pm.update();
  //gps.read(i);
  send_float(12.12);      //longitude
  send_float(2.101);      //latitude
  send_int(55);           //satellites
  send_int(999);          //PDOP
  send_float(14.23);      //speed
  send_int(100);          //heading
  send_int(823);          //distance to home
  send_int(-60);          //heading to home
  send_float(pm.voltage); //battery voltage
  send_float(pm.current); //battery current
}

void send_float(float value)
{
  union send_float_union un;
  un.union_float = value;
  send_bytes(un.union_bytes, 4);
}

void send_int(int value)
{
  union send_float_union un;
  un.union_int = value;
  send_bytes(un.union_bytes, 2);
}

void send_bytes(byte values[], byte counter)
{
  for(int i = 0; i < counter; i++)
  {
    Serial1.write(values[i]);
  }
}

void do_failsafe()
{
  radio_bytes_count = 0;
  engine_left_value = 1000;
  engine_right_value = 1000;
  engine_front_value = 1000;
  engine_back_value = 1000;
  rudder_value = 1500;
  update_engines();
}

void update_engines()
{
  left.speed(engine_left_value);
  right.speed(engine_right_value);
  front.speed(engine_front_value);
  back.speed(engine_back_value);
  rudder.speed(rudder_value+map(rudder_position, 0, 254, -200, 200));
}

void handle_incoming_command()
{
  if(radio_buffer[0] == RUDDER_COMMAND) {rudder_value = map(radio_buffer[1], 0, 254, 1250, 1750);}
  if(radio_buffer[0] == SIDE_COMMAND)
  {
    if(radio_buffer[1] > 127) {engine_left_value = map(radio_buffer[1], 127, 254, 1000, 1300); engine_right_value = 1000;}
    if(radio_buffer[1] < 127) {engine_left_value = 1000; engine_right_value = map(radio_buffer[1], 127, 0, 1000, 1300);}
    if(radio_buffer[1] == 127) {engine_left_value = 1000; engine_right_value = 1000;}
  }
  if(radio_buffer[0] == FORWARD_COMMAND)
  {
    if(radio_buffer[1] > 127) {engine_front_value = map(radio_buffer[1], 127, 254, 1090, 1300); engine_back_value = 1000;}
    if(radio_buffer[1] < 127) {engine_front_value = 1000; engine_back_value = map(radio_buffer[1], 127, 0, 1090, 1300);}
    if(radio_buffer[1] == 127) {engine_front_value = 1000; engine_back_value = 1000;}
  }
  if(radio_buffer[0] == RUDDER_POSITION_COMMAND) {rudder_position = radio_buffer[1];}
  if(radio_buffer[0] == RUDDER_ENDPOINTS_COMMAND) {rudder_endpoints = radio_buffer[1];}
  if(radio_buffer[0] == FRONT_LIGHT_COMMAND) {digitalWrite(A12, radio_buffer[1]);}
  if(radio_buffer[0] == CIRCLE_LIGHTS_COMMAND) {digitalWrite(A13, radio_buffer[1]);}
  if(radio_buffer[0] == TELEMETRY_SEND_COMMAND) {telemetry_send = true;}
}
