// Custom library
#include "lib_and_pin.h"

// Internet & database config
#define WIFI_SSID "HoangAnhHN"
#define WIFI_PASSWORD "Hoanganh2002"
#define FIREBASE_AUTH "vjLt9l2OmXNWUaXyieAUfC4hV49gR6iRAH16s4Nl"
#define FIREBASE_HOST "carparking-4ce1e-default-rtdb.firebaseio.com"

// Create object
FirebaseData firebaseData;
Servo servo_in, servo_out;
LiquidCrystal_I2C lcd(0x27, 16, 2);

TaskHandle_t xTaskSensor;
TaskHandle_t xTaskBarrierControl;

SemaphoreHandle_t xBinarySemaphore;
// QueueHandle_t queue;

byte statusServo;

/************** Define tasks and functions **************/
void init_system();            // Pin mode for pins, connect internet and firebase
void display(byte mode);       // Display on LCD with 4 modes: stand by, show parking space, warning
void gate_in();                // Control gate in: false to close - true to open
void gate_out();               // Control gate out: false to close - true to open
byte push_data_to_firebase();  // Push data (avaiable parking spaces) and count available spaces

void TaskSensor(void *pv);
void TaskBarrierControl(void *pv);

void setup() {
  Serial.begin(9600);
  init_system();
  display(1);  // Stand by mode

  // Create tasks for 2 cores
  xTaskCreatePinnedToCore(
    TaskSensor,
    "TaskSensor",
    10000,
    NULL,
    0,
    &xTaskSensor,
    0);

  xTaskCreatePinnedToCore(
    TaskBarrierControl,
    "TaskBarrierControl",
    2048,
    NULL,
    1,
    &xTaskBarrierControl,
    1);
}
void loop() {
  vTaskDelete(NULL);
}  // 2 Tasks run

void gate_in() {
  servo_in.attach(servoVao_PIN);
  servo_in.write(85);

  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(buzzer_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  vTaskDelay(5000 / portTICK_PERIOD_MS);
  servo_in.write(171);

  statusServo = 1;
  vTaskDelay(200 / portTICK_PERIOD_MS);
  servo_in.detach();
}
void gate_out() {
  servo_out.attach(servoRa_PIN);
  servo_out.write(0);

  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(buzzer_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  vTaskDelay(5000 / portTICK_PERIOD_MS);
  servo_out.write(82);

  statusServo = 1;
  vTaskDelay(200 / portTICK_PERIOD_MS);
  servo_in.detach();
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
  vTaskDelay(250 / portTICK_PERIOD_MS);
  servo_in.detach();
  servo_out.detach();
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
      lcd.print("    Warning     ");
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

void TaskSensor(void *pv) {
  while (1) {
    Serial.println("Task sensor...................");
    // Check fire sensor
    if (digitalRead(fireSensor_PIN) == LOW) {
      display(4);
      servo_in.attach(servoVao_PIN);
      servo_out.attach(servoRa_PIN);
      servo_in.write(85);
      servo_out.write(0);
      digitalWrite(buzzer_PIN, LOW);
    } else {
      // if (statusServo == 0) {
      //   servo_in.write(171);
      //   servo_out.write(82);
      //   vTaskDelay(250 / portTICK_PERIOD_MS);
      //   servo_in.detach();
      //   servo_out.detach();
      // }

      digitalWrite(buzzer_PIN, HIGH);
      if (push_data_to_firebase() == 4) {
        display(5);
      } else {
        display(1);
      }
    }
    Firebase.setInt(firebaseData, "/FireSensor", digitalRead(fireSensor_PIN));

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
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay 1000ms
  }
}
void TaskBarrierControl(void *pv) {
  while (1) {
    Serial.println("Task barie.....................");
    /* Điều khiển đóng mở barie bằng nút bấm */
    if (digitalRead(btnRa_PIN) == 0) {
      display(3);
      gate_in();
    }

    if (digitalRead(btnVao_PIN) == 0) gate_out();

    if (Serial.available()) {
      char key = Serial.read();

      if (key == '1') {
        display(3);
        gate_in();
      }
      if (key == '2') gate_out();
    }
    statusServo = 0;
    // servo_in.write(171);
    // servo_out.write(82);
    // vTaskDelay(250 / portTICK_PERIOD_MS);  // Delay 10ms
    // display(1);
    vTaskDelay(15 / portTICK_PERIOD_MS);  // Delay 10ms
  }
}
