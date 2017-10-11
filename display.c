/*
 * display.c
 *
 *  Created on: Oct 13, 2016
 *      Author: AAdair
 */

#include "display.h"
#include "msp430f4250.h"

const char DIGIT[11]=
{
	0x5F,        // Displays "0"
	0x06,        // Displays "1"
	0x6B,        // Displays "2"
	0x2F,        // Displays "3"
	0x36,        // Displays "4"
	0x3D,        // Displays "5"
	0x7D,        // Displays "6"
	0x07,        // Displays "7"
	0x7F,        // Displays "8"
	0x3F,        // Displays "9"
	0x00         // Displays " "
};


void lcd_initialize(void)
{
	int i;

	LCDACTL = LCDON + LCD4MUX + LCDFREQ_32;        // 4mux LCD, LCD On, Freq 64
	LCDAPCTL0 = 0X0F;                              // LCD Segments 0-13
	LCDAVCTL0 = LCDCPEN;                           // Enable LCD Charge Pump
	LCDAVCTL1 = VLCD_15;                           // Max Bias on LCD (Contrast Control)                                                      // Turn On LCD                  // (A0,WR,CS) Release Chip Select
	P5SEL  = 0x1C;                                 // Set COM pins for LCD

    for(i = 0; i < 7; i++)
    {
        LCDMEM[i] = 0x00;                            // Clear LCD
    }

    lcd_clear(0,0);
}

//------------------------------------------------------------------------------
// Display Function (4mux LCD)
//------------------------------------------------------------------------------

void lcd_update(long value, long overange, unsigned char decimal, unsigned char lcd_6_flags, unsigned char lcd_7_flags)
{
	int i, first_integer = 1;
	int number[6];

	// Set miscellaneous display flags
	if(value < 0)
	{
		lcd_7_flags |= NEG_FLAG;            // set negative flag
		value *= -1;
	}
	else
		lcd_7_flags &= ~NEG_FLAG;

	if(value < 1)
		lcd_7_flags &= ~NEG_FLAG;

	// Numerical Parsing
	if(value < overange)                    // Testing for overanged values
	{
		for (i = 0; i < 6; i++)             // Process 6 digits
		{
			number[i] = value%10;           // Segments to LCD separated to integer array
			value = value/10;// * 0.1;             		// Divide by 10 for next digit
		}

		i = 5;
		while(first_integer)                // Test and eliminate leading extraneous zeros
		{
			if(number[i] == 0)
			{
				if(decimal == i)
					first_integer = 0;
				else
				{
					number[i] = 10;
					i -= 1;
				}
			}
			else if(number[i] == 10)
				i -= 1;
			else
				first_integer = 0;
		}
		// Send integer array to each display register
		LCDM1 = DIGIT[number[0]];
		LCDM2 = DIGIT[number[1]];
		LCDM3 = DIGIT[number[2]];
		LCDM4 = DIGIT[number[3]];
		LCDM5 = DIGIT[number[4]];

		if(number[5] == 1)
			lcd_7_flags |= BC_FLAG;
		else
			lcd_7_flags &= ~BC_FLAG;

	}
	else
	{
		LCDM1 = 0x20;
		LCDM2 = 0x20;
		LCDM3 = 0x20;                       //Overange
		LCDM4 = 0x20;
		LCDM5 = 0x20;
		lcd_7_flags |= BC_FLAG;
	}

	switch (decimal)
	{
		case 0x01:
			LCDM1 += DECIMAL_POINT;               // Decimal Point
			break;
		case 0x02:
			LCDM2 += DECIMAL_POINT;               // Decimal Point
			break;
		case 0x03:
			LCDM3 += DECIMAL_POINT;               // Decimal Point
			break;
		case 0x04:
			LCDM4 += DECIMAL_POINT;               // Decimal Point
			break;
	}

	LCDM6 = lcd_6_flags;                  // Enable display flags
	LCDM7 = lcd_7_flags;
	return;
}

void lcd_clear(unsigned char lcd_6_flags, unsigned char lcd_7_flags)
{
	// Send integer array to each display register
	LCDM1 = DIGIT[10];
	LCDM2 = DIGIT[10];
	LCDM3 = DIGIT[10];
	LCDM4 = DIGIT[10];
	LCDM5 = DIGIT[10];
	LCDM6 = lcd_6_flags;                  // Enable display flags
	LCDM7 = lcd_7_flags;
}

void lcd_deactivate(void)
{
	LCDACTL = 0x00;        // 4mux LCD, LCD On, Freq 64
	LCDAPCTL0 = 0x00;                              // LCD Segments 0-13
	LCDAVCTL0 = 0x00;                           // Enable LCD Charge Pump
	LCDAVCTL1 = 0x00;                           // Max Bias on LCD (Contrast Control)                                                      // Turn On LCD                  // (A0,WR,CS) Release Chip Select
	P5SEL  = 0x00;                                 // Set COM pins for LCD                                                        // LCD Off
}


