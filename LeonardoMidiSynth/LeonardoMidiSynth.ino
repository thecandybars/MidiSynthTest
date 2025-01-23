#include <Mozzi.h>
#include <Oscil.h> // Oscillator template
#include <tables/sin2048_int8.h> // Sine wave table for oscillator
#include <usbmidi.h>

// MIDI Constants
#define MIDI_NOTE_OFF   0b10000000
#define MIDI_NOTE_ON    0b10010000
#define MIDI_CONTROL    0b10110000 // MIDI Control Change message

// Define oscillator for audio synthesis
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA); // Sine wave oscillator

// Convert MIDI note to frequency
int base_a4 = 440; // A4 = 440 Hz
int noteToFreq(uint8_t note) {
  return base_a4 * pow(2.0, (note - 69) / 12.0);
}
float noteToFreqFloat(float note) {
  return base_a4 * pow(2.0, (note - 69) / 12.0);
}

// MIDI variables
uint8_t command = 0, channel = 0, key = 0, velocity = 0;
uint8_t ccNumber = 0, ccValue = 0; // Variables for CC message
uint8_t cc10Value = 0; // Variable to store the value of CC #10

void setup() {
  // Initialize Mozzi
  startMozzi();

  // Initialize MIDI
  Serial.begin(9600); // Optional for debugging
}

void updateControl() {
  // Handle USB MIDI input
  USBMIDI.poll();

  while (USBMIDI.available()) {
    // Read and parse the MIDI message
    while (!(USBMIDI.peek() & 0b10000000)) USBMIDI.read(); // Skip stray data bytes

    command = USBMIDI.read();
    channel = (command & 0b00001111) + 1; // Extract channel
    command = command & 0b11110000; // Extract command type

    if (command == MIDI_NOTE_ON || command == MIDI_NOTE_OFF) {
      if (USBMIDI.peek() & 0b10000000) continue; // Skip if invalid
      key = USBMIDI.read(); // Note number
      if (USBMIDI.peek() & 0b10000000) continue; // Skip if invalid
      velocity = USBMIDI.read(); // Velocity
    }

    // Handle NOTE_ON
    if (command == MIDI_NOTE_ON && velocity > 0) {
      float frequency = noteToFreq(key); // Convert MIDI note to frequency
      Serial.println(frequency);
      aSin.setFreq(frequency); // Set oscillator frequency
    }

    // Handle NOTE_OFF
    if (command == MIDI_NOTE_OFF || (command == MIDI_NOTE_ON && velocity == 0)) {
      aSin.setFreq(0); // Silence the oscillator
    }

    // Handle Control Change (CC)
    if (command == MIDI_CONTROL) {
      if (USBMIDI.peek() & 0b10000000) continue; // Skip if invalid
      ccNumber = USBMIDI.read(); // CC number
      if (USBMIDI.peek() & 0b10000000) continue; // Skip if invalid
      ccValue = USBMIDI.read(); // CC value

      // Check for CC #10 and store its value
      if (ccNumber == 10) {
        cc10Value = ccValue; // Store the value of CC #10
      }
    }
  }
}

AudioOutput updateAudio() {
  // Generate the next audio sample
  return MonoOutput::from8Bit((aSin.next()*cc10Value)>>8);
}

void loop() {
  // Required Mozzi function
  audioHook();
}

