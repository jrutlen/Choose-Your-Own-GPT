
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

### Building with GitHub Actions

A workflow (`.github/workflows/build.yml`) automatically builds the firmware on every push or pull request to `main`/`master` that touches `ChooseYourOwnGPT/` or the workflow file itself. No local toolchain is required to produce a flashable binary.

**What the workflow does:**
1. Checks out the repository.
2. Installs PlatformIO via `pip`.
3. Runs `pio run` inside `ChooseYourOwnGPT/`.
4. Uploads the compiled binaries as a `firmware` artifact:
   - `firmware.bin` — application image
   - `bootloader.bin` — ESP32 bootloader
   - `partitions.bin` — partition table

**Downloading the artifact:**
1. Go to the **Actions** tab of the repository on GitHub.
2. Click the latest successful **Build Firmware** run.
3. Scroll to the **Artifacts** section at the bottom and download `firmware`.
4. Unzip the archive to get the `.bin` files.

**Flashing the downloaded firmware:**

To flash from scratch (first time or full reflash), use `esptool.py`:
```
esptool.py --chip esp32 --baud 921600 \
  write_flash \
  0x1000  bootloader.bin \
  0x8000  partitions.bin \
  0x10000 firmware.bin
```

To update an already-running device, use the OTA firmware update in the web portal — see [OTA Firmware Update](#ota-firmware-update) below.

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

## Network Printing

The device also works as a simple network printer. It listens on TCP port **9100** and forwards every byte it receives, unchanged, to the thermal printer. A job ends when the client closes the connection or sends nothing for 3 seconds. After each job the printer's text formatting is reset, so stories print normally afterwards.

Jobs are handled between story steps. While a chapter is generating or printing, the sender waits until the device is free.

Quick test from any machine on the network:
```
echo "Hello from the network" | nc -q 5 <device-ip> 9100
```

### Python client

`clients/python/cyogpt_printer.py` is a single-file client that builds the printer's command bytes. Plain text and ASCII-art glyphs need only the standard library. Printing images or rendering TrueType text needs [Pillow](https://pypi.org/project/pillow/) (`pip install pillow`).

```python
from cyogpt_printer import Printer

HEART = [
    ".##...##.",
    "####.####",
    "#########",
    ".#######.",
    "..#####..",
    "...###...",
    "....#....",
]

with Printer("192.168.1.50") as p:          # sent as one job when the block exits
    p.justify("C").size("L").bold().text("Hello!").bold(False).size("S")
    p.glyph(HEART, scale=6, align="C")      # ASCII-art glyph, each pixel 6x6 dots
    p.render_text("☀ 21°C ☂", font_path="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")
    p.image("logo.png")                     # scaled to 384 px and dithered
    p.justify("L").text("Printed from a Raspberry Pi")
    p.feed(3)
```

| Method | Description |
|--------|-------------|
| `text(s)` / `write(s)` | Text with or without a newline (32 chars per line at size S, encoded as CP437) |
| `bold()`, `underline(0-2)`, `inverse()`, `size('S'/'M'/'L')`, `justify('L'/'C'/'R')`, `line_height(dots)` | Text styles |
| `feed(n)` | Feed *n* lines |
| `glyph(rows, scale, align)` | Print ASCII-art (`#` = black) as a bitmap |
| `bitmap(w, h, data, align)` | Print a raw 1-bit bitmap (MSB-first rows, 1 = black, max 384 px wide) |
| `image(path_or_pil, align)` | Print an image (needs Pillow) |
| `render_text(s, font_path, font_size, align)` | Draw any Unicode text or symbol with a font (needs Pillow) |
| `raw(bytes)` | Send arbitrary printer command bytes |

It can also be run from the command line:
```
python cyogpt_printer.py <device-ip> "Some text" --center --size M
python cyogpt_printer.py <device-ip> --image photo.jpg
```

> Anyone on your local network can print to the device. There is no authentication.

## Related

Build log with photos

[Hackaday.io](https://hackaday.io/project/192751-choose-your-own-gpt)