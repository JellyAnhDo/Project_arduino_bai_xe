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

