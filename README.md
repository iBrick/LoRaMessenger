# LoRaMessenger
LoRaWAN messenger device for text based communication between LoRaWAN users.

GUI - based on STM32F429 discovery board and emWin library from STM and Segger.
LoRaWAN - IMST im880A(B) module with default firmware

Message format - <id>@<message>
  where <id> - id of user, number from 0 to 999 (0 - default receiver, 998 - send to group, 999 - broadcast)
  <message> - text of message

Out of scope of this repository - server side software for distribution messages between users. 
