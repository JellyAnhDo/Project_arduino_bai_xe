#include <SPI.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseESP32.h>


// Khai bao cac chan
#define buzzer_PIN       2
#define btnVao_PIN       4
#define btnRa_PIN        5
#define servoVao_PIN     23
#define servoRa_PIN      17
#define light_PIN        18
#define lightSensor_PIN  19
#define fireSensor_PIN   16
#define slot1            34
#define slot2            35
#define slot3            36
#define slot4            39

LiquidCrystal_I2C lcd(0x27, 16, 2);


// wifi infor
#define WIFI_SSID "HoangAnhHN"
#define WIFI_PASSWORD "Hoanganh2002"

// Firebase infor
#define FIREBASE_HOST "carparking-4ce1e-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "vjLt9l2OmXNWUaXyieAUfC4hV49gR6iRAH16s4Nl"

FirebaseData firebaseData;

// unsigned long lastUpdateTime = 0;
// const unsigned long updateInterval = 5000; // milliseconds

Servo myservo_vao, myservo_ra;
char sv;

int lightSensor, fireSensor = HIGH;
int s1, s2, s3, s4;

int a = 0, b = 0, c = 0, d = 0;
int count = 0;

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// define two tasks for Blink & AnalogRead
void TaskSensor( void *pvParameters );
void TaskControlBarie( void *pvParameters );

void setup() {
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

  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Khởi tạo kết nối Firebase
  Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");


  myservo_vao.attach(servoVao_PIN);
  myservo_ra.attach(servoRa_PIN);
  myservo_vao.write(172);
  myservo_ra.write(82);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  delay(250);
  LCD();
  delay(250);

  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    TaskSensor
    ,  "TaskSensor"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskControlBarie
    ,  "TaskControlBarie"
    ,  1024  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
}

void TaskSensor(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  while (true) // A Task shall never return or exit.
  {
    sensor();
    pushSensorDataToFirebase();
    // Điều khiển đèn
    lightSensor = digitalRead(lightSensor_PIN);
    Firebase.getInt(firebaseData, "/Light/LightBtn");
    int lightBtn = firebaseData.intData();

    if(lightBtn == 1) {
      digitalWrite(light_PIN, LOW);
    } else {
      digitalWrite(light_PIN, HIGH);
      Firebase.getInt(firebaseData, "/Light/LightAuto");
      int lightBtnAuto = firebaseData.intData();
      if(lightBtnAuto == 1) {
        if (lightSensor == HIGH) {
          digitalWrite(light_PIN, LOW);
        } else {
          digitalWrite(light_PIN, HIGH);
        }
      }
    }

    vTaskDelay(100);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskControlBarie(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  while (true) {
    //Đọc dữ liệu gửi từ hệ thống để đóng mở cổng
    // Vòng lặp đọc dữ liệu cổng
    while (Serial.available()) {
      sv = Serial.read();
      Serial.println(sv);
      delay(200);
    }

    if (sv == '1') {
      barrierVao_open();
    } else {
      if (sv == '2') {
        barrierRa_open();
        } else {
          myservo_ra.write(82);
      }
      myservo_vao.write(172);
    }

    sv = '0';

    // Điều khiển đóng mở barie trong trường hợp nhận diện biển số lỗi hoặc hệ thống bị lỗi
    // Ấn nút cửa vào
    if (digitalRead(btnVao_PIN) == 0) {
      barrierVao_open();
    }else{
      myservo_vao.write(172);
    }

    // Ấn nút cửa ra
    if (digitalRead(btnRa_PIN) == 0) {
      barrierRa_open();
    }else {
      myservo_ra.write(82);
    }
    vTaskDelay(100);  // one tick delay (15ms) in between reads for stability
  }
}

void loop() {
  // Kiểm tra và tái kết nối Firebase nếu mất kết nối
  // if (Firebase.failed()) {
  //   // Firebase connect
  //   Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
  //   Firebase.reconnectWiFi(true);
  //   Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //   Firebase.setwriteSizeLimit(firebaseData, "tiny");
  // }

}

// Ham dong mo barier
void barrier_open(Servo myservo,int value_open,int value_close) {   
  // Serial.println("mocua");
  myservo.write(value_open);
  LCD_TRUE();
  for(int i=0;i<2;i++) {
    digitalWrite(buzzer_PIN,LOW);       
    delay(100); 
    digitalWrite(buzzer_PIN,~LOW);
    delay(100);  
  } 
  delay(5000);
  myservo.write(value_close);
  LCD();
}

void barrierVao_open() {   
  // Serial.println("mocua");
  myservo_vao.write(85);
  LCD_TRUE();
  for(int i=0;i<2;i++) {
    digitalWrite(buzzer_PIN,LOW);       
    delay(100); 
    digitalWrite(buzzer_PIN,~LOW);
    delay(100);  
  } 
  delay(5000);
  myservo_vao.write(172);
  LCD();
}

void barrierRa_open() {   
  // Serial.println("mocua");
  myservo_ra.write(0);
  for(int i=0;i<2;i++) {
    digitalWrite(buzzer_PIN,LOW);       
    delay(100); 
    digitalWrite(buzzer_PIN,~LOW);
    delay(100);  
  } 
  delay(5000);
  myservo_ra.write(82);
}

// Ham cam bien
void sensor() {
  s1 = digitalRead(slot1);
  s2 = digitalRead(slot2);
  s3 = digitalRead(slot3);
  s4 = digitalRead(slot4);
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
  if (s1 == LOW) {
    a = 1;
  } else {
    a = 0;
  }

  if (s2 == LOW) {
    b = 1;
  } else {
    b = 0;
  }

  if (s3 == LOW) {
    c = 1;
  } else {
    c = 0;
  }

  if (s4 == LOW) {
    d = 1;
  } else {
    d = 0;
  }

  count = a + b + c + d;
  return count;
}

// Push dữ liệu cảm biến lên firebase
void pushSensorDataToFirebase() {
  Firebase.setInt(firebaseData, "/FireSensor", fireSensor);
  Firebase.setInt(firebaseData, "/ParkingSpace/ParkingSpaceCount", parking_space_check());
  Firebase.setInt(firebaseData, "/ParkingSpace/Space1", s1);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space2", s2);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space3", s3);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space4", s4);
}

// Cac hàm hiển thị thông báo lên LCD
void LCD() {
    lcd.setCursor(0,0);   
    lcd.print("     Welcome    ");
    lcd.setCursor(0,1);
    lcd.print("Check Your Card ");
}

void LCD_FALSE(){
    lcd.clear();
    lcd.setCursor(3,0);
    lcd.print("Wrong card");
    lcd.setCursor(0,1);
    lcd.print("Check Again");
}

void LCD_TRUE() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1    2    3    4"); 
  if(s1==LOW) { // co xe
    lcd.setCursor(0,1);
    lcd.print("X    "); 
  } else {
    lcd.setCursor(0,1);
    lcd.print("V    "); 
  }

  if(s2==LOW) { // co xe   
    lcd.setCursor(5,1);
    lcd.print("X    "); 
  } else {
    lcd.setCursor(5,1);
    lcd.print("V    ");
  }

  if(s3==LOW) {// co xe
      lcd.setCursor(10,1);
      lcd.print("X    "); 
  } else {
    lcd.setCursor(10,1);
    lcd.print("V    "); 
  }

  if(s4==LOW) { // co xe
    lcd.setCursor(15,1);
    lcd.print("X"); 
  } else {
    lcd.setCursor(15,1);
    lcd.print("V"); 
  }
}

void LCD_BAO_CHAY(){
    lcd.setCursor(0,0);
    lcd.print("*** Warning ****");
    lcd.setCursor(0,1);
    lcd.print("Canh Bao Co Chay");
}
