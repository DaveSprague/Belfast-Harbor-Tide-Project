// adapted from Adafruit Feather M0 LoRa Sample TX code
// by David L. Sprague, Nov 5, 2018

// Uses code for non-blocking serial reading from user Robin2 here:
//    https://forum.arduino.cc/index.php?topic=396450

// Reads data from a pressure based water depth sensor from Don Blair
// When power is supplied to this sensor it sends Temperature and Pressure in millibars
// On a serial TX line at 9600 baud.
// Temperature is sent in degrees C with two digits to the right of the decimal point
//   which should mean a maximum of six characters for the Temperature reading including
//   a minus sign ('-'), a decimal point, and at most two digits to the left of the
//   decimal point (right?)

// A comma separates the Temperature from the Pressure reading (no space character after
//   after the comma).

// The Pressure reading has two digits to the right the decimal point and up to four digits
// to the left of the decimal point.  This number can never be negative so the maximum
//   number of characters should be seven including six digits and the decimal point.

// We start each LoRa packet with a letter 'M' for message followed by a 3 digit sequence
// count that's used to detect packet loss

// Therefore, the maximum length of each transmission, not counting the new line character
//   ('\n') at the end should be 6 + 1 + 7 = 14.

#include <Wire.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <avr/dtostrf.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// BME280 Temp Humid Pressure sensor
Adafruit_BME280 bme; // I2C

// Battery voltage pin
#define VBATPIN A7      // measures Vbatt / 2

const byte numChars = 15;
char receivedChars[numChars];  // buffer for chars coming from depth sensor

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(9600);
  Serial1.begin(9600); // from Depth sensor
  bme.begin();
  delay(100);

  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop()
{

  // TODO: add local pressure sensor and local Ultrasonic sensor
    
    if (recvdWithEndMarkers('\n')) {
      
      char * strtokIndx; // this is used by strtok() as an index
      strtokIndx = strtok(receivedChars, ","); // this continues where the previous call left off
      float depthTemp = atof(strtokIndx);     // convert this part to an integer

      strtokIndx = strtok(NULL, ",");
      float depthPress = atof(strtokIndx);     // convert this part to a float
  
      Serial.print("Temperature: ");
      Serial.print(depthTemp);
      Serial.print(", Pressure: ");
      Serial.println(depthPress);

      // read atmospheric pressure from BME280
      float surfacePress = bme.readPressure() / 100.00F;
      float surfaceTemp = bme.readTemperature();

      float measuredvbat = analogRead(VBATPIN);
      measuredvbat *= 2;    // we divided by 2, so multiply back
      measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
      measuredvbat /= 1024; // convert to voltage
      Serial.print("VBat: " ); Serial.println(measuredvbat);
  
      Serial.println("Transmitting..."); // Send a message to rf95_server
      
      char radiopacket[43];
      sprintf(radiopacket, "M%03d,                                   \x00", packetnum++);
      if (packetnum >= 1000) {
        packetnum = 0;
      }

      int pos = 5;
      dtostrf(depthTemp, 6, 2, &radiopacket[pos]); pos += 6;
      radiopacket[pos] = ','; pos += 1;
      dtostrf(depthPress, 7, 2, &radiopacket[pos]); pos += 7;
      radiopacket[pos] = ','; pos += 1;
      dtostrf(surfaceTemp, 6, 2, &radiopacket[pos]); pos += 6;
      radiopacket[pos] = ','; pos += 1;
      dtostrf(surfacePress, 7, 2, &radiopacket[pos]); pos += 7;
      radiopacket[pos] = ','; pos += 1;
      dtostrf(measuredvbat, 5, 2, &radiopacket[pos]); pos += 5;
      radiopacket[pos] = '\n'; pos += 1;
      radiopacket[pos] = 0; pos += 1;
      
      Serial.print("Sending ");
      Serial.print(pos);Serial.print(" ");
      Serial.println(radiopacket);
      delay(10);
      rf95.send((uint8_t *)radiopacket, pos);
    
      Serial.println("Waiting for packet to complete..."); 
      delay(10);
      rf95.waitPacketSent();
    }
}

bool recvdWithEndMarkers(char endMarker) {
  // this function is based on code from Robin2 on Arduino Forums
  // it implements a nonblocking, character by character read of a serial port
    static byte ndx = 0;
    char rc;
    bool newData = false;
    while (Serial1.available() > 0) {
        rc = Serial1.read();

          if (rc != endMarker) {
              receivedChars[ndx] = rc;
              ndx++;
              if (ndx >= numChars) {
                  ndx = numChars - 1;
              }
          }
          else {
              receivedChars[ndx] = '\0'; // terminate the string
              ndx = 0;
              newData = true;
          }

    }
    return newData;
}
