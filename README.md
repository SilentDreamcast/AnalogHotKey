# AnalogHotKey
Another Arduino thingy I guess... Turns an Arduino with native USB into a MIDI/HID device using an Arduino Micro/Leonardo/DUE.

Built in less than 24 hours as a birthday present. Sorry if it's buggy but you can fix it!

If you use VoiceMeeter there is a MIDI config as an example. You'll need to add "Arduino Leonardo" to MIDI Input Device and MIDI Ouput Device.

The buttons have special combos.
  Button 1 + 4 set the buttons to toggle(default) or momentary.
  Button 2 + 4 set the buttons to keyboard mode.
  Button 3 + 4 nothing yet, add your own!
  
Read the code, there is lots to change. Arduino IDE needs to be set as Arduino Leonardo or Pro Micro depending on your board
You will need the Adafruit_NeoPixel by Adafruit and MIDIUSB by Arduino libraries. Keyboard library is part of base IDE.
  
SketchUp 2017 model is included.
3mm Acrylic used as the body, cut with a laser. SVG file included.
Arduino Code is included.
