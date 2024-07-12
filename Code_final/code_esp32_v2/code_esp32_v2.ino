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
TaskHandle_t xTaskManualBarrierControl;
TaskHandle_t xTaskAutomaticBarrierControl;

SemaphoreHandle_t xMutexDisplay, xMutexServo;

//=========================== Define tasks and functions =====================
void init_system();            // Pin mode for pins, connect internet and firebase
void display(byte mode);       // Display on LCD with 4 modes: stand by, show parking space, warning
void gate_in();                // Control gate in: false to close - true to open
void gate_out();               // Control gate out: false to close - true to open
byte push_data_to_firebase();  // Push data (avaiable parking spaces) and count available spaces

//=========================== Tasks ========================================
void TaskSensor(void *pvParameters);
void TaskManualBarrierControl(void *pvParameters);
void TaskAutomaticBarrierControl(void *pvParameters);

void setup() {
  Serial.begin(9600);
  init_system();
  //display(1);  // Stand by mode

  xMutexDisplay = xSemaphoreCreateMutex();
  if (xMutexDisplay == NULL) {
    Serial.println("Failed to create Display Mutex");
  }

  xMutexServo = xSemaphoreCreateMutex();
  if (xMutexServo == NULL) {
    Serial.println("Failed to create Servo Mutex");
  }


  // Create tasks for 2 cores
  xTaskCreatePinnedToCore(TaskSensor, "TaskSensor", 20000, NULL, 0, &xTaskSensor, 0);  //tskIDLE_PRIORITY
  xTaskCreatePinnedToCore(TaskManualBarrierControl, "TaskManualBarrierControl", 2048, NULL, 1, &xTaskManualBarrierControl, 1);
  xTaskCreatePinnedToCore(TaskAutomaticBarrierControl, "TaskAutomaticBarrierControl", 2048, NULL, 1, &xTaskAutomaticBarrierControl, 1);
}
void loop() {
  vTaskDelete(NULL);
}

void TaskSensor(void *pvParameters) {
  // TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  // TIMERG0.wdt_feed = 1;
  // TIMERG0.wdt_wprotect = 0;
  // disableCore0WDT();
  while (1) {
    Serial.println("Task sensor...................");
    // Check fire sensor
    if (digitalRead(fireSensor_PIN) == LOW) {
      if (xSemaphoreTake(xMutexDisplay, (TickType_t)10) == pdTRUE) {
        display(4);
        xSemaphoreGive(xMutexDisplay);
      }
      if (xSemaphoreTake(xMutexServo, portMAX_DELAY) == pdTRUE) {
        servo_in.write(85);
        servo_out.write(0);
        xSemaphoreGive(xMutexServo);
      }

      digitalWrite(buzzer_PIN, LOW);
    } else {
      if (xSemaphoreTake(xMutexServo, portMAX_DELAY) == pdTRUE) {
        servo_in.write(171);
        servo_out.write(82);
        xSemaphoreGive(xMutexServo);
      }
      digitalWrite(buzzer_PIN, HIGH);
      if (push_data_to_firebase() == 4) {
        if (xSemaphoreTake(xMutexDisplay, (TickType_t)10) == pdTRUE) {
          display(5);
          xSemaphoreGive(xMutexDisplay);
        }
      } else {
        if (xSemaphoreTake(xMutexDisplay, (TickType_t)10) == pdTRUE) {
          display(1);
          Serial.println("Reset LCD into mode 1.............");
          xSemaphoreGive(xMutexDisplay);
        }
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
          digitalWrite(light_PIN, digitalRead(lightSensor_PIN) == HIGH ? LOW : HIGH);
        }
      }
    }
    Firebase.setInt(firebaseData, "/FireSensor", digitalRead(fireSensor_PIN));



    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay 10ms
  }
}

void TaskManualBarrierControl(void *pvParameters) {
  while (1) {
    Serial.println("Task manual barie.....................");
    if (digitalRead(btnRa_PIN) == 0) {
      if (xSemaphoreTake(xMutexDisplay, (TickType_t)10) == pdTRUE) {
        display(3);
        gate_in();
        xSemaphoreGive(xMutexDisplay);
      }
    }

    if (digitalRead(btnVao_PIN) == 0) gate_out();

    vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay 100ms
  }
}

void TaskAutomaticBarrierControl(void *pvParameters) {
  while (1) {
    Serial.println("Task auto barie....................");
    if (Serial.available()) {
      char key = Serial.read();

      if (key == '1') {
        if (xSemaphoreTake(xMutexDisplay, (TickType_t)10) == pdTRUE) {
          display(3);
          gate_in();
          xSemaphoreGive(xMutexDisplay);
        }
      }
      if (key == '2') gate_out();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
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
  if (xSemaphoreTake(xMutexServo, portMAX_DELAY) == pdTRUE) {
    servo_in.write(85);

    for (int i = 0; i < 2; i++) {
      digitalWrite(buzzer_PIN, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(buzzer_PIN, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    servo_in.write(171);
    xSemaphoreGive(xMutexServo);
  }
}

void gate_out() {
  if (xSemaphoreTake(xMutexServo, portMAX_DELAY) == pdTRUE) {
    servo_out.write(0);

    for (int i = 0; i < 2; i++) {
      digitalWrite(buzzer_PIN, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(buzzer_PIN, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    servo_out.write(82);
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
