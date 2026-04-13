#include "config.h"
#include <Preferences.h>

static const char NVS_NAMESPACE[] = "cyogpt";

void configLoadDefaults(AppConfig &cfg) {
  cfg.apiKey = "";
  cfg.model = "gpt-4o";

  cfg.nameCount = 10;
  cfg.names[0] = "Aunt Lily (Mia and Zoe's aunt, married to Uncle Ray)";
  cfg.names[1] = "Mia (a 7-year-old girl, 5-year-old Zoe's sister)";
  cfg.names[2] = "Zoe (a 5-year-old girl, 7-year-old Mia's sister)";
  cfg.names[3] = "Grandma June (Mia and Zoe's grandmother, Dad's mom, the villain in the story)";
  cfg.names[4] = "Buddy (Uncle Ray and Aunt Lily's dog with floppy ears)";
  cfg.names[5] = "Dad (Mia and Zoe's dad)";
  cfg.names[6] = "Mom (Mia and Zoe's mom, Aunt Lily's sister)";
  cfg.names[7] = "Nana (Mia and Zoe's grandmother, Aunt Lily and Mom's mom)";
  cfg.names[8] = "Papa Joe (Mia and Zoe's grandfather, Aunt Lily and Mom's dad)";
  cfg.names[9] = "Uncle Ray (Mia and Zoe's uncle, married to Aunt Lily)";

  cfg.adventureCount = 16;
  cfg.adventures[0]  = " who go on an adventure in the Amazon Rainforest.";
  cfg.adventures[1]  = " who go on an adventure to a Robotic World.";
  cfg.adventures[2]  = " who go on adventure exploring secret caves.";
  cfg.adventures[3]  = " who go to a Magical School.";
  cfg.adventures[4]  = " who go on an adventure in an underground city.";
  cfg.adventures[5]  = " who go on a Time Travel adventure in a Tardis.";
  cfg.adventures[6]  = " who go on a Medieval Fantasy adventure.";
  cfg.adventures[7]  = " who go on a Arctic Expedition adventure.";
  cfg.adventures[8]  = " who go on a Jungle Safari adventure.";
  cfg.adventures[9]  = " who go on an underwater adventure.";
  cfg.adventures[10] = " who go on a Prehistoric Adventure.";
  cfg.adventures[11] = " who go on an adventure in the Enchanted Forest.";
  cfg.adventures[12] = " who go on a Pirate Adventure. Please tell the entire story in pirate speak.";
  cfg.adventures[13] = " who go on adventure in outer space. Make sure there are lasers and aliens.";
  cfg.adventures[14] = " who go on adventure in the African Savanna.";
  cfg.adventures[15] = " who get recruited to join the Superhero Academy.";

  cfg.storyPrompt = "Please write a simple outline for a 5 chapter choose your own adventure story. "
    "The first act should set up the story and provide a pivotal decision at the end that will "
    "completely change the course of the story. The second act should present a challenge and "
    "incorporate a choice that will come back in the 5th and final chapter. The third act should "
    "provide a false victory or twist. The fourth act should be the final push. The 5th and final "
    "act is the climax and resolution. The story is about ";

  cfg.instructions = "Once completed, please respond 'COMPLETED'. When I send the word 'Begin', "
    "respond with the first chapter. Each chapter should be approximately 180 words. End each "
    "chapter with exactly two options, 'yellow' or 'blue' to continue the story in the format "
    "'to do x, press the yellow button' or 'to do y, press the blue button'. At the end of each "
    "chapter you must present these two options. Don't include any chapter titles or numbers and "
    "use only basic punctuation like single quotes, commas, periods, new line, and exclamation "
    "points. Do not use bold, italics, or any text formatting.";

  cfg.surpriseEnding  = "Write a surprise ending to the story that is different than either the blue or the yellow options.";
  cfg.blueContinue    = "Press the blue button.";
  cfg.blueComplete    = "Write the final chapter to the story. Push the blue button.";
  cfg.yellowContinue  = "Press the yellow button.";
  cfg.yellowComplete  = "Write the final chapter to the story. Push the yellow button.";
}

void configLoad(AppConfig &cfg) {
  configLoadDefaults(cfg);

  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    Serial.println("NVS: Could not open for reading, using defaults");
    return;
  }

  if (prefs.isKey("apiKey"))  cfg.apiKey = prefs.getString("apiKey");
  if (prefs.isKey("model"))   cfg.model  = prefs.getString("model");

  if (prefs.isKey("nameCnt")) {
    int count = prefs.getInt("nameCnt", cfg.nameCount);
    if (count > 0 && count <= MAX_NAMES) {
      cfg.nameCount = count;
      for (int i = 0; i < cfg.nameCount; i++) {
        char key[8];
        snprintf(key, sizeof(key), "name%d", i);
        if (prefs.isKey(key)) {
          cfg.names[i] = prefs.getString(key);
        }
      }
    }
  }

  if (prefs.isKey("advCnt")) {
    int count = prefs.getInt("advCnt", cfg.adventureCount);
    if (count > 0 && count <= MAX_ADVENTURES) {
      cfg.adventureCount = count;
      for (int i = 0; i < cfg.adventureCount; i++) {
        char key[8];
        snprintf(key, sizeof(key), "adv%d", i);
        if (prefs.isKey(key)) {
          cfg.adventures[i] = prefs.getString(key);
        }
      }
    }
  }

  if (prefs.isKey("prompt"))   cfg.storyPrompt    = prefs.getString("prompt");
  if (prefs.isKey("instruct")) cfg.instructions    = prefs.getString("instruct");
  if (prefs.isKey("surprise")) cfg.surpriseEnding  = prefs.getString("surprise");
  if (prefs.isKey("blueCont")) cfg.blueContinue    = prefs.getString("blueCont");
  if (prefs.isKey("blueComp")) cfg.blueComplete    = prefs.getString("blueComp");
  if (prefs.isKey("yelCont"))  cfg.yellowContinue  = prefs.getString("yelCont");
  if (prefs.isKey("yelComp"))  cfg.yellowComplete  = prefs.getString("yelComp");

  prefs.end();
  Serial.println("NVS: Configuration loaded");
}

void configSave(const AppConfig &cfg) {
  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println("NVS: Could not open for writing");
    return;
  }

  prefs.putString("apiKey", cfg.apiKey);
  prefs.putString("model",  cfg.model);

  prefs.putInt("nameCnt", cfg.nameCount);
  for (int i = 0; i < cfg.nameCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "name%d", i);
    prefs.putString(key, cfg.names[i]);
  }

  prefs.putInt("advCnt", cfg.adventureCount);
  for (int i = 0; i < cfg.adventureCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "adv%d", i);
    prefs.putString(key, cfg.adventures[i]);
  }

  prefs.putString("prompt",   cfg.storyPrompt);
  prefs.putString("instruct", cfg.instructions);
  prefs.putString("surprise", cfg.surpriseEnding);
  prefs.putString("blueCont", cfg.blueContinue);
  prefs.putString("blueComp", cfg.blueComplete);
  prefs.putString("yelCont",  cfg.yellowContinue);
  prefs.putString("yelComp",  cfg.yellowComplete);

  prefs.end();
  Serial.println("NVS: Configuration saved");
}

void configReset(AppConfig &cfg) {
  Preferences prefs;
  if (prefs.begin(NVS_NAMESPACE, false)) {
    prefs.clear();
    prefs.end();
  }
  configLoadDefaults(cfg);
  Serial.println("NVS: Configuration reset to defaults");
}

void configToJson(const AppConfig &cfg, JsonDocument &doc) {
  doc["apiKey"] = cfg.apiKey;
  doc["model"]  = cfg.model;

  JsonArray names = doc["names"].to<JsonArray>();
  for (int i = 0; i < cfg.nameCount; i++) {
    names.add(cfg.names[i]);
  }

  JsonArray adventures = doc["adventures"].to<JsonArray>();
  for (int i = 0; i < cfg.adventureCount; i++) {
    adventures.add(cfg.adventures[i]);
  }

  JsonObject prompts = doc["prompts"].to<JsonObject>();
  prompts["storyPrompt"]    = cfg.storyPrompt;
  prompts["instructions"]   = cfg.instructions;
  prompts["surpriseEnding"] = cfg.surpriseEnding;
  prompts["blueContinue"]   = cfg.blueContinue;
  prompts["blueComplete"]   = cfg.blueComplete;
  prompts["yellowContinue"] = cfg.yellowContinue;
  prompts["yellowComplete"] = cfg.yellowComplete;
}

bool configFromJson(AppConfig &cfg, const JsonDocument &doc) {
  if (doc["apiKey"].is<const char *>()) cfg.apiKey = doc["apiKey"].as<String>();
  if (doc["model"].is<const char *>())  cfg.model  = doc["model"].as<String>();

  if (doc["names"].is<JsonArrayConst>()) {
    JsonArrayConst arr = doc["names"];
    cfg.nameCount = 0;
    for (JsonVariantConst v : arr) {
      if (cfg.nameCount < MAX_NAMES) {
        cfg.names[cfg.nameCount++] = v.as<String>();
      }
    }
  }

  if (doc["adventures"].is<JsonArrayConst>()) {
    JsonArrayConst arr = doc["adventures"];
    cfg.adventureCount = 0;
    for (JsonVariantConst v : arr) {
      if (cfg.adventureCount < MAX_ADVENTURES) {
        cfg.adventures[cfg.adventureCount++] = v.as<String>();
      }
    }
  }

  if (doc["prompts"].is<JsonObjectConst>()) {
    JsonObjectConst p = doc["prompts"];
    if (p["storyPrompt"].is<const char *>())    cfg.storyPrompt    = p["storyPrompt"].as<String>();
    if (p["instructions"].is<const char *>())   cfg.instructions   = p["instructions"].as<String>();
    if (p["surpriseEnding"].is<const char *>()) cfg.surpriseEnding = p["surpriseEnding"].as<String>();
    if (p["blueContinue"].is<const char *>())   cfg.blueContinue   = p["blueContinue"].as<String>();
    if (p["blueComplete"].is<const char *>())   cfg.blueComplete   = p["blueComplete"].as<String>();
    if (p["yellowContinue"].is<const char *>()) cfg.yellowContinue = p["yellowContinue"].as<String>();
    if (p["yellowComplete"].is<const char *>()) cfg.yellowComplete = p["yellowComplete"].as<String>();
  }

  return true;
}
