//=================== Library  ====================================
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <semphr.h>

//=================================================================
TaskHandle_t xTaskReadSensor;
TaskHandle_t xTaskManualControlBarrier;
TaskHandle_t xTaskAutoControlBarrier;
TaskHandle_t xTaskReadRFID;
TaskHandle_t xTaskReadESP;

SemaphoreHandle_t xBinarySemaphore;
// QueueHandle_t queue;

//===================Define task===================================
void TaskReadSensor(void *pvParameters);
void TaskManualControlBarrier(void *pvParameters);
void TaskAutoControlBarrier(void *pvParameters);
void TaskReadRFID(void *pvParameters);
void TaskReadESP(void *pvParameters);

LiquidCrystal_I2C lcd(0x27, 16, 2);

//======================= Servo ===================================
Servo myservo_vao;
int sv;

//====================== Chan cam bien ============================
int lightSensor, fireSensor = HIGH;
int s1;
int s2;
int s3;
int s4;

//======================= Chan thiet bi dieu khien=================
#define buzzer_PIN 2
#define btnVao_PIN 3
#define servo_PIN 4
#define light_PIN 6
#define lightSensor_PIN 7
#define fireSensor_PIN 8

//======================= Bien diem so luong xe ====================
int a = 0, b = 0, c = 0, d = 0;
int count = 0;

//======================= Milis ====================================


//======================= RFID =====================================
#define RST_PIN 9                  // Configurable, see typical pin layout above
#define SS_PIN 10                  // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

void setup() {
  pinMode(buzzer_PIN, OUTPUT);
  pinMode(btnVao_PIN, INPUT_PULLUP);
  pinMode(servo_PIN, OUTPUT);
  pinMode(light_PIN, OUTPUT);
  pinMode(lightSensor_PIN, INPUT);
  pinMode(fireSensor_PIN, INPUT);
  //    pinMode(SW_PIN, INPUT);

  Serial.begin(9600);  // Initialize serial communications with the PC
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  myservo_vao.attach(servo_PIN);
  myservo_vao.write(172);

  // last = millis();
  // scanCard = millis();
  // openBarrier = millis();

  lcd.init();  // initialize the lcd
  lcd.backlight();
  lcd.clear();

  // delay(250);
  LCD();
  // delay(250);

  while (!Serial)
    ;  // Do nothing if no serial port is opened

  if (xBinarySemaphore == NULL) {
    xBinarySemaphore = xSemaphoreCreateBinary();
    if (xBinarySemaphore != NULL)
      xSemaphoreGive(xBinarySemaphore);
  }

  //==================== Các task ================================
  xTaskCreate(TaskReadSensor, "TaskReadSensor", 128, NULL, 1, &xTaskReadSensor);                                //50ms
  // xTaskCreate(TaskReadRFID, "TaskReadRFID", 128, NULL, 1, &xTaskReadRFID);                                      // 1282ms
  xTaskCreate(TaskManualControlBarrier, "TaskManualControlBarrier", 128, NULL, 1, &xTaskManualControlBarrier);  //Ko hoạt động 1ms, hđ: 5970ms
  // xTaskCreate(TaskAutoControlBarrier, "TaskAutoControlBarrier", 128, NULL, 1, &xTaskAutoControlBarrier);
  // xTaskCreate(TaskReadESP, "TaskReadESP", 128, NULL, 1, &xTaskReadESP);

  Serial.println("Start program....");

  vTaskStartScheduler();
}

void loop() {}

void TaskReadRFID(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdPASS) {
      Serial.println("Task read RFID....");

      // Look for new cards
      if (!mfrc522.PICC_IsNewCardPresent()) {
        xSemaphoreGive(xBinarySemaphore);
        continue;
      }

      // Select one of the cards
      if (!mfrc522.PICC_ReadCardSerial()) {
        xSemaphoreGive(xBinarySemaphore);
        continue;
      }

      // String content= "";
      //  Show some details of the PICC (that is: the tag/card)

      Serial.print("ci");
      for (int i = 0; i < 4; i++) {
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }

      digitalWrite(buzzer_PIN, LOW);
      vTaskDelay(50 / portTICK_PERIOD_MS);
      digitalWrite(buzzer_PIN, HIGH);
      vTaskDelay(50 / portTICK_PERIOD_MS);
      digitalWrite(buzzer_PIN, LOW);
      vTaskDelay(50 / portTICK_PERIOD_MS);
      digitalWrite(buzzer_PIN, HIGH);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      mfrc522.PICC_HaltA();

      xSemaphoreGive(xBinarySemaphore);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskAutoControlBarrier(void *pvParameters) {
  while (1) {
    Serial.println("Task auto control barie....");
    if (Serial.available()) {
      sv = Serial.read();
      Serial.println(sv);
      vTaskDelay(200 / portTICK_PERIOD_MS);
      if (sv != 0) {
        barrier_open();
      } else {
        myservo_vao.write(172);
      }
      sv = 0;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void TaskManualControlBarrier(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdPASS) {
      Serial.println("Task button....");

      if (digitalRead(btnVao_PIN) == 0) {
        LCD_TRUE();
        myservo_vao.write(85);

        for (int i = 0; i < 2; i++) {
          digitalWrite(buzzer_PIN, LOW);
          vTaskDelay(100 / portTICK_PERIOD_MS);
          digitalWrite(buzzer_PIN, HIGH);
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        myservo_vao.write(172);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        LCD();
      }
      xSemaphoreGive(xBinarySemaphore);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void TaskReadSensor(void *pvParameters) {

  while (1) {
    if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdPASS) {
      Serial.println("Task read sensor....");
      s1 = digitalRead(A0);
      s2 = digitalRead(A1);
      s3 = digitalRead(A2);
      s4 = digitalRead(A3);
      lightSensor = digitalRead(lightSensor_PIN);
      fireSensor = digitalRead(fireSensor_PIN);
      if (lightSensor == HIGH) {
        digitalWrite(light_PIN, LOW);  // batden
      } else {
        digitalWrite(light_PIN, HIGH);
      }

      if (fireSensor == LOW) {
        LCD_BAO_CHAY();
        myservo_vao.write(85);
        digitalWrite(buzzer_PIN, LOW);  // loa keu
      } else {
        myservo_vao.write(172);
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
      xSemaphoreGive(xBinarySemaphore);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void TaskReadESP(void *pvParameters) {
  //======================Read status from ESP======================
  while (1) {
    //     while (mySerial.available()) {
    //   char inChar = (char)mySerial.read();
    //   statusFromESP += inChar;
    //   if (inChar == '\n') {
    //     stringComplete = true;
    //   }
    // }
    // if (stringComplete) {
    //   //    Serial.print("Data nhận: ");
    //   //    Serial.println(statusFromESP);
    //   //==================================
    //   //Xử lý dữ liệu
    //   handleData(statusFromESP);
    //   //==================================
    //   statusFromESP = "";
    //   stringComplete = false;
    // }
    //   //    Serial.println("TaskReadESP");
    //   vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void handleData(String statusFromESP) {
  //  //Tìm vị trí kí tự
  //  int findStarChar, findOpenBracketChar = -1;
  //  findStarChar = statusFromESP.indexOf("*");
  //  findOpenBracketChar = statusFromESP.indexOf("(");
  //  //Cắt chuỗi
  //  if (findStarChar >= 0 && findOpenBracketChar >= 0) {
  //    statusFromESP =  statusFromESP.substring(findStarChar + 1, findOpenBracketChar);
  Serial.print("Data đã xử lý: ");
  Serial.println(statusFromESP);
}


void resetCommand() {
}

void barrier_open() {
  // Serial.println("mocua");
  myservo_vao.write(85);
  LCD_TRUE();
  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer_PIN, LOW);
    delay(100);
    digitalWrite(buzzer_PIN, ~LOW);
    delay(100);
  }
  delay(5000);
  myservo_vao.write(172);
}

int parking_space_check() {
  a = (s1 == LOW) ? 1 : 0;
  b = (s2 == LOW) ? 1 : 0;
  c = (s3 == LOW) ? 1 : 0;
  d = (s4 == LOW) ? 1 : 0;

  count = a + b + c + d;
  return count;
}

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
  lcd.setCursor(0, 1);
  lcd.print((digitalRead(A0) == LOW) ? "X    " : "V    ");
  lcd.setCursor(5, 1);
  lcd.print((digitalRead(A1) == LOW) ? "X    " : "V    ");
  lcd.setCursor(10, 1);
  lcd.print((digitalRead(A2) == LOW) ? "X    " : "V    ");
  lcd.setCursor(15, 1);
  lcd.print((digitalRead(A3) == LOW) ? "X" : "V");
}

void LCD_BAO_CHAY() {
  lcd.setCursor(0, 0);
  lcd.print("*** Warning ****");
  lcd.setCursor(0, 1);
  lcd.print("Canh Bao Co Chay");
}
