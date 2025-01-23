#include <usbmidi.h>              // USB 2 MIDI
// MOZZI
#include <Mozzi.h>
#include <Oscil.h>                // Oscillator template
#include <tables/sin2048_int8.h>  // Sine wave table for oscillator
#include <Ead.h>                  // exponential attack decay
//

// MIDI Constants
#define MIDI_NOTE_OFF 0b10000000
#define MIDI_NOTE_ON 0b10010000
#define MIDI_CONTROL 0b10110000  // MIDI Control Change message
// CONTROL RATE Constants
#define MOZZI_CONTROL_RATE 64  // Hz, powers of 2 are most reliable

// CREATE AUDIO ELEMENTS
//Oscillators
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> Osc1(SIN2048_DATA);  // Sine wave oscillator
// Envelopes
Ead Env1(MOZZI_CONTROL_RATE);  // resolution will be MOZZI_CONTROL_RATE

// MIDI variables
uint8_t command = 0, channel = 0, key = 0, velocity = 0;
bool triggerNoteOn = false;
bool triggerNoteOff = false;
uint8_t ccNumber = 0, ccValue = 0;  // Variables for CC message
uint8_t cc10Value = 0;              // Variable to store the value of CC #10
// AUDIO variables
int env1Value = 0, frequency = 0;

void setup() {
  startMozzi();
  Serial.begin(9600);  // Optional for debugging
}


void updateControl() {
  // Handle USB MIDI input, update midi variables
  checkMidi();

  // CONTROL LOOP

  // MidiTrigger-to-Gate Control
  if (triggerNoteOn) {
    Env1.start(10, 100);
    frequency = noteToFreq(key);  // Convert MIDI note to frequency
  }
  if (triggerNoteOff) {
    // frequency = 0;  // Silence the oscillator
  }

  // Update Envelopes values
  env1Value = (int)Env1.next();

  // Update Audio Oscillators settings
  Osc1.setFreq(frequency);  // Set oscillator frequency}
}

AudioOutput updateAudio() {
  // Generate the next audio sample
  int signal = (Osc1.next() * env1Value) >> 8;
  return MonoOutput::from8Bit(signal);
}

void loop() {
  audioHook();  // Required Mozzi function
}

// *******************************************************
// FUNCTIONS
// *******************************************************

// Check Midi IN and update MIDI variables
void checkMidi() {
  USBMIDI.poll();
  while (USBMIDI.available()) {
    // Read and parse the MIDI message
    while (!(USBMIDI.peek() & 0b10000000)) USBMIDI.read();  // Skip stray data bytes

    command = USBMIDI.read();
    channel = (command & 0b00001111) + 1;  // Extract channel
    command = command & 0b11110000;        // Extract command type

    // NOTE
    // Extract key and velocity
    if (command == MIDI_NOTE_ON || command == MIDI_NOTE_OFF) {
      if (USBMIDI.peek() & 0b10000000) continue;  // Skip if invalid
      key = USBMIDI.read();                       // Note number
      if (USBMIDI.peek() & 0b10000000) continue;  // Skip if invalid
      velocity = USBMIDI.read();                  // Velocity
    }
    // Handle NOTE_ON
    if (command == MIDI_NOTE_ON && velocity > 0) {
      triggerNoteOn = true;
      // frequency = noteToFreq(key);  // Should this be here? Convert MIDI note to frequency
    }
    // Handle NOTE_OFF
    if (command == MIDI_NOTE_OFF || (command == MIDI_NOTE_ON && velocity == 0)) {
      triggerNoteOff = true;
    }

    // CONTROL CHANGE
    // Handle Control Change (CC)
    if (command == MIDI_CONTROL) {
      if (USBMIDI.peek() & 0b10000000) continue;  // Skip if invalid
      ccNumber = USBMIDI.read();                  // CC number
      if (USBMIDI.peek() & 0b10000000) continue;  // Skip if invalid
      ccValue = USBMIDI.read();                   // CC value

      // Check for CC #10 and store its value
      if (ccNumber == 10) {
        cc10Value = ccValue;  // Store the value of CC #10
      }
    }
  }
}

// Convert MIDI note to frequency
int base_a4 = 440;                                    // A4 = 440 Hz
constexpr double FREQ_FACTOR = pow(2.0, 1.0 / 12.0);  // Precalculate 12th root of 2
constexpr double BASE_A4_FREQ = 440.0;
int noteToFreq(uint8_t note) {
  return BASE_A4_FREQ * pow(FREQ_FACTOR, note - 69);
}
float noteToFreqFloat(float note) {
  return BASE_A4_FREQ * pow(FREQ_FACTOR, note - 69);
}
//