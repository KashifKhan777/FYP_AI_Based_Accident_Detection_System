/*******************************************************
   FYP: Toy Car Accident Detection System
   STEP 1 + STEP 2


   Functions:
   ✓ Initialize LCD
   ✓ Initialize MPU6050
   ✓ Initialize SIM800L
   ✓ Monitor acceleration
   ✓ Detect accident using threshold


   Author: Kashif Khan
********************************************************/


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <HardwareSerial.h>
#include <math.h>
#include <TinyGPS++.h>


// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);


// ---------------- MPU6050 ----------------
Adafruit_MPU6050 mpu;


// ---------------- GSM ----------------
HardwareSerial GSM(1);


// GSM Pins
#define GSM_RX 26
#define GSM_TX 27

// ---------------- GPS ----------------
HardwareSerial GPS(2);
TinyGPSPlus gps;

// GPS Pins
#define GPS_RX 17
#define GPS_TX 16


// I2C Pins
#define SDA_PIN 32
#define SCL_PIN 33

// Buzzer
#define BUZZER_PIN 5

//Push Button
#define BUTTON_PIN 18


// Threshold (temporary value)
float accidentThreshold = 14.0;


// Module Status
bool lcdStatus = false;
bool mpuStatus = false;
bool gsmStatus = false;


//======================================================
// Wait for "OK" response from SIM800L
//======================================================
bool waitForOK(unsigned long timeout)
{
  String response = "";
  unsigned long start = millis();


  while (millis() - start < timeout)
  {
    while (GSM.available())
    {
      char c = GSM.read();
      Serial.write(c);
      response += c;


      if (response.indexOf("OK") != -1)
      {
        return true;
      }
    }
  }


  return false;
}

//======================================================
// Read and Display GSM Response
//======================================================
String readGSMResponse(unsigned long timeout)
{
  String response = "";
  unsigned long start = millis();

  while (millis() - start < timeout)
  {
    while (GSM.available())
    {
      char c = GSM.read();

      Serial.write(c);
      response += c;
    }
  }

  return response;
}

//======================================================

//======================================================
// Get GPS Coordinates
//======================================================
bool getGPSLocation(double &latitude, double &longitude)
{
  unsigned long start = millis();

  Serial.println("Searching for GPS coordinates...");

  while (millis() - start < 5000)
  {
    while (GPS.available())
    {
      gps.encode(GPS.read());

      if (gps.location.isValid() && gps.location.isUpdated())
      {
        latitude = gps.location.lat();
        longitude = gps.location.lng();

        Serial.println("GPS Location Found");

        Serial.print("Latitude: ");
        Serial.println(latitude, 7);

        Serial.print("Longitude: ");
        Serial.println(longitude, 7);

        return true;
      }
    }
  }

  Serial.println("GPS Location Not Found");

  return false;
}

//======================================================
// Send Accident Alert SMS
//======================================================
//======================================================
// Send Accident Alert SMS
//======================================================
bool sendAlertSMS(double latitude, double longitude)
{
  Serial.println();
  Serial.println("=================================");
  Serial.println(" GSM SMS PROCESS STARTED");
  Serial.println("=================================");

  // Create SMS message
String message = "ACCIDENT DETECTED!\n\n";
message += "Open Location:\n";
message += "https://www.google.com/maps/search/?api=1&query=";
message += String(latitude, 6);
message += ",";
message += String(longitude, 6);

  Serial.println();
  Serial.println("SMS Destination:");
  Serial.println("+923329228145");

  Serial.println();
  Serial.println("SMS Message:");
  Serial.println("-----------------------------");
  Serial.println(message);
  Serial.println("-----------------------------");

  //====================================================
  // STEP 1 - Check SIM
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Checking SIM...");
  
  Serial.println();
  Serial.println("Checking SIM card...");

  GSM.println("AT+CPIN?");
  delay(1000);

  String response = readGSMResponse(2000);

  //====================================================
  // STEP 2 - Check Network
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Checking Network");

  Serial.println();
  Serial.println("Checking network registration...");

  GSM.println("AT+CREG?");
  delay(1000);

  response = readGSMResponse(2000);

  //====================================================
  // STEP 3 - Check Signal
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Checking Signal");

  Serial.println();
  Serial.println("Checking signal strength...");

  GSM.println("AT+CSQ");
  delay(1000);

  response = readGSMResponse(2000);

  //====================================================
  // STEP 4 - Check SMS Service Center
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Checking SMSC");

  Serial.println();
  Serial.println("Checking SMS Service Center...");

  GSM.println("AT+CSCA?");
  delay(1000);

  response = readGSMResponse(2000);

  //====================================================
  // STEP 5 - Set SMS Text Mode
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Preparing SMS...");

  Serial.println();
  Serial.println("Setting SMS text mode...");

  GSM.println("AT+CMGF=1");
  delay(1000);

  response = readGSMResponse(2000);

  //====================================================
  // STEP 6 - Start SMS Transmission
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Sending SMS...");

  Serial.println();
  Serial.println("Starting SMS transmission...");

  GSM.println("AT+CMGS=\"+923329228145\"");

  // Wait for > prompt
  response = readGSMResponse(5000);

  if (response.indexOf(">") == -1)
  {
    Serial.println();
    Serial.println("ERROR: SIM800L did not provide SMS prompt.");
    Serial.println("SMS NOT SENT.");

    lcd.clear();
    lcd.print("SMS Status");
    lcd.setCursor(0, 1);
    lcd.print("No SMS Prompt");

    delay(2000);

    return false;
  }

  //====================================================
  // STEP 7 - Send Message
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Sending Data...");

  Serial.println();
  Serial.println("SMS prompt received.");
  Serial.println("Sending message...");

  GSM.print(message);

  delay(500);

  // CTRL+Z
  GSM.write(26);

  Serial.println("CTRL+Z sent.");

  //====================================================
  // STEP 8 - Wait for Network Response
  //====================================================

  lcd.clear();
  lcd.print("SMS Status");
  lcd.setCursor(0, 1);
  lcd.print("Waiting Reply...");

  Serial.println();
  Serial.println("Waiting for SIM800L confirmation...");

  response = readGSMResponse(20000);

  Serial.println();
  Serial.println("Final GSM Response:");
  Serial.println("-----------------------------");
  Serial.println(response);
  Serial.println("-----------------------------");

  //====================================================
  // STEP 9 - Check Result
  //====================================================

  if (response.indexOf("+CMGS:") != -1 &&
      response.indexOf("OK") != -1)
  {
    Serial.println();
    Serial.println("***** SMS SENT SUCCESSFULLY *****");

    return true;
  }

  if (response.indexOf("ERROR") != -1)
  {
    Serial.println();
    Serial.println("***** SMS FAILED *****");

    return false;
  }

  Serial.println();
  Serial.println("***** SMS STATUS UNKNOWN *****");

  return false;
}


void setup()
{
  //----------------------------------------------------
  // Start Serial Monitor
  //----------------------------------------------------


  Serial.begin(115200);


  Serial.println();
  Serial.println("=================================");
  Serial.println(" Accident Detection System");
  Serial.println(" Initializing Hardware...");
  Serial.println("=================================");


  //----------------------------------------------------
  // Start I2C Communication
  //----------------------------------------------------


  Wire.begin(SDA_PIN, SCL_PIN);


  //----------------------------------------------------
  // Initialize LCD
  //----------------------------------------------------


  lcd.init();
  lcd.backlight();


  lcdStatus = true;


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing");


  lcd.setCursor(0, 1);
  lcd.print("LCD...");


  Serial.println("LCD Initialized");


  delay(1500);


  //----------------------------------------------------
  // Initialize MPU6050
  //----------------------------------------------------


  if (mpu.begin())
  {
    mpuStatus = true;


    Serial.println("MPU6050 Initialized");


    lcd.clear();
    lcd.print("MPU6050");


    lcd.setCursor(0, 1);
    lcd.print("Initialized");


    delay(1500);


    // Configure MPU6050
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
else

{
  Serial.println("MPU6050 FAILED");

  lcd.clear();
  lcd.print("MPU6050 Failed");
  delay(1500);

  Serial.println("SYSTEM FAILED");

  lcd.clear();
  lcd.print("System Failed");
  delay(1500);

  Serial.println("SYSTEM RESTARTING");

  lcd.clear();
  lcd.print("System Restarting");
  delay(2000);

  ESP.restart();
}


  //----------------------------------------------------
  // Initialize GSM
  //----------------------------------------------------


GSM.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);

// Start GPS
GPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

Serial.println("GPS Initialized");

delay(15000);

GSM.println("AT");


  delay(15000);


  GSM.println("AT");


  if (waitForOK(3000))
  {
    gsmStatus = true;


    Serial.println("SIM800L Initialized");


    lcd.clear();
    lcd.print("SIM800L");


    lcd.setCursor(0, 1);
    lcd.print("Initialized");


    delay(1500);
  }
  else
  
{
  Serial.println("SIM800L FAILED");

  lcd.clear();
  lcd.print("SIM800L Failed");
  delay(1500);

  Serial.println("SYSTEM FAILED");

  lcd.clear();
  lcd.print("System Failed");
  delay(1500);

  Serial.println("SYSTEM RESTARTING");

  lcd.clear();
  lcd.print("System Restarting");
  delay(2000);

  ESP.restart();
}


  //----------------------------------------------------
  // Check all modules
  //----------------------------------------------------


  if (lcdStatus && mpuStatus && gsmStatus)
  {
    Serial.println();
    Serial.println("All Modules Ready");


    lcd.clear();
    lcd.print("System Ready");


    lcd.setCursor(0, 1);
    lcd.print("Monitoring...");

  // Start buzzer for 2 seconds after all modules are initialized
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, HIGH);

  digitalWrite(BUZZER_PIN, LOW);   // Buzzer ON
  delay(2000);
  digitalWrite(BUZZER_PIN, HIGH);  // Buzzer OFF

  }
}


//======================================================


void loop()
{
  //----------------------------------------------------
  // Read MPU6050 Data
  //----------------------------------------------------


  sensors_event_t accel, gyro, temp;


  mpu.getEvent(&accel, &gyro, &temp);


  //----------------------------------------------------
  // Calculate Total Acceleration
  //----------------------------------------------------


  float magnitude = sqrt(
                      accel.acceleration.x * accel.acceleration.x +
                      accel.acceleration.y * accel.acceleration.y +
                      accel.acceleration.z * accel.acceleration.z);


  //----------------------------------------------------
  // Print Sensor Values
  //----------------------------------------------------


static unsigned long lastSensorPrint = 0;

if (millis() - lastSensorPrint >= 20000)
{
  lastSensorPrint = millis();

  Serial.println();
  Serial.println("----- Monitoring -----");

  Serial.print("X: ");
  Serial.print(accel.acceleration.x);

  Serial.print("  Y: ");
  Serial.print(accel.acceleration.y);

  Serial.print("  Z: ");
  Serial.print(accel.acceleration.z);

  Serial.print("  Magnitude: ");
  Serial.println(magnitude);
}


  //----------------------------------------------------
  // Accident Detection
  //----------------------------------------------------


 if (magnitude > accidentThreshold)
{
  Serial.println();
  Serial.println("***** ACCIDENT DETECTED *****");

  // Start buzzer continuously
  digitalWrite(BUZZER_PIN, LOW);

  // 10 second countdown
  for (int countdown = 10; countdown >= 0; countdown--)
  {
    // Check push button
    if (digitalRead(BUTTON_PIN) == LOW)
    {
      Serial.println("BUTTON PRESSED");
      Serial.println("Accident Alert Cancelled");

      digitalWrite(BUZZER_PIN, HIGH);

lcd.clear();
lcd.print("Thanks God");
lcd.setCursor(0, 1);
lcd.print("you are safe");
delay(2500);

lcd.clear();
lcd.print("Alert Message");
lcd.setCursor(0, 1);
lcd.print("Not Sent");
delay(2500);

      lcd.clear();
      lcd.print("System Ready");
      lcd.setCursor(0, 1);
      lcd.print("Monitoring...");

      return;
    }

    // Display countdown
    lcd.clear();
    lcd.print("ACCIDENT!");
    lcd.setCursor(0, 1);
    lcd.print("Countdown: ");
    lcd.print(countdown);

    Serial.print("Accident Countdown: ");
    Serial.println(countdown);

    delay(1000);
  }

// 10 seconds completed
digitalWrite(BUZZER_PIN, HIGH);

Serial.println("10 Seconds Completed");
Serial.println("Buzzer OFF");

//======================================================
// Get GPS Location
//======================================================

double latitude;
double longitude;

bool gpsFound = getGPSLocation(latitude, longitude);

//======================================================
// GPS Retry
//======================================================

if (!gpsFound)
{
  Serial.println();
  Serial.println("GPS Retry...");

  lcd.clear();
  lcd.print("GPS Not Found");
  lcd.setCursor(0, 1);
  lcd.print("Retrying...");

  delay(1000);

  gpsFound = getGPSLocation(latitude, longitude);
}

//======================================================
// GPS Backup Location
//======================================================

if (!gpsFound)
{
  Serial.println();
  Serial.println("GPS FAILED");
  Serial.println("Using predefined backup location.");

  latitude = 34.8549004;
  longitude = 72.453139;

  lcd.clear();
  lcd.print("GPS Failed");
  lcd.setCursor(0, 1);
  lcd.print("Using Backup");

  delay(1500);
}

//======================================================
// Send Alert SMS
//======================================================

lcd.clear();
lcd.print("Sending Alert");
lcd.setCursor(0, 1);
lcd.print("Message...");

bool smsSent = sendAlertSMS(latitude, longitude);

//======================================================
// Show SMS Result
//======================================================

delay(2000);

lcd.clear();
lcd.print("Alert Message");
lcd.setCursor(0, 1);

if (smsSent)
{
  lcd.print("Sent");
}
else
{
  lcd.print("Not Sent");
}

delay(2000);

//======================================================
// Return to Monitoring
//======================================================

lcd.clear();
lcd.print("System Ready");
lcd.setCursor(0, 1);
lcd.print("Monitoring...");

delay(100);

} // end accident detection if

} // end loop