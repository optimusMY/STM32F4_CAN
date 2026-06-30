#include <SPI.h>
#include "mcp2515.h"

#define CS_PIN 10

struct can_frame canMsg;
MCP2515 mcp2515(CS_PIN);


void setup() {
  Serial.begin(115200);
  
  mcp2515.reset();
  Serial.println("CANBUS reset done.");
  //mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);
  Serial.println("CANBUS speed = 125KBPS");
  mcp2515.setNormalMode();
  Serial.println("CANBUS mode = NORMAL");
  Serial.println("CANBUS init done.");
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");
    
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i],HEX);
      Serial.print(" ");
    }

    Serial.println();      
  }
}
