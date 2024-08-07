#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

//Chan thiet bi dieu khien
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
}

void loop() {

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
      return;
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
      return;
//String content= "";
  // Show some details of the PICC (that is: the tag/card)

  Serial.print("ci");
  Serial.print(mfrc522.uid.uidByte[0], HEX);
  Serial.print(mfrc522.uid.uidByte[1], HEX);
  Serial.print(mfrc522.uid.uidByte[2], HEX);
  Serial.println(mfrc522.uid.uidByte[3], HEX); 
  //Serial.print(mfrc522.uid.uidByte[4], HEX);

  mfrc522.PICC_HaltA();
}