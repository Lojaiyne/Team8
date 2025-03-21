
#define F_CPU 20E6

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define Baud_Rate 9600UL
#define Baud_Register_Value ((F_CPU / (16 * Baud_Rate)) - 1)

#define IN 1
#define OUT 0

char UART_Buffer[30];
char GPS_Store[58];
int Store = 0;
int state = OUT;
char check [7]; //6 character wide array

int8_t if_index_1 = 0;

#define LCD_Data		PORTA		//LCD Data output     D0-D7 should all be on PORTC
#define LCD_Data_DDR    DDRA		//LCD Direction port
#define LCD_Data_RD		PINA		//LCD Data input
#define LCD_Control     PORTB		//Port where LCD control signals are
#define LCD_Control_RD  PINB
#define LCD_Control_DDR DDRB
#define LCD_RS			0			//Pin on which LCD Register Select is         this is for PORTB0-3 connect as seen by number like PB3 is PORTB3
#define LCD_RW			1			//Pin on which LCD Read/Write is
#define LCD_E			2			//Pin on which LCD Enable is

// These constants are derived from information found in the Hitachi HD44780
// LCD controller datasheet.  This command structure is used on every
// alphanumeric LCD.
#define	DISP_FS		0x3B  	//Function set
#define	DISP_ON		0x0C  	//Display on w/o blink or flash
#define	DISP_OFF	0x08  	//Display, blink, cursor off
#define	DISP_CL		0x01  	//Clear display
#define	DISP_EN 	0x06  	//Data entry mode
#define	LINE_1		0x80	//DDRAM address for first line
#define	LINE_2		0xC0	//DDRAM address for second line
#define	SH_LFT		0x18	//Left-shift display
#define	SH_RT		0x1C 	//Right-shift display
#define	HOME		0x02  	//Return home

/****************************************************************/
/* LCD Function prototypes */
void LCD_Send_Command(uint8_t);
void LCD_Send_Data(uint8_t);
void LCD_Initialise(void);
void LCD_GOTOXY(uint8_t /*X coord*/,uint8_t/*Y coord*/);
void LCD_Send_Data_XY(uint8_t /*data*/,uint8_t/*X coord*/,uint8_t/*Y coord*/);
void LCD_Send_String(char *);

/****************************************************************/
/*this is for storing the characters from the RTC and GPS :) */
char disp[1][25] = {"Greek"};
char arr[10];

int main(void)
{

	_delay_ms(100);
	LCD_Control_DDR = 1<<LCD_E | 1<<LCD_RS | 1<<LCD_RW; 
	LCD_Control &= ~(1<<LCD_RW);
	LCD_Data_DDR = 0xFF;
	
	UCSR1B = 1<<TXEN1 | 1<<RXEN1 | 1<<RXCIE1;
	UBRR1 = Baud_Register_Value;

	LCD_Initialise();
	_delay_ms(5);

	LCD_GOTOXY(0,0);
	//LCD_Send_String(arr); // displays the string
	

	//LCD_GOTOXY(0,1); // next line
	//LCD_Send_String("100.39N 100.65W"); // characters
	sei();
	
	while (1)
	{
	   
	
		if (MCUSR & 1<<BORF)  // for if the voltage drops to a very low level :)
		{
			PORTB |= 1<<PB4;
		}
	}
}
// please put any code you want to use below this please :)

/****************************************************************/
/*
LCD ROUTINES
*/
void LCD_Send_Command(uint8_t command)
{
	_delay_us(100);
	LCD_Control &= ~(1<<LCD_RS);
	LCD_Data = command;
	LCD_Control |= (1<<LCD_E);
	LCD_Control &= ~(1<<LCD_E);
}
/****************************************************************/
void LCD_Send_Data(uint8_t data)
{
	_delay_us(100);
	LCD_Control |= (1<<LCD_RS);
	LCD_Data = data;
	LCD_Control |= (1<<LCD_E);
	LCD_Control &= ~(1<<LCD_E);
}
/****************************************************************/
//Go to a specific location on the LCD.  0,0 is the top left character
void LCD_GOTOXY(uint8_t x,uint8_t y)
{
	if (x>15) x=0;
	if (y>1) y=0;
	//Ensure that the x,y values are within range
	if (y==0) LCD_Send_Command(LINE_1+x);
	if (y==1) LCD_Send_Command(LINE_2+x);


}
/****************************************************************/
//Send a single character to a specific location
void LCD_Send_Data_XY(uint8_t d,uint8_t x,uint8_t y)
{
	LCD_GOTOXY(x,y);
	LCD_Send_Data(d);
}
/****************************************************************/
//Send a string to the display.
void LCD_Send_String(char *ch)
{
	int i=0;

	while(ch[i]!='\0')
	{
		LCD_Send_Data(ch[i++]);
	}
}
/****************************************************************/
//Initialise the LCD
void LCD_Initialise(void)
{
	_delay_ms(10);					//Ensure start-up delay
	LCD_Send_Command(DISP_FS);		//Function set #
	_delay_ms(16);					//16ms delay
	LCD_Send_Command(DISP_FS);		//Function set
	_delay_ms(5);					//5ms Delay
	LCD_Send_Command(DISP_FS);      //Function set
	_delay_ms(5);					//1ms delay
	LCD_Send_Command(DISP_FS); 		//Function set
	_delay_ms(1);					//1ms delay
	LCD_Send_Command(DISP_ON);      //Turn Display on
	_delay_ms(1);					//1ms delay
	LCD_Send_Command(DISP_EN); 		//Set entry mode
	_delay_ms(1);					//1ms delay
	LCD_Send_Command(DISP_CL);      //Clear Display
	_delay_ms(1);					//1ms delay
}
/****************************************************************/

void Transmit_Character(char C)
{
	while(!(UCSR1A & 1<<UDRE1));
	UDR1 = C;
}

void Transmit_String(char String[])
{
	uint8_t String_Length = strlen(String);
	for (uint8_t Index = 0; Index < String_Length; Index++)
	{
		Transmit_Character(String[Index]);
	}
}

void Split_GPS(char *GPS_Data)
{
	char timeStore[10] = {0};
	char dateStore[10] = {0};
	char latitude[15] = {0};
	char NS = ' ';  // north/south
	char longitude[15] = {0};
	char EW = ' ';  // east/west
	
	char garbage_letter_1 = ' ';
	char garbage_letter_2 = ' ';
	char garbage_number[3] = {0};
	
	char *strip_pointer = strtok(GPS_Store, ","); //split by comma
	int jump = 1;
	
	while (strip_pointer != NULL)
	{
		switch (jump)
		{
			case 1: strncpy(timeStore, strip_pointer, sizeof(timeStore) - 1);
			break;
			case 2: garbage_letter_1 = strip_pointer[0];
			break;
			case 3: strncpy(longitude, strip_pointer, sizeof(longitude) - 1);
			break;
			case 4: NS = strip_pointer[0];
			break;
			case 5: strncpy(latitude, strip_pointer, sizeof(latitude) - 1);
			break;
			case 6: EW = strip_pointer[0];
			break;
			case 7: strncpy(garbage_number, strip_pointer, sizeof(garbage_number) - 1);
			break;
			case 8: strncpy(dateStore, strip_pointer, sizeof(dateStore) - 1);
			break;
			case 9: garbage_letter_2 = strip_pointer[0];
			break;
		}
		
		strip_pointer = strtok(NULL, ",");
		jump++;
		
	}
	
  
	

	
	
	Transmit_String("Time: ");
	Transmit_String(timeStore);
	Transmit_Character('\n');
	
	Transmit_String("Date: ");
	Transmit_String(dateStore);
	Transmit_Character('\n');
	
	Transmit_String("Latitude: ");
	Transmit_String(latitude);
	Transmit_String(", ");
	Transmit_Character(NS);
	Transmit_Character('\n');
	
	Transmit_String("Longitude: ");
	Transmit_String(longitude);
	Transmit_String(", ");
	Transmit_Character(EW);
	Transmit_Character('\n');
	
	//Transmit_Character(garbage_letter_1);
	//Transmit_Character('\n');
	//Transmit_Character(garbage_letter_2);
	//Transmit_Character('\n');
	//Transmit_String(garbage_number);
	Transmit_Character('\n');


	_delay_ms(500);
	LCD_GOTOXY(0,0);
    //for (int i = 0; i <= 5; i++)
    //{        
		//if ( i == 1)
		//{
			//LCD_Send_String(dateStore[i]);
			//LCD_Send_String("-");
		//}
		//else if ( i == 3)
		//{
			//LCD_Send_String(dateStore[i]);
			//LCD_Send_String("-");
		//}	
		//else
		//{
	LCD_Send_String(dateStore);
		//}
    //}

	LCD_GOTOXY(0,1);
	LCD_Send_String(timeStore);

	
}

ISR(USART1_RX_vect)
{
	
	volatile char Rx_Char;
	Rx_Char = UDR1;
	//when if_index is greater than 6, we add the new rxchar to the last position and shift everthing left
	if (if_index_1 > 6)
	{
		for (int i = 0; i < 6; i++)
		{
			check[i] = check[i + 1];
		}
		check[6] = Rx_Char;
	}
	//this loads up the check variable starting from 0 till 6
	else
	{
		check[if_index_1++] = Rx_Char;
	}
	
	// check if we are in the gprmc part
	if (memcmp(check, "$GPRMC,", 7) == 0)
	{
		state = IN;
	}
	
	if (Rx_Char == '*') {
		state = OUT;
		//when it reaches * its out of GPRMC
		GPS_Store[Store] = '\0';
		Store = 0;
		Split_GPS(GPS_Store);
		
	}
	
	if (state == IN && Store < sizeof(GPS_Store) - 1)
	{
		// inside the GPRMC
		GPS_Store[Store] = Rx_Char;
		Store++;
	}
}