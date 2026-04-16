#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

static const int MAX_NAMES = 10;
static const int MAX_ADVENTURES = 16;

struct AppConfig {
  // General settings
  String apiKey;
  String model;
  bool hyphenate;  // wrap long words with a hyphen instead of overflowing

  // Character names for the small dial
  String names[MAX_NAMES];
  int nameCount;

  // Adventure destinations for the large dial
  String adventures[MAX_ADVENTURES];
  int adventureCount;

  // Story prompts
  String storyPrompt;
  String instructions;
  String surpriseEnding;
  String blueContinue;
  String blueComplete;
  String yellowContinue;
  String yellowComplete;
};

// Load default values into config
void configLoadDefaults(AppConfig &cfg);

// Load config from persistent storage (NVS), falling back to defaults
void configLoad(AppConfig &cfg);

// Save config to persistent storage (NVS)
void configSave(const AppConfig &cfg);

// Reset config to defaults and clear persistent storage
void configReset(AppConfig &cfg);

// Serialize config to JSON document
void configToJson(const AppConfig &cfg, JsonDocument &doc);

// Deserialize config from JSON document
bool configFromJson(AppConfig &cfg, const JsonDocument &doc);

#endif // CONFIG_H
