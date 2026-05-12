#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// Hairless MIDI default baud rate used in this project.
#define SERIAL_BAUD 115200
#define MIDI_CHANNEL 1
#define IR_PIN 6
#define BOW_CC 11
#define IR_STATE_CC 12

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t lastTouched = 0;
uint16_t currTouched = 0;
bool mpr121Found = false;
bool lastIrState = HIGH;
unsigned long lastIrEdgeMs = 0;
unsigned long lastDecayMs = 0;
int bowValue = 0;
int lastSentBowValue = -1;
int lastSentIrState = -1;

// MPR121 electrodes connected by the user.
// These are mapped to Pure Data buttons/sounds 1-6.
const uint8_t touchPads[6] = {1, 3, 5, 6, 8, 10};

// MIDI note numbers used by the Pure Data patch.
// 60 -> PD 1, 61 -> PD 2, 62 -> PD 3, 63 -> PD 4, 64 -> PD 5, 65 -> PD 6.
const uint8_t midiNotes[6] = {60, 61, 62, 63, 64, 65};

void sendNoteOn(byte note, byte velocity) {
  byte status = 0x90 | ((MIDI_CHANNEL - 1) & 0x0F);
  Serial.write(status);
  Serial.write(note & 0x7F);
  Serial.write(velocity & 0x7F);
}

void sendNoteOff(byte note) {
  byte status = 0x80 | ((MIDI_CHANNEL - 1) & 0x0F);
  Serial.write(status);
  Serial.write(note & 0x7F);
  Serial.write((byte)0);
}

void sendCC(byte controller, byte value) {
  byte status = 0xB0 | ((MIDI_CHANNEL - 1) & 0x0F);
  Serial.write(status);
  Serial.write(controller & 0x7F);
  Serial.write(value & 0x7F);
}

void updateBowFromIr() {
  unsigned long now = millis();
  bool irState = digitalRead(IR_PIN);

  int irMidiState = irState == LOW ? 127 : 0;
  if (irMidiState != lastSentIrState) {
    sendCC(IR_STATE_CC, irMidiState);
    lastSentIrState = irMidiState;
  }

  if (irState != lastIrState) {
    if (lastIrEdgeMs != 0) {
      unsigned long interval = now - lastIrEdgeMs;

      if (interval >= 8 && interval <= 500) {
        int speedValue = map(constrain(interval, 15, 260), 260, 15, 18, 127);
        bowValue = max(bowValue, speedValue);
      } else {
        bowValue = max(bowValue, 45);
      }
    } else {
      bowValue = max(bowValue, 45);
    }

    lastIrEdgeMs = now;
    lastIrState = irState;

    if (bowValue > lastSentBowValue) {
      sendCC(BOW_CC, bowValue);
      lastSentBowValue = bowValue;
    }
  }

  if (now - lastIrEdgeMs > 120 && now - lastDecayMs >= 20) {
    bowValue = max(0, bowValue - 5);
    lastDecayMs = now;
  }

  if (abs(bowValue - lastSentBowValue) >= 2 || (bowValue == 0 && lastSentBowValue != 0)) {
    sendCC(BOW_CC, bowValue);
    lastSentBowValue = bowValue;
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(IR_PIN, INPUT);
  lastIrState = digitalRead(IR_PIN);

  // Send one startup MIDI message so Hairless can confirm the serial bridge works.
  sendCC(BOW_CC, 0);
  lastSentBowValue = 0;

  mpr121Found = cap.begin(0x5A);
  if (mpr121Found) {
    cap.setAutoconfig(true);
  }
}

void loop() {
  updateBowFromIr();

  if (!mpr121Found) {
    delay(5);
    return;
  }

  currTouched = cap.touched();

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t pad = touchPads[i];
    uint8_t note = midiNotes[i];

    bool isTouched = currTouched & _BV(pad);
    bool wasTouched = lastTouched & _BV(pad);

    if (isTouched && !wasTouched) {
      sendNoteOn(note, 100);
    }

    if (!isTouched && wasTouched) {
      sendNoteOff(note);
    }
  }

  lastTouched = currTouched;
  delay(5);
}
