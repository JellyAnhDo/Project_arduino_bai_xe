// Custom library
#include "lib_and_port.h"

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
TaskHandle_t xTaskManualBarrierControl;

/************** Define tasks and functions **************/
void init_system();            // Pin mode for pins, connect internet and firebase
void display(byte mode);       // Display on LCD with 3 modes: stand by, show parking space, warning
void gate_in();                // Control gate in: false to close - true to open
void gate_out();               // Control gate out: false to close - true to open
byte push_data_to_firebase();  // Push data (avaiable parking spaces) and count available spaces

void TaskSensor(void *pv);
void TaskManualBarrierControl(void *pv);

void setup() {
  Serial.begin(9600);
  init_system();
  display(1);  // Stand by mode

  // Create tasks for 2 cores
  xTaskCreatePinnedToCore(
    TaskSensor,    // Hàm xử lý của Task
    "TaskSensor",  // Tên Task
    10000,         // Kích thước stack cho Task
    NULL,          // Tham số truyền vào Task
    1,             // Độ ưu tiên của Task (0 là thấp nhất, 3 là cao nhất)
    &xTaskSensor,  // Biến lưu trữ Task handle
    0              // Lõi CPU để chạy Task (0 hoặc 1)
  );

  xTaskCreatePinnedToCore(
    TaskManualBarrierControl,    // Hàm xử lý của Task
    "TaskManualBarrierControl",  // Tên Task
    1024,                        // Kích thước stack cho Task
    NULL,                        // Tham số truyền vào Task
    1,                           // Độ ưu tiên của Task (0 là thấp nhất, 3 là cao nhất)
    &xTaskManualBarrierControl,  // Biến lưu trữ Task handle
    1                            // Lõi CPU để chạy Task (0 hoặc 1)
  );
}
void loop() {}  // 2 Tasks run

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
  servo_in.write(172);
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
  servo_out.detach();
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

  if (xSemaphoreTake(xMutex, (TickType_t)10) == pdTRUE) {
    Firebase.setInt(firebaseData, "/FireSensor", fireSensor);
    Firebase.setInt(firebaseData, "/ParkingSpace/ParkingSpaceCount", count);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space1", a);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space2", b);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space3", c);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space4", d);
    xSemaphoreGive(xMutex);
  }

  return count;
}

void TaskSensor(void *pv) {
  while (1) {
    // Check fire sensor
    if (fireSensor == LOW) {
      display(4);
      servo_in.write(85);
      servo_out.write(0);
      digitalWrite(buzzer_PIN, LOW);
    } else {
      servo_in.write(172);
      servo_out.write(82);
      digitalWrite(buzzer_PIN, HIGH);
      if (parking_space_check() == 4) {
        lcd.setCursor(0, 0);
        lcd.print("     Sorry!     ");
        lcd.setCursor(0, 1);
        lcd.print(" No space left! ");
      } else {
        LCD();
      }
    }

    // Điều khiển đèn
    lightSensor = digitalRead(lightSensor_PIN);
    Firebase.getInt(firebaseData, "/Light/LightBtn");
    int lightBtn = firebaseData.intData();

    if (lightBtn == 1) {
      digitalWrite(light_PIN, LOW);
    } else {
      digitalWrite(light_PIN, HIGH);
      Firebase.getInt(firebaseData, "/Light/LightAuto");
      int lightBtnAuto = firebaseData.intData();
      if (lightBtnAuto == 1) {
        if (lightSensor == HIGH) {
          digitalWrite(light_PIN, LOW);
        } else {
          digitalWrite(light_PIN, HIGH);
        }
      }
    }
  }
}
void TaskManualBarrierControl(void *pv) {
  while (1) {
    /* Điều khiển đóng mở barie bằng nút bấm */
    if (digitalRead(btnRa_PIN) == 0) gate_in();
    if (digitalRead(btnVao_PIN) == 0) gate_out();

    if (Serial.available()) {
      char key = Serial.read();

      if (key == '1') gate_in();
      else (key == '2') gate_out();
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay 10ms
  }
}
