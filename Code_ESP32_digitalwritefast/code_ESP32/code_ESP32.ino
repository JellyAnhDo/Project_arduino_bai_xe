#include <SPI.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseESP32.h>
#include <digitalWriteFast.h>

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
// #define WIFI_SSID "HoangAnhHN"
// #define WIFI_PASSWORD "Hoanganh2002"
#define WIFI_SSID "OPPOK3"
#define WIFI_PASSWORD "hoanganhk3"

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



void setup() {
  pinModeFast(buzzer_PIN, OUTPUT);
  pinModeFast(btnVao_PIN, INPUT_PULLUP);
  pinModeFast(btnRa_PIN, INPUT_PULLUP);
  pinModeFast(servoVao_PIN, OUTPUT);
  pinModeFast(servoRa_PIN, OUTPUT);
  pinModeFast(light_PIN, OUTPUT);
  pinModeFast(lightSensor_PIN, INPUT);
  pinModeFast(fireSensor_PIN, INPUT);
  pinModeFast(slot1, INPUT);
  pinModeFast(slot2, INPUT);
  pinModeFast(slot3, INPUT);
  pinModeFast(slot4, INPUT);



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

  sensor();
  pushSensorDataToFirebase();


  // Điều khiển đèn
  lightSensor = digitalReadFast(lightSensor_PIN);
  Firebase.getInt(firebaseData, "/Light/LightBtn");
  int lightBtn = firebaseData.intData();

  if(lightBtn == 1) {
    digitalWriteFast(light_PIN, LOW);
  } else {
    digitalWriteFast(light_PIN, HIGH);
    Firebase.getInt(firebaseData, "/Light/LightAuto");
    int lightBtnAuto = firebaseData.intData();
    if(lightBtnAuto == 1) {
      if (lightSensor == HIGH) {
        digitalWriteFast(light_PIN, LOW);
      } else {
        digitalWriteFast(light_PIN, HIGH);
      }
    }
  }


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
  if (digitalReadFast(btnVao_PIN) == 0) {
    barrierVao_open();
  }else{
    myservo_vao.write(172);
  }

  // Ấn nút cửa ra
  if (digitalReadFast(btnRa_PIN) == 0) {
    barrierRa_open();
  }else {
    myservo_ra.write(82);
  }

}

// Ham dong mo barier
void barrierVao_open() {   
  // Serial.println("mocua");
  myservo_vao.write(85);
  LCD_TRUE();
  for(int i=0;i<2;i++) {
    digitalWriteFast(buzzer_PIN,LOW);       
    delay(100); 
    digitalWriteFast(buzzer_PIN,~LOW);
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
    digitalWriteFast(buzzer_PIN,LOW);       
    delay(100); 
    digitalWriteFast(buzzer_PIN,~LOW);
    delay(100);  
  } 
  delay(5000);
  myservo_ra.write(82);
}

// Ham cam bien
void sensor() {
  s1 = digitalReadFast(slot1);
  s2 = digitalReadFast(slot2);
  s3 = digitalReadFast(slot3);
  s4 = digitalReadFast(slot4);
  fireSensor = digitalReadFast(fireSensor_PIN);

  if (fireSensor == LOW) {
    LCD_BAO_CHAY();
    myservo_vao.write(85);
    myservo_ra.write(0);
    digitalWriteFast(buzzer_PIN, LOW);
  } else {
    myservo_vao.write(172);
    myservo_ra.write(82);
    digitalWriteFast(buzzer_PIN, HIGH);
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
  Firebase.setInt(firebaseData, "/ParkingSpace/Space1", a);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space2", b);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space3", c);
  Firebase.setInt(firebaseData, "/ParkingSpace/Space4", d);
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
