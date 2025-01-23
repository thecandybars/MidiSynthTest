#include <midi_serialization.h>
#include <usbmidi.h>
#include <TimerOne.h>

void sendNote(uint8_t channel, uint8_t note, uint8_t velocity) {
  USBMIDI.write((velocity != 0 ? 0x90 : 0x80) | (channel & 0xf));
  USBMIDI.write(note & 0x7f);
  USBMIDI.write(velocity & 0x7f);
}

void setup() {
  Serial.begin(9600);
  Timer1.initialize(500000);  // Set Timer1 to fire every 1,000,000 microseconds (1 second)
  Timer1.attachInterrupt(timerCallback); // Attach the interrupt service routine
}
bool noteOn = false;

void timerCallback() {
  sendNote(0, 64, noteOn==1?127:0);
  // if(noteOn){
  //   tone(10,200);
  // }else{
  //   noTone(10);
  // }
  Serial.println(noteOn);
  noteOn=!noteOn;
}


void loop() {
  //Handle USB communication
  USBMIDI.poll();

  while (USBMIDI.available()) {
    // We must read entire available data, so in case we receive incoming
    // MIDI data, the host wouldn't get stuck.
    u8 b = USBMIDI.read();
  }
  // Flush the output.
	USBMIDI.flush();
}
