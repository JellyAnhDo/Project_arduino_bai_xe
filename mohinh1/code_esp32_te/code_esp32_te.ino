#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <FirebaseESP32.h>
#include <LiquidCrystal_I2C.h>

// Khai báo các chân
#define buzzer_PIN 2
#define btnVao_PIN 4
#define btnRa_PIN 5
#define light_PIN 18
#define servoRa_PIN 17
#define servoVao_PIN 23
#define fireSensor_PIN 16
#define lightSensor_PIN 19
#define slot1 34
#define slot2 35
#define slot3 36
#define slot4 39

//============= Internet & database config ============================
#define WIFI_SSID "HoangAnhHN"
#define WIFI_PASSWORD "Hoanganh2002"
#define FIREBASE_AUTH "vjLt9l2OmXNWUaXyieAUfC4hV49gR6iRAH16s4Nl"
#define FIREBASE_HOST "carparking-4ce1e-default-rtdb.firebaseio.com"

//============== Create object ========================================
FirebaseData firebaseData;
Servo servo_in, servo_out;
LiquidCrystal_I2C lcd(0x27, 16, 2);

//=========================== Define functions =====================
void init_system();            // Pin mode for pins, connect internet and firebase
void display(byte mode);       // Display on LCD with 4 modes: stand by, show parking space, warning
void gate_in();                // Control gate in: false to close - true to open
void gate_out();               // Control gate out: false to close - true to open
byte push_data_to_firebase();  // Push data (available parking spaces) and count available spaces

void TaskSensor();
void TaskManualBarrierControl();
void TaskAutomaticBarrierControl();

unsigned long previousMillisSensor = 0;
const long intervalSensor = 500;  // Interval for sensor task

void setup() {
  Serial.begin(9600);
  init_system();
  //display(1);  // Stand by mode
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisSensor >= intervalSensor) {
    previousMillisSensor = currentMillis;
    TaskSensor();
  }

  TaskManualBarrierControl();
  TaskAutomaticBarrierControl();
}

void TaskSensor() {
  Serial.println("Task sensor...................");
  // Check fire sensor
  if (digitalRead(fireSensor_PIN) == LOW) {
    display(4);
    servo_in.write(85);
    servo_out.write(0);
    digitalWrite(buzzer_PIN, LOW);
  } else {
    servo_in.write(171);
    servo_out.write(82);

    if (push_data_to_firebase() == 4) {
      display(5);
    } else {
      display(1);
      Serial.println("Reset LCD into mode 1.............");
    }
    // Điều khiển đèn
    Firebase.getInt(firebaseData, "/Light/LightBtn");
    byte lightBtn = firebaseData.intData();

    if (lightBtn == 1) {
      digitalWrite(light_PIN, LOW);
    } else {
      digitalWrite(light_PIN, HIGH);
      Firebase.getInt(firebaseData, "/Light/LightAuto");
      int lightBtnAuto = firebaseData.intData();
      if (lightBtnAuto == 1) {
        if (digitalRead(lightSensor_PIN) == HIGH) {
          digitalWrite(light_PIN, LOW);
        } else {
          digitalWrite(light_PIN, HIGH);
        }
      }
    }
  }
  Firebase.setInt(firebaseData, "/FireSensor", digitalRead(fireSensor_PIN));
}

void TaskManualBarrierControl() {
  Serial.println("Task manual barie.....................");
  if (digitalRead(btnRa_PIN) == 0) {
    display(3);
    gate_in();
  }

  if (digitalRead(btnVao_PIN) == 0) gate_out();
}

void TaskAutomaticBarrierControl() {
  Serial.println("Task auto barie....................");
  if (Serial.available()) {
    char key = Serial.read();

    if (key == '1') {
      display(3);
      gate_in();
    }
    if (key == '2') gate_out();
  }
}

void init_system() {
  // Pin mode
  pinMode(slot1, INPUT);
  pinMode(slot2, INPUT);
  pinMode(slot3, INPUT);
  pinMode(slot4, INPUT);
  pinMode(light_PIN, OUTPUT);
  pinMode(buzzer_PIN, OUTPUT);
  pinMode(fireSensor_PIN, INPUT);
  pinMode(lightSensor_PIN, INPUT);
  pinMode(btnRa_PIN, INPUT_PULLUP);
  pinMode(btnVao_PIN, INPUT_PULLUP);

  // Turn LCD on
  lcd.init();
  lcd.backlight();

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Connect firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  if (Firebase.beginStream(firebaseData, "/")) {
    Serial.println("Connected to Firebase");
  } else {
    Serial.println("Failed to connect to Firebase");
    Serial.println(firebaseData.errorReason());
  }

  servo_in.attach(servoVao_PIN);
  servo_out.attach(servoRa_PIN);
  servo_in.write(171);
  servo_out.write(82);
}

void gate_in() {
  servo_in.write(85);

  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer_PIN, LOW);
    delay(100);
    digitalWrite(buzzer_PIN, HIGH);
    delay(100);
  }
  delay(5000);
  servo_in.write(171);
}

void gate_out() {
  servo_out.write(0);

  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer_PIN, LOW);
    delay(100);
    digitalWrite(buzzer_PIN, HIGH);
    delay(100);
  }

  delay(5000);
  servo_out.write(82);
}

void display(byte mode) {
  lcd.clear();
  switch (mode) {
    case 1:  // Stand by mode
      lcd.setCursor(0, 0);
      lcd.print("     Welcome    ");
      lcd.setCursor(0, 1);
      lcd.print("Check Your Card ");
      break;
    case 2:  // Wrong card
      lcd.setCursor(3, 0);
      lcd.print("Wrong card");
      lcd.setCursor(0, 1);
      lcd.print("Check Again");
      break;
    case 3:  // Correct card, check space
      lcd.setCursor(0, 0);
      lcd.print("1    2    3    4");

      lcd.setCursor(0, 1);
      (digitalRead(slot1) == LOW) ? lcd.print("X    ") : lcd.print("V    ");

      lcd.setCursor(5, 1);
      (digitalRead(slot2) == LOW) ? lcd.print("X    ") : lcd.print("V    ");

      lcd.setCursor(10, 1);
      (digitalRead(slot3) == LOW) ? lcd.print("X    ") : lcd.print("V    ");

      lcd.setCursor(15, 1);
      (digitalRead(slot4) == LOW) ? lcd.print("X") : lcd.print("V");
      break;
    case 4:  // Warning by sensor
      lcd.setCursor(0, 0);
      lcd.print("    Warning!    ");
      lcd.setCursor(0, 1);
      lcd.print("Canh Bao Co Chay");
      break;
    case 5:
      lcd.setCursor(0, 0);
      lcd.print("     Sorry!     ");
      lcd.setCursor(0, 1);
      lcd.print(" No space left! ");
      break;
    default:
      break;
  }
}

byte push_data_to_firebase() {
  byte a = (digitalRead(slot1) == LOW) ? 1 : 0;
  byte b = (digitalRead(slot2) == LOW) ? 1 : 0;
  byte c = (digitalRead(slot3) == LOW) ? 1 : 0;
  byte d = (digitalRead(slot4) == LOW) ? 1 : 0;

  byte count = a + b + c + d;

  Firebase.setInt(firebaseData, "/ParkingSpace/Space1", a);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space2", b);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space3", c);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space4", d);
  return count;
}
