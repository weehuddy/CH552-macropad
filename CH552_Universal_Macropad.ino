// Pinout
// 3.4 - WS2812 NeoPixel GRB (D7->D6->D5)
// 3.3 - Encoder button
// 3.0 - Encoder A
// 3.1 - Encoder B
// 1.1 - Key U1
// 1.7 - Key U2
// 1.6 - Key U3
// ??? - Key U4 - I will update these once I get a six-key device
// ??? - Key U5
// ??? - Key U6

#define SERIAL_DEBUG  false

#include <WS2812.h>
#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

// Modifier Keys
#define MODIFIER_NONE     0
#define MODIFIER_CTRL     1
#define MODIFIER_SHIFT    2
#define MODIFIER_ALT      4
#define MODIFIER_GUI      8

// Button Action Types
#define ACTION_TYPE_NONE                0
#define ACTION_TYPE_KEY_PRESS           1
#define ACTION_TYPE_KEY_DOWN_UP         2
#define ACTION_TYPE_STRING              3
#define ACTION_TYPE_MOUSE_CLICK         4
#define ACTION_TYPE_MOUSE_DOUBLE_CLICK  5
#define ACTION_TYPE_MOUSE_DOWN_UP       6
#define ACTION_TYPE_MOUSE_TOGGLE        7

// Additional Mouse/Key Types
#define KEY_NONE          0
#define MOUSE_NONE        0

// Hardware connections
#define LED_PIN         34
#define LED_FUNC        neopixel_show_P3_4
#define KEY0_PIN        11
#define KEY1_PIN        17
#define KEY2_PIN        16
#define KEY3_PIN        12 // TODO: VERIFY
#define KEY4_PIN        13 // TODO: VERIFY
#define KEY5_PIN        14 // TODO: VERIFY
#define ENC_A_PIN       30
#define ENC_B_PIN       31
#define BUTTON_PIN      33

// LED Setup
#define NUM_LEDS        6
#define COLOR_PER_LEDS  3
#define NUM_BYTES       (NUM_LEDS*COLOR_PER_LEDS)
#if NUM_BYTES > 255
#error "NUM_BYTES can not be larger than 255."
#endif
__xdata uint8_t ledData[NUM_BYTES];

// Encoder/Scroll/Button Configuration
#define MIN_SCROLL_STEP 2
#define SCROLL_STEP     1
#define SCROLL_DIST     1
#if INVERT_SCROLLING
#define SCROLL_UP       SCROLL_DIST
#define SCROLL_DOWN     -SCROLL_DIST
#else
#define SCROLL_UP       -SCROLL_DIST
#define SCROLL_DOWN     SCROLL_DIST
#endif
#define KEY_PRESSED     LOW
#define KEY_RELEASED    HIGH

// Delay Configuration
#define ENCODER_SPIN_RESET_MS   200
#define SCROLL_SEND_MS          20
#define DEBOUNCE_MS             10
#define DOUBLE_CLICK_MS         200
#define ENTER_BOOTLOADER_MS     3000

// Key Layout Configuration, don't change this even if your macropad has fewer keys. They will just be ignored
#define MAX_KEYS            6
#define MAX_KEYS_W_ENCODER  7
#define ENCODER_INDEX       MAX_KEYS

/*
 * USER CONFIGURATION START
 */
bool INVERT_SCROLLING = false;
bool ALLOW_BOOTLOADER = true;

uint8_t LED_COLORS[MAX_KEYS][3] = {
  {255, 0, 0}, // Button 1 LED color: red, green, blue
  {0, 255, 0}, // Button 2 LED color
  {0, 0, 255}, // Button 3 LED color
  {0, 0, 0}, // Button 4 LED color
  {0, 0, 0}, // Button 5 LED color
  {0, 0, 0}, // Button 6 LED color
  // There is no LED for the encoder
};

uint8_t KEY_ACTION_TYPE[MAX_KEYS_W_ENCODER] = {
  ACTION_TYPE_KEY_DOWN_UP, // Button 1 action type
  ACTION_TYPE_KEY_DOWN_UP, // Button 2 action type
  ACTION_TYPE_KEY_DOWN_UP, // Button 3 action type
  ACTION_TYPE_KEY_DOWN_UP, // Button 4 action type
  ACTION_TYPE_KEY_DOWN_UP, // Button 5 action type
  ACTION_TYPE_KEY_DOWN_UP, // Button 6 action type
  ACTION_TYPE_KEY_PRESS, // The last "key" is the encoder button, regardless of how many keys are present on a given device
};

uint8_t KEY_MODIFIER_KEYS[MAX_KEYS_W_ENCODER] = {
  MODIFIER_GUI, // Button 1 modifier keys, only applicable to ACTION_TYPE_KEY_DOWN_UP
  MODIFIER_GUI, // Button 2 modifier keys
  MODIFIER_GUI, // Button 3 modifier keys
  MODIFIER_NONE, // Button 4 modifier keys
  MODIFIER_NONE, // Button 5 modifier keys
  MODIFIER_NONE, // Button 6 modifier keys
  MODIFIER_GUI, // Encoder button modifier keys
};

char KEY_VALUE[MAX_KEYS_W_ENCODER] = {
  'a', // Button 1 key value, only applicable to ACTION_TYPE_KEY_DOWN_UP
  'c', // Button 2 key value
  'v', // Button 3 key value
  KEY_NONE, // Button 4 key value
  KEY_NONE, // Button 5 key value
  KEY_NONE, // Button 6 key value
  'z', // Encoder button key value
};

char* KEY_STRING[MAX_KEYS_W_ENCODER] = {
  "", // Button 1 string value, only applicable to ACTION_TYPE_STRING
  "", // Button 2 string value
  "", // Button 3 string value
  "", // Button 4 string value
  "", // Button 5 string value
  "", // Button 6 string value
  "",  // Encoder button string value
};

int8_t KEY_MOUSE_BUTTON[MAX_KEYS_W_ENCODER] = {
  MOUSE_NONE, // Button 1 mouse button, only applicable to ACTION_TYPE_MOUSE_*
  MOUSE_NONE, // Button 2 mouse button
  MOUSE_NONE, // Button 3 mouse button
  MOUSE_NONE, // Button 4 mouse button
  MOUSE_NONE, // Button 5 mouse button
  MOUSE_NONE, // Button 6 mouse button
  MOUSE_NONE, // Encoder button mouse button
};
/*
 * USER CONFIGURATION END
 */

 
uint8_t KEY_PIN[MAX_KEYS_W_ENCODER] = {
  KEY0_PIN,
  KEY1_PIN,
  KEY2_PIN,
  KEY3_PIN,
  KEY4_PIN,
  KEY5_PIN,
  BUTTON_PIN,
};

bool keyState[MAX_KEYS_W_ENCODER] = {
  false,
  false,
  false,
  false,
  false,
  false,
  false,
};

bool keyLastState[MAX_KEYS_W_ENCODER] = {
  false,
  false,
  false,
  false,
  false,
  false,
  false,
};

bool keyToggleState[MAX_KEYS_W_ENCODER] = {
  false,
  false,
  false,
  false,
  false,
  false,
  false,
};

uint8_t counter = 0;
int8_t encoderPos = 0;
uint8_t encoderALastState = 0;
uint32_t lastEncoderSpinMs;
uint32_t lastScrollSendMs;
uint32_t enterBootloaderMs;

/*
 * LED Control Functions
 */
void clearLedData() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, 0, 0, 0);
  }
}

void displayLeds() {
  LED_FUNC(ledData, NUM_BYTES);
  delay(1);
}

void clearLeds() {
  clearLedData();
  displayLeds();
}

void setLedData(uint8_t ledIndex, uint8_t r, uint8_t g, uint8_t b) {
  set_pixel_for_GRB_LED(ledData, ledIndex, r, g, b);
}

void updateLed(uint8_t ledIndex, uint8_t r, uint8_t g, uint8_t b) {
  setLedData(ledIndex, r, g, b);
  displayLeds();
}

/*
 * Keyboard & Mouse Action Functions
 */ 
void pressKey(uint8_t modifierKeys, char value) {
  if (modifierKeys & MODIFIER_CTRL) {
    Keyboard_press(KEY_LEFT_CTRL);
  }
  if (modifierKeys & MODIFIER_SHIFT) {
    Keyboard_press(KEY_LEFT_SHIFT);
  }
  if (modifierKeys & MODIFIER_ALT) {
    Keyboard_press(KEY_LEFT_ALT);
  }
  if (modifierKeys & MODIFIER_GUI) {
    Keyboard_press(KEY_LEFT_GUI);
  }
  if (value != KEY_NONE) {
    Keyboard_press(value);
  }
}

void releaseKey(uint8_t modifierKeys, char value) {
  if (modifierKeys & MODIFIER_CTRL) {
    Keyboard_release(KEY_LEFT_CTRL);
  }
  if (modifierKeys & MODIFIER_SHIFT) {
    Keyboard_release(KEY_LEFT_SHIFT);
  }
  if (modifierKeys & MODIFIER_ALT) {
    Keyboard_release(KEY_LEFT_ALT);
  }
  if (modifierKeys & MODIFIER_GUI) {
    Keyboard_release(KEY_LEFT_GUI);
  }
  if (value != KEY_NONE) {
    Keyboard_release(value);
  }
}

void pressAndReleaseKey(uint8_t modifierKeys, char value) {
  pressKey(modifierKeys, value);
  delay(1);
  releaseKey(modifierKeys, value);
}

void writeString(uint8_t ind) {
  Keyboard_print(KEY_STRING[ind]);
}

void pressMouseButton(uint8_t button) {
  if (button != MOUSE_NONE) {
    Mouse_press(button);
  }
}

void releaseMouseButton(uint8_t button) {
  if (button != MOUSE_NONE) {
    Mouse_release(button);
  }
}

void clickMouseButton(uint8_t button) {
  if (button != MOUSE_NONE) {
    Mouse_click(button);
  }
}

void doubleClickMouseButton(uint8_t button) {
  if (button != MOUSE_NONE) {
    Mouse_click(button);
    delay(DOUBLE_CLICK_MS);
    Mouse_click(button);
  }
}

void toggleMouseButton(uint8_t button, uint8_t ind) {
  if (keyToggleState[ind]) {
    releaseMouseButton(button);
    keyToggleState[ind] = false;
  } else {
    pressMouseButton(button);
    keyToggleState[ind] = true;
  }
}

void handleKeyPress(uint8_t ind) {
    switch(KEY_ACTION_TYPE[ind]) {
      case ACTION_TYPE_KEY_PRESS:
        pressAndReleaseKey(KEY_MODIFIER_KEYS[ind], KEY_VALUE[ind]);
        break;
      case ACTION_TYPE_KEY_DOWN_UP:
        pressKey(KEY_MODIFIER_KEYS[ind], KEY_VALUE[ind]);
        break;
      case ACTION_TYPE_STRING:
        writeString(ind);
        break;
      case ACTION_TYPE_MOUSE_CLICK:
        clickMouseButton(KEY_MOUSE_BUTTON[ind]);
        break;
      case ACTION_TYPE_MOUSE_DOWN_UP:
        pressMouseButton(KEY_MOUSE_BUTTON[ind]);
        break;
      case ACTION_TYPE_MOUSE_DOUBLE_CLICK:
        doubleClickMouseButton(KEY_MOUSE_BUTTON[ind]);
        break;
      case ACTION_TYPE_MOUSE_TOGGLE:
        toggleMouseButton(KEY_MOUSE_BUTTON[ind], ind);
        break;
      default:
        break;
    }
}

void handleKeyRelease(uint8_t ind) {
    switch(KEY_ACTION_TYPE[ind]) {
      case ACTION_TYPE_KEY_PRESS:
        // No action for key press on key release
        break;
      case ACTION_TYPE_KEY_DOWN_UP:
        releaseKey(KEY_MODIFIER_KEYS[ind], KEY_VALUE[ind]);
        break;
      case ACTION_TYPE_STRING:
        // No action for a string on key release
        break;
      case ACTION_TYPE_MOUSE_CLICK:
        // No action for a mouse click on key release
        break;
      case ACTION_TYPE_MOUSE_DOWN_UP:
        releaseMouseButton(KEY_MOUSE_BUTTON[ind]);
        break;
      case ACTION_TYPE_MOUSE_DOUBLE_CLICK:
        // No action for a double click on key release
        break;
      case ACTION_TYPE_MOUSE_TOGGLE:
        // No action for a toggle on key release
        break;
      default:
        break;
    }
}

void encoderScroll() {
  // If we don't have enough movement to actually trigger a scroll, reset the encoder position to 0 after a few mS
  if (millis() - lastEncoderSpinMs > ENCODER_SPIN_RESET_MS) {
    if (encoderPos > -MIN_SCROLL_STEP && encoderPos < MIN_SCROLL_STEP) {
      encoderPos = 0;
    }
  }

  // Only send scroll commands every few mS
  if (millis() - lastScrollSendMs > SCROLL_SEND_MS) {
    lastScrollSendMs = millis();
    // If we have moved the encoder enough to trigger a scroll, then do it
    if (encoderPos >= MIN_SCROLL_STEP) {
      Mouse_scroll(SCROLL_UP);
      encoderPos -= SCROLL_STEP;
    } else if (encoderPos <= -MIN_SCROLL_STEP) {
      Mouse_scroll(SCROLL_DOWN);
      encoderPos += SCROLL_STEP;
    }
  }
}

/*
 * Function to enter bootloader mode
 */
void enterBootloader() {
  // Set the LEDs to display all red
  setLedData(0, 255, 0, 0);
  setLedData(1, 255, 0, 0);
  setLedData(2, 255, 0, 0);
  displayLeds();

  USB_CTRL = 0;
  EA = 0;                     // Disabling all interrupts is required.
  TMOD = 0;
  delayMicroseconds(50000);
  delayMicroseconds(50000);
  
  __asm__ ("lcall #0x3800");  // Jump to bootloader code

  while(1);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(KEY0_PIN, INPUT_PULLUP);
  pinMode(KEY1_PIN, INPUT_PULLUP);
  pinMode(KEY2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  clearLeds();
  lastEncoderSpinMs = millis() - ENCODER_SPIN_RESET_MS - 1;
  lastScrollSendMs = millis() - SCROLL_SEND_MS - 1;
  enterBootloaderMs = millis();
  encoderALastState = digitalRead(ENC_A_PIN);
  USBInit();
}

void loop() {
  bool needsDebounce = false;
  clearLedData();

  // Handle buttons
  for (uint8_t i = 0; i < MAX_KEYS_W_ENCODER; i++) {
    keyState[i] = digitalRead(KEY_PIN[i]);
    if (keyState[i] != keyLastState[i]) {
      needsDebounce = true;
      if (keyState[i] == KEY_PRESSED) {
        handleKeyPress(i);
      } else {
        handleKeyRelease(i);
      }
    }
    if (i < NUM_LEDS && (keyState[i] == KEY_PRESSED || keyToggleState[i])) {
      setLedData(i, LED_COLORS[i][0], LED_COLORS[i][1], LED_COLORS[i][2]);
    }
    keyLastState[i] = keyState[i];
  }

  // Check encoder
  int encoderAState = digitalRead(ENC_A_PIN); // Reads the "current" state of the outputA
  // If the previous and the current state of the outputA are different, that means a pulse has occured
  if (encoderAState != encoderALastState) {
    lastEncoderSpinMs = millis();
    // If the B state is different to the A state, that means the encoder is rotating clockwise
    if (digitalRead(ENC_B_PIN) != encoderAState) {
      encoderPos++;
    } else {
      encoderPos--;
    }
  }
  
  encoderALastState = encoderAState; // Updates the previous state of the outputA with the current state

  // Currently the encoder just scrolls, not configurable yet
  encoderScroll();

  // Display the current LED state
  displayLeds();
  // Debounce if any button was pressed
  if (needsDebounce) {
    delay(DEBOUNCE_MS);
  }

  // Check if we should enter bootloader (encoder button held for 3 seconds)
  if (ALLOW_BOOTLOADER) {
    if (keyState[ENCODER_INDEX] == KEY_PRESSED) {
      if (millis() - enterBootloaderMs > ENTER_BOOTLOADER_MS) {
        handleKeyRelease(ENCODER_INDEX); // Simulate releasing the encoder button
        delay(1);
        enterBootloader();
      }
    } else {
      enterBootloaderMs = millis();
    }
  }
}
