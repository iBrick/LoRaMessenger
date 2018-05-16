# LoRaMessenger
LoRaWAN messenger device for text based communication between LoRaWAN users.

GUI - based on STM32F429 discovery board and emWin library from STM and Segger.
LoRaWAN - IMST im880A(B) module with default firmware

Message format - <id>@<message>
  where <id> - id of user, number from 0 to 999 (0 - default receiver, 998 - send to group, 999 - broadcast)
  <message> - text of message

Out of scope of this repository - server side software for distribution messages between users. 

Options:
1. To turn display in portrait mode in file "tm_stm32f4_emwin.h" change define TM_EMWIN_ROTATE_LCD to 1 instead 0; 

2. IMST UART - Discovery pin connections
  TX -------------------------PD2
  RX--------------------------PC12
 GND------------------------GND

3. New Discovery board have an error in programmer which prevents to use it without connection to PC. To fix it do the following:
- open STM32 ST-Link Utility -> Target -> Settings -> Connection Settings -> Port -> set JTAG and press ОК.
- disconnect board, open menu ST-Link -> Firmware Update -> connect board -> Device Connect -> Yes
- open again Target -> Settings -> Connection Settings -> Port -> set SWD and press ОК.
- все, новая прошивка работает.
