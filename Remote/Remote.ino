#include <PCD8544.h>
#include <Debounce.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RUDDER_COMMAND           0x00
#define SIDE_COMMAND             0x01
#define FORWARD_COMMAND          0x02
#define RUDDER_POSITION_COMMAND  0x03
#define RUDDER_ENDPOINTS_COMMAND 0x04
#define FRONT_LIGHT_COMMAND      0x05
#define CIRCLE_LIGHTS_COMMAND    0x06

#define TELEMETRY_SEND     0x12

#define RUDDER_POSITION_ADDR   0x00
#define RUDDER_ENDPOINTS_ADDR  0x02
#define FRONT_LIGHT_ADDR       0x04
#define CIRCLE_LIGHTS_ADDR     0x06

#define MENU_COUNT            4
#define HEARTRATE_PERIOD      300
#define TELEMETRY_RATE        500
#define MAX_TELEMETRY_PING    300

int prev_rudder = -1;
int prev_side = -1;
int prev_forward = -1;
unsigned long prev_left_button = 0;
unsigned long prev_center_button = 0;
unsigned long prev_right_button = 0;

byte active_menu = 0;
byte menu_pos = 0;
bool is_editing = false;
int value_in_edit = 0;
bool value_read = false;
bool update_data_p_display = true;

unsigned long last_radio_sent = 0;
unsigned long last_telemetry_update = 0;

const char menu1[] PROGMEM = "Front light";
const char menu2[] PROGMEM = "Circle lights";
const char menu3[] PROGMEM = "Rudder pos.";
const char menu4[] PROGMEM = "Rudder endp.";

const char* const menu[] PROGMEM = {
  menu1, //front light
  menu2, //circle lights
  menu3, //rudder position
  menu4  //rudder endpoints
};

const byte eeprom_addr[] = {
  FRONT_LIGHT_ADDR,
  CIRCLE_LIGHTS_ADDR,
  RUDDER_POSITION_ADDR,
  RUDDER_ENDPOINTS_ADDR
};

const byte setting_type[] = {
  1, //bool, front light
  1, //bool, circle lights
  0, //byte, rudder position
  0  //byte, rudder enpoints
};

const byte settings_commands[] = {
  FRONT_LIGHT_COMMAND,
  CIRCLE_LIGHTS_COMMAND,
  RUDDER_POSITION_COMMAND,
  RUDDER_ENDPOINTS_COMMAND
};

const byte active_glyph[] = {0x00, 0x7e, 0x3c, 0x18, 0x00};

static PCD8544 p_display(4, 5, 6, 7, 8);
LiquidCrystal_I2C main_lcd(0x27,16,2);
LiquidCrystal_I2C gps_lcd(0x26,16,2);
Debounce button_left (A3, 1);
Debounce button_center (A4, 1);
Debounce button_right (A5, 1);

union telemetry_union
{
  float union_float;
  int union_int;
  byte union_bytes[4];
};

byte satellite_char1[] = {0x06,0x06,0x06,0x1F,0x1F,0x06,0x06,0x06};
byte satellite_char2[] = {0x02,0x09,0x05,0x15,0x15,0x05,0x09,0x02};
byte accuracy_char[] = {0x00,0x04,0x04,0x0E,0x1B,0x0E,0x04,0x04};
byte speed_units_char1[] = {0x0A,0x0C,0x0A,0x00,0x11,0x1B,0x15,0x11};
byte speed_units_char2[] = {0x00,0x00,0x00,0x10,0x10,0x1C,0x14,0x14};

float longitude = 0;
float latitude = 0;
int DOP = 0;
int satellites = 0;
float speed = 0;
int heading = 0;
int ping = 0;
int rth_dst = 0;
int rth_heading = 0;
float boat_voltage = 0;
float boat_current = 0;
float last_longitude = -1;
float last_latitude = -1;
int last_DOP = -1;
int last_satellites = -1;
float last_speed = -1;
int last_heading = -1;
int last_ping = -1;
int last_rth_dst = -1;
int last_rth_heading = -1;
float last_boat_voltage = -1;
float last_boat_current = -1;

bool connected = false;
bool initialized = false;
bool error_connection_showed = false; //for p_display update

void setup()
{
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  Serial.begin(9600);
  Serial1.begin(57600);
  p_display.begin(84, 48);
  p_display.createChar(0, active_glyph);
  main_lcd.init();
  main_lcd.createChar(0, speed_units_char1);
  main_lcd.createChar(1, speed_units_char2);
  main_lcd.backlight();
  gps_lcd.init();
  gps_lcd.createChar(0, satellite_char1);
  gps_lcd.createChar(1, satellite_char2);
  gps_lcd.createChar(2, accuracy_char);
  gps_lcd.backlight();
}

void loop()
{
  update_rc_values();
  handle_buttons();
  update_p_display();
  if(millis() - last_telemetry_update > TELEMETRY_RATE)
  {
    read_telemetry();
    update_gps_lcd();
    update_main_lcd();
    last_telemetry_update = millis();
  }
  if(millis() - last_radio_sent > HEARTRATE_PERIOD)
  {
    send_radio_command(0xFF, 0xFF);
  }
  if(initialized && connected && error_connection_showed)
  {
    update_all_settings();
  }
  delay(20);
}

void read_telemetry()
{
  send_radio_command(TELEMETRY_SEND, 0);
  bool got = false;
  unsigned long start_time = millis();
  while(Serial1.available() > 0) {Serial1.read();}
  while(!got && millis() - start_time < MAX_TELEMETRY_PING)
  {
    if(Serial1.available() == 30) {got = true;}
  }
  ping = millis()-start_time;
  byte buffer[30];
  if(got)
  {
    for(int i = 0; i < 30; i++) {buffer[i] = Serial1.read();}
    connected = true;
    initialized = true;
    union telemetry_union un;
    un.union_bytes[0] = buffer[0]; un.union_bytes[1] = buffer[1]; un.union_bytes[2] = buffer[2]; un.union_bytes[3] = buffer[3];
    longitude = un.union_float;
    un.union_bytes[0] = buffer[4]; un.union_bytes[1] = buffer[5]; un.union_bytes[2] = buffer[6]; un.union_bytes[3] = buffer[7];
    latitude = un.union_float;
    un.union_bytes[0] = buffer[8]; un.union_bytes[1] = buffer[9];
    satellites = un.union_int;
    un.union_bytes[0] = buffer[10]; un.union_bytes[1] = buffer[11];
    DOP = un.union_int;
    un.union_bytes[0] = buffer[12]; un.union_bytes[1] = buffer[13]; un.union_bytes[2] = buffer[14]; un.union_bytes[3] = buffer[15];
    speed = un.union_float;
    un.union_bytes[0] = buffer[16]; un.union_bytes[1] = buffer[17];
    heading = un.union_int;
    un.union_bytes[0] = buffer[18]; un.union_bytes[1] = buffer[19];
    rth_dst = un.union_int;
    un.union_bytes[0] = buffer[20]; un.union_bytes[1] = buffer[21];
    rth_heading = un.union_int;
    un.union_bytes[0] = buffer[22]; un.union_bytes[1] = buffer[23]; un.union_bytes[2] = buffer[24]; un.union_bytes[3] = buffer[25];
    boat_voltage = un.union_float;
    un.union_bytes[0] = buffer[26]; un.union_bytes[1] = buffer[27]; un.union_bytes[2] = buffer[28]; un.union_bytes[3] = buffer[29];
    boat_current = un.union_float;
  }
  else
  {
    connected = false;
  }
}

void update_main_lcd()
{
  if(speed != last_speed || heading != last_heading || ping != last_ping || boat_voltage != last_boat_voltage || boat_current != last_boat_current)
  {
    main_lcd.setCursor(0, 0);
    main_lcd.print("00");
    if(abs(speed) >= 10) main_lcd.setCursor(0, 0);
    else                 main_lcd.setCursor(1, 0);
    main_lcd.print(speed, 1);
    main_lcd.write(0);
    main_lcd.write(1);

    main_lcd.setCursor(7, 0);
    main_lcd.print(ping);

    main_lcd.setCursor(11, 0);
    if(heading > 0)      main_lcd.print("R");
    else if(heading < 0) main_lcd.print("L");
    else                 main_lcd.print(" ");
    main_lcd.print("00");
    if(abs(heading) >= 100)     main_lcd.setCursor(12, 0);
    else if(abs(heading) >= 10) main_lcd.setCursor(13, 0);
    main_lcd.print(heading);
    main_lcd.write(223);

    main_lcd.setCursor(0, 1);
    main_lcd.print("00");
    if(abs(boat_voltage) >= 10) main_lcd.setCursor(0, 1);
    else                        main_lcd.setCursor(1, 1);
    main_lcd.print(boat_voltage, 1);
    main_lcd.print("V");

    main_lcd.setCursor(6, 1);
    main_lcd.print("00");
    if(abs(boat_current) >= 10) main_lcd.setCursor(6, 1);
    else                        main_lcd.setCursor(7, 1);
    main_lcd.print(boat_current, 1);
    main_lcd.print("A");

    last_speed = speed;
    last_heading = heading;
    last_ping = ping;
    last_boat_voltage = boat_voltage;
    last_boat_current = boat_current;
  }
}

void update_gps_lcd()
{
  if(longitude != last_longitude || latitude != last_latitude || satellites != last_satellites || DOP != last_DOP)
  {
    gps_lcd.setCursor(0, 0);
    if(latitude > 0)      gps_lcd.print("N");
    else if(latitude < 0) gps_lcd.print("S");
    else                  gps_lcd.print(0);
    gps_lcd.print("00");
    if(abs(latitude) >= 100)     gps_lcd.setCursor(1, 0);
    else if(abs(latitude) >= 10) gps_lcd.setCursor(2, 0);
    else                         gps_lcd.setCursor(3, 0);
    gps_lcd.print(abs(latitude), 5);
    
    gps_lcd.setCursor(0, 1);
    if(longitude > 0)      gps_lcd.print("E");
    else if(longitude < 0) gps_lcd.print("W");
    else                   gps_lcd.print(0);
    gps_lcd.print("00");
    if(abs(longitude) >= 100)     gps_lcd.setCursor(1, 1);
    else if(abs(longitude) >= 10) gps_lcd.setCursor(2, 1);
    else                          gps_lcd.setCursor(3, 1);
    gps_lcd.print(abs(longitude), 5);
    
    gps_lcd.setCursor(12, 0);
    gps_lcd.write(0);
    gps_lcd.write(1);
    gps_lcd.print("0");
    if(satellites < 10) gps_lcd.setCursor(15, 0);
    else                gps_lcd.setCursor(14, 0);
    gps_lcd.print(satellites);

    gps_lcd.setCursor(12, 1);
    gps_lcd.write(2);
    gps_lcd.print("00");
    if(DOP >= 100)     gps_lcd.setCursor(13, 1);
    else if(DOP >= 10) gps_lcd.setCursor(14, 1);
    else               gps_lcd.setCursor(15, 1);
    gps_lcd.print(DOP);
    
    last_longitude = longitude;
    last_latitude = latitude;
    last_satellites = satellites;
    last_DOP = DOP;
  }
}

void handle_buttons()
{
  if(prev_right_button < button_right.count())
  {
    if(is_editing)
    {
      switch(setting_type[active_menu])
      {
        case 0:
          if(value_in_edit < 127) {value_in_edit++;}
          break;
        case 1:
          if(value_in_edit < 1) {value_in_edit++;}
          break;
      }
    }
    if(!is_editing && active_menu < MENU_COUNT-1) {active_menu++;}
    prev_right_button = button_right.count();
    update_data_p_display = true;
  }
  if(prev_left_button < button_left.count())
  {
    if(is_editing)
    {
      switch(setting_type[active_menu])
      {
        case 0:
          if(value_in_edit > -127) {value_in_edit--;}
          break;
        case 1:
          if(value_in_edit > 0) {value_in_edit--;}
          break;
      }
    }
    if(!is_editing && active_menu > 0) {active_menu--;}
    prev_left_button = button_left.count();
    update_data_p_display = true;
  }
  if(prev_center_button < button_center.count())
  {
    is_editing = !is_editing;
    prev_center_button = button_center.count();
    update_data_p_display = true;
  }
}

void update_p_display()
{
  if(initialized && connected)
  {
    if(update_data_p_display || error_connection_showed)
    {
      if(!is_editing)
      {
        if(value_read)
        {
          eeprom_write_word(eeprom_addr[active_menu], value_in_edit);
          switch(setting_type[active_menu])
          {
            case 0:
              send_radio_command(settings_commands[active_menu], 127+value_in_edit);
              break;
            case 1:
              send_radio_command(settings_commands[active_menu], value_in_edit);
              break;
          }
          value_read = false;
        }
        p_display.clear();
        byte to_show = MENU_COUNT-menu_pos;
        byte first_index = menu_pos;
        byte second_index = menu_pos+to_show;
        byte counter = 0;
        char progmemBuf[16];
        for(int i = first_index; i < second_index; i++)
        {
          Serial.println(i);
          strcpy_P(progmemBuf, pgm_read_word(&(menu[i])));
          p_display.setCursor(0, 1*counter);
          if(i == active_menu) {p_display.write(0);}
          else {p_display.write(20);}
          p_display.print(progmemBuf);
          counter++;
        }
      }
      else
      {
        if(!value_read)
        {
          p_display.clear();
          value_in_edit = eeprom_read_word(eeprom_addr[active_menu]);
          value_read = true;
          char progmemBuf[16];
          strcpy_P(progmemBuf, pgm_read_word(&(menu[active_menu])));
          Serial.println(active_menu);
          p_display.setCursor(0, 0);
          p_display.print(progmemBuf);
          p_display.setCursor(0, 2);
          p_display.write(0);
          p_display.print(value_in_edit);
        }
        else
        {
          p_display.setCursor(0, 2);
          p_display.clearLine();
          p_display.write(0);
          p_display.print(value_in_edit);
        }
      }
      update_data_p_display = false;
    }
    error_connection_showed = false;
  }
  else if(initialized && !connected)
  {
    if(!error_connection_showed)
    {
      p_display.clear();
      p_display.print("Connection");
      p_display.setCursor(0, 1);
      p_display.print("break!");
      error_connection_showed = true;
    }
  }
  else
  {
    if(!error_connection_showed)
    {
      p_display.clear();
      p_display.print("Waiting for");
      p_display.setCursor(0, 1);
      p_display.print("connection...");
      error_connection_showed = true;
    }
  }
}

void update_rc_values()
{
  int now_side = analogRead(A0);
  int now_forward = analogRead(A1);
  int now_rudder = analogRead(A2);
  if((now_rudder > 520 || now_rudder < 515) && now_rudder != prev_rudder)
  {
    send_radio_command(RUDDER_COMMAND, map(now_rudder, 1024, 0, 0, 254));
    prev_rudder = now_rudder;
  }
  else if(now_rudder < 520 && now_rudder > 515 && now_rudder != prev_rudder)
  {
    send_radio_command(RUDDER_COMMAND, 127);
    prev_rudder = now_rudder;
  }

  if((now_side > 530 || now_side < 430) && now_side != prev_side)
  {
    send_radio_command(SIDE_COMMAND, map(now_side, 1023, 0, 0, 254));
    prev_side = now_side;
  }
  else if(now_side < 530 && now_side > 430 && now_side != prev_side)
  {
    send_radio_command(SIDE_COMMAND, 127);
    prev_side = now_side;
  }

  if((now_forward > 505 || now_forward < 500) && now_forward != prev_forward)
  {
    send_radio_command(FORWARD_COMMAND, map(now_forward, 1024, 0, 0, 254));
    prev_forward = now_forward;
  }
  else if(now_forward < 505 && now_forward > 500 && now_forward != prev_forward)
  {
    send_radio_command(FORWARD_COMMAND, 127);
    prev_forward = now_forward;
  }
}

void update_all_settings()
{
  for(int i = 0; i < MENU_COUNT; i++)
  {
    int value = eeprom_read_word(eeprom_addr[i]);
    switch(setting_type[i])
    {
      case 0:
        send_radio_command(settings_commands[i], 127+value);
        break;
      case 1:
        send_radio_command(settings_commands[i], value
        );
        break;
    }
  }
}

void send_radio_command(byte command, byte value)
{
  Serial1.write(command);
  Serial1.write(value);
  Serial1.write(0xFF);
  last_radio_sent = millis();
}
