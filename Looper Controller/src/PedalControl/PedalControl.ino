#include <Keyboard.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define REC  0
#define PLAY 1
#define VOL  2
#define T_LONGPRESS 1000

/*  > PEDAL SWITCH LAYOUT
    **********************************************************************************************
    *                                          ||    CLEAR    ||    EXTRA    ||                  *
    *---------------------------------------------------------------------------------------------
    *                                          ||             ||             ||                  *
    **********************************************************************************************
    *    REC/PLAY    ||    STOP    ||    MODE    ||    T1    ||    T2    ||    T3    ||    T4    *
    *--------------------------------------------------------------------------------------------*
    *     rb;L0      ||   yo;L1    ||   pg;L2    ||  gb;L3   ||   bw;L4  ||          ||          *
    **********************************************************************************************

    > LED CONTROL
    Through the Serial RX/TX port, a character is sent to the LED controller following the correspondance:
    -Entering PLAY mode  --> p
    -Entering VOL mode   --> v
    -Entering REC mode   --> r
    -Selecting T1 --> 1
    -Selecting T2 --> 2
    -Selecting T3 --> 3
    -Selecting T4 --> 4 */ 

//DIGITAL PINS
#define b_recplay               2        // RecPlay digital pin
#define b_stop                  3        // Stop digital pin
#define b_mode                  4        // Mode digital pin
#define b_t1                    5        // Track1 digital pin
#define b_t2                    6        // Track2 digital pin
#define b_t3                    7        // Track3 digital pin
#define b_t4                    8        // Track4 digital pin
#define b_clear                 9        // Clear digital pin
#define b_extra                 10       // Extra digital pin

//KEYBINDINGS TO MOBIUS SCRIPTS  
#define k_recplay_REC           'q'      // Recplay pressed in REC
#define k_recplay_PLAY          'w'      // Recplay pressed in PLAY
 
#define k_stop_REC              'm'      // Stop pressed in REC
#define k_stop_PLAY             'y'      // Stop pressed in PLAY
 
#define k_mode_REC              'p'      // Sets mode to PLAY
#define k_mode_PLAY             'r'      // Sets mode to REC
#define k_mode_VOL              'v'      // Sets mode to VOL
 
#define k_t1_REC                '1'      // Track1 pressed in REC
#define k_t2_REC                '2'      // Track2 pressed in REC
#define k_t3_REC                '3'      // Track3 pressed in REC
#define k_t4_REC                '4'      // Track4 pressed in REC
 
#define k_t1_PLAY               'a'      // Track1 pressed in PLAY
#define k_t2_PLAY               'b'      // Track2 pressed in PLAY
#define k_t3_PLAY               'c'      // Track3 pressed in PLAY
#define k_t4_PLAY               'd'      // Track4 pressed in PLAY
 
#define k_t1_VOL                'o'      // Track1 pressed in VOL
#define k_t2_VOL                'n'      // Track2 pressed in VOL
#define k_t3_VOL                'u'      // Track3 pressed in VOL
 
#define k_clear_GLOBAL          'c'      // Clear GLOBAL
#define k_extra_GLOBAL          'x'      // Reset all GLOBAL
#define k_globalreset_GLOBAL    'g'      // Globl reset COMBINATION
 
//SERIAL CODING FOR LED COMMUNICATION
#define REC_SERIAL_CODE         "r"      
#define PLAY_SERIAL_CODE        "p"      
#define VOL_SERIAL_CODE         "v"   
#define RESET_SERIAL_CODE       "g"
#define T1_SERIAL_CODE          "1"      
#define T2_SERIAL_CODE          "2"      
#define T3_SERIAL_CODE          "3"      
#define T4_SERIAL_CODE          "4"      

//BUTTON STATE CONTROL VARIABLES
unsigned int last_recplay, this_recplay; 
unsigned int last_stop, this_stop; 
unsigned int last_mode, this_mode; 
unsigned int last_t1, this_t1; 
unsigned int last_t2, this_t2; 
unsigned int last_t3, this_t3;
unsigned int last_t4, this_t4;
unsigned int last_clear, this_clear; 
unsigned int last_extra, this_extra; 
//BUTTON STATE TIMING VARIABLES (For longpress buttons only)
unsigned int startT_stop, stopT_stop; 
unsigned int startT_mode, stopT_mode; 

//MOBIUS MODE
unsigned int mode; //0 = REC; 1 = PLAY; 2 = VOL
char current_track; //Currently selected track [for LED management]
int recpresses; //Number of recording presses since reset (REC or Tracks)
unsigned int startT_loop=0; //Loop start moment and length [in millis()]
unsigned int loop_duration=0; 

// LCD Update
LiquidCrystal_I2C lcd(0x27, 20, 4);

void updateProgressBar(unsigned long count, unsigned long totalCount, int lineToPrintOn) {
    double factor = totalCount/80.0;          
    int percent = (count+1)/factor;
    int number = percent/5;
    int remainder = percent%5;
    if(number > 0) {
       lcd.setCursor(number-1,lineToPrintOn);
       lcd.write(5);
    }
    lcd.setCursor(number,lineToPrintOn);
    lcd.write(remainder);   
 }

void update_lcd() {
  lcd.home(); lcd.print("Mode: ");
  switch(mode) {
    case REC:  lcd.print("RECORD  "); break;
    case PLAY: lcd.print("PLAY    "); break;
    case VOL:  lcd.print("VOLUME  "); break;
  }
  if (millis()-startT_loop == loop_duration) startT_loop = millis();
  updateProgressBar(millis()-startT_loop,loop_duration,3);
  updateProgressBar(millis()-startT_loop,loop_duration,4);
}

void send_command(char key_command) {
  Keyboard.press(key_command);
  Keyboard.release(key_command);
}

void global_reset() {
  mode = REC; 
  recpresses = 0;
  current_track = T1_SERIAL_CODE;
  // Serial.print(RESET_SERIAL_CODE);
  // Serial.print(REC_SERIAL_CODE); delay(100); Serial.print(current_track);
  lcd.clear();
  lcd.setCursor(2,1); lcd.print("RESETTING...");
  last_recplay = LOW;
  last_stop = LOW;
  last_mode = LOW;
  last_t1 = LOW;
  last_t2 = LOW;
  last_t3 = LOW;
  last_t4 = LOW;
  last_clear = LOW; 
  last_extra = LOW;
  //Global reset command sent to Mobius
  send_command(k_globalreset_GLOBAL);
  delay(100);
  lcd.clear();
  //Default MODE set to REC
  send_command(k_mode_PLAY);
}

//BUTTON PRESSED EVENTS
void recplay_pressed() {
  if (mode == PLAY) {
    send_command(k_recplay_PLAY);
  }else{
    send_command(k_recplay_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
    }
  }
}

void stop_shortpressed() {
  if (mode == PLAY) {
    send_command(k_stop_PLAY);
  }else{
    send_command(k_stop_REC);
  }
}

void mode_shortpressed() {
  if (mode == PLAY) {
    send_command(k_mode_PLAY);
    mode = REC;
    Serial.print(REC_SERIAL_CODE);
  }else{
    send_command(k_mode_REC);
    mode = PLAY;
    Serial.print(PLAY_SERIAL_CODE);
  }
}

void t1_pressed() {
  if (mode == PLAY) {
    send_command(k_t1_PLAY);
  }else if (mode == REC) {
    send_command(k_t1_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
    }
  }else{
    send_command(k_t1_VOL);
  }
  current_track = T1_SERIAL_CODE;
  Serial.print(current_track);
}

void t2_pressed() {
  if (mode == PLAY) {
    send_command(k_t2_PLAY);
  }else if (mode == REC) {
    send_command(k_t2_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
    }
  }else{
    send_command(k_t2_VOL);
  }
  current_track = T2_SERIAL_CODE;
  Serial.print(current_track);
  delay(1000);
}

void t3_pressed() {
  if (mode == PLAY) {
    send_command(k_t3_PLAY);
  }else if (mode == REC) {
    send_command(k_t3_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
    }
  }else{
    send_command(k_t3_VOL);
  }
  current_track = T3_SERIAL_CODE;
  Serial.print(current_track);
}

void t4_pressed() {
  if (mode == PLAY) {
    send_command(k_t4_PLAY);
  }else{
    send_command(k_t4_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
    }
  }
  current_track = T4_SERIAL_CODE;
  Serial.print(current_track);
}

void clear_pressed() {
  send_command(k_clear_GLOBAL);
}

void extra_pressed() {
  send_command(k_extra_GLOBAL);
}

void stop_longpressed() {
  send_command(k_globalreset_GLOBAL);
}

void mode_longpressed() {
  if (mode != VOL) {
    mode = VOL;
    send_command(k_mode_VOL);
    Serial.print(VOL_SERIAL_CODE);
  }
  else mode = REC;
  Serial.print(REC_SERIAL_CODE);
}

void setup() { 
  pinMode(b_recplay, INPUT_PULLUP);
  pinMode(b_stop, INPUT_PULLUP);
  pinMode(b_mode, INPUT_PULLUP);
  pinMode(b_t1, INPUT_PULLUP);
  pinMode(b_t2, INPUT_PULLUP);
  pinMode(b_t3, INPUT_PULLUP);
  pinMode(b_t4, INPUT_PULLUP);
  pinMode(b_clear, INPUT_PULLUP);
  pinMode(b_extra, INPUT_PULLUP);
  Serial.begin(9600);
  delay(500);
  Keyboard.begin();
  lcd.init(); lcd.backlight();
  global_reset();
}

void loop() {
  //RecPlay switch detection
  this_recplay = digitalRead(b_recplay);
  if (this_recplay == LOW && last_recplay == HIGH) {
    recplay_pressed();
  }
  last_recplay = this_recplay;
  
  //Stop switch detection - SHORT PRESS
  this_stop = digitalRead(b_stop);
  if (this_stop == LOW && last_stop == HIGH) {
    stop_shortpressed();
    startT_stop = millis();
  }
  //Stop switch detection - LONG PRESS
  if (this_stop == HIGH && last_stop == LOW) {
    stopT_stop = millis();
    if (stopT_stop-startT_stop>=T_LONGPRESS) {
      stop_longpressed();
    }
  }
  last_stop = this_stop;
  
  //Mode switch detection - SHORT PRESS
  this_mode = digitalRead(b_mode);
  if (this_mode == LOW && last_mode == HIGH) {
    mode_shortpressed();
    startT_mode = millis();
  }
  //Mode switch detection - LONG PRESS
  if (this_mode == HIGH && last_mode == LOW) {
    stopT_mode = millis();
    if (stopT_mode-startT_mode>=T_LONGPRESS) {
      mode_longpressed();
    }
  }
  last_mode = this_mode;

  //Track 1 switch detection
  this_t1 = digitalRead(b_t1);
  if (this_t1 == LOW && last_t1 == HIGH) {
    t1_pressed();
  }
  last_t1 = this_t1;
  
  //Track 2 switch detection
  this_t2 = digitalRead(b_t2);
  if (this_t2 == LOW && last_t2 == HIGH) {
    t2_pressed();
  }
  last_t2 = this_t2;
  
  //Track 3 switch detection
  this_t3 = digitalRead(b_t3);
  if (this_t3 == LOW && last_t3 == HIGH) {
    t3_pressed();
  }
  last_t3 = this_t3;
  
  //Track 4 switch detection
  this_t4 = digitalRead(b_t4);
  if (this_t4 == LOW && last_t4 == HIGH) {
    t4_pressed();
  }
  last_t4 = this_t4;

  //Clear switch detection
  this_clear = digitalRead(b_clear);
  if (this_clear == LOW && last_clear == HIGH) {
    clear_pressed();
  }
  last_clear = this_clear;

  //Extra switch detection
  this_extra = digitalRead(b_extra);
  if (this_extra == LOW && last_extra == HIGH) {
    extra_pressed();
  }
  last_extra = this_extra;

  update_lcd();
  delay(10);
}
