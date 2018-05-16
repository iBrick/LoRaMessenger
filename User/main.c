/**
 *	LoRaWAN Messenger based on emWin
 *	
 *	@author		Vladislav Sheshalevich 
 *	@packs		STM32F4xx Keil packs version 2.2.0 or greater required
 *	@stdperiph	STM32F4xx Standard peripheral drivers version 1.4.0 or greater required
 */
/* Include core modules */
#include "stm32f4xx.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_emwin.h"

/* GUI modules */
#include "button.h"
#include "DIALOG.h"
#define BUTTON_FONT_KEYBOARD  GUI_Font24_ASCII //GUI_Font20_ASCII GUI_Font20_1 GUI_Font20B_ASCII GUI_Font20B_1 1. B_asc 2. Asci 3. 20_1 4.B_1
#define BUTTON_FONT_ACTIONS GUI_Font16B_ASCII //Excepting "Send new message" in incoming window
#define EDIT_FONT GUI_Font20_1 //EDIT font
#define POLY 0x8408

#define GREENLIGHT 1<<0
#define BLUELIGHT 1<<1
#define REDLIGHT 1<<2
#define YELLOWLIGHT (REDLIGHT|GREENLIGHT)
#define NOLIGHT 0

BUTTON_SKINFLEX_PROPS Props;
 WM_HWIN hWin;
  WM_HWIN hItem;
char light_flag = NOLIGHT;

USART_InitTypeDef huart5;
uint32_t sec_counter = 0;
uint32_t blink_counter = 100000;
uint32_t buzz_counter = 100000;
char receive_flag = 0;
char rflag2=0;
char rflag3=0;
char flagtowrite = 0;
char translate_buf;
char translate_flag=0;
char radio_data[150];
const uint8_t radio_data_size = 150;
char first_message[50]; 
char second_message[50]; 
char third_message[50];
const uint8_t incoming_message_size = 50;
static void MX_UART5_Init(void);
void UART_SendPacket( char *buff, int length);
void RGBLed(char light);
//unsigned short mbCrc(unsigned char *buf, unsigned short size);
unsigned short crc16(char *data_p, unsigned short length);


 int main(void) {
	//uint8_t i;

	GUI_UC_SetEncodeUTF8(); //Accept unicode symbols
	
	/* Initialize system */
	SystemInit();
	TM_GPIO_Init(GPIOE, GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);

	/* Initialize delay functions */
	TM_DELAY_Init();
	
	/* Initialize LEDs */
	TM_DISCO_LedInit();
	
	/* Initialize emWin */
	if (TM_EMWIN_Init() != TM_EMWIN_Result_Ok) {
		/* Initialization error */
		while (1) {
			/* Toggle RED led */
			TM_DISCO_LedToggle(LED_RED);
			
			/* Delay */
			Delayms(100);
		}
	}
	TM_EMWIN_Rotate(TM_EMWIN_Rotate_90);
	BUTTON_SetDefaultSkin(BUTTON_SKIN_FLEX); //Setting nice skin for buttons

	MX_UART5_Init();
	
	//	hItem = WM_GetDialogItem(hWin, Incoming);             	// Get the handle of the button
  	//BUTTON_SetBkColor(hItem,BUTTON_CI_UNPRESSED,GUI_RED);  	// Unfortunately this is not working with flex skinning
  	BUTTON_GetSkinFlexProps(&Props, BUTTON_SKINFLEX_PI_ENABLED); // Get the default properties for an enabled button, also possible for other state
  	Props.aColorFrame[0] = GUI_DARKCYAN;                      	// Set new colors for different areas, take a look into the manual under skinning
  	Props.aColorFrame[1] = GUI_CYAN;
  	Props.aColorFrame[2] = GUI_LIGHTCYAN;
		  	Props.aColorUpper[0]=GUI_LIGHTCYAN;
				Props.aColorUpper[1]=GUI_LIGHTCYAN;
				Props.aColorLower[0]=GUI_LIGHTCYAN;
				Props.aColorLower[1]=GUI_LIGHTCYAN;
  	BUTTON_SetSkinFlexProps(&Props, BUTTON_SKINFLEX_PI_ENABLED); // Set the new fancy colors for every state (as an example)
 	BUTTON_GetSkinFlexProps(&Props, BUTTON_SKINFLEX_PI_PRESSED); // Get the default properties for an enabled button, also possible for other state
  	Props.aColorFrame[0] = GUI_DARKRED;                      	// Set new colors for different areas, take a look into the manual under skinning
  	Props.aColorFrame[1] = GUI_RED;
  	Props.aColorFrame[2] = GUI_LIGHTRED;
		  	Props.aColorUpper[0]=GUI_LIGHTRED;
				Props.aColorUpper[1]=GUI_LIGHTRED;
				Props.aColorLower[0]=GUI_LIGHTRED;
				Props.aColorLower[1]=GUI_LIGHTRED;
		BUTTON_SetSkinFlexProps(&Props, BUTTON_SKINFLEX_PI_PRESSED);
		 	BUTTON_GetSkinFlexProps(&Props, BUTTON_SKINFLEX_PI_FOCUSSED); // Get the default properties for an enabled button, also possible for other state
  	Props.aColorFrame[0] = GUI_DARKGREEN;                      	// Set new colors for different areas, take a look into the manual under skinning
  	Props.aColorFrame[1] = GUI_GREEN;
  	Props.aColorFrame[2] = GUI_LIGHTGREEN;
		  	Props.aColorUpper[0]=GUI_LIGHTGREEN;
				Props.aColorUpper[1]=GUI_LIGHTGREEN;
				Props.aColorLower[0]=GUI_LIGHTGREEN;
				Props.aColorLower[1]=GUI_LIGHTGREEN;
  	BUTTON_SetSkinFlexProps(&Props, BUTTON_SKINFLEX_PI_FOCUSSED);
  	BUTTON_SetSkinFlexProps(&Props, BUTTON_SKINFLEX_PI_DISABLED);
  	WM_Invalidate(hItem);                                    	// Maybe the most importand part, tell button something has changed, otherwise nothing would happen
	
	
		loop1:
		TM_ILI9341_Fill(ILI9341_COLOR_GRAY);
		TM_ILI9341_Rotate(TM_ILI9341_Orientation_Portrait_1);
	
		//Send button
	BUTTON_Handle hButton = BUTTON_CreateEx(10, 10, 95, 30, 0, WM_CF_SHOW, 0, 11);
	BUTTON_SetText(hButton, "Send");
	BUTTON_SetFont(hButton, &BUTTON_FONT_ACTIONS);
	BUTTON_SetBkColor(hButton, 0, GUI_LIGHTGREEN);
	
		//Fast message button
		BUTTON_Handle Fast_msg = BUTTON_CreateEx(110, 10, 95, 30, 0, WM_CF_SHOW, 0, 43);
	BUTTON_SetText(Fast_msg, "Send fast");
	BUTTON_SetFont(Fast_msg, &BUTTON_FONT_ACTIONS );
	BUTTON_SetBkColor(Fast_msg, 0, GUI_LIGHTGREEN);
	
	

	EDIT_Handle E_12 = EDIT_CreateEx(10, 40, 300, 30, 0, WM_CF_SHOW, 0, 42, 100);//VladM
	EDIT_SetText(E_12, ""); //may tape there smth in russian to debug
	EDIT_SetFont(E_12, &EDIT_FONT);
	
	//To incoming Button
	BUTTON_Handle Incoming = BUTTON_CreateEx(210, 10, 95, 30, 0, WM_CF_SHOW, 0, 41);
	BUTTON_SetText(Incoming, "Incoming");
	BUTTON_SetFont(Incoming, &BUTTON_FONT_ACTIONS);
	BUTTON_SetBkColor(Incoming, 0, GUI_LIGHTGREEN);
	

// Boris writting the table 1-9
	BUTTON_Handle hB1 = BUTTON_CreateEx(10, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON1);
	BUTTON_Handle hB2 = BUTTON_CreateEx(40, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON2);
	BUTTON_Handle hB3 = BUTTON_CreateEx(70, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON3);
	BUTTON_Handle hB4 = BUTTON_CreateEx(100, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON4);
	BUTTON_Handle hB5 = BUTTON_CreateEx(130, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON5);
	BUTTON_Handle hB6 = BUTTON_CreateEx(160, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON6);
	BUTTON_Handle hB7 = BUTTON_CreateEx(190, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON7);
	BUTTON_Handle hB8 = BUTTON_CreateEx(220, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON8);
	BUTTON_Handle hB9 = BUTTON_CreateEx(250, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON9);
	BUTTON_Handle hB0 = BUTTON_CreateEx(280, 70, 30, 40, 0, WM_CF_SHOW, 0, GUI_ID_BUTTON0);

	BUTTON_SetFont(hB1, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB2, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB3, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB4, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB5, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB6, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB7, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB8, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB9, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB0, &BUTTON_FONT_KEYBOARD);

	BUTTON_SetText(hB1, "1");
	BUTTON_SetText(hB2, "2");
	BUTTON_SetText(hB3, "3");
	BUTTON_SetText(hB4, "4");
	BUTTON_SetText(hB5, "5");
	BUTTON_SetText(hB6, "6");
	BUTTON_SetText(hB7, "7");
	BUTTON_SetText(hB8, "8");
	BUTTON_SetText(hB9, "9");
	BUTTON_SetText(hB0, "0");
	
	BUTTON_SetBkColor(hB1, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB2, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB3, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB4, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB5, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB6, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB7, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB8, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB9, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB0, 0, GUI_LIGHTCYAN);
	
	// Boris writting the table q-p
	BUTTON_Handle hBq = BUTTON_CreateEx(10, 110,  30, 40, 0, WM_CF_SHOW, 0, 12);
	BUTTON_Handle hBw = BUTTON_CreateEx(40, 110,  30, 40, 0, WM_CF_SHOW, 0, 13);
	BUTTON_Handle hBe = BUTTON_CreateEx(70, 110, 30, 40, 0, WM_CF_SHOW, 0, 14);
	BUTTON_Handle hBr = BUTTON_CreateEx(100, 110,  30, 40, 0, WM_CF_SHOW, 0, 15);
	BUTTON_Handle hBt = BUTTON_CreateEx(130, 110,  30, 40, 0, WM_CF_SHOW, 0, 16);
	BUTTON_Handle hBy = BUTTON_CreateEx(160, 110,  30, 40, 0, WM_CF_SHOW, 0, 17);
	BUTTON_Handle hBu = BUTTON_CreateEx(190, 110, 30, 40, 0, WM_CF_SHOW, 0, 18);
	BUTTON_Handle hBi = BUTTON_CreateEx(220, 110,  30, 40, 0, WM_CF_SHOW, 0, 19);
	BUTTON_Handle hBo = BUTTON_CreateEx(250, 110, 30, 40, 0, WM_CF_SHOW, 0, 20);
	BUTTON_Handle hBp = BUTTON_CreateEx(280, 110, 30, 40, 0, WM_CF_SHOW, 0, 21);

	BUTTON_SetFont(hBq, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBw, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBe, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBr, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBt, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBy, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBu, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBi, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBo, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBp, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetText(hBq, "q");
	BUTTON_SetText(hBw, "w");
	BUTTON_SetText(hBe, "e");
	BUTTON_SetText(hBr, "r");
	BUTTON_SetText(hBt, "t");
	BUTTON_SetText(hBy, "y");
	BUTTON_SetText(hBu, "u");
	BUTTON_SetText(hBi, "i");
	BUTTON_SetText(hBo, "o");
	BUTTON_SetText(hBp, "p");
	
	BUTTON_SetBkColor(hBq, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBw, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBe, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBr, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBt, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBy, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBu, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBi, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBo, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBp, 0, GUI_LIGHTCYAN);
	// Boris writting the table a-l
	BUTTON_Handle hBa = BUTTON_CreateEx(20, 150, 30, 40, 0, WM_CF_SHOW, 0, 22);
	BUTTON_SetFrameColor(hBa, GUI_WHITE);
	BUTTON_SetBkColor(hBa, 0, GUI_WHITE);
	BUTTON_Handle hBs = BUTTON_CreateEx(50, 150, 30, 40, 0, WM_CF_SHOW, 0, 23);
	BUTTON_Handle hBd = BUTTON_CreateEx(80, 150, 30, 40, 0, WM_CF_SHOW, 0, 24);
	BUTTON_Handle hBf = BUTTON_CreateEx(110, 150, 30, 40, 0, WM_CF_SHOW, 0, 25);
	BUTTON_Handle hBg = BUTTON_CreateEx(140, 150, 30, 40, 0, WM_CF_SHOW, 0, 26);
	BUTTON_Handle hBh = BUTTON_CreateEx(170, 150, 30, 40, 0, WM_CF_SHOW, 0, 27);
	BUTTON_Handle hBj = BUTTON_CreateEx(200, 150,30, 40, 0, WM_CF_SHOW, 0, 28);
	BUTTON_Handle hBk = BUTTON_CreateEx(230, 150, 30, 40, 0, WM_CF_SHOW, 0, 29);
	BUTTON_Handle hBl = BUTTON_CreateEx(260, 150, 30, 40, 0, WM_CF_SHOW, 0, 30);

	BUTTON_SetFont(hBa, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBs, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBd, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBf, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBg, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBh, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBj, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBk, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBl, &BUTTON_FONT_KEYBOARD);
	
	BUTTON_SetText(hBa, "a");
	BUTTON_SetText(hBs, "s");
	BUTTON_SetText(hBd, "d");
	BUTTON_SetText(hBf, "f");
	BUTTON_SetText(hBg, "g");
	BUTTON_SetText(hBh, "h");
	BUTTON_SetText(hBj, "j");
	BUTTON_SetText(hBk, "k");
	BUTTON_SetText(hBl, "l");
	
	BUTTON_SetBkColor(hBa, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBs, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBd, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBf, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBg, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBh, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBj, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBk, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBl, 0, GUI_LIGHTCYAN);

	//z-m, BACKSPACE AND @
	BUTTON_Handle hBz = BUTTON_CreateEx(10, 190, 30, 40, 0, WM_CF_SHOW, 0, 31);
	BUTTON_Handle hBx = BUTTON_CreateEx(40, 190, 30, 40, 0, WM_CF_SHOW, 0, 32);
	BUTTON_Handle hBc = BUTTON_CreateEx(70, 190, 30, 40, 0, WM_CF_SHOW, 0, 33);
	BUTTON_Handle hBv = BUTTON_CreateEx(100, 190,30, 40, 0, WM_CF_SHOW, 0, 34);
	BUTTON_Handle hBb = BUTTON_CreateEx(130, 190, 30, 40, 0, WM_CF_SHOW, 0, 35);
	BUTTON_Handle hBn = BUTTON_CreateEx(160, 190, 30, 40, 0, WM_CF_SHOW, 0, 36);
	BUTTON_Handle hBm = BUTTON_CreateEx(190, 190, 30, 40, 0, WM_CF_SHOW, 0, 37);
	BUTTON_Handle hB_ = BUTTON_CreateEx(220, 190, 30, 40, 0, WM_CF_SHOW, 0, 38);
	BUTTON_Handle hBdog = BUTTON_CreateEx(250, 190, 30, 40, 0, WM_CF_SHOW, 0, 39);
	BUTTON_Handle hBback = BUTTON_CreateEx(280, 190, 30, 40, 0, WM_CF_SHOW, 0, 40); 
	
	BUTTON_SetFont(hBz, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBx, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBc, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBv, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBb, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBn, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBm, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hB_, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBdog, &BUTTON_FONT_KEYBOARD);
	BUTTON_SetFont(hBback, &BUTTON_FONT_KEYBOARD);
	
	BUTTON_SetText(hBz, "z");
	BUTTON_SetText(hBx, "x");
	BUTTON_SetText(hBc, "c");
	BUTTON_SetText(hBv, "v");
	BUTTON_SetText(hBb, "b");
	BUTTON_SetText(hBn, "n");
	BUTTON_SetText(hBm, "m");
	BUTTON_SetText(hB_, "_");
	BUTTON_SetText(hBdog, "@");
	BUTTON_SetText(hBback, "<");
	
	BUTTON_SetBkColor(hBz, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBx, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBc, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBv, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBb, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBn, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBm, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hB_, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBdog, 0, GUI_LIGHTCYAN);
	BUTTON_SetBkColor(hBback, 0, GUI_LIGHTCYAN);
		
	Delayms(200);

	GUI_Exec();
	char message[50];
	for (int i = 0; i < 50; i++) message[i] = 0; //VladM
	uint8_t index = 0;
	uint8_t prev_index = 0; //VladM
	
	while (1) {
		prev_index = index;  //VladM
		/* Get pressed key */
		switch (GUI_GetKey()) {
			case GUI_ID_BUTTON1: {message[index] = '1'; index++; break;}
			case GUI_ID_BUTTON2: {message[index] = '2'; index++; break;} 	
			case GUI_ID_BUTTON3: {message[index] = '3'; index++;break;}
			case GUI_ID_BUTTON4: {message[index] = '4'; index++;break;}
			case GUI_ID_BUTTON5: {message[index] = '5'; index++;break;}
			case GUI_ID_BUTTON6: {message[index] = '6'; index++;break;}
			case GUI_ID_BUTTON7: {message[index] = '7'; index++;break;}
			case GUI_ID_BUTTON8: {message[index] = '8'; index++;break;}
			case GUI_ID_BUTTON9: {message[index] = '9'; index++;break;}
			case GUI_ID_BUTTON0: {message[index] = '0'; index++;break;}
			case 11: {
				//send button
				//transmit via uart
			if(index>0)	UART_SendPacket(message, index);

				//clear data, index and data field
				for (int i = 0; i < 50; i++) message[i] = 0; index = 0;break;} 
			case 12: {message[index] = 'q'; index++;break;}
			case 13: {message[index] = 'w'; index++;break;}
			case 14: {message[index] = 'e'; index++;break;}
			case 15: {message[index] = 'r'; index++;break;}
			case 16: {message[index] = 't'; index++;break;}
			case 17: {message[index] = 'y'; index++;break;}
			case 18: {message[index] = 'u'; index++;break;}
			case 19: {message[index] = 'i'; index++;break;}
			case 20: {message[index] = 'o'; index++;break;}
			case 21: {message[index] = 'p'; index++;break;}
			case 22: {message[index] = 'a'; index++;break;}
			case 23: {message[index] = 's'; index++;break;}
			case 24: {message[index] = 'd'; index++;break;}
			case 25: {message[index] = 'f'; index++;break;}
			case 26: {message[index] = 'g'; index++;break;}
			case 27: {message[index] = 'h'; index++;break;}
			case 28: {message[index] = 'j'; index++;break;}
			case 29: {message[index] = 'k'; index++;break;}
			case 30: {message[index] = 'l'; index++;break;}
			case 31: {message[index] = 'z'; index++;break;}
			case 32: {message[index] = 'x'; index++;break;}
			case 33: {message[index] = 'c'; index++;break;}
			case 34: {message[index] = 'v'; index++;break;}
			case 35: {message[index] = 'b'; index++;break;}
			case 36: {message[index] = 'n'; index++;break;}
			case 37: {message[index] = 'm'; index++;break;}
			case 38: {message[index] = ' '; index++;break;}
			case 39: {message[index] = '@'; index++;break;}
			case 40: {
							//Backspace button pressed
							if (index != 0)
							{
							message[index-1] = 0;
							index--;
							}
							break;
								}
			case 41:{ //Incoming button	pressed 
	//deleting widgets functionally
	EDIT_Delete(E_12);			
	BUTTON_Delete(hButton);		
	BUTTON_Delete(Incoming);
	BUTTON_Delete(Fast_msg);
  BUTTON_Delete(hB1);
	BUTTON_Delete(hB2);
	BUTTON_Delete(hB3);
	BUTTON_Delete(hB4);
	BUTTON_Delete(hB5);
	BUTTON_Delete(hB6);
	BUTTON_Delete(hB7);
	BUTTON_Delete(hB8);
	BUTTON_Delete(hB9);
  BUTTON_Delete(hB0);
  BUTTON_Delete(hBq);
	BUTTON_Delete(hBw);
	BUTTON_Delete(hBe);
	BUTTON_Delete(hBr);
	BUTTON_Delete(hBt);
	BUTTON_Delete(hBy);
	BUTTON_Delete(hBu);
	BUTTON_Delete(hBi);
	BUTTON_Delete(hBo);
  BUTTON_Delete(hBp);
	BUTTON_Delete(hBa);
	BUTTON_Delete(hBs);
	BUTTON_Delete(hBd);
	BUTTON_Delete(hBf);
	BUTTON_Delete(hBg);
	BUTTON_Delete(hBh);
	BUTTON_Delete(hBj);
	BUTTON_Delete(hBk);
	BUTTON_Delete(hBl);
  BUTTON_Delete(hBz);
	BUTTON_Delete(hBx);
	BUTTON_Delete(hBc);
	BUTTON_Delete(hBv);
	BUTTON_Delete(hBb);
	BUTTON_Delete(hBn);
	BUTTON_Delete(hBm);
	BUTTON_Delete(hB_);
	BUTTON_Delete(hBdog);
	BUTTON_Delete(hBback);
	GUI_Clear();  //deleting buttons visually
	//Init incoming block widgets
	
	loop2:
	TM_ILI9341_Fill(ILI9341_COLOR_GRAY);
	BUTTON_Handle Incoming_Back_Button = BUTTON_CreateEx(10, 10, 300, 45, 0, WM_CF_SHOW, 0, 44);
	BUTTON_SetFont(Incoming_Back_Button, &GUI_Font20_ASCII);
	BUTTON_SetText(Incoming_Back_Button, "Send new message");
  BUTTON_SetBkColor(Incoming_Back_Button, 0, GUI_LIGHTGREEN);	
	EDIT_Handle E_13 = EDIT_CreateEx(10, 65, 300, 45, 0, WM_CF_SHOW, 0, 42, 101);//VladM
	EDIT_SetFont(E_13, &EDIT_FONT);
	EDIT_Handle E_14 = EDIT_CreateEx(10, 120, 300, 45, 0, WM_CF_SHOW, 0, 42, 103);//VladM
	EDIT_SetFont(E_14, &EDIT_FONT);
	EDIT_Handle E_15 = EDIT_CreateEx(10, 175, 300, 45, 0, WM_CF_SHOW, 0, 42, 104);//VladM
	EDIT_SetFont(E_15, &EDIT_FONT);
	looprn:
	;
	uint8_t key_symbol_cnt = 0;
	for (int i = 0; i < radio_data_size; i++) {if(radio_data[i] == '&') key_symbol_cnt++;}
	  switch (key_symbol_cnt){ //How many messages we have in radio_data
		case 0:
		{
			break;
		}
		case 1:
		{			
				uint8_t buf_case1 = 0;
				for (int i = 0; i < incoming_message_size; i++)				
				{
				third_message[i] = second_message[i];
				second_message[i] = first_message[i];
				if (radio_data[i] == '&') buf_case1 = 1;
				if (buf_case1 == 0) first_message[i] = radio_data[i];
					else first_message[i]=0; //last symbols of previous packets
				}
				break;
		}
		case 2:
		{
				for (int i = 0; i < incoming_message_size; i ++) 
				{
					third_message[i] = first_message[i];
					first_message[i] = 0;
					second_message[i] = 0;
				}
				EDIT_SetText(E_15, (const char*) third_message);
				int i = 0;
				while (radio_data[i] != '&')
				{
					first_message[i] = radio_data[i];
					i++;
				}
				i++;
				int j = 0;
				while (radio_data[i] != '&')
				{
					second_message[j] = radio_data[i];
					i++;
					j++;
				}
				break;
		}
		default: // 3 msgs
		{	
			for (int i = 0; i < incoming_message_size; i ++) 
				{
					third_message[i] = 0;
					first_message[i] = 0;
					second_message[i] = 0;
				}
				int i = 0;
				while(radio_data[i] != '&')
				{
					third_message[i] = radio_data[i];
					i++;
				}
				i++;
				int j = 0;
					while(radio_data[i] != '&')
				{
					second_message[j] = radio_data[i];
					i++;
					j++;
				}
				i++;
				j = 0;
				while(radio_data[i] != '&')
				{
					first_message[j] = radio_data[i];
					i++;
					j++;
				}
				i++;
				break;				
			} 
		}
				EDIT_SetText(E_13, (const char*) first_message);
				EDIT_SetText(E_14, (const char*) second_message);
				EDIT_SetText(E_15, (const char*) third_message);
				for (int i = 0; i < radio_data_size; i++) radio_data[i] = 0;
				receive_flag = 0; 
		GUI_Exec(); //Show widgets
		while(1) //wait till Back_Button is pushed
			{
		    if (GUI_GetKey() == 44) 
			{
				
				//delete incoming block widgets functionally
				EDIT_Delete(E_13);
				EDIT_Delete(E_14);
				EDIT_Delete(E_15);
				BUTTON_Delete(Incoming_Back_Button);
				GUI_Clear(); // delete visually
				goto loop1; //back to the main function
			}
			Delayms(1000);
			goto looprn;
		}
	}
		case 43: //Fast Send button clicked;
		{
			EDIT_Delete(E_12);			
			BUTTON_Delete(hButton);		
			BUTTON_Delete(Incoming);
			BUTTON_Delete(Fast_msg);
			BUTTON_Delete(hB1);
			BUTTON_Delete(hB2);
			BUTTON_Delete(hB3);
			BUTTON_Delete(hB4);
			BUTTON_Delete(hB5);
			BUTTON_Delete(hB6);
			BUTTON_Delete(hB7);
			BUTTON_Delete(hB8);
			BUTTON_Delete(hB9);
			BUTTON_Delete(hB0);
			BUTTON_Delete(hBq);
			BUTTON_Delete(hBw);
			BUTTON_Delete(hBe);
			BUTTON_Delete(hBr);
			BUTTON_Delete(hBt);
			BUTTON_Delete(hBy);
			BUTTON_Delete(hBu);
			BUTTON_Delete(hBi);
			BUTTON_Delete(hBo);
			BUTTON_Delete(hBp);
			BUTTON_Delete(hBa);
			BUTTON_Delete(hBs);
			BUTTON_Delete(hBd);
			BUTTON_Delete(hBf);
			BUTTON_Delete(hBg);
			BUTTON_Delete(hBh);
			BUTTON_Delete(hBj);
			BUTTON_Delete(hBk);
			BUTTON_Delete(hBl);
			BUTTON_Delete(hBz);
			BUTTON_Delete(hBx);
			BUTTON_Delete(hBc);
			BUTTON_Delete(hBv);
			BUTTON_Delete(hBb);
			BUTTON_Delete(hBn);
			BUTTON_Delete(hBm);
			BUTTON_Delete(hB_);
			BUTTON_Delete(hBdog);
			BUTTON_Delete(hBback);
			GUI_Clear();  
			
			BUTTON_Handle Fast_back_button = BUTTON_CreateEx(10, 10, 95, 45, 0, WM_CF_SHOW, 0, 45);
			BUTTON_SetFont(Fast_back_button, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(Fast_back_button, "Back");
			BUTTON_Handle Fast_send_send = BUTTON_CreateEx(115, 10, 95, 45, 0, WM_CF_SHOW, 0, 46);
			BUTTON_SetFont(Fast_send_send, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(Fast_send_send, "Send");
			BUTTON_Handle Fast_incoming = BUTTON_CreateEx(220, 10, 90, 45, 0, WM_CF_SHOW, 0, 47);
			BUTTON_SetFont(Fast_incoming, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(Fast_incoming, "Incoming");
			BUTTON_Handle E_16 = BUTTON_CreateEx(10, 65, 95, 45, 0, WM_CF_SHOW, 0, 48);
			BUTTON_SetFont(E_16, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(E_16, "Default");
			BUTTON_Handle E_17 = BUTTON_CreateEx(115, 65, 95, 45, 0, WM_CF_SHOW, 0, 49);
			BUTTON_SetFont(E_17, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(E_17, "Groups");
			BUTTON_Handle E_18 = BUTTON_CreateEx(220, 65, 90, 45, 0, WM_CF_SHOW, 0, 50);
			BUTTON_SetFont(E_18, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(E_18, "Broadcast");
			BUTTON_Handle B_10 = BUTTON_CreateEx(10, 120, 50, 45, 0, WM_CF_SHOW, 0, 51);
			BUTTON_SetFont(B_10, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_10, "text");
			BUTTON_Handle B_20 = BUTTON_CreateEx(70, 120, 50, 45, 0, WM_CF_SHOW, 0, 52);
			BUTTON_SetFont(B_20, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_20, "text");
			BUTTON_Handle B_30 = BUTTON_CreateEx(130, 120, 50, 45, 0, WM_CF_SHOW, 0, 53);
			BUTTON_SetFont(B_30, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_30, "text");
			BUTTON_Handle B_40 = BUTTON_CreateEx(190, 120, 50, 45, 0, WM_CF_SHOW, 0, 54);
			BUTTON_SetFont(B_40, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_40, "text");
			BUTTON_Handle B_50 = BUTTON_CreateEx(250, 120, 50, 45, 0, WM_CF_SHOW, 0, 55);
			BUTTON_SetFont(B_50, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_50, "text");
			
			BUTTON_Handle B_15 = BUTTON_CreateEx(10, 175, 50, 45, 0, WM_CF_SHOW, 0, 56);
			BUTTON_SetFont(B_15, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_15, "text");
			BUTTON_Handle B_25 = BUTTON_CreateEx(70, 175, 50, 45, 0, WM_CF_SHOW, 0, 57);
			BUTTON_SetFont(B_25, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_25, "text");
			BUTTON_Handle B_35 = BUTTON_CreateEx(130, 175, 50, 45, 0, WM_CF_SHOW, 0, 58);
			BUTTON_SetFont(B_35, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_35, "text");
			BUTTON_Handle B_45 = BUTTON_CreateEx(190, 175, 50, 45, 0, WM_CF_SHOW, 0, 59);
			BUTTON_SetFont(B_45, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_45, "text");
			BUTTON_Handle B_55 = BUTTON_CreateEx(250, 175, 50, 45, 0, WM_CF_SHOW, 0, 60);
			BUTTON_SetFont(B_55, &BUTTON_FONT_ACTIONS);
			BUTTON_SetText(B_55, "text");
			
			BUTTON_SetBkColor(Fast_back_button, 0, GUI_LIGHTGREEN);
			BUTTON_SetBkColor(Fast_send_send, 0, GUI_LIGHTGREEN);
			BUTTON_SetBkColor(Fast_incoming, 0, GUI_LIGHTGREEN);
			BUTTON_SetBkColor(E_16, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(E_17, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(E_18, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_10, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_15, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_20, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_25, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_30, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_35, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_40, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_45, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_50, 0, GUI_LIGHTCYAN);
			BUTTON_SetBkColor(B_55, 0, GUI_LIGHTCYAN);
			
			TM_ILI9341_Fill(ILI9341_COLOR_GRAY);
			GUI_Exec();
			char fast_message[6]; //3 bytes of adress, 1 byte '@', 2 bytes of short message (10, 20, etc)
			uint8_t fast_message_size = 6;
			for (int z = 0; z < fast_message_size; z++) fast_message[z] = 0;
			while (1)
			{
				switch(GUI_GetKey()){
						case 45: { //Back
							BUTTON_Delete(Fast_back_button);
							BUTTON_Delete(Fast_send_send);
							BUTTON_Delete(Fast_incoming);
							BUTTON_Delete(E_16);
							BUTTON_Delete(E_17);
							BUTTON_Delete(E_18);
							BUTTON_Delete(B_10);
							BUTTON_Delete(B_15);
							BUTTON_Delete(B_20);
							BUTTON_Delete(B_25);
							BUTTON_Delete(B_30);
							BUTTON_Delete(B_35);
							BUTTON_Delete(B_40);
							BUTTON_Delete(B_45);
							BUTTON_Delete(B_50);
							BUTTON_Delete(B_55);
							GUI_Clear();
							goto loop1;
						}
						case 46: { //Send button
							UART_SendPacket(fast_message, fast_message_size);
							for (int z = 0; z < 50; z++) fast_message[z] = 0;
							break;
						}
						case 47: { //Incoming
							BUTTON_Delete(Fast_back_button);
							BUTTON_Delete(Fast_send_send);
							BUTTON_Delete(Fast_incoming);
							BUTTON_Delete(E_16);
							BUTTON_Delete(E_17);
							BUTTON_Delete(E_18);
							BUTTON_Delete(B_10);
							BUTTON_Delete(B_15);
							BUTTON_Delete(B_20);
							BUTTON_Delete(B_25);
							BUTTON_Delete(B_30);
							BUTTON_Delete(B_35);
							BUTTON_Delete(B_40);
							BUTTON_Delete(B_45);
							BUTTON_Delete(B_50);
							BUTTON_Delete(B_55);
							GUI_Clear();
							goto loop2;
						}					
						case 48: { //default
						fast_message[0] = '0'; fast_message[1] = '0'; fast_message[2] = '0'; fast_message[3] = '@';
						break;
					}
					case 49: { //groups
						fast_message[0] = '9'; fast_message[1] = '9'; fast_message[2] = '8'; fast_message[3] = '@';
						break;
					}
					case 50: { //broadcast
						fast_message[0] = '9'; fast_message[1] = '9'; fast_message[2] = '9'; fast_message[3] = '@';
						break;
					}
					case 51: { 
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 52: { 
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 53: { 
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 54: { 
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 55: {
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 56: { 
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 57: {
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 58: { 
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 59: { 
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					case 60: {
						fast_message[4] = 'a'; fast_message[5] = 'b';
						break;
					}
					default: break;
				}
			}
		}
			default:
				break;
		}
		if(prev_index != index){//VladM	
		EDIT_SetText(E_12, (const char*) message);
  	GUI_Exec1();
		}
	}
}

/* User handler for 1ms interrupts */
void TM_DELAY_1msHandler(void) {
	/* Call periodically each 1ms */
	TM_EMWIN_UpdateTouch();
	
	if(sec_counter<10*60000) sec_counter++;
	if(blink_counter<10000) blink_counter++; 
	if(buzz_counter<10000) buzz_counter++; 

	
		if((blink_counter<=2000)){//blink
	 TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_5);
	}
		else{
			TM_GPIO_SetPinLow(GPIOE, GPIO_PIN_5);
		}
		
		if((buzz_counter<=1000)){//buzzer //1 sec (1000msec)
	 TM_GPIO_TogglePinValue(GPIOE, GPIO_Pin_6);
	}	
				else{
			TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_6);
		}
				
	if((sec_counter<=3*60000)&&(light_flag!=GREENLIGHT)){
		RGBLed(GREENLIGHT);
		light_flag = GREENLIGHT;
	}
	if((sec_counter>3*60000)&&(sec_counter<=6*60000)&&(light_flag!=YELLOWLIGHT)) {
		RGBLed(YELLOWLIGHT);
			light_flag = YELLOWLIGHT;
	}
	if((sec_counter>6*60000)&&(light_flag!=REDLIGHT)){ 
		RGBLed(REDLIGHT);
				light_flag = REDLIGHT;
	}
}

static void MX_UART5_Init(void)
{
	RCC->APB1ENR|= RCC_APB1ENR_UART5EN;
		TM_GPIO_InitAlternate(GPIOD, GPIO_PIN_2, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, GPIO_AF_UART5);
		TM_GPIO_InitAlternate(GPIOC, GPIO_PIN_12, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, GPIO_AF_UART5);

 
  huart5.USART_BaudRate = 115200;// huart5.USART_BaudRate = 57600;
  huart5.USART_WordLength = USART_WordLength_8b;
  huart5.USART_StopBits = USART_StopBits_1;
  huart5.USART_Parity = USART_Parity_No;
  huart5.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
  huart5.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_Init(UART5, &huart5);
	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(UART5_IRQn);
	USART_Cmd(UART5, ENABLE);
  
}

void UART5_IRQHandler(void){
	TM_GPIO_TogglePinValue(GPIOG, GPIO_Pin_14);
	
	USART_ClearITPendingBit(UART5, USART_IT_RXNE);
  char tmp2[3];
	char tmp3[9];
	char tmp;
	
	tmp2[0] = 0xC0;
	tmp2[1] = 0x10;
	tmp2[2] = 0x10;
	
	tmp3[0] = 0xC0;
	tmp3[1] = 0x10;
	tmp3[2] = 0x14;
	tmp3[3] = 0x00;
	tmp3[4] = 0x07;
	tmp3[5] = 'K';
	tmp3[6] = 'A';  // keep alive messages
	
	tmp = USART_ReceiveData(UART5);
	if (flagtowrite >0){

	if(rflag2>1)	radio_data[receive_flag+rflag2-2] = tmp; //0 1 byte are service bytes
	flagtowrite--;
	if(tmp == '&'){
									flagtowrite = 0; 
									blink_counter = 0; //blink
									buzz_counter = 0;//buzz
									receive_flag+=rflag2-1;
									}//3 last bytes of packet
			rflag2++;
			return;
	}
	
	if(tmp == tmp2[rflag2]) rflag2 ++;
	else if ((tmp == tmp2[0])&&(rflag2==1)) rflag2 = 1; //2 times 0xC0
	else rflag2 = 0;
	if(rflag2 == 3) { //if head of frame is normal
		flagtowrite = 70; 
		
		rflag2=0;
	}
	
	if(rflag3 == 3) tmp = 0; //4 byte
	if(tmp == tmp3[rflag3]) rflag3 ++;
	else if ((tmp == tmp3[0])&&(rflag3==1)) rflag3 = 1; //2 times 0xC0
	else rflag3 = 0;
	if(rflag3 == 3) { //if head of frame is normal
		sec_counter = 0; //wait for next keepalive
		
		rflag3=0;
	}
	
	
}

//Send Packet to IMST im880 module

void UART_SendPacket( char *buff, int length){
	char send_buf[55]; //max length + 5 bytes for Raw-frame
	unsigned short crc1; //crc16 declaration
	
//Performing the raw-frame for IM-Module
	TM_GPIO_TogglePinValue(GPIOG, GPIO_Pin_13);
	blink_counter = 0; //blink
	//Send Unreliable Data Request
	send_buf[0] = 0x10; //LORAWAN_ID
	send_buf[1] = 0x0D; //Send Unconfirmed Data Request LORAWAN_MSG_SEND_UDATA_REQ
	send_buf[2] = 0x01; //LoRaWAN Port number (> 0)
	
	//Writting the data from buff to send_buf
	for (int z = 0; z < length; z++){
		send_buf[3 + z] = buff[z]; // Application Layer Payload
	}
	send_buf[3 + length] = 0x26;
	
	//Calculating and splitting 16-bit crc into 2 bytes and writing to send_buf
	crc1 = crc16(send_buf, length + 4);
	send_buf[4 + length] = crc1 >> 8;
	send_buf[5 + length] = crc1 & 0xFF;
	
//End of Performing the raw-frame for IM-Module
	
//Sending the slip-frame for IM-Module

	//send 50 wakeup chars as done in Devtool
	for (int z = 0; z < 50; z++)
	{
		USART_SendData(UART5, 0x00);
		Delayms(1);
	}
	Delayms(1);
	USART_SendData(UART5, 0xC0); // sending the 1st byte of the slip-frame
	Delayms(1);
	
	//Sending the raw-frame
	for (int z = 0; z < length + 6; z++)
	{
		USART_SendData(UART5, send_buf[z]);
		Delayms(1);
	}
	
	USART_SendData(UART5, 0xC0); //Sending the last byte of the slip-frame
	Delayms(1); 
//End of Sending the slip-frame for IM-Module
}


unsigned short crc16(char *data_p, unsigned short length)
{
      unsigned char i;
      unsigned int data;
      unsigned int crc = 0xffff;

      if (length == 0)
            return (~crc);

      do
      {
            for (i=0, data=(unsigned int)0xff & *data_p++;
                 i < 8; 
                 i++, data >>= 1)
            {
                  if ((crc & 0x0001) ^ (data & 0x0001))
                        crc = (crc >> 1) ^ POLY;
                  else  crc >>= 1;
            }
      } while (--length);

      crc = ~crc;
      data = crc;
      crc = (crc << 8) | (data >> 8 & 0xff);

      return (crc);
}

void RGBLed(char light){
switch (light){
	case GREENLIGHT:{
				TM_GPIO_SetPinLow(GPIOE, GPIO_PIN_2);
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_3);
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_4);
		break;
	}
		case BLUELIGHT:{
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_2);
				TM_GPIO_SetPinLow(GPIOE, GPIO_PIN_3);
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_4);
		break;
	}
		case REDLIGHT:{
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_2);
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_3);
				TM_GPIO_SetPinLow(GPIOE, GPIO_PIN_4);
		break;
	}
		case YELLOWLIGHT:{
				TM_GPIO_SetPinLow(GPIOE, GPIO_PIN_2);
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_3);
				TM_GPIO_SetPinLow(GPIOE, GPIO_PIN_4);
		break;
	}
		case NOLIGHT:{
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_2);
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_3);
				TM_GPIO_SetPinHigh(GPIOE, GPIO_PIN_4);
		break;
	}
	default:{
	break;
	}
}
}
