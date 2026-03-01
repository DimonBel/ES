# Complete Code Documentation

Project: Embedded LED Control System via Serial Interface

---

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Main Application (main.cpp)](#main-application-maincpp)
3. [Serial STDIO Module](#serial-stdio-module)
4. [Command Parser Module](#command-parser-module)
5. [LED Control Module](#led-control-module)

---

## Architecture Overview

This is an embedded system project for controlling LEDs via a serial terminal. The system uses a **layered architecture**:

```
┌─────────────────────────────────────────┐
│      User (Serial Terminal Input)       │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│   Serial STDIO Module (Input/Output)    │  ← Handles communication
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│    Command Parser Module (Processing)    │  ← Interprets commands
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│      LED Module (Hardware Control)       │  ← Controls physical LEDs
└─────────────────────────────────────────┘
```

**How it works:**
1. User types commands like "led1 on" into the serial terminal
2. Serial STDIO captures the input and makes it available to the program
3. Command Parser interprets what the user wants to do
4. LED Module actually turns the LEDs on/off
5. Feedback is sent back to the user via Serial STDIO

---

## Main Application (main.cpp)

### Hardware Configuration

```cpp
// Hardware Configuration
static const unsigned long SERIAL_BAUD_RATE = 9600;
static const uint8_t LED1_PIN = 9;
static const uint8_t LED2_PIN = 13;
```

**What this does:**
- `SERIAL_BAUD_RATE = 9600`: Sets the speed of serial communication (9600 bits per second). Both the microcontroller and your terminal must use the same speed.
- `LED1_PIN = 9`: LED 1 is connected to digital pin 9 on the microcontroller
- `LED2_PIN = 13`: LED 2 is connected to digital pin 13 (common for Arduino's built-in LED)

**Why use `static const`?**
- `static`: These variables are only visible within this file (encapsulation)
- `const`: These values cannot be changed during runtime (safety)
- Together they create compile-time constants that the compiler can optimize

---

### Hardware Instances

```cpp
// Hardware instances
static Led led1(LED1_PIN);
static Led led2(LED2_PIN);
```

**What this does:**
- Creates two `Led` objects to control each LED
- Each object is initialized with the pin number it should control
- `led1` controls the LED on pin 9, `led2` controls the LED on pin 13

**Why objects?**
- Object-oriented programming (OOP) encapsulates all LED-related logic
- Each LED knows its own state (on/off) and pin number
- Makes code cleaner: `led1.on()` is clearer than `digitalWrite(9, HIGH)`

---

### executeCommand() Function

```cpp
/**
 * @brief Execute a parsed command
 * @param cmd The command to execute
 *
 * Maps high-level commands to LED hardware operations.
 * This represents the application logic layer.
 */
static void executeCommand(CommandType cmd)
{
  switch (cmd)
  {
  case CMD_LED1_ON:
    led1.on();
    printf("OK: LED1 is ON\n");
    break;

  case CMD_LED1_OFF:
    led1.off();
    printf("OK: LED1 is OFF\n");
    break;

  case CMD_LED2_ON:
    led2.on();
    printf("OK: LED2 is ON\n");
    break;

  case CMD_LED2_OFF:
    led2.off();
    printf("OK: LED2 is OFF\n");
    break;

  case CMD_BOTH_ON:
    led1.on();
    printf("OK: LED1 is ON (starting sequence)\n");
    delay(500);
    led2.on();
    printf("OK: LED2 is ON (sequence complete)\n");
    break;

  case CMD_BOTH_OFF:
    led1.off();
    led2.off();
    printf("OK: Both LEDs are OFF\n");
    break;

  case CMD_EMPTY:
    // Ignore empty input
    break;

  case CMD_UNKNOWN:
  default:
    printf("ERR: Unknown command\n");
    break;
  }
}
```

**What this does:**
This is the **command execution layer**. It translates abstract commands into actual hardware actions.

**How the switch statement works:**
1. Receives a `CommandType` enum value from the parser
2. Uses `switch` to select the appropriate action
3. Each `case` handles one specific command
4. `break` prevents fallthrough to other cases

**Special cases:**
- `CMD_BOTH_ON`: Turns on both LEDs in sequence with a 500ms delay between them (creates a staggered effect)
- `CMD_BOTH_OFF`: Turns off both LEDs simultaneously
- `CMD_EMPTY`: Does nothing (user just pressed Enter)
- `CMD_UNKNOWN`: Returns an error message for invalid commands

**Why printf feedback?**
- Provides confirmation to the user that the command worked
- Helps with debugging and user experience
- Uses `printf()` instead of `Serial.print()` because of STDIO redirection

---

### setup() Function

```cpp
/**
 * @brief Application initialization
 *
 * Initializes all hardware peripherals and software modules.
 * Order of initialization is critical:
 * 1. Serial interface (required for STDIO)
 * 2. LED hardware
 * 3. STDIO redirection (must be after Serial.begin())
 */
void setup()
{
  // Initialize serial interface first
  SerialStdio::begin(SERIAL_BAUD_RATE);

  // Initialize LED hardware
  led1.begin();
  led2.begin();

  // Display welcome message and available commands
  SerialStdio::printWelcome();
}
```

**What this does:**
`setup()` runs **once** when the microcontroller starts (after power-on or reset).

**Why order matters:**
1. **Serial first**: Must initialize before any STDIO operations (`printf`, `scanf`)
2. **LEDs second**: Configure pins and set initial LED state
3. **Welcome message last**: Can only print after Serial is ready

**What `led1.begin()` does:**
- Configures the pin as OUTPUT mode
- Sets initial state to OFF (LOW)
- Resets internal state tracking

**What `SerialStdio::printWelcome()` does:**
- Shows user that system is ready
- Lists all available commands
- Helps users understand how to interact with the system

---

### loop() Function

```cpp
/**
 * @brief Main application loop
 *
 * Continuous processing loop:
 * 1. Read input from serial terminal
 * 2. Parse the command
 * 3. Execute the command
 * 4. Provide feedback to user
 *
 * This demonstrates the event-driven architecture of embedded systems.
 */
void loop()
{
  char line[SerialStdio::LINE_BUF_SIZE];

  // Read line from serial input with backspace support
  SerialStdio::readLine(line, sizeof(line));

  // Parse the command
  CommandType cmd = CommandParser::parse(line);

  // Debug output for monitoring
  printf("DEBUG: Received '%s' -> %s\n",
         line,
         CommandParser::toString(cmd));

  // Execute the parsed command
  executeCommand(cmd);
}
```

**What this does:**
`loop()` runs **continuously and repeatedly** after `setup()` completes.

**The 4-step process:**

1. **Read Input**
   ```cpp
   char line[SerialStdio::LINE_BUF_SIZE];
   SerialStdio::readLine(line, sizeof(line));
   ```
   - Creates a buffer to store user input (80 characters max)
   - Waits for user to type and press Enter
   - Handles backspace/delete for editing

2. **Parse Command**
   ```cpp
   CommandType cmd = CommandParser::parse(line);
   ```
   - Passes the raw input string to the parser
   - Parser determines what command was typed
   - Returns an enum value (like `CMD_LED1_ON`)

3. **Debug Output**
   ```cpp
   printf("DEBUG: Received '%s' -> %s\n", line, CommandParser::toString(cmd));
   ```
   - Shows what was received and how it was interpreted
   - Helpful for development and debugging
   - Example output: `DEBUG: Received 'led1 on' -> led1 on`

4. **Execute Command**
   ```cpp
   executeCommand(cmd);
   ```
   - Passes the parsed command to executeCommand()
   - Actually performs the LED control
   - Sends feedback to user

**Why this architecture?**
- **Event-driven**: System waits for input, then processes it
- **Separation of concerns**: Each module has one job
- **Testability**: Each step can be tested independently
- **Debuggable**: Debug output shows each step

---

## Serial STDIO Module

### Purpose
This module creates a bridge between:
- **Standard C library** (`printf`, `scanf`, `getchar`, `putchar`)
- **Arduino Serial** hardware

### Why use STDIO?
Instead of writing `Serial.print("message")`, you can use `printf("message")`. This is more familiar to C programmers and allows formatted output like `printf("Value: %d", x)`.

---

### serial_stdio.h

```cpp
/**
 * @brief STDIO Serial Interface Module
 *
 * Provides hardware-software interface for serial communication using STDIO library.
 * Redirects standard input/output streams to Arduino Serial interface.
 * This module abstracts the low-level serial communication details.
 */
class SerialStdio {
```

**What this class does:**
- Wraps all Serial STDIO functionality in a single class
- Static methods (no need to create instances)
- Hides complex STDIO setup details

---

### begin() Method

```cpp
/**
 * @brief Initialize STDIO over Serial
 * @param baudRate The baud rate for serial communication
 *
 * Sets up Serial interface and redirects stdin/stdout to use it.
 * Must be called before any printf/scanf operations.
 */
static void begin(unsigned long baudRate);
```

**What this does:**
```cpp
void SerialStdio::begin(unsigned long baudRate) {
    Serial.begin(baudRate);

    // Attach stdio streams
    fdev_setup_stream(&serial_stdout, serialPutchar, NULL, _FDEV_SETUP_WRITE);
    fdev_setup_stream(&serial_stdin, NULL, serialGetchar, _FDEV_SETUP_READ);
    stdout = &serial_stdout;
    stdin = &serial_stdin;
}
```

**How it works:**
1. `Serial.begin(baudRate)`: Starts the hardware serial port
2. `fdev_setup_stream()`: Creates file streams for STDIO (AVR-specific)
   - For output: Uses `serialPutchar` to write characters
   - For input: Uses `serialGetchar` to read characters
3. `stdout = &serial_stdout`: Redirects printf to our serial
4. `stdin = &serial_stdin`: Redirects scanf to our serial

**Why file streams?**
- C's STDIO library uses file streams (FILE*) for I/O
- By creating custom streams, we redirect printf/scanf to work with Serial
- This is how printf works in embedded systems

---

### printWelcome() Method

```cpp
/**
 * @brief Print welcome message
 *
 * Displays system ready message and available commands.
 */
static void printWelcome();
```

**What this does:**
```cpp
void SerialStdio::printWelcome() {
    printf("STDIO serial ready. Commands: 'led1 on', 'led1 off', 'led2 on', 'led2 off'\n");
    printf("                           'led both on', 'led both off'\n");
}
```

**Purpose:**
- Shows user the system is ready
- Lists all available commands
- Provides help without needing a separate command

---

### readLine() Method

```cpp
/**
 * @brief Read a line from serial input with backspace support
 * @param buffer Buffer to store the input line
 * @param bufferSize Maximum buffer size
 * @return int Number of characters read (excluding null terminator)
 *
 * Reads characters until newline/return is received.
 * Properly handles backspace/delete characters to edit input.
 */
static int readLine(char* buffer, int bufferSize);
```

**What this does:**
```cpp
int SerialStdio::readLine(char* buffer, int bufferSize) {
    int pos = 0;

    // Read characters one by one to handle backspace properly
    while (true) {
        int c = getchar();

        // Handle Enter key (newline or carriage return)
        if (c == '\r' || c == '\n') {
            buffer[pos] = '\0';
            return pos;
        }

        // Handle backspace (ASCII 8) or delete (ASCII 127)
        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
            }
            continue;
        }

        // Handle printable characters
        if (c >= 32 && c <= 126 && pos < bufferSize - 1) {
            buffer[pos++] = (char)c;
        }
    }
}
```

**How it works:**

1. **Character-by-character reading:**
   - Uses `getchar()` which now reads from Serial
   - Waits for each character (blocking)
   - Processes immediately

2. **Enter/Return key:**
   ```cpp
   if (c == '\r' || c == '\n') {
       buffer[pos] = '\0';
       return pos;
   }
   ```
   - Both `\r` (carriage return) and `\n` (newline) indicate Enter was pressed
   - Adds null terminator to make it a valid C string
   - Returns the length

3. **Backspace/Delete:**
   ```cpp
   if (c == '\b' || c == 127) {
       if (pos > 0) {
           pos--;
       }
       continue;
   }
   ```
   - `\b` is backspace (ASCII 8)
   - `127` is delete key
   - Moves the position back (effectively deleting the last character)
   - Echoes backspace sequence in `serialGetchar()` (see below)

4. **Printable characters:**
   ```cpp
   if (c >= 32 && c <= 126 && pos < bufferSize - 1) {
       buffer[pos++] = (char)c;
   }
   ```
   - ASCII 32-126 are printable characters
   - Checks buffer size to prevent overflow
   - Adds character and moves position forward

**Why handle backspace?**
- Users make mistakes when typing
- Without backspace, they'd have to press Enter and retype
- Provides a better user experience

---

### serialPutchar() - Output Function

```cpp
/**
 * @brief putchar implementation for STDOUT
 */
static int serialPutchar(char c, FILE* stream);
```

**What this does:**
```cpp
int SerialStdio::serialPutchar(char c, FILE* stream) {
    if (c == '\n') Serial.write('\r');
    Serial.write(c);
    return 0;
}
```

**How it works:**
- This function is called by `printf()` for every character
- Handles newline conversion: `\n` becomes `\r\n` (Windows-style line endings)
- Writes character to Arduino Serial
- Returns 0 (success) per C convention

**Why add `\r` before `\n`?**
- Serial terminals expect `\r\n` for line breaks
- `\r` moves cursor to start of line (carriage return)
- `\n` moves to next line (line feed)
- Without `\r`, output might not display correctly

---

### serialGetchar() - Input Function

```cpp
/**
 * @brief getchar implementation for STDIN
 */
static int serialGetchar(FILE* stream);
```

**What this does:**
```cpp
int SerialStdio::serialGetchar(FILE* stream) {
    while (!Serial.available());
    int c = Serial.read();

    // Handle backspace character (ASCII 8 or 127)
    if (c == '\b' || c == 127) {
        // Echo backspace sequence to erase character from terminal
        Serial.write("\b \b");
        return c;
    }

    return c;
}
```

**How it works:**

1. **Wait for data:**
   ```cpp
   while (!Serial.available());
   ```
   - Blocks (waits) until a character is received
   - This is why `readLine()` waits for user input

2. **Read character:**
   ```cpp
   int c = Serial.read();
   ```
   - Reads one character from Serial buffer
   - Returns it as an integer

3. **Backspace handling:**
   ```cpp
   if (c == '\b' || c == 127) {
       Serial.write("\b \b");
       return c;
   }
   ```
   - Sends `\b \b` (backspace, space, backspace) to terminal
   - This visual erases the character from screen
   - Returns the backspace character to caller

**Why echo backspace sequence?**
- When user presses backspace, we need to:
  1. Move cursor back (`\b`)
  2. Write a space to overwrite character
  3. Move cursor back again (`\b`)
- This creates the visual effect of erasing

---

## Command Parser Module

### Purpose
This module interprets user input and converts it into commands that the system can execute.

### Why have a parser?
- Users type natural commands like "led1 on"
- System needs to understand these as actions
- Parser bridges human language and machine logic
- Centralizes command interpretation

---

### command.h - Command Types

```cpp
/**
 * @brief Command types for LED control
 *
 * Enumerates all supported commands for the LED control system.
 * Each command corresponds to a specific LED operation.
 */
enum CommandType {
    CMD_LED1_ON,          ///< Turn LED1 on
    CMD_LED1_OFF,         ///< Turn LED1 off
    CMD_LED2_ON,          ///< Turn LED2 on
    CMD_LED2_OFF,         ///< Turn LED2 off
    CMD_BOTH_ON,          ///< Turn both LEDs on
    CMD_BOTH_OFF,         ///< Turn both LEDs off
    CMD_UNKNOWN,          ///< Unknown/invalid command
    CMD_EMPTY             ///< Empty input
};
```

**What this does:**
- Defines all possible commands as enum values
- Each value represents a specific action
- Enums are better than strings because:
  - Compile-time type checking
  - More efficient (integers vs strings)
  - Self-documenting

**Why separate `CMD_EMPTY` from `CMD_UNKNOWN`?**
- `CMD_EMPTY`: User just pressed Enter (no input)
- `CMD_UNKNOWN`: User typed something invalid
- Different handling: ignore empty, error on unknown

---

### command.cpp - Helper Functions

#### trimWhitespace()

```cpp
/**
 * @brief Trim whitespace from string
 * @param str The string to trim
 */
static void trimWhitespace(char* str) {
    char* end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    // All spaces?
    if (*str == 0) {
        *str = 0;
        return;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;
}
```

**What this does:**
- Removes spaces from start and end of string
- Example: `"  led1 on  "` → `"led1 on"`

**How it works:**
1. **Leading spaces:** Move pointer past initial spaces
2. **Check empty:** If all spaces, make empty string
3. **Trailing spaces:** Find last non-space character
4. **Terminate:** Add null terminator after last character

**Why trim?**
- Users might accidentally type extra spaces
- `strcmp()` requires exact match
- Makes input more forgiving

---

#### toLowerCase()

```cpp
/**
 * @brief Convert string to lowercase
 * @param str The string to convert
 */
static void toLowerCase(char* str) {
    for (; *str; ++str) {
        *str = tolower((unsigned char)*str);
    }
}
```

**What this does:**
- Converts all characters to lowercase
- Example: `"LED1 ON"` → `"led1 on"`

**How it works:**
- Loops through each character until null terminator
- Uses `tolower()` function from ctype.h
- Modifies string in place

**Why lowercase?**
- Makes commands case-insensitive
- User can type "LED1 ON" or "led1 on" or "Led1 On"
- Easier for users

---

### parse() Method

```cpp
CommandType CommandParser::parse(const char* input) {
    if (input == nullptr || input[0] == '\0') {
        return CMD_EMPTY;
    }

    // Create a mutable copy for processing
    char buffer[MAX_CMD_LENGTH];
    strncpy(buffer, input, MAX_CMD_LENGTH - 1);
    buffer[MAX_CMD_LENGTH - 1] = '\0';

    // Normalize the input
    trimWhitespace(buffer);
    toLowerCase(buffer);

    // Parse commands
    if (strcmp(buffer, "led1 on") == 0) {
        return CMD_LED1_ON;
    } else if (strcmp(buffer, "led1 off") == 0) {
        return CMD_LED1_OFF;
    } else if (strcmp(buffer, "led2 on") == 0) {
        return CMD_LED2_ON;
    } else if (strcmp(buffer, "led2 off") == 0) {
        return CMD_LED2_OFF;
    } else if (strcmp(buffer, "led both on") == 0) {
        return CMD_BOTH_ON;
    } else if (strcmp(buffer, "led both off") == 0) {
        return CMD_BOTH_OFF;
    }

    return CMD_UNKNOWN;
}
```

**What this does:**
Main parser function that converts input string to command type.

**Step-by-step:**

1. **Check empty input:**
   ```cpp
   if (input == nullptr || input[0] == '\0') {
       return CMD_EMPTY;
   }
   ```
   - Handle NULL pointer or empty string
   - Return early to avoid processing

2. **Copy to buffer:**
   ```cpp
   char buffer[MAX_CMD_LENGTH];
   strncpy(buffer, input, MAX_CMD_LENGTH - 1);
   buffer[MAX_CMD_LENGTH - 1] = '\0';
   ```
   - Create mutable copy (can't modify const input)
   - Use `strncpy` to prevent overflow
   - Ensure null terminator

3. **Normalize input:**
   ```cpp
   trimWhitespace(buffer);
   toLowerCase(buffer);
   ```
   - Remove extra spaces
   - Convert to lowercase
   - Makes comparison consistent

4. **Parse with strcmp:**
   ```cpp
   if (strcmp(buffer, "led1 on") == 0) {
       return CMD_LED1_ON;
   }
   ```
   - Compare normalized input to each command
   - `strcmp` returns 0 for match
   - Return corresponding enum value

5. **Unknown command:**
   ```cpp
   return CMD_UNKNOWN;
   ```
   - If no match, return unknown
   - Caller will show error

**Why this approach?**
- Simple and clear
- Easy to add new commands
- Efficient (string comparison is fast)

**Alternative approaches not used:**
- Regular expressions: Too complex for this use case
- Lookup table: Would be better for many commands
- Token-based parsing: Overkill for simple commands

---

### isValid() Method

```cpp
/**
 * @brief Check if a command is valid
 * @param cmd The command type to check
 * @return true if command is valid, false otherwise
 */
static bool isValid(CommandType cmd);
```

**What this does:**
```cpp
bool CommandParser::isValid(CommandType cmd) {
    return (cmd >= CMD_LED1_ON && cmd <= CMD_BOTH_OFF) || cmd == CMD_EMPTY;
}
```

**How it works:**
- Checks if command is in valid range
- Includes `CMD_EMPTY` as valid
- Returns `true` for valid, `false` for invalid

**Why needed?**
- Allows validation before execution
- Could be used for error checking
- Not currently used in main code but good to have

---

### toString() Method

```cpp
/**
 * @brief Get command string representation
 * @param cmd The command type
 * @return const char* String representation of the command
 */
static const char* toString(CommandType cmd);
```

**What this does:**
```cpp
const char* CommandParser::toString(CommandType cmd) {
    switch (cmd) {
        case CMD_LED1_ON:      return "led1 on";
        case CMD_LED1_OFF:     return "led1 off";
        case CMD_LED2_ON:      return "led2 on";
        case CMD_LED2_OFF:     return "led2 off";
        case CMD_BOTH_ON:      return "led both on";
        case CMD_BOTH_OFF:     return "led both off";
        case CMD_EMPTY:        return "(empty)";
        case CMD_UNKNOWN:
        default:               return "unknown";
    }
}
```

**How it works:**
- Converts enum back to string
- Used for debug output
- Returns human-readable name

**Why useful?**
- Debugging: Show what command was received
- Logging: Record commands
- Feedback: Tell user what was parsed

---

## LED Control Module

### Purpose
Provides a clean, object-oriented interface for controlling LEDs.

### Why a Led class?
- **Encapsulation**: Pin and state stored together
- **Abstraction**: Hide digitalWrite details
- **Reusability**: Easy to add more LEDs
- **State tracking**: Know if LED is on or off

---

### led.h - Class Definition

```cpp
class Led
{
public:
    explicit Led(uint8_t pin);
    void begin();
    void on();
    void off();
    void toggle();
    bool state() const;

private:
    uint8_t _pin;
    bool _state;
};
```

**What this defines:**
- Constructor with explicit keyword
- Public methods for control
- Private members for encapsulation

**Why `explicit` keyword?**
```cpp
explicit Led(uint8_t pin);
```
- Prevents implicit conversions
- Example: `void foo(Led led)` wouldn't accept `foo(9)`
- Forces explicit construction: `Led led1(9);`

**Private members:**
```cpp
uint8_t _pin;   // Which pin the LED is on
bool _state;    // true = ON, false = OFF
```
- Underscore prefix indicates private members
- External code can't modify these directly
- Must use public methods

---

### led.cpp - Implementation

#### Constructor

```cpp
Led::Led(uint8_t pin)
    : _pin(pin), _state(false) {}
```

**What this does:**
- Initializes member variables using member initializer list
- Stores the pin number
- Sets initial state to false (OFF)

**Why member initializer list?**
- More efficient than assignment in body
- Required for const/reference members
- Good C++ practice

---

#### begin()

```cpp
void Led::begin()
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  _state = false;
}
```

**What this does:**
1. `pinMode(_pin, OUTPUT)`: Configure pin as output (can drive LED)
2. `digitalWrite(_pin, LOW)`: Turn LED off initially
3. `_state = false`: Update internal state

**Why separate begin()?**
- Constructor can't call Arduino functions (runs too early)
- begin() called from setup() after Arduino init
- Allows delayed initialization

---

#### on()

```cpp
void Led::on()
{
  digitalWrite(_pin, HIGH);
  _state = true;
}
```

**What this does:**
- Sets pin to HIGH (turns LED on)
- Updates state to true
- Both hardware and software state updated

**Why track state?**
- Can check if LED is on without reading pin
- Reading pin might not work if LED is disconnected
- Faster than hardware read

---

#### off()

```cpp
void Led::off()
{
  digitalWrite(_pin, LOW);
  _state = false;
}
```

**What this does:**
- Sets pin to LOW (turns LED off)
- Updates state to false
- Both hardware and software state updated

---

#### toggle()

```cpp
void Led::toggle()
{
  if (_state)
    off();
  else
    on();
}
```

**What this does:**
- Checks current state
- If on, turns off
- If off, turns on
- Flips the state

**Why toggle()?**
- Common operation (think push-button switches)
- Simpler than checking state externally
- Cleaner than: `if (led1.state()) led1.off(); else led1.on();`

---

#### state()

```cpp
bool Led::state() const { return _state; }
```

**What this does:**
- Returns current LED state
- `const` keyword: doesn't modify object
- Getter for private _state member

**Why const?**
- Promises not to change object
- Can be called on const Led objects
- Good practice for getters

---

## How Everything Works Together

### Complete Flow Example

Let's trace what happens when user types "led1 on" and presses Enter:

```
1. User types: "led1 on" + Enter
   ↓
2. SerialStdio::readLine() in loop()
   - Reads each character
   - Handles Enter key
   - Stores "led1 on" in line buffer
   ↓
3. CommandParser::parse("led1 on")
   - Trims whitespace (no change)
   - Converts to lowercase (already lowercase)
   - Compares to "led1 on"
   - Returns CMD_LED1_ON
   ↓
4. Debug printf()
   - Shows: DEBUG: Received 'led1 on' -> led1 on
   ↓
5. executeCommand(CMD_LED1_ON)
   - Switch matches CMD_LED1_ON case
   - Calls led1.on()
   ↓
6. Led::on()
   - digitalWrite(pin9, HIGH) → LED1 turns on
   - _state = true → Internal state updated
   ↓
7. printf("OK: LED1 is ON\n")
   - Sent via SerialStdio::serialPutchar()
   - User sees: OK: LED1 is ON
```

### Architecture Benefits

**Modularity:**
- Each module can be tested independently
- Can replace LED module with different hardware
- Can add new commands without changing LED code

**Maintainability:**
- Changes to Serial handling don't affect commands
- LED logic is isolated
- Easy to understand and debug

**Extensibility:**
- Add new LED: Create new Led object
- Add new command: Add enum value and case
- Change hardware: Modify Led class only

**Debugging:**
- Debug output at each step
- Clear error messages
- State tracking

---

## Common Patterns Used

### 1. RAII (Resource Acquisition Is Initialization)
```cpp
Led led1(LED1_PIN);  // Acquires resource (pin)
// ... led1 is automatically managed
```

### 2. Singleton-like Static Methods
```cpp
SerialStdio::begin();  // No instance needed
CommandParser::parse();
```

### 3. State Pattern
```cpp
enum CommandType { ... };  // Represent states/commands
```

### 4. Strategy Pattern
```cpp
switch (cmd) { ... }  // Different strategies per command
```

### 5. Facade Pattern
```cpp
SerialStdio  // Hides complex STDIO setup
Led         // Hides digitalWrite complexity
```

---

## Potential Improvements

### Add to Command Parser
- Command history
- Tab completion
- Command aliases (e.g., "l1 on" for "led1 on")

### Add to LED Module
- Brightness control (PWM)
- Blinking patterns
- Multiple LEDs in array

### Add to Serial Module
- Command echoing
- Line editing (arrows, delete)
- Command timeout

### Error Handling
- Invalid pin numbers
- Hardware not connected
- Input overflow

### Testing
- Unit tests for parser
- Mock hardware for testing
- Integration tests

---

## Summary

This project demonstrates:
- ✅ Layered architecture
- ✅ Hardware abstraction
- ✅ Command pattern
- ✅ Clean interfaces
- ✅ Modularity and encapsulation
- ✅ Embedded best practices

Each component has a single responsibility:
- **SerialStdio**: Input/output handling
- **CommandParser**: Command interpretation
- **Led**: Hardware control
- **main.cpp**: Application coordination

Together they form a robust, maintainable embedded system.
