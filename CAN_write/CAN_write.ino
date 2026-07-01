#include <SPI.h>
#include "mcp2515.h"
#define CS_PIN 10

struct can_frame canMsg1;
//struct can_frame canMsg2;
MCP2515 mcp2515(CS_PIN); 

int count = 0;

void setup() {
  
  while (!Serial);
  Serial.begin(9600);
  
  mcp2515.reset();
  Serial.println("CANBUS reset done.");
  
  //mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);
  Serial.println("CANBUS speed = 125KBPS");
  
  mcp2515.setNormalMode();
  Serial.println("CANBUS mode = NORMAL");
  
  Serial.println("CANBUS init done.");
}

void loop() 
{
  count++;

  if (count ==255)
  count =0;

  
  canMsg1.can_id  = 0x244; //Transmitter node ID
  canMsg1.can_dlc = 8;      // Data Length num of bytes in data section
  canMsg1.data[0] = 0xAB;   //Data byte1
  canMsg1.data[1] = 0xCD;   //Data byte2
  canMsg1.data[2] = 0xEF;   //Data byte3
  canMsg1.data[3] = 0x00;   //Data byte4
  canMsg1.data[4] = 0x11;   //Data byte5
  canMsg1.data[5] = 0x22;   //Data byte6
  canMsg1.data[6] = 0x33;      //Data byte7
  canMsg1.data[7] = (uint8_t)count ; //Data byte8
  
  mcp2515.sendMessage(&canMsg1);
  //mcp2515.sendMessage(&canMsg2);

  Serial.println(count); //show count val on console
  
  delay(1000);
}
