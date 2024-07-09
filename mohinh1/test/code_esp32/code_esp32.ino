//============= Custom library ========================================
#include "lib_and_pin.h"

//============= Internet & database config ============================
#define WIFI_SSID "HoangAnhHN"
#define WIFI_PASSWORD "Hoanganh2002"
#define FIREBASE_AUTH "vjLt9l2OmXNWUaXyieAUfC4hV49gR6iRAH16s4Nl"
#define FIREBASE_HOST "carparking-4ce1e-default-rtdb.firebaseio.com"

//============== Create object ========================================
FirebaseData firebaseData;
Servo servo_in, servo_out;
LiquidCrystal_I2C lcd(0x27, 16, 2);

TaskHandle_t xTaskSensor;
TaskHandle_t xTaskBarrierControl;

SemaphoreHandle_t xMutexDisplay, xMutexServo;
QueueHandle_t xQueue;

byte modeDisplay;

//=========================== Define tasks and functions =====================
void init_system();            // Pin mode for pins, connect internet and firebase
void display(byte mode);       // Display on LCD with 4 modes: stand by, show parking space, warning
void gate_in();                // Control gate in: false to close - true to open
void gate_out();               // Control gate out: false to close - true to open
byte push_data_to_firebase();  // Push data (avaiable parking spaces) and count available spaces

//=========================== Tasks ========================================
void TaskSensor(void *pvParameters);
void TaskBarrierControl(void *pvParameters);
void TaskDisplayLCD(void *pvParameters);

void setup() {
  Serial.begin(9600);
  init_system();
  display(1);  // Stand by mode

  xMutexDisplay = xSemaphoreCreateMutex();
  if (xMutexDisplay == NULL) {
    Serial.println("Failed to create Display Mutex");
  }

  xMutexServo = xSemaphoreCreateMutex();
  if (xMutexServo == NULL) {
    Serial.println("Failed to create Servo Mutex");
  }


  // Create tasks for 2 cores
  xTaskCreatePinnedToCore(TaskSensor, "TaskSensor", 10000, NULL, 0, &xTaskSensor, 0);
  xTaskCreatePinnedToCore(TaskBarrierControl, "TaskBarrierControl", 2048, NULL, 1, &xTaskBarrierControl, 0);
  xTaskCreatePinnedToCore(TaskDisplayLCD, "TaskDisplayLCD", 2048, NULL, 1, NULL, 1);
}
void loop() {
  vTaskDelete(NULL);
}

void TaskSensor(void *pvParameters) {
  while (1) {
    Serial.println("Task sensor...................");
    // Check fire sensor
    if (digitalRead(fireSensor_PIN) == LOW) {
      modeDisplay = 4;
      // servo_in.attach(servoVao_PIN);
      // servo_out.attach(servoRa_PIN);
      servo_in.write(85);
      servo_out.write(0);
      digitalWrite(buzzer_PIN, LOW);
    } else {
      if (xSemaphoreTake(xMutexServo, portMAX_DELAY) == pdTRUE) {
        // servo_in.attach(servoVao_PIN);
        // servo_out.attach(servoRa_PIN);
        servo_in.write(171);
        servo_out.write(82);
        // vTaskDelay(300 / portTICK_PERIOD_MS);
        // servo_in.detach();
        // servo_out.detach();
        xSemaphoreGive(xMutexServo);
      }

      // digitalWrite(buzzer_PIN, HIGH);
      if (push_data_to_firebase() == 4) {
        modeDisplay = 5;
      } else {
        modeDisplay = 1;
      }
    }
    // xQueueSend(xQueue, &modeDisplay, (TickType_t)0);
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

void TaskBarrierControl(void *pvParameters) {
  while (1) {
    Serial.println("Task barie.....................");
    /* Điều khiển đóng mở barie bằng nút bấm */
    if (digitalRead(btnRa_PIN) == 0) {
      modeDisplay = 3;
      // xQueueSend(xQueue, &modeDisplay, (TickType_t)0);
      gate_in();
    }

    if (digitalRead(btnVao_PIN) == 0) gate_out();

    if (Serial.available()) {
      char key = Serial.read();

      if (key == '1') {
        modeDisplay = 3;
        // xQueueSend(xQueue, &modeDisplay, (TickType_t)0);
        gate_in();
      }
      if (key == '2') gate_out();
    }
    vTaskDelay(15 / portTICK_PERIOD_MS);  // Delay 10ms
  }
}

void TaskDisplayLCD(void *pvParameters) {
  // if (xQueue == NULL) {
  //   xQueue = xQueueCreate(1, sizeof(byte));
  // }

  while (1) {
    Serial.println("Task display....................");
    if (xSemaphoreTake(xMutexDisplay, (TickType_t)10) == pdTRUE) {
      display(modeDisplay);
      Serial.println("Done show LCD.~!!!!!!");
      xSemaphoreGive(xMutexDisplay);
    }
    vTaskDelay(1200 / portTICK_PERIOD_MS);
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
  // vTaskDelay(250 / portTICK_PERIOD_MS);
  // servo_in.detach();
  // servo_out.detach();
}

void gate_in() {
  if (xSemaphoreTake(xMutexServo, portMAX_DELAY) == pdTRUE) {
    // servo_in.attach(servoVao_PIN);
    servo_in.write(85);

    for (int i = 0; i < 2; i++) {
      digitalWrite(buzzer_PIN, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(buzzer_PIN, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    servo_in.write(171);
    // vTaskDelay(300 / portTICK_PERIOD_MS);
    // servo_in.detach();
    xSemaphoreGive(xMutexServo);
  }
}

void gate_out() {
  if (xSemaphoreTake(xMutexServo, portMAX_DELAY) == pdTRUE) {
    // servo_out.attach(servoRa_PIN);
    servo_out.write(0);

    for (int i = 0; i < 2; i++) {
      digitalWrite(buzzer_PIN, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(buzzer_PIN, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    servo_out.write(82);
    // vTaskDelay(250 / portTICK_PERIOD_MS);
    // servo_in.detach();
    xSemaphoreGive(xMutexServo);
  }
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
      vTaskDelay(4000 / portTICK_PERIOD_MS);
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

// // Function to generate buzzer sound
// void generateTone(unsigned int frequency, unsigned long duration) {
//   unsigned long period = 1000000 / frequency;  // Calculate the period of one cycle in microseconds
//   unsigned long startTime = micros();          // Get the current time

//   while (micros() - startTime < duration * 1000) {  // Convert duration to microseconds
//     digitalWrite(buzzer_PIN, HIGH);                        // Turn on the buzzer
//     delayMicroseconds(period / 2);                  // Wait for half of the period
//     digitalWrite(buzzer_PIN, LOW);                         // Turn off the buzzer
//     delayMicroseconds(period / 2);                  // Wait for the other half of the period
//   }
// }

// // Function to generate access granted buzzer sound
// void barrierOpenSound() {
//   int frequency = 1400;
//   int duration = 100;
//   generateTone(frequency, duration);  // Frequency, duration
//   vTaskDelay(50 / portTICK_PERIOD_MS);     // Pause between tones
//   generateTone(frequency, duration);
//   vTaskDelay(50 / portTICK_PERIOD_MS);
// }

// // Function to generate access denied buzzer sound
// void accessDeniedSound() {
//   int frequency = 400;
//   int duration = 100;
//   generateTone(frequency, duration);  // Frequency, duration
//   vTaskDelay(50 / portTICK_PERIOD_MS);     // Pause between tones
//   generateTone(frequency, duration);
//   vTaskDelay(50 / portTICK_PERIOD_MS);
// }
