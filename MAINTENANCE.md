# Maintenance Guide

## Code Quality and Best Practices

This guide outlines best practices for maintaining the Choose Your Own GPT codebase.

## Error Handling

The code includes comprehensive error handling for:

1. **OpenAI API Calls**: All API calls check for errors and provide feedback
   - Visual feedback via red LED on error
   - Serial logging for debugging
   - Graceful recovery to ready state on failure

2. **Memory Management**: 
   - Malloc operations check for NULL returns
   - Memory is properly freed after use to prevent leaks

3. **Network Connectivity**:
   - WiFi status is checked before API calls
   - Connection errors display error feedback

4. **Input Validation**:
   - Dial positions are validated before array access
   - Printer functions check for NULL pointers

## OpenAI API Configuration

The code uses the OpenAI GPT-4o model, which is recommended as of 2024. Key configuration:

- **Model**: `gpt-4o` - Latest GPT-4 Optimized model
- **Alternatives**: 
  - `gpt-4o-mini` - Faster and cheaper for simple stories
  - `gpt-4-turbo` - Older but stable alternative
- **Timeout**: 30 seconds (configured via `SERVER_RESPONSE_WAIT_TIME`)
- **Token Limit**: 750 tokens per response

### Updating the Model

To use a different model, update the `model` variable in `src/main.cpp`:

```cpp
const char *model = "gpt-4o-mini";  // or your preferred model
```

Check https://platform.openai.com/docs/models for current model availability.

## Library Updates

The project uses pinned library versions for stability. To check for updates:

```bash
cd ChooseYourOwnGPT
pio pkg outdated
```

To update libraries (test thoroughly after updating):

```bash
pio pkg update
```

### Current Libraries

- **Adafruit NeoPixel** (1.12.4): LED control
- **Adafruit Thermal Printer** (1.4.1): Receipt printer
- **NetWizard** (1.2.0): WiFi configuration portal
- **ArduinoJson** (7.3.1): JSON parsing (v7 is current major version)
- **ChatGPTuino** (0.1.0): OpenAI API interface

## Building and Testing

### Prerequisites

Install PlatformIO Core or use the PlatformIO IDE extension for VS Code.

### Building

```bash
cd ChooseYourOwnGPT
pio run
```

### Uploading

```bash
pio run --target upload
```

### Monitoring Serial Output

```bash
pio device monitor
```

## API Key Management

Never commit your OpenAI API key to version control!

1. Copy `src/credentials.h.template` to `src/credentials.h`
2. Add your API key to `credentials.h`
3. The `.gitignore` file ensures `credentials.h` is not committed

## Common Issues

### API Timeout Errors

If you see timeout errors:
1. Increase `SERVER_RESPONSE_WAIT_TIME` in `main.cpp`
2. Check your internet connection
3. Verify your OpenAI API key is valid

### Memory Allocation Failures

If memory allocation fails:
1. Check Serial output for "Memory allocation failed" messages
2. The device will show a red LED and return to ready state
3. Restart the device if the issue persists

### WiFi Connection Issues

The device will show connection status through LEDs:
- **Blue LED**: Connecting to WiFi
- **Green LED**: Ready to generate story
- **Red LED**: Error state

## Code Review Checklist

When making changes:

- [ ] Add error handling for all API calls
- [ ] Check for memory leaks (malloc without free)
- [ ] Validate array indices before access
- [ ] Check WiFi connection before network operations
- [ ] Add Serial logging for debugging
- [ ] Test error recovery paths
- [ ] Update documentation if behavior changes

## Security Best Practices

1. **API Keys**: Never hardcode API keys in the source
2. **Input Validation**: Always validate user input (dial positions, etc.)
3. **Memory Safety**: Check all malloc/free operations
4. **Network Security**: Use HTTPS for API calls (handled by library)

## Future Improvements

Potential areas for enhancement:

1. Add retry logic for transient API failures
2. Implement exponential backoff for API rate limits
3. Add support for storing multiple API keys
4. Implement story caching to reduce API calls
5. Add telemetry for monitoring device health
