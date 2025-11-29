
# Choose Your Own GPT

Choose Your Own GPT is a seemingly analog device without any screens or visible digital elements. Spinning the physical dials to select characters and a story setting and then hitting the large arcade 'go' button, the built in printer will spit out a Choose your own Adventure type story on the built in receipt printer. 


## Features

- Built in 58mm thermal printer to dispense stories on demand
- Designed to be printable on 200mm^2 3D Printers
- Top dials can be snapped off of main PCB to allow modification to any size
- Optional 4th dial can be added to increase story objects or add an aditional element (talk like a pirate, etc) setting
- Support for e-ink display (untested)
- **Web-based configuration portal** for easy setup and customization
- **Configurable dial labels**: Customize all character names and adventure settings via web interface
- **Configurable story prompts**: Adjust the AI story generation parameters
- **Persistent storage**: All configuration saved to device flash memory
- **GitHub Actions CI/CD**: Automated builds for every commit

## Installation

### Using PlatformIO (Recommended)

1. Install [PlatformIO](https://platformio.org/install/cli) or use the PlatformIO IDE extension for VS Code.

2. Clone this repository:
   ```bash
   git clone https://github.com/jrutlen/Choose-Your-Own-GPT.git
   cd Choose-Your-Own-GPT/ChooseYourOwnGPT
   ```

3. Copy the credentials template:
   ```bash
   cp src/credentials.h.template src/credentials.h
   ```

4. Build and upload:
   ```bash
   pio run --target upload
   ```

### Initial Configuration

Once powered on, the device will create a Wi-Fi access point:

1. Connect to the `ChooseYourOwnGPT` Wi-Fi network (password: `itMightBeMagic`)
2. Your device should automatically open the configuration portal, or navigate to `http://192.168.4.1`
3. Configure the following:
   - **OpenAI API Key**: Get one from [platform.openai.com/api-keys](https://platform.openai.com/api-keys)
   - **WiFi Network**: Connect to your home network
   - **Dial Labels**: Customize the 10 character names and 16 adventure settings (optional)
   - **Story Prompts**: Customize the story generation prompts (optional)

All configuration is saved to the device's flash memory and persists across reboots.

### Configuration Options

The web portal allows you to configure:

- **API Key**: Your OpenAI API key for generating stories
- **Story Prompt**: The initial prompt that sets up the story structure
- **Instructions**: Formatting and style instructions for the AI
- **Dial 1 Labels** (10 positions): Character names for the first dial
- **Dial 2 Labels** (10 positions): Character names for the second dial
- **Adventure Settings** (16 positions): Different adventure scenarios for the large dial

### Reconfiguring

To access the configuration portal after initial setup:
1. The device hosts a web server at its IP address on your network
2. You can also trigger AP mode by following the NetWizard library instructions

## Building from Source

### Prerequisites

- Python 3.7 or newer
- PlatformIO Core

### Build

```bash
cd ChooseYourOwnGPT
pio run
```

### GitHub Actions

This project includes GitHub Actions workflows that automatically build the firmware on every push. You can download pre-built firmware binaries from the Actions tab.

## Related

Build log with photos

[Hackaday.io](https://hackaday.io/project/192751-choose-your-own-gpt)