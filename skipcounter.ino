/*
 * Skip Counter, by Laurel Carraway, room K2
 *
 * Translated from verbal algorithm description (see attached
 * notes) by her father.
 *
 * Works on an adafruit Trinket Pro microcontroller board;
 * works on cheaper Trinket boards if one sacrifices the encoder
 * in favor of a skip-number-advance button.
 *
 * Encoder handling routine by Mike Barela.
 *
 */

#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <avr/power.h>

Adafruit_AlphaNum4 disp = Adafruit_AlphaNum4();

uint8_t button_states = 0;

int total = 0;
int skip_value = 2;
static uint8_t enc_prev_pos   = 0;
static uint8_t enc_flags      = 0;

#define TRINKET_PINx    PIND
#define ADD_BUTTON      3
#define SUBTRACT_BUTTON 4

#define SKIP_ADJUST_PIN_LEFT 6
#define SKIP_ADJUST_PIN_RIGHT 5

void setup() {
  if(F_CPU == 16000000) clock_prescale_set(clock_div_1);

  // enable the display
  disp.begin(0x70);
  updateDisplay();

  pinMode(ADD_BUTTON, INPUT_PULLUP);
  pinMode(SUBTRACT_BUTTON, INPUT_PULLUP);
  pinMode(SKIP_ADJUST_PIN_LEFT, INPUT_PULLUP);
  pinMode(SKIP_ADJUST_PIN_RIGHT, INPUT_PULLUP);

  // Get the initial state of the encoder position
  if (digitalRead(SKIP_ADJUST_PIN_LEFT) == LOW) {
    enc_prev_pos |= (1 << 0);
  }
  if (digitalRead(SKIP_ADJUST_PIN_RIGHT) == LOW) {
    enc_prev_pos |= (1 << 1);
  }
}

// Return a -1, 0 or 1 signifying the encoder knob having been turned
// left, not at all, or right respectively.  Cribbed almost verbatim from
// Mike Barela's encoder-volume-control at 
// https://learn.adafruit.com/pro-trinket-rotary-encoder/example-rotary-encoder-volume-control
int8_t knobTurned()
{
  int8_t enc_action = 0; // 1 or -1 if moved, sign is direction
  uint8_t enc_cur_pos = 0;
  
  // read in the encoder state first
  if (bit_is_clear(TRINKET_PINx, SKIP_ADJUST_PIN_LEFT)) {
    enc_cur_pos |= (1 << 0);
  }
  if (bit_is_clear(TRINKET_PINx, SKIP_ADJUST_PIN_RIGHT)) {
    enc_cur_pos |= (1 << 1);
  }
 
  // if any rotation at all
  if (enc_cur_pos != enc_prev_pos)
  {
    if (enc_prev_pos == 0x00)
    {
      // this is the first edge
      if (enc_cur_pos == 0x01) {
        enc_flags |= (1 << 0);
      }
      else if (enc_cur_pos == 0x02) {
        enc_flags |= (1 << 1);
      }
    }
 
    if (enc_cur_pos == 0x03)
    {
      // this is when the encoder is in the middle of a "step"
      enc_flags |= (1 << 4);
    }
    else if (enc_cur_pos == 0x00)
    {
      // this is the final edge
      if (enc_prev_pos == 0x02) {
        enc_flags |= (1 << 2);
      }
      else if (enc_prev_pos == 0x01) {
        enc_flags |= (1 << 3);
      }
 
      // check the first and last edge
      // or maybe one edge is missing, if missing then require the middle state
      // this will reject bounces and false movements
      if (bit_is_set(enc_flags, 0) && (bit_is_set(enc_flags, 2) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 2) && (bit_is_set(enc_flags, 0) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 1) && (bit_is_set(enc_flags, 3) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      }
      else if (bit_is_set(enc_flags, 3) && (bit_is_set(enc_flags, 1) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      }
 
      enc_flags = 0; // reset for next time
    }
  }
 
  enc_prev_pos = enc_cur_pos;
  return enc_action;
}


// Return 1 if the given button has been released
// since the last call (falling-edge detect).
int buttonReleased(int b) {
  int state = digitalRead(b);  
  if (button_states & (1 << b)) {
    if (state == LOW) {
      button_states &= ~(1 << b);
      return 1;
    }
  } else if (state == HIGH) {
    button_states |= (1 << b);
  }
  return 0;
}

// Update the display with whatever ought to
// be shown.
void updateDisplay() {
  char buffer[4];
  if (skip_value >= 10) {
    sprintf(buffer, "%2d", skip_value);
    disp.writeDigitAscii(0, buffer[0]);
    disp.writeDigitAscii(1, buffer[1], true);
    sprintf(buffer, "%2d", total);
    disp.writeDigitAscii(2, buffer[0]);
    disp.writeDigitAscii(3, buffer[1]);
  } else if (skip_value < 10) {
    disp.writeDigitAscii(0, '0' + skip_value, true);
    sprintf(buffer, "%3d", total);
    disp.writeDigitAscii(1, buffer[0]);
    disp.writeDigitAscii(2, buffer[1]);
    disp.writeDigitAscii(3, buffer[2]);
  }
  disp.writeDisplay();
}

// If there's room, show the current arithmetic operation for a fraction
// of a second.
void showArithmetic(char ch) {
  int pos;
  if (skip_value < 10) {
    pos = 1;
  } else if (total < 10) {
    pos = 2;
  } else {
    // no room to show arithmetic op
    return;
  }
  disp.writeDigitAscii(pos, ch);
  disp.writeDisplay();
  delay(250);
  disp.writeDigitAscii(pos, ' ');
}

// Main loop
void loop() {
  int8_t knob_action = knobTurned();
  if (knob_action == -1) {
    if (skip_value > 0) {
      skip_value--;
      updateDisplay();
    }
  } else if (knob_action == 1) {
    if (skip_value < 10) {
      if (skip_value < 9 || total < 100) {
        skip_value++;
        updateDisplay();
      }
    }
  }
  
  if (buttonReleased(ADD_BUTTON)) {
    showArithmetic('+');
    total += skip_value;
    // Wrap back to 0 if we're out of room on the display to go higher
    if (total == 100 && skip_value < 10) {
      // tolerate this case, since there's still some separation on the display
    }
    else if (total > 100) {
      total = 0;
    }
    updateDisplay();
  } else if (buttonReleased(SUBTRACT_BUTTON)) {
    showArithmetic('-');
    total -= skip_value;
    if (total < 0) {
      total = 0;
    }
    updateDisplay();
  }
  delay(5); 
}
