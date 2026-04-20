
# Choose Your Own GPT

Choose Your Own GPT is a seemingly analog device without any screens or visible digital elements. Spinning the physical dials to select characters and a story setting and then hitting the large arcade 'go' button, the built-in printer will spit out a Choose Your Own Adventure type story on the built-in receipt printer.


## Features

- Built-in 58mm thermal printer to dispense stories on demand
- Designed to be printable on 200mm² 3D printers
- Top dials can be snapped off of the main PCB to allow modification to any size
- Optional 4th dial can be added to increase story objects or add an additional element (talk like a pirate, etc.)
- Support for e-ink display (untested)
- Over-the-air (OTA) firmware updates via the web portal
- Import/export configuration as JSON

## Hardware Requirements

- ESP32 development board
- 58mm thermal printer (connected to ESP32 Serial2: RX=16, TX=17)
- Adafruit NeoPixel LEDs (WaitEnd ring: 13 LEDs on pin 26; Go button: 3 LEDs on pin 33)
- Arcade-style buttons with LEDs (Red: pin 27, Blue: pin 12, Green: pin 2, Yellow: pin 15)
- Rotary dials wired to input matrix (see PCB directory for schematic)
- 3D-printed enclosure (STL files in the `STLs` directory)
- Bill of materials in `Bill of Materials.csv`

## Building and Flashing

This project uses [PlatformIO](https://platformio.org/). All library dependencies are declared in `ChooseYourOwnGPT/platformio.ini` and are fetched automatically on first build.

1. Install [PlatformIO IDE](https://platformio.org/install) or the PlatformIO CLI.
2. Open the `ChooseYourOwnGPT` folder as a PlatformIO project.
3. Build and upload to the ESP32:
   ```
   pio run --target upload
   ```
4. You do **not** need to add your API key to the code before uploading — it is set via the web portal after the device is running.

### Library Dependencies

| Library | Version |
|---------|---------|
| Adafruit NeoPixel | 1.12.4 |
| Adafruit Thermal Printer | 1.4.1 |
| NetWizard | 1.2.0 |
| ArduinoJson | 7.3.1 |

## First-Time Setup

1. Power on the device. It will start a Wi-Fi access point named **`ChooseYourOwnGPT`** with the password **`itMightBeMagic`**.
2. Connect to that network from your phone or computer.
3. A captive portal will open automatically (or navigate to `http://192.168.4.1`).
4. Enter your home Wi-Fi credentials and save. The device will reboot and connect to your network.

## Configuration

Once the device is connected to your Wi-Fi network, open a browser and go to:

```
http://<device-ip>/config
```

The device IP is printed to the serial console on connection. The configuration portal lets you adjust all settings without reflashing.

### General Settings

| Setting | Description |
|---------|-------------|
| **OpenAI API Key** | Your `sk-…` key from [platform.openai.com](https://platform.openai.com/api-keys). Stored on the device only — never sent anywhere except the OpenAI API. |
| **OpenAI Model** | The model to use for story generation (default: `gpt-4o`). |
| **Hyphenate long words** | When enabled, words that are too long for a line are broken with a hyphen instead of overflowing. |

### Character Names (Small Dial)

Up to **10** character names can be assigned to positions on the small dial. Each position corresponds to a dial setting (0–9). Names can include relationship context (e.g. `"Mia (a 7-year-old girl)"`), which is passed directly to the story prompt.

### Adventure Destinations (Large Dial)

Up to **16** adventure destinations can be assigned to positions on the large dial. Each entry is a sentence fragment appended to the story prompt (e.g. `" who go on an adventure in the Amazon Rainforest."`).

### Story Prompts

These fields control the text sent to the OpenAI API at each stage of the story:

| Field | Description |
|-------|-------------|
| **Story Prompt** | Opening prompt prepended before the selected character names. |
| **Instructions** | Instructions appended after the adventure destination (formatting rules, chapter length, button options, etc.). |
| **Surprise Ending Prompt** | Sent when the red button is pressed to request an unexpected ending. |
| **Blue Button – Continue** | Message sent when the blue button is pressed to continue the story. |
| **Blue Button – Final Chapter** | Message sent when the blue button is pressed in the final chapter. |
| **Yellow Button – Continue** | Message sent when the yellow button is pressed to continue the story. |
| **Yellow Button – Final Chapter** | Message sent when the yellow button is pressed in the final chapter. |

### Import / Export

Use the **Export Settings JSON** button to download all current settings as a `cyogpt-config.json` file. Use **Import Settings JSON** to restore or share a configuration.

### OTA Firmware Update

The `/config` portal includes a firmware update section. Select a compiled `.bin` file (from `pio run` build output) and click **Upload Firmware**. The device will reboot automatically after a successful update.

## Usage

1. Power on the device and wait for it to connect to Wi-Fi (the LEDs will indicate status).
2. Spin the **small dial** to select a character and the **large dial** to choose an adventure destination.
3. Press the **green Go button** to generate a story. The printer will output the first chapter.
4. At the end of each chapter, press the **blue** or **yellow** button to choose how the story continues.
5. Press the **red button** at any point to trigger a surprise ending.
6. After the final (5th) chapter, the story is complete. Set the dials and press Go to start a new story.

## Related

Build log with photos

[Hackaday.io](https://hackaday.io/project/192751-choose-your-own-gpt)