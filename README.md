# LoopStation DIY Project
***
An ATMEGA328-based MIDI pedalboard for looping the sound from a guitar; the board is set up to work with the Mobius Looper VST plugin. The pedalboard allows 2 modes for looping on 4 tracks independently, with overdubbing, resetting, muting and multiplying functions: modes and selected tracks are displayed with different LED colors and from an LCD screen..

[See Ed Sheeran](https://www.youtube.com/watch?v=84GwOw-QVIQ) using the Chewie Monsta, the pedalboard that inspired this project!

![pedalboard](https://github.com/alberto-rota/LoopStation-DIY-Project/blob/main/pedalboard_photo.png)

## Pedals
The board consists of a set of 3 pedals for controlling the looping - REC/PLAY, STOP and MUTE - 4 pedals for selecting a track - BASE, RIFF, GROOVE and CHORUS - and two functional pedals -CUSTOM and UNDO-

Actions performed when pedals are pressed:
* **MODE:** Switches between *Record mode* and *Play mode*
* **REC/PLAY:** In *Record mode*, overdubs on the current track,even if unselected. In *Play mode*, restarts the loop for the tracks at the loop start 
* **STOP:** In *Record mode*, multiplies the current track . In *Play mode* mutes all tracks
* **BASE,RIFF,GROOVE,CHORUS:** In *Record mode*, toggles overdubbing on the selected track. In *Play mode* toggles the muting of the selected track
* **UNDO:** Deletes the most recent overdub from the loop stack
* **CUSTOM:** Performs the actions in the "Custom" Mobious script

## Materials
The board is build with: 
* 1 Arduino Leonardo (as an HID peripheral)
* 1 Arduino Nano
* 1 Multiplexer
* Jumper Wires
* 5 RGB LEDs
* 1 LCD I2C screen
* 9 Pedal Switches

The pedals are 3D printed in PLA

![wiring](https://github.com/alberto-rota/LoopStation-DIY-Project/blob/main/connection_schematics.png)
## A picture of the finished board
![finished](https://github.com/alberto-rota/LoopStation-DIY-Project/blob/main/finished_pedalboard.jpg)
