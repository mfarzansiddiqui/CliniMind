#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

//=====================DHT22===================
#define DHTPIN 32          // GPIO where DHT22 is connected
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
// ================= MAX30102 =================
MAX30105 particleSensor;

#define VITALS_BUFFER_SIZE 100
uint32_t irBuffer[VITALS_BUFFER_SIZE];
uint32_t redBuffer[VITALS_BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

unsigned long lastVitalsUpdate = 0;
const unsigned long vitalsInterval = 500; // ms


// ================= OLED =================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106G display(128, 64, &Wire, -1);
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= BUTTON PINS =================
#define BTN_UP     12
#define BTN_DOWN   13
#define BTN_BACK   14
#define BTN_SELECT 26
#define BTN_DONE 27
#define BTN_EMERGENCY 25
#define BUZZER 16

void sendTemp();
void sendEmergencyAlert();
void showVitalsPage();
void checkAppointmentAPI();
void moveDown();
void moveUp();
void drawMenu();
void drawAppointmentResult();
void drawAppointmentMenu();
void drawAboutPage();
void moveCursor(int dir);
void selectAnswer();
void doneButtonPressed();
String urlencode(String str);
void sendWhatsApp(String recipient, String key, String message);
void drawEmergencyPage();
void showVitalsPage();
float calculateSpO2(uint32_t red[], uint32_t ir[], int size);

//for sending whatsapp
unsigned long pressStartTime = 0;
bool buttonPressed = false;
bool actionExecuted = false;
const char* recipient = "+923199266432"; // recipient phone number
const char* doctor = "+923333118616";

String apiKey = "oDRyt28bZkzR";
// Base URL of your WhatsApp API
String baseURL = "https://api.textmebot.com/send.php";



bool inRiskEvaluator = false;
void runRiskEvaluator();
void displayQuestion();
void computeRisk();
// ================= WIFI =================
const char* ssid = "Lower Classes 2.4G";
const char* password = "lclass@2025";

// ================= API =================
const char* emergencyApiUrl = "https://clinimind.vercel.app/api/emergency";
const char* envoirmentApiUrl   = "https://clinimind.vercel.app/api/dht";
const char* checkApiUrl   = "https://clinimind.vercel.app/api/destination?key=123";
const char* requestApiUrl = "https://clinimind.vercel.app/api/appointment";
const char* vitalsApiUrl = "https://clinimind.vercel.app/api/vitals";

// ================= DATA =================
String patientName = "Farzan";
String doctorName = "Talha Shams";
String id = "123";

String patientAddress = "House-123, Street A, Kohat, Pakistan.";

unsigned long lastApiCheck = 0;
const unsigned long apiInterval = 60000; // 30 seconds

String hasAppointment = "";
String appointmentDateTime = "";

// ================= MENU DATA =================
const char* mainMenu[] = {
  "Risk Evaluator",
  "Emergency Alert",
  "Appointment",
  "Measure Vitals",
  "About",
  "Reset"
};

const char* appointmentMenu[] = {
  "Check Status",
  "Request New"
};

int menuSize = 6;
int selectedIndex = 0;
int scrollOffset = 0;

bool inPage = false;
bool inAppointmentMenu = false;
bool inResetConfirmation = false;
int appointmentIndex = 0;

// =================================================

void showCliniMindStartup() {
  // Clear display
  display.clearDisplay();
  digitalWrite(BUZZER, HIGH);
  delay(850);
  digitalWrite(BUZZER, LOW);
  // 1. Draw decorative border with animation
  for(int i = 0; i < 32; i++) {
    display.drawPixel(i*4, 0, SH110X_WHITE);
    display.drawPixel(i*4, 63, SH110X_WHITE);
    display.drawPixel(0, i*2, SH110X_WHITE);
    display.drawPixel(127, i*2, SH110X_WHITE);
    display.display();
    
    delay(20);
  }
  
  delay(200);
  
  // 2. Draw medical cross with transformation
  display.clearDisplay();
  
  // Draw medical cross
  display.fillRect(52, 10, 20, 20, SH110X_WHITE);
  display.fillRect(57, 5, 10, 30, SH110X_BLACK);
  display.fillRect(47, 15, 30, 10, SH110X_BLACK);
  
  display.display();
  delay(500);
  
  // Transform cross into brain/heart shape
  for(int i = 0; i < 10; i++) {
    display.drawCircle(64, 20, 10 + i, SH110X_WHITE);
    display.display();
    delay(50);
  }
  
  delay(300);
  
  // 3. Display CliniMind with style
  display.clearDisplay();
  
  // Draw decorative top border
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.drawLine(10, 15, 117, 15, SH110X_WHITE);
  
  // "CLINI" in normal text
  display.setTextSize(2);
  display.setCursor(10, 18);
  display.print("CLINI");
  
  // "MIND" with filled background (inverse video)
  display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(70, 18);
  display.print("MIND");
  
  // Back to normal text color
  display.setTextColor(SH110X_WHITE);
  
  // Tagline
  display.setTextSize(1);
  display.setCursor(4, 42);
  display.print("Your Health Guardian");
  display.setCursor(30, 52);
  display.print("System v2.0");
  
  display.display();
  
  // 4. Sparkle effect
  for(int sparkle = 0; sparkle < 5; sparkle++) {
    int x = random(20, 108);
    int y = random(30, 60);
    
    // Draw 3x3 sparkle
    for(int dx = -1; dx <= 1; dx++) {
      for(int dy = -1; dy <= 1; dy++) {
        if(abs(dx) + abs(dy) <= 1) { // Diamond shape
          display.drawPixel(x + dx, y + dy, SH110X_WHITE);
        }
      }
    }
    
    display.display();
    delay(150);
    
    // Remove sparkle
    for(int dx = -1; dx <= 1; dx++) {
      for(int dy = -1; dy <= 1; dy++) {
        if(abs(dx) + abs(dy) <= 1) {
          display.drawPixel(x + dx, y + dy, SH110X_BLACK);
        }
      }
    }
  }
  
  delay(1500);
  
  // 5. Fade to main menu
  for(int i = 0; i < 64; i += 2) {
    display.drawLine(0, i, 127, i, SH110X_BLACK);
    display.display();
    delay(10);
  }
  
  delay(300);
}

// =================================================
// ================= SIMPLE RESET FUNCTION =========
// =================================================

void handleReset() {
  // Show confirmation screen
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(15, 10);
  display.println("RESET CONFIRMATION");
  display.drawLine(10, 20, 117, 20, SH110X_WHITE);
  display.setCursor(10, 30);
  display.println("DONE = Yes");
  display.setCursor(10, 40);
  display.println("BACK = Cancel");
  display.setCursor(20, 50);
  display.println("10 sec timeout");
  display.display();
  
  inResetConfirmation = true;
  
  // Wait for user decision with timeout
  unsigned long startTime = millis();
  bool decisionMade = false;
  
  while (!decisionMade && (millis() - startTime < 10000)) {
    if (digitalRead(BTN_DONE) == LOW) {
      // User confirmed reset
      display.clearDisplay();
      display.drawRect(0, 0, 127, 63, SH110X_WHITE);
      display.setCursor(30, 25);
      display.println("Resetting...");
      display.display();
      delay(1000);
      ESP.restart(); // Reset ESP32
    }
    
    if (digitalRead(BTN_BACK) == LOW) {
      // User cancelled
      decisionMade = true;
      delay(200); // Debounce
    }
    
    // Optional: Show countdown
    if (millis() - startTime > 8000) {
      display.fillRect(50, 50, 30, 10, SH110X_BLACK);
      display.setCursor(55, 50);
      display.print("2");
      display.display();
    } else if (millis() - startTime > 9000) {
      display.fillRect(50, 50, 30, 10, SH110X_BLACK);
      display.setCursor(55, 50);
      display.print("1");
      display.display();
    }
    
    delay(50); // Small delay to prevent CPU hogging
  }
  
  // Timeout or cancelled - return to menu
  inResetConfirmation = false;
  inPage = false;
  Serial.println("[SYSTEM] Reset cancelled or timeout");
}

// =================================================
// ================= WIFI CONNECTION ===============
// =================================================

void connectWiFi() {
  // Show WiFi connecting message on OLED
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(19, 10);
  display.println("WiFi Connecting");
  display.setCursor(49, 30);
  display.println(".....");
  display.display();
  
  Serial.print("[WiFi] Connecting");
  WiFi.begin(ssid, password);

  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Update dots animation on OLED
    display.fillRect(45, 30, 40, 10, SH110X_BLACK); // Clear dots area
    display.setCursor(45, 30);
    for (int i = 0; i <= dots; i++) {
      display.print(".");
    }
    display.display();
    
    dots = (dots + 1) % 5; // Cycle through 0-4 dots
  }

  // Show connected message
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(19, 20);
  display.println("WiFi Connected!");
  display.setCursor(14, 40);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(800); // Show connected message for 1.5 seconds
  
  
  Serial.println("\n[WiFi] Connected");
}

void setup() {
  Serial.begin(115200);
  Serial.println("CliniMind System starting...");
  dht.begin();  

  // Initialize button pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_DONE, INPUT_PULLUP);
  pinMode(BTN_EMERGENCY, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  // Initialize OLED display
  if(!display.begin(0x3C, true)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(100);
  
  // Set text properties
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  
  // Show startup animation
  showCliniMindStartup();
  
  // Clear display for main interface
  display.clearDisplay();
  display.display();
  // Connect to WiFi with OLED status
  
  // ===== MAX30102 INIT =====
if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
  Serial.println("[MAX30102] Not found");
} else {
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x2A);
  particleSensor.setPulseAmplitudeIR(0x2A);
  particleSensor.setPulseAmplitudeGreen(0);
  Serial.println("[MAX30102] Ready");
}

  connectWiFi();
  // Initial API check

  checkAppointmentAPI();
  sendTemp();
  
  Serial.println("CliniMind Ready!");


}

// =================================================

void loop() {
  // ===== BACKGROUND API CHECK =====
  if (millis() - lastApiCheck >= apiInterval) {
    lastApiCheck = millis();
    Serial.println("[API] Checking appointment...");
    checkAppointmentAPI();
    sendTemp();
  }

  // ===== EMERGENCY BUTTON LONG PRESS =====
  if (digitalRead(BTN_EMERGENCY) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      pressStartTime = millis();
      actionExecuted = false;
      digitalWrite(BUZZER, HIGH);
    }

    digitalWrite(BUZZER, LOW);

    if (!actionExecuted && millis() - pressStartTime >= 2500) {
      actionExecuted = true;

      Serial.println("Button held for 2.5 seconds!");
      Serial.println("🚨 Emergency Alert!");

      String msgd = "🚨 Extreme Medical Emergency Alert from " + patientName +
                   " at " + patientAddress;
      String msgr = "🚨 I have an Extreme Medical Emergency. Please check me out or send someone as soon as possible. -" + patientName;
      drawEmergencyPage();
      sendEmergencyAlert();
      sendWhatsApp(recipient, apiKey, msgr);
      sendWhatsApp(doctor, apiKey, msgd);
      
      display.setTextSize(1);
    }
  } else {
    // Reset emergency button state
    buttonPressed = false;
    actionExecuted = false;
  }

  // ===== MAIN MENU MODE =====
  if (!inPage && !inResetConfirmation) {
    // Navigate menu
    if (digitalRead(BTN_DOWN) == LOW) {
      moveDown();
      digitalWrite(BUZZER, HIGH);
      delay(200);
    }
    if (digitalRead(BTN_UP) == LOW) {
      moveUp();
      digitalWrite(BUZZER, HIGH);
      delay(200);
    }
    digitalWrite(BUZZER, LOW);

    // Select option
    if (digitalRead(BTN_DONE) == LOW) {
      Serial.print("[BTN] DONE -> ");
      Serial.println(mainMenu[selectedIndex]);

      if (selectedIndex == 5) {       // Reset
        handleReset();
      } else if (selectedIndex == 2) { // Appointment
        inPage = true;
        inAppointmentMenu = true;
        appointmentIndex = 0;
      } else {
        inPage = true;                // Enter page for other options
      }

      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
    }

    // Draw menu
    drawMenu();
  }

  // ===== PAGE MODE =====
  else if (inPage && !inResetConfirmation) {
    // ----- APPOINTMENT SUBMENU -----
    if (selectedIndex == 2 && inAppointmentMenu) {
      if (digitalRead(BTN_DOWN) == LOW) {
        appointmentIndex = min(appointmentIndex + 1, 1);
        Serial.print("[APPT] Index: "); Serial.println(appointmentIndex);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
      }
      if (digitalRead(BTN_UP) == LOW) {
        appointmentIndex = max(appointmentIndex - 1, 0);
        Serial.print("[APPT] Index: ");
        Serial.println(appointmentIndex);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
      }
      if (digitalRead(BTN_DONE) == LOW) {
        Serial.println("[APPT] DONE");
        drawAppointmentResult();
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        
      }
      if (digitalRead(BTN_BACK) == LOW) {
        Serial.println("[APPT] BACK");
        inAppointmentMenu = false;
        inPage = false;
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
      }
      drawAppointmentMenu();
      return;
    }

    // ----- BACK BUTTON EXIT FOR ANY PAGE -----
    if (digitalRead(BTN_BACK) == LOW) {
      inPage = false;
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      display.clearDisplay();
      drawMenu();
      return;
    }

    // ----- MEASURE VITALS PAGE -----
    if (selectedIndex == 3) {
      showVitalsPage();
      return;
    }

    // ----- RISK EVALUATOR -----
    if (selectedIndex == 0) {
      if (!inRiskEvaluator) {
        inRiskEvaluator = true;
        displayQuestion();
      }
      runRiskEvaluator();
      return;
    }

    // ----- EMERGENCY ALERT -----
    if (selectedIndex == 1) {
      drawEmergencyPage();
      String msgd = "🚨 Medical Emergency Alert from " + patientName + " at " + patientAddress;
      String msgr = "🚨 I have an Extreme Medical Emergency. Please check me out or send someone as soon as possible. -" + patientName;
      sendEmergencyAlert();
      sendWhatsApp(recipient, apiKey, msgr);
      sendWhatsApp(doctor, apiKey, msgd);
      return;
    }

    // ----- ABOUT PAGE -----
    if (selectedIndex == 4) {
      
      drawAboutPage();
      return;
    }
  }
}

// =================================================
// ================= API FUNCTIONS =================
// =================================================

void checkAppointmentAPI() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] WiFi not connected");
    return;
  }

  HTTPClient http;
  http.begin(checkApiUrl);

  int code = http.GET();
  Serial.print("[API] HTTP Code: ");
  Serial.println(code);

  if (code == 200) {
    String payload = http.getString();
    Serial.print("[API] Payload: ");
    Serial.println(payload);

    StaticJsonDocument<768> doc;
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      hasAppointment = doc["appointment_status"].as<String>();

      if (hasAppointment == "scheduled") {
        appointmentDateTime = doc["time"].as<String>();
        Serial.print("[API] Appointment: ");
        Serial.println(appointmentDateTime);
      } else {
        Serial.println("[API] No appointment");
      }
    }
  }

  http.end();
  return;
}

void requestAppointmentAPI() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(requestApiUrl);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> doc;
  doc["patient"] = patientName;
  doc["key"] = id;
  doc["doctor"] = doctorName;

  String body;
  serializeJson(doc, body);

  Serial.print("[API] Sending request: ");
  Serial.println(body);

  http.POST(body);
  http.end();
  String msg = "📅 I have requested an appointment from you. Kindly update the available slot in CliniMind Website. -" + patientName;
  sendWhatsApp(doctor, apiKey, msg);


}

// ========================================================================
// ============================= UI FUNCTIONS =============================
// ========================================================================

// =================================================
// ================= APPOINTMENT ===================
// =================================================


void drawAppointmentMenu() {
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(32, 4);
  display.println("APPOINTMENT");
  display.drawLine(6, 14, 120, 14, SH110X_WHITE);

  display.setCursor(0, 24);
  for (int i = 0; i < 2; i++) {
    display.print(i == appointmentIndex ? " >  " : "   ");
    display.println(appointmentMenu[i]);
    display.println("");
  }
  display.display();
}

void drawAppointmentResult() {
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(5, 10);

  if (appointmentIndex == 0){
  

    if (hasAppointment == "scheduled") {
      display.println("Appointment:");
      display.setCursor(12, 28);
      display.println(appointmentDateTime);
      display.display();
      delay(3000);
    } else {
      display.setCursor(18, 22);
      display.println("Not appointed");
      display.setCursor(32, 32);
      display.println("till now");
      display.display();
      delay(2000);
    
  } 
}
  else if (appointmentIndex == 1){
    display.println("Requesting...");
    display.display();
    requestAppointmentAPI();
    display.clearDisplay();
    display.drawRect(0, 0, 127, 63, SH110X_WHITE);
    display.setCursor(30, 20);
    display.println("Request Sent");
    display.display();
  }
  return;
}

  


// =================================================
// ================= ABOUT =========================
// =================================================

void drawAboutPage() {
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(49, 4);
  display.println("ABOUT");
  display.drawLine(6, 14, 120, 14, SH110X_WHITE);
  display.setCursor(27, 18);
display.println("DEVELOPED BY");
display.setCursor(27, 28);
display.println("MECHA CODERS");
display.setCursor(25, 41);
display.println("COPYRIGHT (C)");
display.setCursor(19, 51);
display.println("2025  CliniMind");


display.display();
}



void moveDown() {
  selectedIndex++;
  if (selectedIndex >= menuSize) selectedIndex = menuSize - 1;
  if (selectedIndex >= scrollOffset + 4) scrollOffset++;
}

void moveUp() {
  selectedIndex--;
  if (selectedIndex < 0) selectedIndex = 0;
  if (selectedIndex < scrollOffset) scrollOffset--;
}

void drawMenu() {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.setCursor(0, 5);

  for (int i = 0; i < 4; i++) {
    int idx = scrollOffset + i;
    if (idx >= menuSize) break;
    display.print(idx == selectedIndex ? " >  " : "   ");
    display.println(mainMenu[idx]);
    display.println("");
  }
  display.display();
}


// =================================================
// ================= RISK EVALUATOR =================
// =================================================

struct Question {
  String text;
  String answers[6];
  int points[6];
  int numOptions;
  bool multiSelect;
};

Question questions[10] = {
  {"Most worrying symptom?",
   {"Chest pain","Breathing difficulty","Fainting/Collapse","Severe pain","Fever/Illness","Other"},
   {15,15,15,10,6,4},6,false},

  {"How did this start?",
   {"Sudden","Gradual"}, {12,5}, 2,false},

  {"Severity at worst?",
   {"Mild","Moderate","Severe"}, {3,10,20},3,false},

  {"Symptom change?",
   {"Getting worse","Same","Improving"}, {12,6,0},3,false},

  {"Immediate danger signs (multi)",
   {"Severe chest pressure","Severe breathlessness","Fainting/confusion","Uncontrolled bleeding","Bluish lips/face","None"},
   {25,25,25,25,25,0},6,true},

  {"Associated symptoms (multi)",
   {"Sweating/nausea","Pain radiates","Fever/chills","Weakness/slurred speech"}, {8,10,8,15},4,true},

  {"Perform daily activities?",
   {"No","Yes"}, {10,0},2,false},

  {"Medical conditions (multi)",
   {"Heart disease/stroke","Diabetes","Asthma/COPD","Kidney disease","Pregnancy","None"}, {12,6,8,6,8,0},6,true},

  {"Medications (multi)",
   {"Blood thinners","Insulin/Glucose drugs","Steroids/Immunosuppressants","None"}, {10,8,8,0},4,true},

  {"Confidence in answers?",
   {"Very confident","Somewhat unsure","Not sure"}, {0,5,10},3,false}
};

int currentQuestion = 0;
int cursorIndex = 0;
int scrollIndex = 0;
bool selectedAnswers[6];
int totalScore = 0;
bool dangerFlag = false;

void displayQuestion();
void computeRisk();

void runRiskEvaluator() {
  if (digitalRead(BTN_UP) == LOW) { 
    moveCursor(-1); 
    delay(150); 
}
  if (digitalRead(BTN_DOWN) == LOW) {
    moveCursor(1); 
    delay(150); 
}
  if (digitalRead(BTN_SELECT) == LOW) { 
    selectAnswer(); 
    delay(150); 
}
  if (digitalRead(BTN_DONE) == LOW) { 
    doneButtonPressed();
    digitalWrite(BUZZER, HIGH); 
    delay(150); 
}
digitalWrite(BUZZER, LOW);

  if (digitalRead(BTN_BACK) == LOW) {
    currentQuestion = 0;
    totalScore = 0;
    dangerFlag = false;
    inRiskEvaluator = false;
    inPage = false;
    digitalWrite(BUZZER, HIGH);
    delay(200);
  }
digitalWrite(BUZZER, LOW);
}

void moveCursor(int dir) {
  Question q = questions[currentQuestion];
  cursorIndex += dir;
  if(cursorIndex < 0) cursorIndex = 0;
  if(cursorIndex >= q.numOptions) cursorIndex = q.numOptions - 1;

  if(cursorIndex < scrollIndex) scrollIndex = cursorIndex;
  if(cursorIndex >= scrollIndex + 3) scrollIndex = cursorIndex - 2;

  displayQuestion();
}

void selectAnswer() {
  Question q = questions[currentQuestion];

  if(q.multiSelect){
    if(cursorIndex == q.numOptions-1){
      for(int i=0;i<6;i++) selectedAnswers[i]=false;
      selectedAnswers[cursorIndex]=true;
    } else {
      selectedAnswers[cursorIndex]=!selectedAnswers[cursorIndex];
      selectedAnswers[q.numOptions-1]=false;
    }
  }
  displayQuestion();
}

void doneButtonPressed() {
  Question q = questions[currentQuestion];

  if(q.multiSelect){
    for(int i=0;i<q.numOptions;i++){
      if(selectedAnswers[i]) totalScore += q.points[i];
      if(currentQuestion==4 && i<5 && selectedAnswers[i]) dangerFlag=true;
    }
  } else {
    totalScore += q.points[cursorIndex];
    if(currentQuestion==4 && cursorIndex<5) dangerFlag=true;
  }

  currentQuestion++;
  cursorIndex = 0;
  scrollIndex = 0;
  for(int i=0;i<6;i++) selectedAnswers[i]=false;

  if(currentQuestion >= 10) {
    computeRisk();
    return;
  }

  displayQuestion();
}

void displayQuestion() {
  Question q = questions[currentQuestion];
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Q"+String(currentQuestion+1));
  display.println(q.text);
  display.println("---------------------");

  int maxDisplay = 3;
  for(int i=0;i<maxDisplay;i++){
    int optionIndex = scrollIndex + i;
    if(optionIndex >= q.numOptions) break;

    if(optionIndex==cursorIndex) display.print(">");
    else display.print(" ");
    display.print(q.answers[optionIndex]);
    if(q.multiSelect && selectedAnswers[optionIndex]) display.print(" [x]");
    display.println();
  }

  // Hint for buttons
  if(q.multiSelect) {
    display.println("---------------------");
    display.println("DONE=Next BACK=Prev");
}  
  else {
    display.println("--------------------");
    display.println("DONE=Cnfrm BACK=Prev");
}
  display.display();
}

void computeRisk() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("RESULT:");

  if(dangerFlag || totalScore >= 60){
    display.println("HIGH RISK");
  } else if(totalScore >= 30){
    display.println("MEDIUM RISK");
  } else {
    display.println("LOW RISK");
  }

  display.println("---------------------");
  display.print("Score: ");
  display.println(totalScore);
  

  if(dangerFlag || totalScore >= 60){
    display.println("You should immediately report to your DOCTOR :(");
  } else if(totalScore >= 30){
    display.println("It is good if you    checkout the DOCTOR  soon :|");
  } else {
    display.println("No need of visiting  the DOCTOR :)");
  }
 
  display.display();
  delay(3600);
  
  currentQuestion = 0;
  totalScore = 0;
  dangerFlag = false;
  inRiskEvaluator = false;
  inPage = false;
  
}

// ================= SEND WHATSAPP FUNCTION =================
void sendWhatsApp(String recipient, String key, String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ No WiFi connection.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // skip SSL verification for testing

  HTTPClient https;

  // POST data using 'recipient' parameter
  String postData = "recipient=" + String(recipient) +
                    "&apikey=" + key +
                    "&text=" + urlencode(message);

  Serial.print("POST data: ");
  Serial.println(postData);

  https.begin(client, baseURL);
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = https.POST(postData);
  String payload = https.getString();

  Serial.print("HTTP code: ");
  Serial.println(httpCode);
  Serial.print("Response: ");
  Serial.println(payload);

  https.end();
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(39, 4);
  display.setTextSize(1);
  display.println("WHATSAPP");
  display.drawLine(6, 14, 120, 14, SH110X_WHITE);
  display.setCursor(27, 30);
    display.println("Message Sent");

display.display();
delay(500);
display.clearDisplay();
inPage = false;
digitalWrite(BUZZER, HIGH);
delay(200);
digitalWrite(BUZZER, LOW);
}

// ================= URL ENCODE FUNCTION =================
String urlencode(String str) {
  String encoded = "";
  char c;
  char code0, code1;

  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += (char)(code0 > 9 ? code0 + 'A' - 10 : code0 + '0');
      encoded += (char)(code1 > 9 ? code1 + 'A' - 10 : code1 + '0');
    }
  }
  return encoded;
}

void drawEmergencyPage(){
    display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(18, 4);
  display.setTextSize(1);
  display.println("EMERGENCY ALERT");
  display.drawLine(6, 14, 120, 14, SH110X_WHITE);
  display.setCursor(7, 18);
    display.println("Sending in:");
display.setCursor(59, 35);
display.setTextSize(2);
display.println("3");
display.display();
digitalWrite(BUZZER, HIGH);
delay(50);
digitalWrite(BUZZER, LOW);
delay(950);
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(18, 4);
  display.setTextSize(1);
  display.println("EMERGENCY ALERT");
  display.drawLine(6, 14, 120, 14, SH110X_WHITE);
  display.setCursor(7, 18);
    display.println("Sending in:");
display.setCursor(59, 35);
display.setTextSize(2);
display.println("2");
display.display();
digitalWrite(BUZZER, HIGH);
delay(50);
digitalWrite(BUZZER, LOW);
delay(950);
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(18, 4);
  display.setTextSize(1);
  display.println("EMERGENCY ALERT");
  display.drawLine(6, 14, 120, 14, SH110X_WHITE);
  display.setCursor(7, 18);
    display.println("Sending in:");
display.setCursor(59, 35);
display.setTextSize(2);
display.println("1");
digitalWrite(BUZZER, HIGH);
delay(50);
digitalWrite(BUZZER, LOW);
display.display();
delay(100);
  display.clearDisplay();
  inPage = false;
  
}

void showVitalsPage() {

  // Exit immediately on BACK
  if (digitalRead(BTN_BACK) == LOW) {
    inPage = false;
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    return;
  }

  

  // Update vitals at controlled interval
  if (millis() - lastVitalsUpdate < vitalsInterval) return;
  lastVitalsUpdate = millis();

  // Finger detection
  if (particleSensor.getIR() < 5000) {
    display.clearDisplay();
    display.drawRect(0, 0, 127, 63, SH110X_WHITE);
    display.setCursor(30, 28);
    display.println("Place Finger");
    display.display();
    return;
  }

  // Collect samples (SparkFun recommended)
  for (int i = 0; i < VITALS_BUFFER_SIZE; i++) {
    while (!particleSensor.available())
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // Compute SpO2 + BPM
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer, VITALS_BUFFER_SIZE,
    redBuffer,
    &spo2, &validSPO2,
    &heartRate, &validHeartRate
  );

  // Display
  display.clearDisplay();
  display.drawRect(0, 0, 127, 63, SH110X_WHITE);
  display.setCursor(35, 4);
  display.println("VITALS");
  display.drawLine(6, 14, 120, 14, SH110X_WHITE);

  display.setCursor(10, 20);
  display.print("BPM: ");
  validHeartRate ? display.println(heartRate) : display.println("--");

  display.setCursor(10, 34);
  display.print("SpO2: ");
  validSPO2 ? display.print(spo2) : display.print("--");
  display.println(" %");
  display.setCursor(2, 46);
  display.println("(Keep for atleast 10s for better result.)");
  display.display();


  // ======= API SEND PART (NEW) =======
  if (WiFi.status() == WL_CONNECTED && validHeartRate && validSPO2) {
    HTTPClient http;
    http.begin(vitalsApiUrl); // vitalsApiUrl should be defined elsewhere
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<128> doc;
    doc["heartRate"] = heartRate;
    doc["pulse"] = spo2;
    

    String body;
    serializeJson(doc, body);

    Serial.print("[API] Sending Vitals: ");
    Serial.println(body);

    http.POST(body);
    http.end();
  }
  return;
}

void sendEmergencyAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[EMERGENCY] WiFi not connected!");
    return;
  }

  HTTPClient http;
  http.begin(emergencyApiUrl); 
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["status"] = true;
  doc["patient"] = patientName;
  doc["address"] = patientAddress;

  String body;
  serializeJson(doc, body);

  Serial.print("[EMERGENCY] Sending alert: ");
  Serial.println(body);

  int httpResponseCode = http.POST(body);

  if (httpResponseCode > 0) {
    Serial.print("[EMERGENCY] Response code: ");
    Serial.println(httpResponseCode);
    Serial.println("[EMERGENCY] Response: " + http.getString());
  } else {
    Serial.print("[EMERGENCY] POST failed, error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return;
}

void sendTemp() {
 float temperature = dht.readTemperature(); // Celsius
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[DHT] Failed to read sensor!");
    return;
  }

  Serial.print("[DHT] Temperature: "); Serial.print(temperature); Serial.print("°C, ");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(envoirmentApiUrl); 
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<128> doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;

    String body;
    serializeJson(doc, body);

    Serial.print("[API] Sending Data: ");
    Serial.println(body);

    int httpResponseCode = http.POST(body);
    if (httpResponseCode > 0) {
      Serial.print("[API] Response code: ");
      Serial.println(httpResponseCode);
      Serial.println("[API] Response: " + http.getString());
    } else {
      Serial.print("[API] POST failed, error: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("[API] WiFi not connected");
  }
   return;

}