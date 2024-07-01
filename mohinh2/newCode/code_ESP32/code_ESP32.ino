#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <FreeRTOS.h>
#include <ESP32Servo.h>
#include <FirebaseESP32.h>
#include <LiquidCrystal_I2C.h>

// Khai báo các chân
#define slot1 34
#define slot2 35
#define slot3 36
#define slot4 39
#define btnRa_PIN 5
#define buzzer_PIN 2
#define light_PIN 18
#define btnVao_PIN 4
#define servoRa_PIN 17
#define servoVao_PIN 23
#define fireSensor_PIN 16
#define lightSensor_PIN 19

// Internet & database config
#define WIFI_SSID "VyGarden Coffee"
#define WIFI_PASSWORD "88888888"
#define FIREBASE_AUTH "vjLt9l2OmXNWUaXyieAUfC4hV49gR6iRAH16s4Nl"
#define FIREBASE_HOST "carparking-4ce1e-default-rtdb.firebaseio.com"

FirebaseData firebaseData;
Servo myservo_vao, myservo_ra;
LiquidCrystal_I2C lcd(0x27, 16, 2);

TaskHandle_t xTaskSensor;
TaskHandle_t xTaskManualBarrierControl;
// TaskHandle_t xTaskAutoBarrierControl;
char sv;
// SemaphoreHandle_t xMutex;
int lightSensor, fireSensor = HIGH, s1, s2, s3, s4, a = 0, b = 0, c = 0, d = 0, count = 0;

/*===================Define task============================*/
void TaskSensor(void *pvParameters);
void TaskManualBarrierControl(void *pvParameters);
void TaskAutomaticBarrierControl(void *pvParameters);

void pin_mode() {
  pinMode(buzzer_PIN, OUTPUT);
  pinMode(btnVao_PIN, INPUT_PULLUP);
  pinMode(btnRa_PIN, INPUT_PULLUP);
  pinMode(servoVao_PIN, OUTPUT);
  pinMode(servoRa_PIN, OUTPUT);
  pinMode(light_PIN, OUTPUT);
  pinMode(lightSensor_PIN, INPUT);
  pinMode(fireSensor_PIN, INPUT);
  pinMode(slot1, INPUT);
  pinMode(slot2, INPUT);
  pinMode(slot3, INPUT);
  pinMode(slot4, INPUT);
}

void connect_internet() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Khởi tạo kết nối Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
}
void lcd_init() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  LCD();
}

// Cac hàm hiển thị thông báo lên LCD
void LCD() {
  lcd.setCursor(0, 0);
  lcd.print("     Welcome    ");
  lcd.setCursor(0, 1);
  lcd.print("Check Your Card ");
}
void LCD_FALSE() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Wrong card");
  lcd.setCursor(0, 1);
  lcd.print("Check Again");
}
void LCD_TRUE() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1    2    3    4");
  if (digitalRead(slot1) == LOW) {  // co xe
    lcd.setCursor(0, 1);
    lcd.print("X    ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("V    ");
  }

  if (digitalRead(slot2) == LOW) {  // co xe
    lcd.setCursor(5, 1);
    lcd.print("X    ");
  } else {
    lcd.setCursor(5, 1);
    lcd.print("V    ");
  }

  if (digitalRead(slot3) == LOW) {  // co xe
    lcd.setCursor(10, 1);
    lcd.print("X    ");
  } else {
    lcd.setCursor(10, 1);
    lcd.print("V    ");
  }

  if (digitalRead(slot4) == LOW) {  // co xe
    lcd.setCursor(15, 1);
    lcd.print("X");
  } else {
    lcd.setCursor(15, 1);
    lcd.print("V");
  }
}

void setup() {
  Serial.begin(9600);

  pin_mode();
  connect_internet();
  lcd_init();

  myservo_vao.attach(servoVao_PIN);
  myservo_ra.attach(servoRa_PIN);
  myservo_vao.write(172);
  myservo_ra.write(82);

  // Tạo mutex
  // xMutex = xSemaphoreCreateMutex();
  // if (xMutex == NULL) {
  //   Serial.println("Mutex creation failed.");
  //   while (1)
  //     ;  // Dừng chương trình nếu không tạo được mutex
  // }

  // Tạo 2 Task:
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
    4048,                        // Kích thước stack cho Task
    NULL,                        // Tham số truyền vào Task
    1,                           // Độ ưu tiên của Task (0 là thấp nhất, 3 là cao nhất)
    &xTaskManualBarrierControl,  // Biến lưu trữ Task handle
    1                            // Lõi CPU để chạy Task (0 hoặc 1)
  );

  // xTaskCreatePinnedToCore(
  //     TaskAutoBarrierControl,   // Hàm xử lý của Task
  //     "TaskAutoBarrierControl", // Tên Task
  //     2048,                     // Kích thước stack cho Task
  //     NULL,                     // Tham số truyền vào Task
  //     2,                        // Độ ưu tiên của Task (0 là thấp nhất, 3 là cao nhất)
  //     &xTaskAutoBarrierControl, // Biến lưu trữ Task handle
  //     1                         // Lõi CPU để chạy Task (0 hoặc 1)
  // );

  Serial.println("Start program.................");
}

void loop() {}

// Ham dong mo barier
void barrierVao_open() {
  myservo_vao.write(85);
  LCD_TRUE();
  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(buzzer_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  myservo_vao.write(172);
  LCD();
}
void barrierRa_open() {
  myservo_ra.write(0);
  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(buzzer_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  myservo_ra.write(82);
}

// Ham cam bien
void sensor() {
    fireSensor = digitalRead(fireSensor_PIN);

    if (fireSensor == LOW) {
      LCD_BAO_CHAY();
      myservo_vao.write(85);
      myservo_ra.write(0);
      digitalWrite(buzzer_PIN, LOW);
    } else {
      myservo_vao.write(172);
      myservo_ra.write(82);
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
}
int parking_space_check() {
  a = (digitalRead(slot1) == LOW) ? 1 : 0;
  b = (digitalRead(slot2) == LOW) ? 1 : 0;
  c = (digitalRead(slot3) == LOW) ? 1 : 0;
  d = (digitalRead(slot4) == LOW) ? 1 : 0;

  count = a + b + c + d;
  return count;
}
// Push dữ liệu cảm biến lên firebase
void pushSensorDataToFirebase() {
    Firebase.setInt(firebaseData, "/FireSensor", fireSensor);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space1", a);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space2", b);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space3", c);
    Firebase.setInt(firebaseData, "/ParkingSpace/Space4", d);
}

void LCD_BAO_CHAY() {
  lcd.setCursor(0, 0);
  lcd.print("*** Warning ****");
  lcd.setCursor(0, 1);
  lcd.print("Canh Bao Co Chay");
}

// Hàm cho Task 1: Điều khiển đóng/mở barrier và đèn
void TaskSensor(void *pvParameters) {
  for (;;) {
    Serial.println("Task sensor..................");
    sensor();
    pushSensorDataToFirebase();

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
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay 10ms
  }
}
//
// void TaskAutoBarrierControl(void *pvParameters) {
//   for (;;) {
//     // Đọc dữ liệu gửi từ hệ thống để đóng mở cổng
//     if (Serial.available()) {
//       sv = Serial.read();
//       Serial.println(sv);
//       delay(200);
//     }

//     if (sv == '1') {
//       barrierVao_open();
//     } else {
//       if (sv == '2') {
//         barrierRa_open();
//       } else {
//         myservo_ra.write(82);
//       }
//       myservo_vao.write(172);
//     }

//     sv = '0';
//   }
// }
// Hàm cho Task 2: Điều khiển đóng/mở barrier bằng nút bấm
void TaskManualBarrierControl(void *pvParameters) {
  int sv = 0;
  for (;;) {
    Serial.println("Task manual................");
    // Điều khiển đóng mở barie bằng nút bấm
    // Ấn nút cửa vào
    if (Serial.available()) {
      sv = Serial.read();
      Serial.println(sv);
      delay(200);
    }
    if (digitalRead(btnVao_PIN) == 0 || sv == '1') {
      barrierVao_open();
    } else {
      myservo_vao.write(172);
    }

    // Ấn nút cửa ra
    if (digitalRead(btnRa_PIN) == 0 || sv == '2') {
      barrierRa_open();
    } else {
      myservo_ra.write(82);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay 10ms
  }
}