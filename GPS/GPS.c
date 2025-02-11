/*
 * GPS.c
 *
 * Created: 28/01/2025 13:28:40
 * Author : 230184712
 */ 
#define F_CPU 20E6

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define Baud_Rate 9600UL
#define Baud_Register_Value ((F_CPU / (16 * Baud_Rate)) - 1)

char UART_Buffer[30];

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
	Transmit_Character(Rx_Char);
}
