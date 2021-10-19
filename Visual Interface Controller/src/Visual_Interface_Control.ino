#include <Arduino.h>

#define REC          "r"      // --> Corresponds to REC_SERIAL_CODE
#define PLAY_RELOOP  "s"      // --> Corresponds to PLAY_RELOOP_CODE
#define PLAY         "p"      // --> Corresponds to PLAY_SERIAL_CODE
#define VOL          "v"      // --> Corresponds to VOL_SERIAL_CODE
#define RESET        "g"      // --> Corresponds to RESET_SERIAL_CODE
#define MUTE         "m"      // --> Corresponds to MUTE_SERIAL_CODE
#define T1           "1"      // --> Corresponds to T1_SERIAL_CODE
#define T2           "2"      // --> Corresponds to T2_SERIAL_CODE
#define T3           "3"      // --> Corresponds to T3_SERIAL_CODE
#define T4           "4"      // --> Corresponds to T4_SERIAL_CODE

// Pins hosting the RED terminal of each LED [pins referred as on the MUX]
#define MODE_LED      0    
#define T1_LED        3
#define T2_LED        6
#define T3_LED        9
#define T4_LED        12
// Pins hosting MUX control terminals [pins referred as on the microController]
#define MUX_S0        2
#define MUX_S1        3
#define MUX_S2        4
#define MUX_S3        5
#define MUX_ENABLE    6         
  
#define RED           0  
#define GREEN         1  
#define BLUE          2          
  


#define DEFAULT_BAUD_RATE 9600

/* > MUX PINS CORRESPONDENCE
   M_r  | M_g  | M_b  | T1_r | T1_g | T1_b | T2_r | T2_g | T2_b | T3_r | T3_g | T3_b | T4_r | T4_g | T4_b |
    0   |  1   |  2   |   3  |   4  |   5  |   6  |   7  |   8  |   9  |  10  |  11  |  12  |  13  |  14  | */

String cmd; //Recieved command 
String status[5]; //The current statuses in which each mode track is [mode, t1, t2, t3, t4]
String saved_status[5]; //The statuses are saved every time they are going to be modified

void led(int led_code_dec, int color) {
  // Serial.print("> Led: "); // Serial.print(led_code_dec); // Serial.print("  Color: "); // Serial.println(color);
  led_code_dec = led_code_dec+color;
  // Serial.print("  DEC: "); // Serial.print(led_code_dec); // Serial.print(" BIN: ");
  int led_code_bin[4]={0,0,0,0};
  int i = 0;
  while (led_code_dec > 0) {
      led_code_bin[i] = led_code_dec % 2;
      led_code_dec = led_code_dec / 2;
      i++;
  }

  digitalWrite(MUX_S3, led_code_bin[0]);
  digitalWrite(MUX_S2, led_code_bin[1]);
  digitalWrite(MUX_S1, led_code_bin[2]);
  digitalWrite(MUX_S0, led_code_bin[3]);
}

void MUX_disable() {
  digitalWrite(MUX_ENABLE, HIGH);
}
void MUX_enable() {
  digitalWrite(MUX_ENABLE, LOW);
}

void ledcontroller_reset() {
  //When resetting, all LEDs are turned on and off in sequence in the course of a second
  for (int l = 0; l < 14; l+=3) {
    led(l, RED);
    delay(1000/3/5); 
  }
  for (int l = 0; l < 14; l+=3) {
    led(l, GREEN);
    delay(1000/3/5); 
  }
  for (int l = 0; l < 14; l+=3) {
    led(l, BLUE);
    delay(1000/3/5); 
  }
  // delay(500); 
  // HERE the controller is ready to work
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);
  // delay(20);
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  pinMode(MUX_ENABLE, OUTPUT);
  ledcontroller_reset();
  // Serial.readString();
  // All tracks are initialized to PLAY,  MODE initialized to REC 
  status[0] = REC; 
  for (int i=1;i<5;i++) status[i] = PLAY;
  for (int i=0;i<5;i++) saved_status[i] = "n";
}

void loop() {
  if (Serial.available() > 0) {
    cmd = Serial.readString();

    // for (int i=0;i<5;i++) Serial.print(saved_status[i]);
    // Serial.println();11mggr11pbcyyyy

    // MODE_SERIAL recieved
    if (cmd == REC || cmd == PLAY || cmd == VOL ) {
      if (cmd == VOL) {  
        if (status[0]==VOL) {                                   //When VOL is recieved in VOL MODE, exiting VOL MODE and everything is PLAY
          status[1] = PLAY; status[2] = PLAY;               
          status[3] = PLAY; status[4] = PLAY;
          status[0] = PLAY; 
        }     
      } else if (cmd == PLAY) {
        if (status[1]==REC) status[1]=PLAY; 
        if (status[2]==REC) status[2]=PLAY; 
        if (status[3]==REC) status[3]=PLAY; 
        if (status[4]==REC) status[4]=PLAY;
      }                                      
      status[0] = cmd;                                          // Changes MODE    
    } 
    else if (cmd == PLAY_RELOOP ) {
        status[1]=PLAY; status[2]=PLAY;                         // PLAY unmutes everything and restarts the loop
        status[3]=PLAY; status[4]=PLAY; 
        status[0]=PLAY;
    }

    // MUTE_SERIAL recieved
    else if (cmd == MUTE) {
      if (status[1]==MUTE && status[2]==MUTE && 
      status[3]==MUTE && status[4]==MUTE) {
        for (int i = 1; i < 5; i++)
          status[i] = saved_status[i];
        Serial.print("Saved status: ");
        // for (int i=1; i<5;i++) Serial.print(saved_status[i]);
        // Serial.println(); delay(500);
        // status[1] = MUTE; status[2] = MUTE;                     // All tracks are set to the MUTE status
        // status[3] = MUTE; status[4] = MUTE; 
      }else{
        for (int i = 1; i < 5; i++) {
          saved_status[i] = status[i];
        }
        // for (int i=1; i<5;i++) Serial.print(saved_status[i]);
        // Serial.println(); delay(500);
        status[1] = MUTE; status[2] = MUTE;                     // All tracks are set to the MUTE status
        status[3] = MUTE; status[4] = MUTE; 
      }
    }

    // T1_SERIAL recieved
    else if (cmd == T1) {                             
        if (status[0] == REC) {                                 // -> When mode is REC
            if (status[1]!=REC) status[1]=REC;
            else status[1]=PLAY; 
            if (status[2]==REC) status[2]=PLAY; 
            if (status[3]==REC) status[3]=PLAY; 
            if (status[4]==REC) status[4]=PLAY;                 // --> Set all other tracks to PLAY                                                       // --> Change it to REC if it was PLAY
        } else if (status[0] == PLAY) {                         // -> When mode is PLAY
          if (status[1]!=PLAY) {
            status[1]=PLAY;                                     // --> Change it to PLAY if it was MUTE
          } else {
            status[1]=MUTE;
          }                                                     // --> Change it to MUTE if it was PLAY     
        } else if (status[0] == VOL) {
          
        }                    
    }

    // T2_SERIAL_RECIEVED
    else if (cmd == T2) {                                 
        if (status[0] == REC)  {                                // -> When mode is REC
            if (status[2]!=REC) status[2]=REC;
            else status[2]=PLAY; 
            if (status[1]==REC) status[1]=PLAY; 
            if (status[3]==REC) status[3]=PLAY; 
            if (status[4]==REC) status[4]=PLAY;                 // --> Set all other tracks to PLAY                                                     // --> Change it to REC if it was PLAY
        } else if (status[0] == PLAY) {                         // -> When mode is PLAY
          if (status[2]!=PLAY) {
            status[2]=PLAY;                                     // --> Change it to PLAY if it was MUTE
          } else {
            status[2]=MUTE;
          }                                                     // --> Change it to MUTE if it was PLAY 
        } else if (status[0] == VOL) {
          
        }                     
    }

    // T3_SERIAL_RECIEVED
    else if (cmd == T3) {                                  
        if (status[0] == REC) {                                 // -> When mode is REC
            if (status[3]!=REC) status[3]=REC;
            else status[3]=PLAY; 
            if (status[1]==REC) status[1]=PLAY; 
            if (status[2]==REC) status[2]=PLAY; 
            if (status[4]==REC) status[4]=PLAY;                 // --> Set all other tracks to PLAY                                                     // --> Change it to REC if it was PLAY
        } else if (status[0] == PLAY) {                         // -> When mode is PLAY
          if (status[3]!=PLAY) {
            status[3]=PLAY;                                     // --> Change it to PLAY if it was MUTE
          } else {
            status[3]=MUTE;
          }                                                     // --> Change it to MUTE if it was PLAY   
        } else if (status[0] == VOL) {
          
        }                    
    }

    // T4_SERIAL_RECIEVED
    else if (cmd == T4) {                                   
        if (status[0] == REC) {                                 // -> When mode is REC
            if (status[4]!=REC) status[4]=REC;
            else status[4]=PLAY; 
            if (status[1]==REC) status[1]=PLAY; 
            if (status[2]==REC) status[2]=PLAY; 
            if (status[3]==REC) status[3]=PLAY;                 // --> Set all other tracks to PLAY                                                     // --> Change it to REC if it was PLAY
        } else if (status[0] == PLAY) {                         // -> When mode is PLAY
          if (status[4]!=PLAY) {
            status[4]=PLAY;                                     // --> Change it to PLAY if it was MUTE
          } else {
            status[4]=MUTE;
          }                                                     // --> Change it to MUTE if it was PLAY 
        }                      
    }
    else if (cmd == RESET) {                          // RESET request recieved
      setup();
    }

  // delay(10);
  // Serial.println(cmd);
  cmd = "Empty";
  
  // Serial.readString();11pabyyy

  }
  // Here status is correctly updated. 
  // Serial.print("STATUS: ");
  // for (int i=0; i<5; i++) {
  //   // Serial.print(status[i]);
  // }
  // Serial.println();
  // State control on MODE status
    if (status[0] == REC) { // REC mode turns led RED
      led(MODE_LED,RED);
      Serial.print("R");
    }else if (status[0] == PLAY) { // REC mode turns led RED
      led(MODE_LED,GREEN);
      Serial.print("G");
    }else if (status[0] == VOL) { // REC mode turns led RED
      led(MODE_LED,BLUE);
      Serial.print("B");
    }
    // delay(500);
  // State control on Tracks status
  for (int i = 1; i<5; i++) {
    if (status[i] == REC) { // REC mode turns led RED
      led(i*3,RED);
      Serial.print("R");
    }
    if (status[i] == PLAY) { // REC mode turns led RED
      led(i*3,GREEN);
      Serial.print("G");
    }
    if (status[i] == MUTE) { // REC mode turns led RED
      led(i*3,BLUE);
      Serial.print("B");
    }
    // delay(500);
  }
  Serial.println();
  // Serial.readString();
  // delay(5000);
}

