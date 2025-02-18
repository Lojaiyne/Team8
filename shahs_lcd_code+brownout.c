
#define F_CPU 20E6

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>


#define LCD_Data		PORTC		//LCD Data output     D0-D7 should all be on PORTC
#define LCD_Data_DDR    DDRC		//LCD Direction port
#define LCD_Data_RD		PINC		//LCD Data input
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
char arr[1][19] = {"21:2:25 10:29:30"};

int main(void)
{

	_delay_ms(100);
	strcpy(disp[0],arr[0]); // replaces disp0 with arr 0
	LCD_Control_DDR = 1<<LCD_E | 1<<LCD_RS | 1<<LCD_RW; 
	LCD_Control &= ~(1<<LCD_RW);
	LCD_Data_DDR = 0xFF;

	LCD_Initialise();
	_delay_ms(5);

	LCD_GOTOXY(0,0);
	LCD_Send_String(arr[0]); // displays the string
	

	LCD_GOTOXY(0,1); // next line
	LCD_Send_String("100.39N 100.65W"); // characters
	
	
	while (1)
	{
		if (MCUSR & 1<<BORF)  // for if the voltage drops to a very low level :)
		{
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
	if (x>16) x=0;
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