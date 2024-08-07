#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>

// WiFi credentials
#define WIFI_SSID "Abc"
#define WIFI_PASSWORD "abcdefgh"

// Firebase configuration
#define API_KEY "AIzaSyCbFKic6XziEe4FJEeWFW-ea3Qt-rEhDE0"
#define DATABASE_URL "https://replace-6601a-default-rtdb.firebaseio.com/"
#define USER_EMAIL "test01@gamil.com"
#define USER_PASSWORD "test01_DLP"

// Firebase objects
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;

unsigned long previousMillis = 0; // Time when the power was last updated
unsigned long previousFirebaseUpdateMillis = 0; // Time when Firebase was last updated
const long interval = 1000; // Interval to measure power (1 second)
const unsigned long firebaseUpdateInterval = 1000 * 60 * 5; // Interval to update Firebase (5 minutes)

float totalEnergy = 0; // Total energy consumption

// Ultrasonic sensor pins
const int Vin = 34; // GPIO pin for the trigger pin of the ultrasonic sensor
const int Iin = 35; // GPIO pin for the echo pin of the ultrasonic sensor

int Switch = 2; // GPIO pin for built-in LED or any digital pin
int Relay = 27; // GPIO pin for the relay

float High_Voltage = 400;
float High_Current = 5;

int Max_V = 400;
int Max_I = 5;

bool Switch_Status;
bool Button_press = true;

void setup() {
  Serial.begin(115200);

  pinMode(Switch, OUTPUT);
  pinMode(Vin, INPUT); 
  pinMode(Iin, INPUT);  
  pinMode(Relay, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Set Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

  Serial.println("Connected to Firebase!");
  Firebase.setBool(fbdo, "/R0001/Connected", true);
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Data collection
    int Voltage_read = analogRead(Vin);
    int Current_read = analogRead(Iin);

    float Voltage = Voltage_read * High_Voltage / 4095.0; // ESP32 ADC resolution is 12-bit (0-4095)
    float Current = Current_read * High_Current / 4095.0;
    float Power = Voltage * Current / 2; // Power in watts

    // Update total energy consumption
    totalEnergy += Power * (interval / 1000.0) / 3600.0; // Convert interval to hours and accumulate energy in Wh

    is_Button_pressed();

    // App control
    Firebase.getBool(fbdo, "/R0001/Switch"); 
    bool Switch_Status = fbdo.boolData();
    
    // Check voltage and current safety zone
    if ((Voltage < Max_V) && (Current < Max_I)) {
      digitalWrite(Relay, Switch_Status);
    }
  }

  // Check if 5 minutes have passed for Firebase update
  if (currentMillis - previousFirebaseUpdateMillis >= firebaseUpdateInterval) {
    previousFirebaseUpdateMillis = currentMillis; // Reset the Firebase update time
    
    // Update Firebase
    Firebase.setFloat(fbdo, "/R0001/Voltage", Voltage);
    Firebase.setFloat(fbdo, "/R0001/Current", Current);
    Firebase.setFloat(fbdo, "/R0001/Power", Power);
    Firebase.setFloat(fbdo, "/R0001/TotalEnergy", totalEnergy); // Store total energy consumption

    totalEnergy = 0; // Reset total energy after updating Firebase
  }
}

void is_Button_pressed() {
  bool Button = digitalRead(Switch);
  if (Button) {
    Button_press = true;
  } else {
    if (Button_press) {
      Switch_Status = !Switch_Status;
      Button_press = false;
    }
  }
}
