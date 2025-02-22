/*
 * gps_w_split.c
 *
 * Created: 18/02/2025 15:44:40
 * Author : 230184712
 */ 

/*
* GPS module.c
*
* Created: 11/02/2025 14:27:31
* Author : 230049329
*/

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

int8_t if_index_1 = 0; //  this variable is used in the uart receive isr, to loop through the statements that fill up the check


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
}

int main(void)
{
	UCSR1B = 1<<TXEN1 | 1<<RXEN1 | 1<<RXCIE1;
	UBRR1 = Baud_Register_Value;
	sei();
	
	while (1)
	{
		
		asm("nop");
	}
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



