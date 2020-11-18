#include "ESC.h"
#include <Servo.h>

Servo rudder;
ESC right (5, 1000, 2000, 500);
ESC left (4, 1000, 2000, 500);
ESC front (2, 1000, 2000, 500);
ESC back (3, 1000, 2000, 500);

void setup() {
  Serial1.begin(57600);
  rudder.attach(A0);
  right.arm();
  left.arm();
  front.arm();
  back.arm();
}

void loop() {
  if (Serial1.available() > 1) {
    int command = Serial1.read();
    int value = Serial1.read();
    if(command == 'R') //rudder
    {
      rudder.write(value);
    }
    if(command == 'S') //side
    {
      if(value > 127)
      {
        left.speed(map(value, 127, 254, 1000, 1300));
        right.speed(1000);
      }
      if(value < 127)
      {
        right.speed(map(value, 127, 0, 1000, 1300));
        left.speed(1000);
      }
      if(value == 127)
      {
        right.speed(1000);
        left.speed(1000);
      }
    }
    if(command == 'F') //forward
    {
      if(value > 127)
      {
        front.speed(map(value, 127, 254, 1000, 1300));
        back.speed(1000);
      }
      if(value < 127)
      {
        back.speed(map(value, 127, 0, 1000, 1300));
        front.speed(1000);
      }
      if(value == 127)
      {
        front.speed(1000);
        back.speed(1000);
      }
    }
  }
}
