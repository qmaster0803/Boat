int prev_rudder = -1;
int prev_side = -1;
int prev_forward = -1;

void setup() {
  Serial.begin(9600);
  Serial1.begin(57600);
}

void loop() {
  int now_side = analogRead(A0);
  int now_forward = analogRead(A1);
  int now_rudder = analogRead(A2);
  if((now_rudder > 520 || now_rudder < 510) && now_rudder != prev_rudder) //rudder
  {
    Serial1.write('R'); Serial1.write(map(analogRead(A2), 0, 1024, 45, 135));
    prev_rudder = now_rudder;
    Serial.println("Rudder active");
  }
  else if(now_rudder < 520 && now_rudder > 510 && now_rudder != prev_rudder)
  {
    Serial1.write('R'); Serial1.write(90);
    prev_rudder = now_rudder;
    Serial.println("Rudder centered");
  }

  if((now_side > 605 || now_side < 510) && now_side != prev_side) //side
  {
    Serial1.write('S'); Serial1.write(map(now_side, 0, 1023, 0, 254));
    prev_side = now_side;
    Serial.println("Side active");
  }
  else if(now_side < 605 && now_side > 510 && now_side != prev_side)
  {
    Serial1.write('S'); Serial1.write(127);
    prev_side = now_side;
    Serial.println("Side stopped");
  }

  if((now_forward > 510 || now_forward < 495) && now_forward != prev_forward) //forward
  {
    Serial1.write('F'); Serial1.write(map(now_forward, 0, 1023, 0, 254));
    prev_forward = now_forward;
    Serial.println("Forward active");
  }
  else if(now_forward < 510 && now_forward > 495 && now_forward != prev_forward)
  {
    Serial1.write('F'); Serial1.write(127);
    prev_forward = now_forward;
    Serial.println("Forward stopped");
  }
  delay(50);
}
