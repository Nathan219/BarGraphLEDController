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

## GET Commands

Query the current state of BoardName and Perimeter settings.

### Syntax
```
GET BOARDNAME
GET PERIMETER
GET PER          # Same as GET PERIMETER
```

### GET BOARDNAME
Returns the current BoardName pattern setting.

**Response Format:**
```
BOARDNAME <pattern>
```

**Example Responses:**
```
BOARDNAME PER WORD
BOARDNAME SOLID
BOARDNAME SOLID WHITE
BOARDNAME BLINK
BOARDNAME EVERY BODY SAME
BOARDNAME EVERY BODY DIF
BOARDNAME OFF
```

**Example:**
```
GET BOARDNAME
# Response: BOARDNAME PER WORD
```

### GET PERIMETER (or GET PER)
Returns the current Perimeter pattern and color settings.

**Response Format:**
- For `OFF` or `RAINBOW`: `PER <pattern>`
- For `SOLID`: `PER SOLID <COLOR_NAME>`
- For `ALT`: `PER ALT <COLOR1> <COLOR2> [CHASE [delay_ms]]`

**Example Responses:**
```
PER OFF
PER SOLID RED
PER ALT YELLOW BLUE
PER ALT RED BLUE CHASE
PER ALT RED BLUE CHASE 250
PER RAINBOW
```

**Example:**
```
GET PERIMETER
# Response: PER PER ALT YELLOW BLUE

GET PER
# Response: PER SOLID PURPLE
```

## Perimeter Commands

Control the perimeter RGB strip (232 LEDs on pin 5: 44 top, 44 bottom, 72 each side).

### Syntax
```
PER <pattern> [color parameters]
```

**Note**: `PERIMETER` is still supported for backwards compatibility, but `PER` is the preferred command.

### Available Colors

The following color names can be used with `SOLID` and `PER ALT` patterns (case-insensitive):

- `RED` - Red
- `GREEN` - Green
- `BLUE` - Blue
- `YELLOW` - Yellow
- `PURPLE` - Purple
- `CYAN` - Cyan
- `MAGENTA` - Magenta
- `ORANGE` - Orange
- `PINK` - Pink
- `WHITE` - White (uses white LED channel)
- `BLACK` - Black (off)
- `LIME` - Bright yellow-green
- `TEAL` - Teal
- `NAVY` - Navy blue
- `MAROON` - Maroon
- `OLIVE` - Olive
- `GOLD` - Gold
- `SILVER` - Silver
- `CORAL` - Coral
- `TURQUOISE` - Turquoise

### Patterns

#### `OFF`
Turns off the perimeter strip.

**Example:**
```
PER OFF
PERIMETER OFF    # Also works (backwards compatibility)
```

#### `SOLID <COLOR_NAME>`
Sets all LEDs to a solid color using a color name.

**Example:**
```
PER SOLID RED
PER SOLID BLUE
PER SOLID PURPLE
PER SOLID WHITE
PER SOLID GOLD
```

#### `ALT <COLOR1> <COLOR2> [CHASE [delay_ms]]`
Alternates between two colors (every other LED).
- Pattern: LED 0 = Color 1, LED 1 = Color 2, LED 2 = Color 1, etc.
- Optional `CHASE [delay_ms]`: Makes the alternating pattern chase around the perimeter
  - If `delay_ms` is not specified, defaults to 500ms
  - `delay_ms` is the time in milliseconds between each step of the chase

**Example:**
```
PER ALT YELLOW BLUE
PER ALT PURPLE CYAN
PER ALT WHITE RED
PER ALT GOLD SILVER
PER ALT RED BLUE CHASE        # Chase with default 500ms delay
PER ALT RED BLUE CHASE 250    # Chase with 250ms delay (faster)
PER ALT YELLOW GREEN CHASE 1000  # Chase with 1000ms delay (slower)
```

#### `RAINBOW`
Cycles a rainbow pattern across the perimeter strip at the same speed as the floor rainbow animations.

**Example:**
```
PER RAINBOW
PERIMETER RAINBOW    # Also works (backwards compatibility)
PER CHASING_RAINBOW  # Also works (backwards compatibility, same as RAINBOW)
```

## Brightness Commands

### `SET BRITE <TARGET> <VALUE>`
Sets the brightness for BoardName or Perimeter separately.

**Parameters:**
- `<TARGET>`: `PER`/`PERIMETER` for perimeter, or `BD`/`BOARDNAME` for BoardName
- `<VALUE>`: Brightness value from 0-255 (0 = off, 255 = full brightness)

**Examples:**
```
SET BRITE PER 200
SET BRITE PERIMETER 150
SET BRITE BD 255
SET BRITE BOARDNAME 100
```

**Note:** BoardName defaults to 255 (100% brightness), Perimeter defaults to 150.

### `SET TESTMODE <TRUE/FALSE>`
Enables or disables test mode. When enabled, all floors cycle through values 0-6, increasing every 5 seconds.

**Parameters:**
- `<TRUE/FALSE>`: `TRUE`/`ON`/`1` to enable, `FALSE`/`OFF`/`0` to disable

**Response:**
- `TESTMODE ACCEPTED` when the command is successfully processed

**Example:**
```
SET TESTMODE TRUE
# Response: TESTMODE ACCEPTED
# All floors will cycle: 0 -> 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 0 (every 5 seconds)

SET TESTMODE FALSE
# Response: TESTMODE ACCEPTED
# Test mode disabled, floors return to normal control
```

**Note:** In test mode, all floors are set to the same value (0-6) and rainbow mode is disabled.

**Note:** In test mode, all floors are set to the same value simultaneously, and rainbow mode is disabled.

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
- **Perimeter** (Pin 5): 232 LEDs (44 top, 44 bottom, 72 each side)

