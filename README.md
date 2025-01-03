# CH552 Macropad
![An example of a three key and encoder wheel macropad.](/images/macropad.jpg "An example of a three key and encoder wheel macropad.")
_An example of a three key and encoder wheel macropad that this project can be used for_

This project is a simple firmware for generic macropads based on the [CH55xduino library](https://github.com/DeqingSun/ch55xduino/).
The configuration is done directly within the firmware, so it removes the need to use configuration software from unknown or untrusted sources.
This project was developed and tested on a three key macropad with an encoder wheel, but it was written with other configurations in mind (namely four key and six key with encoder wheel), though those have not been tested yet.

### Where To Buy a Macropad
These are commonly  available on AliExpress and similar websites by searching for `Macro Keypad`. They're also available for a higher price on Amazon. 
Make sure to get one that looks like the picture above and make sure it is wired, not wireless.
Some variations may come without RGB LEDs populated.

## How To Program the Macropad
While blank chips will boot directly into the bootloader, the chips that come with the macropads are already programmed, so you will need to enter the bootloader to reprogram the device.
Initially, you will need to use a hardware method to enter the bootloader, but after flashing the code, it is possible to enter the bootloader from software.

### Enter the Bootloader Via Hardware
In order to enter bootloader mode using hardware, start by unplugging the macropad from power, then you will need to first unscrew the four bottom screws and remove the two bottom acrylic plates to reveal the CH552G microchip.
Now connect the USB D+ pin to the 3.3V line, plug it back in to USB and then disconnect the USB D+ line from 3.3V. This will put the device in bootloader mode for a few seconds, allowing you time to program the device.
The USB D+ pin is pin 12, which is the fifth pin from the left on the top row (the row closer to side of the device with the USB C port). The 3.3V pin is 16, the leftmost pin in the top row.
One way to do this is to solder a button and optionally a resistor between these two pins as show in the picture below:

![An example showing a button to enter bootloader mode.](/images/bootloader_button.jpg "An example showing a button to enter bootloader mode.")

An alternative method to connect these pins is to simply use a jumper wire or a pair of metal tweezers to touch both of the pins together. 
> :warning: **If you use this method**: Be very careful not to short voltage and ground (pin 14)! This could cause too much current to be drawn from your USB port and on some computers could damage them

### Enter the Bootloader Via Software
After loading this firmware onto your macropad, assuming you left `ALLOW_BOOTLOADER = true`, you can enter the bootloader by holding down the encoder wheel button for three seconds.
The three top LEDs will turn red and the device will enter bootloader mode for a few seconds, allowing it to be reprogrammed.

## Configuring Your Macropad
Follow the instructions in the [CH55xduino library](https://github.com/DeqingSun/ch55xduino/) to install those boards into your Arduino IDE, then open `CH552_Universal_Macropad.ino`.
Look for the lines that say:
```
/*
 * USER CONFIGURATION START
 */
```
This is where all configuration is done.

### LED_COLORS
This allows you to configure the LED colors. _Note: Some macropad variations do not populate the LEDs._ The LEDs will light up in a few different circumstances:
- While a key is pressed
- While a "toggle" type key is active
- When the device enters bootloader mode from software

This is an array with six elements, one for each LED under the keys. Each element of the array is another three element array that represents the red, green and blue components of the value you want for that LED.

### KEY_ACTION_TYPE
This is the type of action each key will perform. It is an array with seven elements, these represent the (up to) six keys and the encoder button. If your device has fewer than six keys, the encoder button will still be the 
seventh entry, just ignore the entries that represent buttons you do not have. The options are as follows:
- `ACTION_TYPE_NONE`: No action will be performed
- `ACTION_TYPE_KEY_PRESS`: This sends both the press and release key commands on key down. This prevents repeated key presses if the key is held down. No action is performed when the key is released
- `ACTION_TYPE_KEY_DOWN_UP`: This acts like a keyboard key press. A key value and optional modifier keys will be pressed when the key is pressed and will be released when the key is released. This allows repeated key presses as long as the key is held down
- `ACTION_TYPE_STRING`: This will act like a keyboard writing a string when the key is pressed down. No action is performed when the key is released
- `ACTION_TYPE_MOUSE_CLICK`: This acts like a mouse clicking and releasing a button when the key is pressed. No action is performed when the key is released
- `ACTION_TYPE_MOUSE_DOUBLE_CLICK`: This acts like a mouse double-clicking a button when the key is pressed. No action is performed when the key is released
- `ACTION_TYPE_MOUSE_DOWN_UP`: This acts like a mouse clicking a button when the key is pressed. The mouse button is released when the key is released
- `ACTION_TYPE_MOUSE_TOGGLE`: This acts like a mouse clicking a button when the key is pressed. The button will remain pressed (and the LED will stay active) until the key is pressed again. This is useful for simulating a middle click button in CAD software so you can pan without holding a button down


### KEY_MODIFIER_KEYS
If a key action has been set as `ACTION_TYPE_KEY_PRESS` or `ACTION_TYPE_KEY_DOWN_UP`, then you can use this to assign which modifier keys (`Control`, `Shift`, `Alt`/`Option`, `GUI`/`Windows`/`Command`) are sent along with the key press. The options are:
- `MODIFIER_NONE`
- `MODIFIER_CTRL`
- `MODIFIER_SHIFT`
- `MODIFIER_ALT`
- `MODIFIER_GUI`

You can send multiple modifier keys by `OR`ing them together: `MODIFIER_CTRL | MODIFIER_SHIFT` will send `Control` and `Shift`

### KEY_VALUE
If a key action has been set as `ACTION_TYPE_KEY_PRESS` or `ACTION_TYPE_KEY_DOWN_UP`, then you can use this to assign which key will be sent along with the optional modifier keys. This can be a single ASCII character like `'a'` or `'c'`, or it can be a special key,
defined in `src/USBHIDKeyboardMouse.h`, such as `KEY_UP_ARROW`, `KEY_RETURN` or `KEY_F1`

### KEY_STRING
If a key action has been set to `ACTION_TYPE_STRING`, then you can define the string for that key here. Enter your string like `"Hello, World!"` into the element of the array that corresponds to the key you are configuring

### KEY_MOUSE_BUTTON
If a key action has been set to `ACTION_TYPE_MOUSE_CLICK`, `ACTION_TYPE_MOUSE_DOUBLE_CLICK`, `ACTION_TYPE_MOUSE_DOWN_UP`, or `ACTION_TYPE_MOUSE_TOGGLE`, this will configure which button will be pressed. The options are:
- `MOUSE_NONE` - This will not perform any action
- `MOUSE_LEFT`
- `MOUSE_MIDDLE`
- `MOUSE_RIGHT`


Additionally, there are two other configurable options:
- `INVERT_SCROLLING`: This inverts the direction that scrolling occurs. You may find this useful depending on how your computer scrolling is configured
- `ALLOW_BOOTLOADER`: This allows the device to be reprogrammed without taking it apart, but it also means that you cannot assign an action that requires holding the encoder button down for more than three seconds
