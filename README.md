# LED Controller UART Commands

This document describes all UART commands available for controlling the LED display system.

## Communication Settings

- **Baud Rate**: 38400
- **Serial Port**: Primary `Serial` (USB) and `extSerial` (Hardware Serial on pins 8/7)
- **Format**: Commands are case-insensitive and should be terminated with a newline (`\n`)

## Floor Commands

Control individual floors/areas on the LED strips. Each floor can display a number of pixels (0-6) and optionally enable a rainbow effect.

### Syntax
```
<FLOOR_NAME> <value>[*]
```

### Parameters
- `<FLOOR_NAME>`: One of the following floor identifiers:
  - `FLOOR11` - Floor 11
  - `FLOOR12` - Floor 12
  - `FLOOR15` - Floor 15
  - `FLOOR16` - Floor 16
  - `FLOOR17` - Floor 17
  - `POOL` - Pool area
  - `TEAROOM` - Tea Room area

- `<value>`: Number of pixels to light (0-6)
  - `0` = All LEDs off
  - `1-6` = Number of pixels lit (each pixel spans multiple columns)

- `*` (optional): Enable rainbow effect
  - Without `*`: Solid color (uses the floor's static color)
  - With `*`: Rainbow cycling effect with smooth wave transition

### Examples
```
FLOOR11 3        # Light 3 pixels on Floor 11 with solid color
FLOOR12 6*      # Light all 6 pixels on Floor 12 with rainbow effect
FLOOR15 0       # Turn off Floor 15
POOL 2*         # Light 2 pixels on Pool with rainbow effect
TEAROOM 4       # Light 4 pixels on Tea Room with solid color
```

### Behavior
- When transitioning to rainbow mode (adding `*`), the rainbow effect wipes forward (pixel 1 to 6)
- When transitioning away from rainbow mode (removing `*`), the rainbow effect wipes backward (pixel 6 to 1)
- State is automatically saved to non-volatile memory and restored on power-up

## Easter Egg Command

Enable/disable the beat-synchronized brightness effect.

### Syntax
```
EASTER_EGG <ON|OFF>
```

### Examples
```
EASTER_EGG ON   # Enable beat-synchronized brightness
EASTER_EGG OFF  # Disable beat-synchronized brightness
```

## BoardName Commands

Control the "Where is Every\nBody At?!" display (84 LEDs on pin 4).

### Syntax
```
BOARDNAME <pattern>
```

### Patterns

#### `OFF`
Turns off the BoardName display.

#### `PER WORD`
- Picks 5 relatively distinct colors (one per word)
- Each color fades to a different distinct color every 5 minutes
- Words: "Where", "is", "Every", "Body", "At?!"

#### `SOLID`
- Picks a random color for all words
- Fades to a different random color every 5 minutes

#### `SOLID WHITE`
- Sets all words to white
- Stays white (no fading)

#### `BLINK`
- Picks a random solid color for the entire display
- Lights up each word sequentially:
  - None lit → First word → First two words → ... → All words
- 5 seconds between each step

#### `EVERY BODY SAME`
- Picks a random color (purple 1/3 of the time) for "Where", "is", and "At?!"
- "Every" and "Body" cycle through rainbow colors together (same color)

#### `EVERY BODY DIF`
- Picks a random color (purple 1/3 of the time) for "Where", "is", and "At?!"
- "Every" and "Body" cycle through rainbow colors from opposite starting points

### Examples
```
BOARDNAME OFF
BOARDNAME PER WORD
BOARDNAME SOLID
BOARDNAME SOLID WHITE
BOARDNAME BLINK
BOARDNAME EVERY BODY SAME
BOARDNAME EVERY BODY DIF
```

## Perimeter Commands

Control the perimeter RGBW strip (250 LEDs on pin 5, WS2814).

### Syntax
```
PERIMETER <pattern> [color parameters]
```

### Patterns

#### `OFF`
Turns off the perimeter strip.

#### `SOLID <R> <G> <B> [W]`
Sets all LEDs to a solid color.
- `R`, `G`, `B`: Red, Green, Blue values (0-255)
- `W`: White value (0-255, optional, defaults to 0)

**Example:**
```
PERIMETER SOLID 255 0 0 0      # Red
PERIMETER SOLID 0 0 255 128    # Blue with white
PERIMETER SOLID 0 0 0 255      # White only
```

#### `ALTERNATING <R1> <G1> <B1> <W1> <R2> <G2> <B2> [W2]`
Alternates between two colors (every other LED).
- First set: `R1`, `G1`, `B1`, `W1` - First color
- Second set: `R2`, `G2`, `B2`, `W2` - Second color (W2 optional, defaults to 0)
- Pattern: LED 0 = Color 1, LED 1 = Color 2, LED 2 = Color 1, etc.

**Example:**
```
PERIMETER ALTERNATING 255 255 0 0 0 0 255 0    # Yellow and Blue
PERIMETER ALTERNATING 0 0 0 255 255 255 0 0   # White and Yellow
```

#### `CHASING_RAINBOW`
Cycles a rainbow pattern that chases around the perimeter strip.

**Example:**
```
PERIMETER CHASING_RAINBOW
```

## Notes

- All commands are case-insensitive
- Commands must be terminated with a newline character
- Floor states are automatically saved and restored on power-up
- Rainbow wave transitions are smooth and animated
- BoardName color fades occur every 5 minutes (for applicable patterns)
- BoardName blink pattern cycles every 5 seconds
- Perimeter chasing rainbow continuously cycles

## Hardware Configuration

- **Strip 1** (Pin 3): FLOOR17, FLOOR16, FLOOR15 (3 floors)
- **Strip 2** (Pin 2): FLOOR12, FLOOR11, TEAROOM, POOL (4 floors)
- **BoardName** (Pin 4): 84 LEDs (2 rows of 42)
- **Perimeter** (Pin 5): 250 LEDs (WS2814 RGBW)

