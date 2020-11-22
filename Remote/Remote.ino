#include <PCD8544.h>
#include <Debounce.h>
#include <EEPROM.h>

#define RUDDER_COMMAND           0x00
#define SIDE_COMMAND             0x01
#define FORWARD_COMMAND          0x02
#define RUDDER_POSITION_COMMAND  0x03
#define RUDDER_ENDPOINTS_COMMAND 0x04

#define RUDDER_POSITION_ADDR   0x00
#define RUDDER_ENDPOINTS_ADDR  0x02

#define DEFAULT_RUDDER_POSITION   0
#define DEFAULT_RUDDER_ENDPOINTS  45

#define MENU_COUNT        2
#define HEARTRATE_PERIOD  300

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

const char menu1[] PROGMEM = "Rudder pos.";
const char menu2[] PROGMEM = "Rudder endp.";

const char* const menu[] PROGMEM = {
  menu1, //rudder position
  menu2  //rudder endpoints
};
const byte eeprom_addr[] PROGMEM = {
  RUDDER_POSITION_ADDR,
  RUDDER_ENDPOINTS_ADDR
};

const byte settings_commands[] = {
  RUDDER_POSITION_COMMAND,
  RUDDER_ENDPOINTS_COMMAND
};

const byte active_glyph[] = {0x00, 0x7e, 0x3c, 0x18, 0x00};

static PCD8544 p_display;
Debounce button_left (A3, 1);
Debounce button_center (A4, 1);
Debounce button_right (A5, 1);

void setup()
{
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  Serial.begin(9600);
  Serial1.begin(57600);
  p_display.begin(84, 48);
  p_display.createChar(0, active_glyph);
}

void loop()
{
  update_rc_values();
  handle_buttons();
  update_p_display();
  if(millis() - last_radio_sent > HEARTRATE_PERIOD)
  {
    send_radio_command(0xFF, 0xFF);
  }
  delay(50);
}

void handle_buttons()
{
  if(prev_right_button < button_right.count())
  {
    if(is_editing && value_in_edit < 127) {value_in_edit++;}
    if(!is_editing && active_menu < MENU_COUNT-1) {active_menu++;}
    prev_right_button = button_right.count();
    update_data_p_display = true;
  }
  if(prev_left_button < button_left.count())
  {
    if(is_editing && value_in_edit > -127) {value_in_edit--;}
    if(active_menu > 0) {active_menu--;}
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
  if(update_data_p_display)
  {
    if(!is_editing)
    {
      if(value_read)
      {
        eeprom_write_word(eeprom_addr[active_menu], value_in_edit);
        send_radio_command(settings_commands[active_menu], 127+value_in_edit);
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

void send_radio_command(byte command, byte value)
{
  Serial1.write(command);
  Serial1.write(value);
  Serial1.write(0xFF);
  last_radio_sent = millis();
}
