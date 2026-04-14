// WiFi and Web
#include <Arduino.h>
#include <WebServer.h>
#include <NetWizard.h>
#include <memory>

// Hardware
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_Thermal.h"

// OpenAI API
#define SERVER_RESPONSE_WAIT_TIME (30 * 1000)
#include <ChatGPTuino.h>

// Configuration and Web Portal
#include "config.h"
#include "web_portal.h"

// ─── OpenAI Chat ────────────────────────────────────────────
static const int TOKENS = 750;
static const int NUM_MESSAGES = 14;
std::unique_ptr<ChatGPTuino> chat = std::make_unique<ChatGPTuino>(TOKENS, NUM_MESSAGES);

// ─── Networking ─────────────────────────────────────────────
WebServer server(80);
NetWizard NW(&server);

// ─── Application Configuration ──────────────────────────────
AppConfig appConfig;

// ─── Hardware: LEDs ─────────────────────────────────────────
Adafruit_NeoPixel WaitEnd(13, 26, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel Go(3, 33, NEO_GRB + NEO_KHZ800);

// ─── Hardware: Thermal Printer ──────────────────────────────
Adafruit_Thermal printer(&Serial2, 4);

// ─── Pin Definitions: Button LEDs ───────────────────────────
static const int PIN_LED_RED    = 27;
static const int PIN_LED_BLUE   = 12;
static const int PIN_LED_GREEN  = 2;
static const int PIN_LED_YELLOW = 15;

// ─── Pin Definitions: Dial LEDs ─────────────────────────────
static const int PIN_DIAL_LED1 = 21;
static const int PIN_DIAL_LED2 = 13;
static const int PIN_DIAL_LED3 = 5;

// ─── Pin Definitions: Input Matrix Columns ──────────────────
static const int PIN_SCAN_1 = 25;  // Dial 3
static const int PIN_SCAN_2 = 36;  // Dial 2
static const int PIN_SCAN_4 = 35;  // Dial 1A
static const int PIN_SCAN_8 = 39;  // Dial 1B

// ─── Pin Definitions: Input Matrix Rows ─────────────────────
static const int PIN_COM_D0  = 22;
static const int PIN_COM_D1  = 18;
static const int PIN_COM_D2  = 19;
static const int PIN_COM_BTN = 23;
static const int PIN_COM_MAG = 32;

// ─── WaitEnd LED Segments ───────────────────────────────────
static const int WAIT_START  = 0;
static const int WAIT_LENGTH = 9;
static const int END_START   = 9;
static const int END_LENGTH  = 4;

// ─── PWM Configuration ─────────────────────────────────────
static const int PWM_FREQ         = 5000;
static const int PWM_CHANNEL_DIAL = 0;
static const int PWM_CHANNEL_BTN  = 1;
static const int PWM_RESOLUTION   = 8;

// ─── Matrix Scanning ───────────────────────────────────────
static const int SCAN_DELAY_MS = 100;
static const int DIAL_READ_DELAY_MS = 10;  // Settle time between sequential dial reads
unsigned long lastScanTime = 0;

int  dial[3]     = {0, 0, 0};
int  lastDial[3] = {0, 0, 0};
bool dialSet[3]  = {false, false, false};
bool button[4]   = {false, false, false, false};

// ─── State Machine ─────────────────────────────────────────
//  0 = boot
//  1 = connecting to WiFi
//  2 = connected to WiFi
//  3 = initializing (reading initial dial positions)
//  4 = ready / waiting for dial input
//  5 = story requested (first chapter transition)
//  6 = generating first chapter
//  8 = generating next chapter
//  9 = story printed, waiting for button decision
// 11 = story complete
int state = 0;
int currentChapter = 1;
static const int MAX_CHAPTERS = 5;
int decision = 0;  // 0 = none, 1 = red (surprise), 2 = blue, 3 = yellow

// ─── LED Fade Animation ────────────────────────────────────
int fade = 0;
int slowFade = 0;
unsigned long lastFadeTime = 0;
unsigned long lastSlowFadeTime = 0;
int fadeDirection = 1;
int slowFadeDirection = 1;
static const unsigned long FADE_SPEED_MS      = 1;
static const unsigned long SLOW_FADE_SPEED_MS = 10;
static const int MAX_SLOW_FADE = 100;

// ─── Forward Declarations ──────────────────────────────────
void onConfigSaved();
void printTitle(int chapterNumber);
void sendToPrint(const char *message);
int  checkDial(int dialNumber);
void checkButton();
void fadeCalc();
void slowFadeCalc();
void resetHardwareState();
void initChatIfConfigured();

// ─── Chat Initialization Helper ────────────────────────────
void initChatIfConfigured() {
  if (appConfig.apiKey.length() > 0) {
    chat->init(appConfig.apiKey.c_str(), appConfig.model.c_str());
    Serial.println("Chat initialized");
  } else {
    Serial.println("Chat NOT initialized - configure API key at /config");
  }
}

// ─── Configuration Callback ────────────────────────────────
void onConfigSaved() {
  initChatIfConfigured();
}

// ─── Setup ─────────────────────────────────────────────────
void setup() {
  state = 1;
  WaitEnd.clear();
  WaitEnd.fill(WaitEnd.Color(0, 0, 255), WAIT_START, WAIT_LENGTH);
  WaitEnd.show();

  Serial.begin(115200);
  delay(1000);

  // Load persistent configuration from NVS
  configLoad(appConfig);
  Serial.printf("Config loaded. Model: %s\n", appConfig.model.c_str());
  if (appConfig.apiKey.length() == 0) {
    Serial.println("WARNING: No API key configured. Visit /config after WiFi setup.");
  }

  // WiFi provisioning via NetWizard
  NW.setStrategy(NetWizardStrategy::BLOCKING);

  NW.onConnectionStatus([](NetWizardConnectionStatus status) {
    const char *statusStr = "Unknown";
    switch (status) {
      case NetWizardConnectionStatus::DISCONNECTED:      statusStr = "Disconnected"; break;
      case NetWizardConnectionStatus::CONNECTING:        statusStr = "Connecting"; break;
      case NetWizardConnectionStatus::CONNECTED:         statusStr = "Connected"; break;
      case NetWizardConnectionStatus::CONNECTION_FAILED: statusStr = "Connection Failed"; break;
      case NetWizardConnectionStatus::CONNECTION_LOST:   statusStr = "Connection Lost"; break;
      case NetWizardConnectionStatus::NOT_FOUND:         statusStr = "Not Found"; break;
      default: break;
    }
    Serial.printf("WiFi status: %s\n", statusStr);

    if (status == NetWizardConnectionStatus::CONNECTED) {
      Serial.printf("Local IP: %s\n", NW.localIP().toString().c_str());
      Serial.printf("Config portal: http://%s/config\n", NW.localIP().toString().c_str());
      initChatIfConfigured();
      // (Re)start the web server every time WiFi connects or reconnects.
      // This is safe here because the lwIP stack is up by the time this
      // callback fires.  It also handles the reconnect case where the
      // server socket was lost after a WiFi drop.
      server.begin();
    }
  });

  NW.onPortalState([](NetWizardPortalState portalState) {
    const char *stateStr = "Unknown";
    switch (portalState) {
      case NetWizardPortalState::IDLE:                   stateStr = "Idle"; break;
      case NetWizardPortalState::CONNECTING_WIFI:        stateStr = "Connecting to WiFi"; break;
      case NetWizardPortalState::WAITING_FOR_CONNECTION: stateStr = "Waiting for Connection"; break;
      case NetWizardPortalState::SUCCESS:                stateStr = "Success"; break;
      case NetWizardPortalState::FAILED:                 stateStr = "Failed"; break;
      case NetWizardPortalState::TIMEOUT:                stateStr = "Timeout"; break;
      default: break;
    }
    Serial.printf("Portal state: %s\n", stateStr);
  });

  // Register config routes BEFORE autoConnect so they survive the captive-portal
  // lifecycle: NetWizard's _stopHTTP() only removes its own tracked handlers
  // (_index_handler, _status_handler, …), never the app's routes registered here.
  webPortalSetup(server, appConfig, onConfigSaved);

  NW.autoConnect("ChooseYourOwnGPT", "itMightBeMagic");
  // server.begin() is called inside the onConnectionStatus CONNECTED callback
  // above, which fires during autoConnect() once WiFi is up and the lwIP stack
  // is initialised.  It is also called on any subsequent reconnection.

  if (NW.isConfigured()) {
    Serial.println("WiFi configured");
  } else {
    Serial.println("WiFi not configured");
  }

  // Configure button LED pins
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);

  // Configure matrix input pins
  pinMode(PIN_SCAN_1, INPUT);
  pinMode(PIN_SCAN_2, INPUT);
  pinMode(PIN_SCAN_4, INPUT);
  pinMode(PIN_SCAN_8, INPUT);

  // Configure matrix output pins
  pinMode(PIN_COM_D0, OUTPUT);
  pinMode(PIN_COM_D1, OUTPUT);
  pinMode(PIN_COM_D2, OUTPUT);
  pinMode(PIN_COM_BTN, OUTPUT);
  pinMode(PIN_COM_MAG, OUTPUT);

  state = 2;

  // Turn off all LEDs
  Go.clear();
  WaitEnd.clear();
  Go.fill(Go.Color(0, 0, 0), 0, 3);
  WaitEnd.fill(WaitEnd.Color(0, 0, 0), WAIT_START, WAIT_LENGTH);
  WaitEnd.fill(WaitEnd.Color(0, 0, 0), END_START, END_LENGTH);
  Go.show();
  WaitEnd.show();

  // Initialize thermal printer
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  printer.begin();
  printer.reset();
  printer.wake();
  printer.setSize('S');
  printer.setDefault();

  // Print device name on boot (original logo style: medium, centred, bold inverse)
  printer.setSize('M');
  printer.justify('C');
  printer.boldOn();
  printer.inverseOn();
  printer.println("-Choose-Your-Own-GPT-");
  printer.boldOff();
  printer.inverseOff();
  printer.justify('L');
  printer.setSize('S');
  printer.feed(2);

  // Configure PWM for dial LEDs
  ledcAttachChannel(PIN_DIAL_LED1, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
  ledcAttachChannel(PIN_DIAL_LED2, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
  ledcAttachChannel(PIN_DIAL_LED3, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);

  state = 3;
}

// ─── Helper Functions ──────────────────────────────────────

void printTitle(int chapterNumber) {
  printer.wake();
  printer.setSize('L');
  printer.justify('C');
  printer.boldOn();
  printer.println("Chapter " + String(chapterNumber));
  printer.boldOff();
  printer.justify('L');
  printer.setSize('S');
  printer.feed(2);
}

void sendToPrint(const char *message) {
  printer.wake();
  printer.setSize('S');
  printer.println(message);
  printer.feed(3);
}

int checkDial(int dialNumber) {
  // Activate the row common pin for the selected dial
  digitalWrite(PIN_COM_D0, dialNumber == 0 ? HIGH : LOW);
  digitalWrite(PIN_COM_D1, dialNumber == 1 ? HIGH : LOW);
  digitalWrite(PIN_COM_D2, dialNumber == 2 ? HIGH : LOW);
  digitalWrite(PIN_COM_BTN, LOW);
  digitalWrite(PIN_COM_MAG, LOW);

  // Read 4-bit value from column pins
  int value = 0;
  if (digitalRead(PIN_SCAN_1)) value += 1;
  if (digitalRead(PIN_SCAN_2)) value += 2;
  if (digitalRead(PIN_SCAN_4)) value += 4;
  if (digitalRead(PIN_SCAN_8)) value += 8;

  // Reset all row commons
  digitalWrite(PIN_COM_D0, LOW);
  digitalWrite(PIN_COM_D1, LOW);
  digitalWrite(PIN_COM_D2, LOW);

  return value;
}

void checkButton() {
  digitalWrite(PIN_COM_BTN, HIGH);
  digitalWrite(PIN_COM_D0, LOW);
  digitalWrite(PIN_COM_D1, LOW);
  digitalWrite(PIN_COM_D2, LOW);
  digitalWrite(PIN_COM_MAG, LOW);

  button[0] = digitalRead(PIN_SCAN_1);
  button[1] = digitalRead(PIN_SCAN_2);
  button[2] = digitalRead(PIN_SCAN_8);
  button[3] = digitalRead(PIN_SCAN_4);

  digitalWrite(PIN_COM_BTN, LOW);
}

void fadeCalc() {
  if (millis() - lastFadeTime > FADE_SPEED_MS) {
    lastFadeTime = millis();
    if (fade < 255 && fadeDirection == 1) {
      fade++;
    } else if (fade > 75 && fadeDirection == 0) {
      fade--;
    } else if (fade <= 75) {
      fadeDirection = 1;
      fade = 76;
    } else if (fade >= 255) {
      fadeDirection = 0;
      fade = 254;
    }
    ledcWriteChannel(PWM_CHANNEL_DIAL, fade);
  }
}

void slowFadeCalc() {
  if (millis() - lastSlowFadeTime > SLOW_FADE_SPEED_MS) {
    lastSlowFadeTime = millis();
    if (slowFade < MAX_SLOW_FADE && slowFadeDirection == 1) {
      slowFade++;
    } else if (slowFade > 0 && slowFadeDirection == 0) {
      slowFade--;
    } else if (slowFade == 0) {
      slowFadeDirection = 1;
    } else if (slowFade == MAX_SLOW_FADE) {
      slowFadeDirection = 0;
    }
    ledcWriteChannel(PWM_CHANNEL_BTN, slowFade);
  }
}

void resetHardwareState() {
  Go.clear();
  Go.show();
  WaitEnd.clear();
  WaitEnd.show();

  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);

  digitalWrite(PIN_DIAL_LED1, LOW);
  digitalWrite(PIN_DIAL_LED2, LOW);
  digitalWrite(PIN_DIAL_LED3, LOW);

  dialSet[0] = false;
  dialSet[1] = false;
  dialSet[2] = false;

  ledcAttachChannel(PIN_DIAL_LED1, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
  ledcAttachChannel(PIN_DIAL_LED2, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
  ledcAttachChannel(PIN_DIAL_LED3, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);

  currentChapter = 1;

  // Reset the chat session for the next story so the previous story's message
  // history is not included in the next API request.  Reassigning the
  // unique_ptr automatically destroys the old object and creates a fresh one
  // with _msgCount = 0.
  chat = std::make_unique<ChatGPTuino>(TOKENS, NUM_MESSAGES);
  initChatIfConfigured();
}

// ─── Clamped index helpers ─────────────────────────────────

static int clampIndex(int value, int maxCount) {
  if (maxCount <= 0) return 0;
  if (value >= maxCount) return maxCount - 1;
  return value;
}

// ─── Main Loop ─────────────────────────────────────────────

void loop() {
  server.handleClient();
  // NW.loop() is intentionally NOT called here. We use NetWizardStrategy::BLOCKING
  // so autoConnect() handles the entire portal session synchronously. Calling
  // NW.loop() afterward causes it to eventually invoke _stopHTTP() (portal
  // cleanup timeout), which stops the WebServer socket and makes /config
  // unreachable. WiFi reconnection is handled natively by the ESP32 WiFi stack;
  // server.begin() is re-called from the onConnectionStatus CONNECTED callback.

  switch (state) {

  case 3: {
    // Read current dial positions as baseline; also sync dial[] to prevent
    // stale values from triggering false dialSet detections on the first
    // pass through state 4 (which would permanently detach the dial LED pins
    // from the PWM channel and break the fade animation).
    lastDial[0] = dial[0] = checkDial(0); delay(DIAL_READ_DELAY_MS);
    lastDial[1] = dial[1] = checkDial(1); delay(DIAL_READ_DELAY_MS);
    lastDial[2] = dial[2] = checkDial(2);
    state = 4;
    break;
  }

  case 4: {
    // Ready: wait for all three dials to be turned, then wait for green button
    fadeCalc();

    if (millis() - lastScanTime > SCAN_DELAY_MS) {
      lastScanTime = millis();
      dial[0] = checkDial(0); delay(DIAL_READ_DELAY_MS);
      dial[1] = checkDial(1); delay(DIAL_READ_DELAY_MS);
      dial[2] = checkDial(2);
    }

    // Detect dial changes
    if (dial[0] != lastDial[0]) dialSet[0] = true;
    if (dial[1] != lastDial[1]) dialSet[1] = true;
    if (dial[2] != lastDial[2]) dialSet[2] = true;

    // Light up dial LEDs as each is set
    if (dialSet[0]) { pinMode(PIN_DIAL_LED1, OUTPUT); digitalWrite(PIN_DIAL_LED1, HIGH); }
    if (dialSet[1]) { pinMode(PIN_DIAL_LED2, OUTPUT); digitalWrite(PIN_DIAL_LED2, HIGH); }
    if (dialSet[2]) { pinMode(PIN_DIAL_LED3, OUTPUT); digitalWrite(PIN_DIAL_LED3, HIGH); }

    if (dialSet[0] && dialSet[1] && dialSet[2]) {
      // All dials set — enable green button with fade and wait for press
      ledcAttachChannel(PIN_LED_GREEN, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
      Go.clear();
      Go.fill(Go.Color(0, 255, 0), 0, 3);
      Go.setBrightness(255);
      Go.show();

      checkButton();
      while (!button[2]) {
        server.handleClient();
        checkButton();
        fadeCalc();
      }
      state = 5;
    } else {
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_YELLOW, LOW);
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_BLUE, LOW);
    }
    break;
  }

  case 5: {
    // Transition: turn off Go LEDs, show wait indicator
    Go.clear();
    Go.show();
    WaitEnd.clear();
    WaitEnd.fill(WaitEnd.Color(0, 255, 0), WAIT_START, WAIT_LENGTH);
    WaitEnd.show();

    ledcDetach(PIN_LED_GREEN);
    pinMode(PIN_LED_GREEN, OUTPUT);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_DIAL_LED1, LOW);
    digitalWrite(PIN_DIAL_LED2, LOW);
    digitalWrite(PIN_DIAL_LED3, LOW);

    state = 6;
    break;
  }

  case 6: {
    // Generate the story outline and first chapter
    int nameIdx0 = clampIndex(dial[0], appConfig.nameCount);
    int nameIdx1 = clampIndex(dial[1], appConfig.nameCount);
    int advIdx   = clampIndex(dial[2], appConfig.adventureCount);

    String selectedName  = appConfig.names[nameIdx0];
    String selectedName2 = appConfig.names[nameIdx1];
    if (selectedName == selectedName2) {
      selectedName2 = "their evil twin";
    }

    // Build the full prompt
    String initialPrompt = appConfig.storyPrompt
      + selectedName + " and " + selectedName2
      + appConfig.adventures[advIdx]
      + appConfig.instructions;

    chat->putMessage(initialPrompt.c_str(), initialPrompt.length());
    printTitle(currentChapter);

    // Get the outline (not printed)
    chat->getResponse();

    // Request and print the first chapter
    const char *startStory = "Begin";
    chat->putMessage(startStory, strlen(startStory));
    chat->getResponse();
    sendToPrint(chat->getLastMessageContent());

    // Enable decision buttons
    ledcAttachChannel(PIN_LED_YELLOW, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
    ledcAttachChannel(PIN_LED_BLUE, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
    ledcAttachChannel(PIN_LED_RED, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_BTN);

    state = 9;
    break;
  }

  case 8: {
    // Generate next chapter based on player decision
    Go.clear();
    Go.show();
    WaitEnd.clear();
    WaitEnd.fill(WaitEnd.Color(0, 255, 0), WAIT_START, WAIT_LENGTH);
    WaitEnd.show();

    ledcDetach(PIN_LED_GREEN);
    pinMode(PIN_LED_GREEN, OUTPUT);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_DIAL_LED1, LOW);
    digitalWrite(PIN_DIAL_LED2, LOW);
    digitalWrite(PIN_DIAL_LED3, LOW);

    if (decision == 1) {
      // Surprise ending (red button)
      chat->putMessage(appConfig.surpriseEnding.c_str(), appConfig.surpriseEnding.length());
    } else if (decision == 2) {
      // Blue button
      if (currentChapter < MAX_CHAPTERS) {
        chat->putMessage(appConfig.blueContinue.c_str(), appConfig.blueContinue.length());
      } else {
        chat->putMessage(appConfig.blueComplete.c_str(), appConfig.blueComplete.length());
      }
    } else if (decision == 3) {
      // Yellow button
      if (currentChapter < MAX_CHAPTERS) {
        chat->putMessage(appConfig.yellowContinue.c_str(), appConfig.yellowContinue.length());
      } else {
        chat->putMessage(appConfig.yellowComplete.c_str(), appConfig.yellowComplete.length());
      }
    }

    Serial.println("Requesting Chapter: " + String(currentChapter));
    printTitle(currentChapter);
    chat->getResponse();
    printer.println(chat->getLastMessageContent());
    printer.feed(3);

    if (currentChapter >= MAX_CHAPTERS) {
      state = 11;
    } else {
      ledcAttachChannel(PIN_LED_YELLOW, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
      ledcAttachChannel(PIN_LED_BLUE, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_DIAL);
      ledcAttachChannel(PIN_LED_RED, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL_BTN);
      state = 9;
    }
    break;
  }

  case 9: {
    // Waiting for player decision
    fadeCalc();
    slowFadeCalc();

    Go.clear();
    WaitEnd.clear();
    WaitEnd.fill(WaitEnd.Color(slowFade, 0, 0), END_START, END_LENGTH);
    WaitEnd.show();
    Go.show();

    checkButton();
    if (button[1]) {
      // Red button — surprise ending
      state = 8;
      decision = 1;
    } else if (button[0]) {
      // Blue button — continue / complete
      state = 8;
      currentChapter++;
      decision = 2;
    } else if (button[3]) {
      // Yellow button — continue / complete
      state = 8;
      currentChapter++;
      decision = 3;
    }
    break;
  }

  case 11: {
    // Story complete — reset for next play
    resetHardwareState();
    state = 3;
    break;
  }

  default:
    break;
  }
}
