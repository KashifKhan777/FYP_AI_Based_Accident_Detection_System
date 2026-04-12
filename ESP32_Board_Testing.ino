#include <Wire.h>
#include <MPU6050.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPSPlus.h>

MPU6050 mpu;
LiquidCrystal_I2C lcd(0x27, 16, 2);
TinyGPSPlus gps;

// ---------------- PINS ----------------
#define BUZZER_PIN 5
#define BUTTON_PIN 18

// NEO-6M GPS pins
#define GPS_RX 16   // ESP32 RX <- GPS TX
#define GPS_TX 17   // ESP32 TX -> GPS RX

// GSM module pins
#define GSM_RX 26   // ESP32 RX <- GSM TX
#define GSM_TX 27   // ESP32 TX -> GSM RX

// ---------------- SERIAL ----------------
HardwareSerial gpsSerial(2);
HardwareSerial gsmSerial(1);

// ---------------- USER SETTINGS ----------------
String phoneNumber = "+923329228145";   // replace with your phone number

// ---------------- SYSTEM VARIABLES ----------------
int accidentCount = 0;

bool buzzerActive = false;
bool safeMode = false;
bool smsSent = false;

unsigned long buzzerStartTime = 0;
unsigned long safeStartTime = 0;

String lastLine1 = "";
String lastLine2 = "";

// ---------------- LCD ----------------
void updateLCD(String line1, String line2) {
  if (line1 != lastLine1 || line2 != lastLine2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    lastLine1 = line1;
    lastLine2 = line2;
  }
}

// ---------------- GPS ----------------
void feedGPS() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
}

String getGoogleMapsLink() {
  unsigned long start = millis();

  while (millis() - start < 3000) {
    feedGPS();
  }

  if (gps.location.isValid()) {
    String link = "https://maps.google.com/?q=";
    link += String(gps.location.lat(), 6);
    link += ",";
    link += String(gps.location.lng(), 6);
    return link;
  }

  return "Location not available";
}

// ---------------- GSM ----------------
void sendATCommand(String command, unsigned long waitTime) {
  gsmSerial.println(command);
  delay(waitTime);

  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());
  }
}

void sendSMS(String message) {
  sendATCommand("AT", 1000);
  sendATCommand("AT+CMGF=1", 1000);

  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");
  delay(1000);

  gsmSerial.print(message);
  delay(500);

  gsmSerial.write(26); // Ctrl+Z
  delay(5000);

  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());
  }
}

void sendAccidentSMS() {
  String locationLink = getGoogleMapsLink();

  String msg = "Accident detected!\n";
  msg += "Emergency alert active.\n";
  msg += "Location: ";
  msg += locationLink;

  sendSMS(msg);
  Serial.println("SMS sent.");
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  mpu.initialize();

  lcd.init();
  lcd.backlight();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);   // active-low buzzer OFF

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);   // NEO-6M
  gsmSerial.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);   // SIM800L/SIM900 etc.

  updateLCD("System Ready", "");
  delay(2000);
  updateLCD("System is", "Working...");
}

// ---------------- LOOP ----------------
void loop() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float magnitude = sqrt((long)ax * ax + (long)ay * ay + (long)az * az);
  Serial.println(magnitude);

  feedGPS();

  // NORMAL MODE
  if (!buzzerActive && !safeMode) {
    updateLCD("System is", "Working...");
  }

  // ACCIDENT DETECTION
  if (magnitude > 20000) {
    accidentCount++;
  } else {
    accidentCount = 0;
  }

  // START SYSTEM
  if (accidentCount >= 3 && !buzzerActive && !safeMode) {
    Serial.println("Accident!");

    buzzerActive = true;
    buzzerStartTime = millis();
    accidentCount = 0;
    smsSent = false;
  }

  // BUZZER + COUNTDOWN
  if (buzzerActive) {
    unsigned long elapsed = millis() - buzzerStartTime;
    int remainingTime = 10 - (elapsed / 1000);

    // BUTTON -> SAFE MODE
    if (digitalRead(BUTTON_PIN) == LOW) {
      delay(50);
      if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("SAFE");

        digitalWrite(BUZZER_PIN, HIGH); // OFF
        buzzerActive = false;
        safeMode = true;
        safeStartTime = millis();
        smsSent = false;

        updateLCD("Thank God You", "are Safe :)");
        return;
      }
    }

    // Accident detection rhythm: beep-beep-pause
    unsigned long patternTime = elapsed % 1000;
    if ((patternTime < 200) || (patternTime >= 400 && patternTime < 600)) {
      digitalWrite(BUZZER_PIN, LOW);   // ON
    } else {
      digitalWrite(BUZZER_PIN, HIGH);  // OFF
    }

    // COUNTDOWN
    if (remainingTime > 0) {
      updateLCD("ACCIDENT!", "Time: " + String(remainingTime) + " sec");
    }

    // CONFIRMED ACCIDENT -> SEND SMS
    else if (elapsed <= 15000) {
      updateLCD("EMERGENCY!!!", "Alert Active");

      if (!smsSent) {
        sendAccidentSMS();
        smsSent = true;
      }
    }

    // AUTO STOP
    else {
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerActive = false;

      updateLCD("Timeout Stop", "");
      delay(2000);
      updateLCD("System is", "Working...");
    }
  }

  // SAFE MODE
  if (safeMode) {
    unsigned long safeElapsed = millis() - safeStartTime;

    updateLCD("Thank God You", "are Safe :)");

    if (safeElapsed > 5000) {
      safeMode = false;

      updateLCD("System Reset", "");
      delay(2000);
      updateLCD("System is", "Working...");
    }
  }

  delay(50);
}
