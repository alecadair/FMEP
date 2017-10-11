/*
 * display.h
 *
 *  Created on: Oct 13, 2016
 *      Author: AAdair
 */

#ifndef SRC_DISPLAY_H_
#define SRC_DISPLAY_H_

#define DECIMAL_POINT 0x80
#define B4_CRT_FLAG		0x01
#define B3_CRT_FLAG		0x02
#define B2_CRT_FLAG     0x08
#define B1_CRT_FLAG		0x04
#define LOBATT_FLAG		0x10
#define HOLD_FLAG		0x20
#define PEAK_FLAG       0x40
#define STANDBY_FLAG	0x80
#define PEAKHOLD_FLAG	0x60

#define T4_CRT_FLAG		0x01
#define T3_CRT_FLAG		0x02
#define T2_CRT_FLAG		0x04
#define T1_CRT_FLAG		0x08
#define DC_FLAG			0x10
#define AC_FLAG			0x20
#define BC_FLAG			0x40
#define NEG_FLAG		0x80


void lcd_initialize(void);
void lcd_deactivate(void);
void lcd_clear(unsigned char, unsigned char );
void lcd_update(long , long , unsigned char , unsigned char , unsigned char);

#endif /* SRC_DISPLAY_H_ */
