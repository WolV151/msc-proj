// RFM69HCW Example Sketch
// Send serial input characters from one RFM69 node to another
// Based on RFM69 library sample code by Felix Rusu
// http://LowPowerLab.com/contact
// Modified for RFM69HCW by Mike Grusin, 4/16

// This sketch will show you the basics of using an
// RFM69HCW radio module. SparkFun's part numbers are:
// 915MHz: https://www.sparkfun.com/products/12775
// 434MHz: https://www.sparkfun.com/products/12823

// See the hook-up guide for wiring instructions:
// https://learn.sparkfun.com/tutorials/rfm69hcw-hookup-guide

// Uses the RFM69 library by Felix Rusu, LowPowerLab.com
// Original library: https://www.github.com/lowpowerlab/rfm69
// SparkFun repository: https://github.com/sparkfun/RFM69HCW_Breakout

// Include the RFM69 and SPI libraries:

#include <RFM69.h>
#include <SPI.h>

// Addresses for this node. CHANGE THESE FOR EACH NODE!

#define NETWORKID     100   // Must be the same for all nodes
#define MYNODEID      1   // My node ID
#define TONODEID      2   // Destination node ID

// RFM69 frequency, uncomment the frequency of your module:

#define FREQUENCY   RF69_433MHZ
// #define FREQUENCY     RF69_915MHZ

// AES encryption (or not):

#define ENCRYPT       true // Set to "true" to use encryption
#define ENCRYPTKEY    "abcdefghijklmnop" // Use the same 16-byte key on all nodes

// Use ACKnowledge when sending messages (or not):

#define USEACK        true // Request ACKs or not
#define IS_RFM69HCW   true

// Packet sent/received indicator LED (optional):

#define LED           9 // LED positive pin
#define GND           8 // LED ground pin

// Create a library object for our RFM69HCW module:

#define RFM69_CS      10
#define RFM69_IRQ     2
#define RFM69_IRQN    1  // Pin 7 is IRQ 4!
#define RFM69_RST     9

char buff[256];
uint8_t input = 0;
size_t bytes_read = 0;

RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);

void setup()
{
  // Open a serial port so we can send keystrokes to the module:

  Serial.begin(9600);
  Serial.print("Node ");
  Serial.print(MYNODEID,DEC);
  Serial.println(" ready");  

  // Set up the indicator LED (optional):
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  // Initialize the RFM69HCW:
  // radio.setCS(10);  //uncomment this if using Pro Micro
  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower(); // Always use this for RFM69HCW
  // radio.setFrequency(433000000);
  // Turn on encryption if desired:

  if (ENCRYPT)
    radio.encrypt(ENCRYPTKEY);
}

void loop()
{
  // SENDING

  // In this section, we'll gather serial characters and
  // send them to the other node if we (1) get a carriage return,
  // or (2) the buffer is full (61 characters).

  // If there is any serial input, add it to the buffer:

  if (Serial.available() > 0)
  {

    bytes_read = Serial.readBytes(buff, 60);
    radio.sendWithRetry(TONODEID, (uint8_t*)buff, bytes_read);
    memset(buff, 0, 60);
    bytes_read = 0;    

    // If the input is a carriage return, or the buffer is full
  }



  // RECEIVING

  // In this section, we'll check with the RFM69HCW to see
  // if it has received any packets:

//   if (radio.receiveDone()) // Got one!
//   {
//     // Print out the information:

//     Serial.print("received from node ");
//     Serial.print(radio.SENDERID, DEC);
//     Serial.print(", message [");

//     // The actual message is contained in the DATA array,
//     // and is DATALEN bytes in size:

//     for (byte i = 0; i < radio.DATALEN; i++)
//       Serial.print((char)radio.DATA[i]);

//     // RSSI is the "Receive Signal Strength Indicator",
//     // smaller numbers mean higher power.

//     Serial.print("], RSSI ");
//     Serial.println(radio.RSSI);

//     // Send an ACK if requested.
//     // (You don't need this code if you're not using ACKs.)

//     if (radio.ACKRequested())
//     {
//       radio.sendACK();
//       Serial.println("ACK sent");
//     }
//   //   Blink(LED,10);
//   // }
//   }
}