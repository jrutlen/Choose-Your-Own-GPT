//Wifi
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NetWizard.h>
//Hardware
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_Thermal.h"
//OpenAI API connection
#include <ChatGPTuino.h>
#include "credentials.h"
const int TOKENS = 750; // How lengthy a response you want, every token is about 3/4 a word
const int NUM_MESSAGES = 14; 
ChatGPTuino chat{ TOKENS, NUM_MESSAGES }; // Will store and send your most recent messages (up to NUM_MESSAGES)
// OpenAI Model being used - gpt-4o is the current recommended model
// Alternatives: "gpt-4o-mini" (faster, cheaper), "gpt-4-turbo" (older but stable)
// See https://platform.openai.com/docs/models for current model availability
const char *model = "gpt-4o";
#define SERVER_RESPONSE_WAIT_TIME (30 * 1000) // Override ChatGPTuino default timeout of 15 seconds

//Netwizard
WebServer server(80);
NetWizard NW(&server);
// Setup configuration parameters
NetWizardParameter nw_header(&NW, NW_HEADER, "MQTT");
NetWizardParameter nw_divider1(&NW, NW_DIVIDER);
NetWizardParameter nw_mqtt_host(&NW, NW_INPUT, "Host", "", "mqtt.example.com");
NetWizardParameter nw_mqtt_port(&NW, NW_INPUT, "Port", "", "1883");

//Hardware
//LEDs
Adafruit_NeoPixel WaitEnd(13, 26, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel Go(3, 33, NEO_GRB + NEO_KHZ800);
//Thermal Printer
Adafruit_Thermal printer(&Serial2, 4);

//Small Dial Names
const char* names[] = {
  "Aunt Lily (Mia and Zoe's aunt, married to Uncle Ray)",  
  "Mia (a 7-year-old girl, 5-year-old Zoe's sister)",  
  "Zoe (a 5-year-old girl, 7-year-old Mia's sister)",  
  "Grandma June (Mia and Zoe's grandmother, Dad's mom, the villain in the story)",  
  "Buddy (Uncle Ray and Aunt Lily's dog with floppy ears)",  
  "Dad (Mia and Zoe's dad)",  
  "Mom (Mia and Zoe's mom, Aunt Lily's sister)",  
  "Nana (Mia and Zoe's grandmother, Aunt Lily and Mom's mom)",  
  "Papa Joe (Mia and Zoe's grandfather, Aunt Lily and Mom's dad)",  
  "Uncle Ray (Mia and Zoe's uncle, married to Aunt Lily)",  
};
//Large Dial Names
const char* adventureNames[] = {
  " who go on an adventure in the Amazon Rainforest.",
  " who go on an adventure to a Robotic World.",
  " who go on adventure exploring secret caves.",
  " who go to a Magical School.",
  " who go on an adventure in an underground city.",
  " who go on a Time Travel adventure in a Tardis.",
  " who go on a Medieval Fantasy adventure.",
  " who go on a Arctic Expedition adventure.",
  " who go on a Jungle Safari adventure.",
  " who go on an underwater adventure.",
  " who go on a Prehistoric Adventure.",
  " who go on an adventure in the Enchanted Forest.",
  " who go on a Pirate Adventure. Please tell the entire story in pirate speak.",
  " who go on adventure in outer space. Make sure there are lasers and aliens.",
  " who go on adventure in the African Savanna.",
  " who get recruited to join the Superhero Academy."
};


const char* prompt = "Please write a simple outline for a 5 chapter choose your own adventure story. The first act should set up the story and provide a pivotal decision at the end that will completely change the course of the story. The second act should present a challenge and incorporate a choice that will come back in the 5th and final chapter. The third act should provide a false victory or twist. The fourth act should be the final push. The 5th and final act is the climax and resolution. The story is about "; //initial prompt added before the names and adventure
const char* instructions = "Once completed, please respond 'COMPLETED'. When I send the word 'Begin', respond with the first chapter. Each chapter should be approximately 180 words. End each chapter with exactly two options, 'yellow' or 'blue' to continue the story in the format 'to do x, press the yellow button' or 'to do y, press the blue button'. At the end of each chapter you must present these two options. Don't include any chapter titles or numbers and use only basic punctuation like single quotes, commas, periods, new line, and exclamation points. Do not use bold, italics, or any text formatting."; //instructions for gpt, added after the names
const char* surpriseEnding = "Write a surprise ending to the story that is different than either the blue or the yellow options."; //ending for the surprise ending
const char* blueContinue = "Press the blue button.";
const char* blueComplete = "Write the final chapter to the story. Push the blue button.";
const char* yellowContinue = "Press the yellow button.";
const char* yellowComplete = "Write the final chapter to the story. Push the yellow button.";

//LEDs
#define waitStart 0
#define waitLength 9
#define endStart 9
#define endLength 4

//Button LEDs
#define btnLEDrd 27
#define btnLEDbl 12
#define btnLEDgn 2
#define btnLEDyl 15

//Dial LEDs
#define dialLED1 21
#define dialLED2 13
#define dialLED3 5

//Input Matrix
//Columns
#define scan1 25 //dial 3
#define scan2 36 //dial 2
#define scan4 35 //dial 1 a
#define scan8 39 //dial 1 b

//Rows
#define comD0 22
#define comD1 18  
#define comD2 19
#define comBtn 23
#define comMag 32

int scanDelay = 100; //delay between scans
unsigned long lastScan = 0; //last time the matrix was scanned

//Arrays to store matrix states
int dial[3] = {0,0,0};
int lastDial[3] = {0,0,0};
bool dialSet[3] = {false,false,false};
bool button[4] = {false,false,false,false};
bool ring[4] = {false,false,false,false};

/* keep track of state
0 = boot
1 = connecting to wifi
2 = connected to wifi
3 = configuring
4 = ready/waiting for input
5 = story requested (1st chapter)
6 = story generating
7 = story generated
8 = story requested (n chapter)
9 = story printed, waiting for decision
10 = surprise ending requested
11 = story complete
*/
int state = 0;
int currentChapter = 1;
int maxChapter = 5;
int decision = 0; //0 = no decision, 1 = red, 2 = blue, 3 = yellow, 4 = green

int fade = 0; //global fade value
int slowFade = 0; //global fade value
int fadeSpeed = 1; //fade speed
int slowFadeSpeed = 10; //fade speed
unsigned long lastFade = 0; //last time the fade was updated
unsigned long lastSlowFade = 0; //last time the fade was updated
int direction = 1; //fade direction
int slowDirection = 1;
const int buttonFreq = 5000;
const int dialFreq = 5000;
const int ledChannelDials = 0;
const int ledChannelBtn = 1;
const int resolution = 8;

void setup() {
  state = 1; //connecting to wifi
  WaitEnd.clear();
  WaitEnd.fill(WaitEnd.Color(0,0,255), waitStart, waitLength);
  WaitEnd.show();
  
  Serial.begin(115200);
  delay(1000);

  NW.setStrategy(NetWizardStrategy::BLOCKING);

  // Listen for connection status changes
  NW.onConnectionStatus([](NetWizardConnectionStatus status) {
    String status_str = "";

    switch (status) {
      case NetWizardConnectionStatus::DISCONNECTED:
        status_str = "Disconnected";
        break;
      case NetWizardConnectionStatus::CONNECTING:
        status_str = "Connecting";
        break;
      case NetWizardConnectionStatus::CONNECTED:
        status_str = "Connected";
        break;
      case NetWizardConnectionStatus::CONNECTION_FAILED:
        status_str = "Connection Failed";
        break;
      case NetWizardConnectionStatus::CONNECTION_LOST:
        status_str = "Connection Lost";
        break;
      case NetWizardConnectionStatus::NOT_FOUND:
        status_str = "Not Found";
        break;
      default:
        status_str = "Unknown";
    }

    Serial.printf("NW connection status changed: %s\n", status_str.c_str());
    if (status == NetWizardConnectionStatus::CONNECTED) {
      // Local IP
      Serial.printf("Local IP: %s\n", NW.localIP().toString().c_str());
      // Gateway IP
      Serial.printf("Gateway IP: %s\n", NW.gatewayIP().toString().c_str());
      // Subnet mask
      Serial.printf("Subnet mask: %s\n", NW.subnetMask().toString().c_str());
      //Start Chat
      chat.init(key, model);
    }
  });

  // Listen for portal state changes
  NW.onPortalState([](NetWizardPortalState state) {
    String state_str = "";

    switch (state) {
      case NetWizardPortalState::IDLE:
        state_str = "Idle";
        break;
      case NetWizardPortalState::CONNECTING_WIFI:
        state_str = "Connecting to WiFi";
        break;
      case NetWizardPortalState::WAITING_FOR_CONNECTION:
        state_str = "Waiting for Connection";
        break;
      case NetWizardPortalState::SUCCESS:
        state_str = "Success";
        break;
      case NetWizardPortalState::FAILED:
        state_str = "Failed";
        break;
      case NetWizardPortalState::TIMEOUT:
        state_str = "Timeout";
        break;
      default:
        state_str = "Unknown";
    }

    Serial.printf("NW portal state changed: %s\n", state_str.c_str());
  });

  NW.onConfig([&]() {
    Serial.println("NW onConfig Received");

    // Print new parameter values
    Serial.printf("Host: %s\n", nw_mqtt_host.getValueStr().c_str());
    Serial.printf("Port: %s\n", nw_mqtt_port.getValueStr().c_str());
    return true; // <-- return true to approve request, false to reject
  });

  // Start NetWizard
  NW.autoConnect("ChooseYourOwnGPT", "itMightBeMagic");
  
  // Check if configured
  if (NW.isConfigured()) {
    Serial.println("Device is configured");
  } else {
    Serial.println("Device is not configured!");
  }

  // Demo Route
  server.on("/demo", HTTP_GET, []() {
    server.send(200, "text/plain", "Hi! This is NetWizard Demo.");
  });

  // Start WebServer
  server.begin();

  // Configure pins
  //button LEDs
  //ledcDetach(btnLEDrd);
  //ledcDetach(btnLEDbl);
  //ledcDetach(btnLEDgn);
  //ledcDetach(btnLEDyl);
  pinMode(btnLEDrd, OUTPUT);
  pinMode(btnLEDbl, OUTPUT);
  pinMode(btnLEDgn, OUTPUT);
  pinMode(btnLEDyl, OUTPUT);
  digitalWrite(btnLEDrd, LOW);
  digitalWrite(btnLEDbl, LOW);
  digitalWrite(btnLEDgn, LOW);
  digitalWrite(btnLEDyl, LOW);
  //matrix inputs
  pinMode(scan1, INPUT);
  pinMode(scan2, INPUT);
  pinMode(scan4, INPUT);
  pinMode(scan8, INPUT);
  //matrix outputs
  pinMode(comD0, OUTPUT);
  pinMode(comD1, OUTPUT);
  pinMode(comD2, OUTPUT);
  pinMode(comBtn, OUTPUT);
  pinMode(comMag, OUTPUT);

  state = 2; //connected to wifi
  //turn off all marquees
  Go.clear();
  WaitEnd.clear();
  Go.fill(Go.Color(0,0,0),0,3);
  WaitEnd.fill(WaitEnd.Color(0,0,0), waitStart, waitLength);
  WaitEnd.fill(WaitEnd.Color(0,0,0), endStart, endLength);
  Go.show();
  WaitEnd.show();

  //Set up printer
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Initialize printer on hardware serial 2
  printer.begin(); 
  printer.reset(); 
  printer.wake();       // Wake printer up
  // printer.setSize('M');
  // printer.justify('C');
  // printer.boldOn();
  // printer.inverseOn();
  // printer.println("-Choose-Your-Own-GPT-");
  // printer.boldOff();
  // printer.inverseOff();
  // printer.justify('L');
  printer.setSize('S');        // Set type size, accepts 'S', 'M', 'L'
  // printer.feed(5);
  printer.setDefault();
  //printer.sleep();      // Tell printer to sleep

  //configure PWM outputs for LEDS
  ledcAttachChannel(dialLED1, dialFreq, resolution, ledChannelDials);
  ledcAttachChannel(dialLED2, dialFreq, resolution, ledChannelDials);
  ledcAttachChannel(dialLED3, dialFreq, resolution, ledChannelDials);

  state = 3; //ready for input
}

void printTitle(int chapterNumber) {
  if (!printer.wake()) {
    Serial.println("Warning: Printer wake failed");
  }
  printer.setSize('L');
  //center text 
  printer.justify('C');
  printer.boldOn();
  printer.println("Chapter " + String(chapterNumber));
  printer.boldOff();
  //reset text justification
  printer.justify('L');
  printer.setSize('S');
  printer.feed(2);
}

void sendToPrint(const char* message) {
  if (message == NULL) {
    Serial.println("Error: Null message passed to sendToPrint");
    return;
  }
  if (!printer.wake()) {
    Serial.println("Warning: Printer wake failed");
  }
  printer.setSize('S'); 
  printer.println(message);
  printer.feed(3);
  // printer.sleep();      // Tell printer to sleep
}

int checkDial(int dialNumber) { //binary dip switch
  // set the row common pins based on dialNumber
  int dialValue = 0;
  switch (dialNumber) {
    case 0:
      digitalWrite(comD0, HIGH);
      digitalWrite(comD1, LOW);
      digitalWrite(comD2, LOW);
      break;
    case 1:
      digitalWrite(comD0, LOW);
      digitalWrite(comD1, HIGH);
      digitalWrite(comD2, LOW);
      break;
    case 2:
      digitalWrite(comD0, LOW);
      digitalWrite(comD1, LOW);
      digitalWrite(comD2, HIGH);
      break;
    default:
      // do nothing if dialNumber is out of range
      break;
  }
  digitalWrite(comBtn, LOW);
  digitalWrite(comMag, LOW);
  //check each column
  if(digitalRead(scan1) == HIGH) {
    dialValue += 1;
  }
  if(digitalRead(scan2) == HIGH) {
    dialValue += 2;
  }
  if(digitalRead(scan4) == HIGH) {
    dialValue += 4;
  }
  if(digitalRead(scan8) == HIGH) {
    dialValue += 8;
  }
  //dial[dialNumber] = dialValue;
  digitalWrite(comD0, LOW); //turn off the dial 1 row common
  digitalWrite(comD1, LOW); //turn off the dial 2 row common
  digitalWrite(comD2, LOW); //turn off the dial 3 row common
  return dialValue;
}


void checkButton() {
  digitalWrite(comBtn, HIGH); //turn on the button row common
  digitalWrite(comD0, LOW); //turn off the other commons
  digitalWrite(comD1, LOW);
  digitalWrite(comD2, LOW);
  digitalWrite(comMag, LOW);

  //check each column
  if(digitalRead(scan1) == HIGH) {
    button[0] = true;
  }
  else {
    button[0] = false;
  }
  if(digitalRead(scan2) == HIGH) {
    button[1] = true;
  }
  else {
    button[1] = false;
  }
  if(digitalRead(scan8) == HIGH) {
    button[2] = true;
  }
  else {
    button[2] = false;
  }
  if(digitalRead(scan4) == HIGH) {
    button[3] = true;
  }
  else {
    button[3] = false;
  }
  digitalWrite(comBtn, LOW); //turn off the button row common
}

void checkRing() {
  digitalWrite(comMag, HIGH); //turn on the ring row common
  digitalWrite(comD0, LOW); //turn off the other commons
  digitalWrite(comD1, LOW);
  digitalWrite(comD2, LOW);
  digitalWrite(comBtn, LOW);

  //check each column
  if(digitalRead(scan1) == HIGH) {
    ring[3] = true;
  }
  else {
    ring[3] = false;
  }
  if(digitalRead(scan2) == HIGH) {
    ring[2] = true;
  }
  else {
    ring[2] = false;
  }
  if(digitalRead(scan4) == HIGH) {
    ring[0] = true;
  }
  else {
    ring[0] = false;
  }
  if(digitalRead(scan8) == HIGH) {
    ring[1] = true;
  }
  else {
    ring[1] = false;
  }
  digitalWrite(comMag, LOW); //turn off the ring row common
}

void fadeCalc() {
  if (millis() - lastFade > fadeSpeed) {
      lastFade = millis();
    if (fade < 255 && direction == 1) {
      fade += 1;
    } else if (fade > 75 && direction == 0) {
      fade -= 1;
    } else if (fade == 75) {
      direction = 1;
      fade = 76;
    } else if (fade == 255) {
      direction = 0;
      fade = 254;
    }  
    ledcWriteChannel(ledChannelDials, fade); 
  }
}

void slowFadeCalc() {
  const int maxFadeValue = 100;
  if (millis() - lastSlowFade > slowFadeSpeed) {
      lastSlowFade = millis();
    if (slowFade < maxFadeValue && slowDirection == 1) {
      slowFade += 1;
    } else if (slowFade > 0 && slowDirection == 0) {
      slowFade -= 1;
    } else if (slowFade == 0) {
      slowDirection = 1;
    } else if (slowFade == maxFadeValue) {
      slowDirection = 0;
    }  
    ledcWriteChannel(ledChannelBtn, slowFade);
  }
}

void loop() {
  //Serial.print("top of loop");
  // Handle WebServer
  server.handleClient();
  // NetWizard Loop Task
  NW.loop();
  //States
  if (state == 3) {//initial configuration
  lastDial[0] = checkDial(0); //update last dial position to current position
  delay(10);
  lastDial[1] = checkDial(1);
  delay(10);
  lastDial[2] = checkDial(2);
  state = 4;
  } else if (state == 4) { //ready for input
    fadeCalc(); //update global fade value
    //every scanDelay update one dial
    if (millis() - lastScan > scanDelay) {
      lastScan = millis();
      dial[0] = checkDial(0);
      delay(10);
      dial[1] = checkDial(1);
      delay(10);
      dial[2] = checkDial(2);
      //checkRing();
    }
    //check if dial has changed
    if (dial[0] != lastDial[0]) {
      dialSet[0] = true;
    }
    if (dial[1] != lastDial[1]) {
      dialSet[1] = true;
    }
    if (dial[2] != lastDial[2]) {
      dialSet[2] = true;
    }

    if (dialSet[0] == false) {
      //do nothing
    } else {
      pinMode(dialLED1, OUTPUT);
      digitalWrite(dialLED1, HIGH);
    }
    if (dialSet[1] == false) {
      //do nothing
    } else {
      pinMode(dialLED2, OUTPUT);
      digitalWrite(dialLED2, HIGH);
    } 
    if (dialSet[2] == false) {
      //do nothing
    } else {
      pinMode(dialLED3, OUTPUT);
      digitalWrite(dialLED3, HIGH);
    }
    if (dialSet[0] == true && dialSet[1] == true && dialSet[2] == true) { //if all 3 dials have been set 
      ledcAttachChannel(btnLEDgn, dialFreq, resolution, ledChannelDials); //fast fade go button
      Go.clear();
      Go.fill(Go.Color(0,255,0),0,3);
      Go.setBrightness(255);
      Go.show();
      checkButton();
      while (button[2] == false) {
        checkButton();
        fadeCalc();
      }
      state = 5; //story requested
    } else {
      digitalWrite(btnLEDgn, LOW);
      digitalWrite(btnLEDyl, LOW);
      digitalWrite(btnLEDrd, LOW);
      digitalWrite(btnLEDbl, LOW);
    }
  } else if (state == 5) { //story requested
    Go.clear();
    Go.show();
    WaitEnd.clear();
    WaitEnd.fill(WaitEnd.Color(0,255,0), waitStart, waitLength);
    WaitEnd.show();
    ledcDetach(btnLEDgn); //turn off green button LED
    pinMode(btnLEDgn, OUTPUT);
    digitalWrite(btnLEDgn, LOW);
    digitalWrite(dialLED1, LOW);
    digitalWrite(dialLED2, LOW);
    digitalWrite(dialLED3, LOW);
    state = 6; //story generating
  } else if (state == 6) { //story generating
    // Validate dial indices before accessing arrays
    if (dial[0] < 0 || dial[0] >= 10 || dial[1] < 0 || dial[1] >= 10 || dial[2] < 0 || dial[2] >= 16) {
      Serial.println("Error: Invalid dial position");
      Serial.printf("Dial values: %d, %d, %d\n", dial[0], dial[1], dial[2]);
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      state = 4; // Return to ready state
      return;
    }
    const char* selectedAdventure =  adventureNames[dial[2]];
    const char* selectedName = names[dial[0]];
    const char* selectedName2 = names[dial[1]];
    const char* separator = " and ";
    //check if the names are the same
    if (strcmp(selectedName, selectedName2) == 0) {
      selectedName2 = "their evil twin";
    }
    //construct the prompt with the separator between the names
    size_t promptLength = 1 + strlen(prompt) + strlen(selectedName) + strlen(separator) + strlen(selectedName2) + strlen(selectedAdventure) + strlen(instructions);
    char *initialPrompt = (char*) malloc(promptLength);
    if (initialPrompt == NULL) {
      Serial.println("Error: Memory allocation failed for initial prompt");
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      state = 4; // Return to ready state
      return;
    }
    strcpy(initialPrompt, prompt);
    strcat(initialPrompt, selectedName);
    strcat(initialPrompt, separator);
    strcat(initialPrompt, selectedName2);
    strcat(initialPrompt, selectedAdventure);
    strcat(initialPrompt, instructions); //add instructions to the prompt

    // Check WiFi connection before making API call
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Error: WiFi not connected");
      free(initialPrompt);
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      state = 4; // Return to ready state
      return;
    }

    chat.putMessage(initialPrompt, strlen(initialPrompt)); //request story outline
    printTitle(currentChapter);
    
    // Get outline with error handling
    int outlineResult = chat.getResponse();
    if (outlineResult != 0) {
      Serial.println("Error: Failed to get story outline from API");
      Serial.printf("Error code: %d\n", outlineResult);
      free(initialPrompt);
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      state = 4; // Return to ready state
      return;
    }
    Serial.println("Story outline received successfully");
    
    const char* startStory = "Begin";
    chat.putMessage(startStory, strlen(startStory)); //request first chapter of story
    
    // Get first chapter with error handling
    int chapterResult = chat.getResponse();
    if (chapterResult != 0) {
      Serial.println("Error: Failed to get first chapter from API");
      Serial.printf("Error code: %d\n", chapterResult);
      free(initialPrompt);
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      state = 4; // Return to ready state
      return;
    }
    Serial.println("First chapter received successfully");
    
    const char* content = chat.getLastMessageContent();
    if (content == NULL || strlen(content) == 0) {
      Serial.println("Error: Empty response from API");
      free(initialPrompt);
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      state = 4; // Return to ready state
      return;
    }
    
    sendToPrint(content);
    free(initialPrompt); // Clean up allocated memory
    ledcAttachChannel(btnLEDyl, dialFreq, resolution, ledChannelDials);
    ledcAttachChannel(btnLEDbl, dialFreq, resolution, ledChannelDials); 
    ledcAttachChannel(btnLEDrd, buttonFreq, resolution, ledChannelBtn);
    state = 9; //story generated
  } else if (state == 8) { //nth chapter
    Go.clear();
    Go.show();
    WaitEnd.clear();
    WaitEnd.fill(WaitEnd.Color(0,255,0), waitStart, waitLength);
    WaitEnd.show();
    ledcDetach(btnLEDgn); //turn off green button LED
    pinMode(btnLEDgn, OUTPUT);
    digitalWrite(btnLEDgn, LOW);
    digitalWrite(dialLED1, LOW);
    digitalWrite(dialLED2, LOW);
    digitalWrite(dialLED3, LOW);

    // Check WiFi connection before making API call
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Error: WiFi not connected");
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      currentChapter--; // Decrement chapter since we didn't complete it
      state = 9; // Return to waiting for decision
      return;
    }

    if (decision == 1) { //surprise ending requested (red button pressed)
      chat.putMessage(surpriseEnding, strlen(surpriseEnding));
    } else if (decision == 2) { //blue button pressed
      if (currentChapter < maxChapter) {
      //set follow up prompt to the blue continue prompt
      chat.putMessage(blueContinue, strlen(blueContinue));
      } else { //if the max chapter has been reached add complete prompt
      chat.putMessage(blueComplete, strlen(blueComplete));
      }
    } else if (decision == 3) { //yellow button pressed
      if (currentChapter < maxChapter) {
      //set follow up prompt to the yellow continue prompt
      chat.putMessage(yellowContinue, strlen(yellowContinue));
      } else { //if the max chapter has been reached add complete prompt
      chat.putMessage(yellowComplete, strlen(yellowComplete));
      }
    }
    Serial.println("Requesting Chapter: " + String(currentChapter));
    printTitle(currentChapter);
    
    // Get chapter with error handling
    int chapterResult = chat.getResponse();
    if (chapterResult != 0) {
      Serial.println("Error: Failed to get chapter from API");
      Serial.printf("Error code: %d\n", chapterResult);
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      currentChapter--; // Decrement chapter since we didn't complete it
      state = 9; // Return to waiting for decision to retry
      return;
    }
    Serial.println("Chapter received successfully");
    
    const char* content = chat.getLastMessageContent();
    if (content == NULL || strlen(content) == 0) {
      Serial.println("Error: Empty response from API");
      // Signal error with red LED
      WaitEnd.clear();
      WaitEnd.fill(WaitEnd.Color(255,0,0), waitStart, waitLength);
      WaitEnd.show();
      delay(3000);
      currentChapter--; // Decrement chapter since we didn't complete it
      state = 9; // Return to waiting for decision to retry
      return;
    }
    
    printer.println(content);
    printer.feed(3);
    //printer.sleep();      // Tell printer to sleep
    
    //wait for input or end of story
    if (currentChapter == maxChapter) {
      state = 11; //story complete
    } else {
      ledcAttachChannel(btnLEDyl, dialFreq, resolution, ledChannelDials);
      ledcAttachChannel(btnLEDbl, dialFreq, resolution, ledChannelDials); 
      ledcAttachChannel(btnLEDrd, buttonFreq, resolution, ledChannelBtn);
      state = 9; //story generated
    }
  } else if (state == 9) { //story printed, waiting for decision
    fadeCalc();
    slowFadeCalc();
    Go.clear();
    WaitEnd.clear();
    //WaitEnd.fill(WaitEnd.Color(0,255,0), waitStart, waitLength);
    WaitEnd.fill(WaitEnd.Color(slowFade,0,0), endStart, endLength);
    WaitEnd.show();
    Go.show();
    checkButton();
    if (button[1] == true) { //if the red button is pressed
      state = 8; //surprise ending requested
      decision = 1;
    } else if (button[0] == true) { //if the blue button is pressed
      state = 8; //story requested
      currentChapter++;
      decision = 2;
    } else if (button[3] == true) { //if the yellow button is pressed
      state = 8; //story requested
      currentChapter++;
      decision = 3;
    }
  } else if (state == 11) { //story complete
    Go.clear();
    Go.show();
    WaitEnd.clear();
    WaitEnd.show();
    //reset buttons
    digitalWrite(btnLEDgn, LOW);
    digitalWrite(btnLEDyl, LOW);
    digitalWrite(btnLEDrd, LOW);
    digitalWrite(btnLEDbl, LOW);
    //reset dials
    digitalWrite(dialLED1, LOW);
    digitalWrite(dialLED2, LOW);
    digitalWrite(dialLED3, LOW);
    dialSet[0] = false;
    dialSet[1] = false;
    dialSet[2] = false;
    ledcAttachChannel(dialLED1, dialFreq, resolution, ledChannelDials);
    ledcAttachChannel(dialLED2, dialFreq, resolution, ledChannelDials);
    ledcAttachChannel(dialLED3, dialFreq, resolution, ledChannelDials);
    //reset chapters
    currentChapter = 1;
    //clear chat messages

    state = 3; //ready for input
  }
}

