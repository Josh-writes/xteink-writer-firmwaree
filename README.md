# MicroSlate - Dedicated Writing Device for Xteink X4

MicroSlate is a specialized firmware for the Xteink X4 e-paper device that transforms it into a dedicated writing device with Bluetooth keyboard support.

## Features

- **Bluetooth Keyboard Support**: Full HID keyboard functionality via BLE
  - Manual device scanning and selection
  - Tested with Logitech Keys-To-Go 2
  - Supports both 7-byte and 8-byte HID report formats
  - "Just Works" pairing (no passkey required for most keyboards)
- **Simple Menu System**: Main menu with Browse Files, New Note, and Settings
- **Text Editor**: Dedicated editor optimized for e-paper displays
- **File Management**: Browse, create, rename, and edit text files
- **Bluetooth Settings**: Device scanning, connection, and disconnection
- **Display Orientation**: Adjustable screen orientation (Portrait, Landscape, Inverted)
- **Text Formatting**: Configurable characters per line (20-60)
- **MicroSD Storage**: Automatic saving to SD card in `/notes/` directory
- **E-Paper Optimized**: Fast refresh mode for responsive typing
- **Low Memory Footprint**: Designed to run efficiently on ESP32-C3's limited RAM
- **Device Restart**: Hold BACK button for 5 seconds to restart

## Hardware Requirements

- Xteink X4 e-paper device
- MicroSD card (FAT32 formatted)
- External Bluetooth keyboard (physical or smartphone app)

## BLE HID Host Functionality

MicroSlate acts as a BLE HID Host, connecting to external Bluetooth keyboards as a client. The pairing process:

1. Navigate to **Settings → Bluetooth Settings** from the main menu
2. Device performs a 5-second scan for nearby Bluetooth HID keyboards
3. Select your keyboard from the discovered devices list (up to 10 devices shown)
4. Device pairs using "Just Works" mode (no PIN/passkey required for most keyboards)
5. Once connected, all keypresses are routed to the active application mode

### Tested Keyboards

- **Logitech Keys-To-Go 2**: Fully working (7-byte HID report format)
- Other BLE HID keyboards should work but are untested

### Bluetooth Features

- **One-shot scanning**: Scan stops after 5 seconds to preserve battery and prevent ADC noise
- **Manual re-scan**: Press Right button in Bluetooth Settings to scan again
- **Disconnect**: Press Left button to disconnect current keyboard
- **Auto-reconnect**: Device attempts to reconnect to last paired keyboard on startup

## Building and Installation

### Prerequisites

1. Install [PlatformIO](https://platformio.org/install/)
2. Clone or download this repository
3. Ensure `crosspoint-reader` sibling project is available (for shared libraries)

### Dependencies

The project uses libraries from the crosspoint-reader SDK:
- **NimBLE-Arduino 1.4.3**: Bluetooth Low Energy stack
- **GfxRenderer**: E-ink display rendering with text truncation
- **InputManager**: Physical button debouncing with ADC
- **SDCardManager**: MicroSD card file operations
- **EInkDisplay**: Hardware abstraction for e-paper display
- **Preferences**: NVS storage for paired device information

Libraries are symlinked from `../crosspoint-reader/` project.

### Build Instructions

```bash
cd xteink-writer-firmware
pio run
```

### Upload Instructions

```bash
# Build and upload in one command
pio run --target upload

# Or just upload if already built
pio run --target upload --upload-port <PORT>
```

## Usage Instructions

### Initial Setup

1. Power on your Xteink X4 with MicroSlate firmware installed
2. Insert a FAT32-formatted MicroSD card (required for file storage)
3. Navigate to **Settings → Bluetooth Settings** from the main menu
4. Wait for device scan to complete (5 seconds)
5. Select your Bluetooth keyboard from the list and press Enter
6. Once connected (status shows "Connected"), return to main menu
7. You can now create or open files and start typing

### Main Menu

MicroSlate presents a simple menu system:
- **Browse Files**: View and select existing notes
- **New Note**: Create a new text file
- **Settings**: Access configuration options (future enhancement)

Navigate using arrow keys and press Enter to select.

### File Browser Mode

- Shows all `.txt` files in the `/notes/` directory
- Displays both the file title and filename
- Use arrow keys to navigate
- Press Enter to open a file for editing
- Press F2 or Ctrl+R to rename a selected file
- Press Ctrl+N to create a new file

### Text Editor Mode

- Full keyboard support for text entry and editing
- Use arrow keys to move cursor
- Backspace/Delete to remove characters
- Press Ctrl+S to save the current file
- Press Ctrl+Q to return to file browser

### File Renaming Mode

- Access by pressing F2 or Ctrl+R in file browser
- Type the new filename using your connected keyboard
- Press Enter to confirm or Esc to cancel

### Bluetooth Settings Mode

- Access from Settings menu
- **Up/Down**: Navigate device list
- **Enter**: Connect to selected device (or start scan if no devices)
- **Right**: Re-scan for devices
- **Left**: Disconnect current keyboard
- **Escape**: Return to Settings menu
- Device list shows: name, address, RSSI signal strength, and connection status

### Supported Key Mappings

**Global**
- **Hold BACK button 5s**: Restart device (physical button on device)

**Main Menu / File Browser**
- **Up/Down**: Navigate menu/file list
- **Enter**: Select option / Open file
- **Ctrl+N**: Create new file
- **Ctrl+R** or **F2**: Rename selected file
- **Escape**: Return to previous menu

**Text Editor**
- **Arrow keys**: Move cursor
- **Home/End**: Jump to start/end of line
- **Backspace/Delete**: Remove characters
- **Ctrl+S**: Save current file
- **Ctrl+Q**: Return to file browser

**Settings Menu**
- **Up/Down**: Navigate options
- **Left/Right**: Adjust orientation and characters per line
- **Enter**: Select Bluetooth Settings or Back

**Bluetooth Settings**
- **Up/Down**: Navigate device list
- **Enter**: Connect to device / Start scan
- **Right**: Re-scan for devices
- **Left**: Disconnect current keyboard
- **Escape**: Return to Settings

## Memory and Performance

- Total application RAM: Under 200KB
- Text buffer: 16KB capacity for larger documents
- BLE keyboard stack: Optimized for minimal overhead
- Display updates: Partial refresh for cursor movement

## Project Structure

### Source Files

- `src/main.cpp`: Main loop, button handling, UI orchestration
- `src/ble_keyboard.cpp/h`: BLE scanning, pairing, HID keyboard handling
- `src/input_handler.cpp/h`: Keyboard event queue and UI input dispatch
- `src/text_editor.cpp/h`: Text buffer, cursor management, editing operations
- `src/file_manager.cpp/h`: SD card file operations (browse, create, rename)
- `src/ui_renderer.cpp/h`: Screen rendering for all UI modes
- `src/config.h`: Global configuration and constants

### Saved Files

- Files are saved in the `/notes/` directory on the SD card
- Default naming: `note_X_TIMESTAMP.txt` (where X is incrementing ID)
- Compatible with any text editor
- UTF-8 encoding

## Troubleshooting

### BLE Keyboard Issues

**Keyboard not appearing in scan:**
- Ensure keyboard is in pairing mode and not connected to another device
- Some keyboards require you to press a pairing button
- Try re-scanning with the Right button in Bluetooth Settings

**Connection fails:**
- Most modern keyboards use "Just Works" pairing (no PIN required)
- If your keyboard requires a passkey, it may not be compatible yet
- Check serial monitor output (115200 baud) for detailed connection logs

**Typing not working:**
- Ensure you're in the Text Editor mode (open a file first)
- Check connection status in Bluetooth Settings
- Some keyboards may need to be disconnected from other devices first

**Physical buttons not responding:**
- This can happen if BLE scanning is stuck - hold BACK button for 5s to restart
- Ensure you're not pressing buttons during screen refresh (~430ms blocking period)

### Display Issues

- E-paper refresh takes ~430ms in fast mode
- Screen updates happen after most keypresses
- If screen appears frozen, wait for refresh to complete
- Text wraps at configured characters per line (adjustable in Settings)

## Development

### Architecture

MicroSlate uses a modular architecture with shared UI state:
- **Shared state** (currentState, screenDirty, etc.) defined in `main.cpp`, extern'd in modules
- **Event-driven input**: Physical buttons and BLE keyboard both enqueue events into a common queue
- **Non-blocking BLE**: Connection/pairing runs on FreeRTOS task to keep main loop responsive
- **One-shot scanning**: 5-second BLE scans prevent continuous radio noise that interferes with ADC button reads

### Technical Details

**BLE Implementation:**
- Uses NimBLE-Arduino 1.4.3 for low memory footprint
- "Just Works" pairing: `setSecurityAuth(true, false, false)` + `BLE_HS_IO_NO_INPUT_OUTPUT`
- Supports both 7-byte (compact) and 8-byte (standard) HID keyboard reports
- Protocol Mode set to Report Protocol (1) before subscribing to characteristics
- Subscribes to all notifiable Report characteristics (0x2a4d) to find keyboard input

**Button Handling:**
- Physical buttons use resistor ladder on ADC1 pins (GPIO 1, 2)
- InputManager provides 5ms debounce
- BLE scanning can cause ADC noise, hence one-shot scanning approach

**Display:**
- 800x480 physical, 480x800 logical (portrait mode)
- Fast refresh mode: ~430ms per update (blocking)
- Refresh triggered by screenDirty flag after UI state changes

## Limitations

- No EPUB or PDF reading capabilities (focus is on text editing)
- Limited to text files only (no rich formatting)
- BLE keyboard only (no on-screen keyboard)

## Future Enhancements

- **Screen refresh optimization**: Reduce refreshes during continuous typing
- **Battery indicator**: Show device battery level
- **Keyboard battery**: Display connected keyboard battery level
- **File organization**: Folders, search, sorting options
- **Text formatting**: Bold, italic, headings (if display supports partial updates)
- **Export options**: Sync to cloud or computer
- **Multiple keyboard support**: Quick switching between paired keyboards
- **Customizable keybindings**: User-defined shortcuts