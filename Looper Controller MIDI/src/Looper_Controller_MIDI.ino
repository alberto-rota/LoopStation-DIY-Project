#include <Keyboard.h>
#include <Wire.h>
#include <MIDIUSB.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include "lcd_bar_chars.h"

#define T_LONGPRESS 500
#define DEFAULT_BAUD_RATE 9600

#define REC  0
#define PLAY 1
#define VOL  2



/*  > PEDAL SWITCH LAYOUT
    **********************************************************************************************
    *                                          ||    CLEAR    ||    CUSTOM   ||                  *
    *---------------------------------------------------------------------------------------------
    *                                          ||   whi-gra   ||   bro-bla   ||                  *
    **********************************************************************************************
    *    REC/PLAY    ||    STOP    ||    MODE    ||    T1    ||    T2    ||    T3    ||    T4    *
    *--------------------------------------------------------------------------------------------*
    *    pur-blu     ||   red-bro   ||  blu-gre  || pur-gre  || bla-whi  || ora-yel  || yel-gre  *
    **********************************************************************************************

    > LED CONTROL
    Through the Serial1 RX/TX port, a character is sent to the LED controller following the correspondance:
    -Entering PLAY mode  --> p
    -Entering VOL mode   --> v  
    -Entering REC mode   --> r
    -Selecting T1 --> 1
    -Selecting T2 --> 2
    -Selecting T3 --> 3
    -Selecting T4 --> 4 */ 

//DIGITAL PINS
#define b_recplay               12      // RecPlay digital pin
#define b_stop                  11        // Stop digital pin
#define b_mode                  10        // Mode digital pin
#define b_t1                    9        // Track1 digital pin
#define b_t2                    8        // Track2 digital pin
#define b_t3                    7        // Track3 digital ping
#define b_t4                    6       // Track4 digital pin
#define b_clear                 5       // Clear digital pin
#define b_custom                4      // Custom digital pin

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
 
#define k_clear_GLOBAL          'k'      // Clear GLOBAL
#define k_custom_GLOBAL         'x'      // Reset all GLOBAL
#define k_globalreset_GLOBAL    'g'      // Globl reset COMBINATION
 
//SERIAL CODING FOR LED COMMUNICATION
#define REC_SERIAL_CODE         'r'      
#define PLAY_SERIAL_CODE        'p'   
#define PLAY_RELOOP             's'   
#define VOL_SERIAL_CODE         'v'   
#define RESET_SERIAL_CODE       'g'
#define MUTE_SERIAL_CODE        'm'
#define T1_SERIAL_CODE          '1'      
#define T2_SERIAL_CODE          '2'      
#define T3_SERIAL_CODE          '3'
#define T4_SERIAL_CODE          '4'

//BUTTON STATE CONTROL VARIABLES
unsigned int last_recplay, this_recplay; 
unsigned int last_stop, this_stop; 
unsigned int last_mode, this_mode; 
unsigned int last_t1, this_t1; 
unsigned int last_t2, this_t2; 
unsigned int last_t3, this_t3;
unsigned int last_t4, this_t4;
unsigned int last_clear, this_clear; 
unsigned int last_custom, this_custom; 
//BUTTON STATE TIMING VARIABLES (For longpress buttons only)
unsigned int startT_clear, stopT_clear; 
unsigned int startT_mode, stopT_mode; 

//MOBIUS MODE
unsigned int mode; //0 = REC; 1 = PLAY; 2 = VOL
char current_track; //Currently selected track [for LED management]
int recpresses; //Number of recording presses since reset (REC or Tracks)
unsigned long int startT_loop=0; //Loop start moment and length [in millis()]
unsigned int loop_duration=0; 

// MIDI Initialization
void noteOn(int pitch) {
  MidiUSB.sendMIDI({0x09, 0x90, pitch, 127});
  MidiUSB.flush();
}

void noteOff(int pitch) {
  MidiUSB.sendMIDI({0x08, 0x80, pitch, 0});
  MidiUSB.flush();
}
// LCD Initialization
LiquidCrystal_I2C lcd(0x27, 20, 4);

void updateProgressBar(unsigned long count, unsigned long totalCount, int lineToPrintOn) {
    double factor = totalCount/100.0;          
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
  if (recpresses>=2) {
    updateProgressBar(millis()-startT_loop,loop_duration,2);
    updateProgressBar(millis()-startT_loop,loop_duration,3);
  }
  if (millis()-startT_loop>=loop_duration && recpresses>=2) {
    startT_loop = millis();
    lcd.clear();
  }
}

void send_command(char key_command) {
  noteOn(key_command);
  // Command is logged in the Serial buffer to be read form a PCgr
  // Serial.print("Recieved: "); // Serial.println(key_command);
}

void global_reset() {
  current_track = T1_SERIAL_CODE;
  Serial1.begin(9600);
  Serial.begin(9600);
  Serial1.print(RESET_SERIAL_CODE);
  lcd.clear();
  lcd.setCursor(2,1); lcd.print("RESETTING...");
  last_recplay = HIGH;
  last_stop = HIGH;
  last_mode = HIGH;
  last_t1 = HIGH;
  last_t2 = HIGH;
  last_t3 = HIGH;
  last_t4 = HIGH;
  last_clear = HIGH; 
  last_custom = HIGH;
  //Global reset command sent to Mobius
  send_command(k_globalreset_GLOBAL);
  delay(1000);
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("READY");
  delay(500);
  lcd.clear();
  //Default MODE set to REC
  send_command(k_mode_PLAY);
  lcd.init(); lcd.backlight();
  lcd.createChar(0, zero);lcd.createChar(1, one);lcd.createChar(2, two);lcd.createChar(3, three);
  lcd.createChar(4, four);lcd.createChar(5, five);
  startT_loop = 0; loop_duration = 0;
  mode = REC;
  recpresses = 0;
}

//BUTTON PRESSED EVENTS
void recplay_pressed() {
  if (mode == PLAY) {
    send_command(k_recplay_PLAY);
    startT_loop=millis();
    lcd.clear();
    Serial1.print(PLAY_RELOOP);
  }else{
    send_command(k_recplay_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
      // Serial.print("> First press: ");
      // Serial.print(startT_loop); // Serial.println("ms");
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
      // Serial.print("> Second press: ");
      // Serial.print(millis()); // Serial.println("ms");
      // Serial.print(" --> Loop Duration: ");
      // Serial.print(loop_duration); // Serial.println("ms");
    }
  }
}

void stop_shortpressed() {
  if (mode == PLAY) {
    send_command(k_stop_PLAY);
  }else{
    send_command(k_stop_REC);
  }
  Serial1.print(MUTE_SERIAL_CODE);
}

void mode_shortpressed() {
  if (mode == PLAY) {
    send_command(k_mode_PLAY);
    mode = REC;
    Serial1.print(REC_SERIAL_CODE);
  }else{
    send_command(k_mode_REC);
    mode = PLAY;
    Serial1.print(PLAY_SERIAL_CODE);
  }
}

void t1_pressed() {
  digitalWrite(13,!digitalRead(13));
  if (mode == PLAY) {
    send_command(k_t1_PLAY);
  }else if (mode == REC) {
    send_command(k_t1_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
      // Serial.print("> First press: ");
      // Serial.print(startT_loop); // Serial.println("ms");
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
      // Serial.print("> Second press: ");
      // Serial.print(millis()); // Serial.println("ms");
      // Serial.print(" --> Loop Duration: ");
      // Serial.print(loop_duration); // Serial.println("ms");
    }
  }else{
    send_command(k_t1_VOL);
  }
  current_track = T1_SERIAL_CODE;
  Serial1.print(current_track);
}

void t2_pressed() {
  digitalWrite(13,!digitalRead(13));
  if (mode == PLAY) {
    send_command(k_t2_PLAY);
  }else if (mode == REC) {
    send_command(k_t2_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
      // Serial.print("> First press: ");
      // Serial.print(startT_loop); // Serial.println("ms");
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
      // Serial.print("> Second press: ");
      // Serial.print(millis()); // Serial.println("ms");
      // Serial.print(" --> Loop Duration: ");
      // Serial.print(loop_duration); // Serial.println("ms");
    }
  }else{
    send_command(k_t2_VOL);
  }
  current_track = T2_SERIAL_CODE;
  Serial1.print(current_track);
}

void t3_pressed() {
  digitalWrite(13,!digitalRead(13));
  if (mode == PLAY) {
    send_command(k_t3_PLAY);
  }else if (mode == REC) {
    send_command(k_t3_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
      // Serial.print("> First press: ");
      // Serial.print(startT_loop); // Serial.println("ms");
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
      // Serial.print("> Second press: ");
      // Serial.print(millis()); // Serial.println("ms");
      // Serial.print(" --> Loop Duration: ");
      // Serial.print(loop_duration); // Serial.println("ms");
    }
  }else{
    send_command(k_t3_VOL);
  }
  current_track = T3_SERIAL_CODE;
  Serial1.print(current_track);
}

void t4_pressed() {
  if (mode == PLAY) {
    send_command(k_t4_PLAY);
  }else{
    send_command(k_t4_REC);
    if (recpresses == 0) {
      startT_loop = millis();
      recpresses++;
      // Serial.print("> First press: ");
      // Serial.print(startT_loop); // Serial.println("ms");
    }else if (recpresses == 1) {
      loop_duration = millis()-startT_loop;
      recpresses++;
      // Serial.print("> Second press: ");
      // Serial.print(millis()); // Serial.println("ms");
      // Serial.print(" --> Loop Duration: ");
      // Serial.print(loop_duration); // Serial.println("ms");
    }
  }
  current_track = T4_SERIAL_CODE;
  Serial1.print(current_track);
}

void clear_pressed() {
  send_command(k_clear_GLOBAL);
}

void custom_pressed() {
  send_command(k_custom_GLOBAL);
}

void clear_longpressed() {
  send_command(k_globalreset_GLOBAL);
  Serial1.print(RESET_SERIAL_CODE);
  setup(); 
}

void mode_longpressed() {
  if (mode != VOL) {
    ;
    send_command(k_mode_VOL);
    Serial1.print(VOL_SERIAL_CODE); 
  }
  else {
    mode = REC;
    Serial1.print(REC_SERIAL_CODE);
  }
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
  pinMode(b_custom, INPUT_PULLUP);
    // MIDI.begin();
  Serial1.begin(DEFAULT_BAUD_RATE);
  // Serial.begin(DEFAULT_BAUD_RATE);
  delay(20);
  Keyboard.begin();
  lcd.init(); lcd.backlight();
  lcd.createChar(0, zero);
  lcd.createChar(1, one);
  lcd.createChar(2, two);
  lcd.createChar(3, three);
  lcd.createChar(4, four);
  lcd.createChar(5, five);
  global_reset();
  // MIDI.sendNoteOn(10,20,30);
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
    startT_clear = millis();
    clear_pressed();
  }
  //Clear switch detection - LONG PRESS
  if (this_clear == HIGH && last_clear== LOW) {
    stopT_clear = millis();
    if (stopT_clear-startT_clear>=T_LONGPRESS) {
      clear_longpressed();
    }
  }
  last_clear = this_clear;

  //Custom switch detection
  this_custom = digitalRead(b_custom);
  if (this_custom == LOW && last_custom == HIGH) {
    custom_pressed();
  }
  last_custom = this_custom;

  update_lcd();

  delay(10);
}
