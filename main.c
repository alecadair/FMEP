/**
 * @file 	main.c
 * @author 	Josh Schmeiser(jschmeiser@msn.com) and Pollux Polaris(polluxpolaris@gmail.com) and Alec Adair(alecadair1@gmail.com)
 * @date	February 2, 2015
 * @brief	This is the program for the DC Gaussmeter Model GM-1-ST and GM-1-HS
 *
 * 	GM-1-ST measures DC magnetic fields from -200,000 to +200,000 with +/- 1% accuracy
 * 	GM-1-ST measures DC magnetic fields from -80,000 to +80,000 with +/- 1% accuracy
 */

/* 8-13-2017 */


#include <math.h>
#include "dc_measurement.h"
#include "display.h"
#include "msp430f4250.h"


//#define FIRMWARE_VERSION 13

#define UPDATE_SCREEN_FLAG 0x01
#define LOW_BAT_FLAG 0x02
#define RESET_COUNT_FLAG 0x04
#define AUTO_ZERO_FLAG 0x08
#define SYS_INIT_FLAG 0x10
#define BOUNCE_FLAG 0x20

#define FIRMWARE_FLAG 0x20
#define FIRMWARE_VERSION_TIME 5 //time in seconds for button hold to show firmware version
#define FIRMWARE_VERSION 16 //removed offset and oversampling calculation
#define FIRMWARE_TICKS 32



#define SCREEN_LOW_BAT 0x40 //for lcd6flags
//#define INTERRUPT_FLAG 0x20

#define RESET_COUNT_TIME 8 // this number divided by 8 is the amount of seconds to wait on discharge

// Value Registers
static volatile long displayVal = 0;

// Display Registers
static long overrange = 200000;
static volatile unsigned char decimalPos;
static volatile unsigned char lcd6Flags=0;
static volatile unsigned char lcd7Flags=0;
static volatile long measurement = 0;

static volatile long temp = 0; //delete

static double compressionRatio = .06251; // = ((40000 + .025(40000))/65536) * .1;
//static double compressionRatio = 1.2502; //((40000 + .025(40000)))/(65536/2);

static volatile unsigned int firmware_counter = 0;
static volatile unsigned char flags = 0;//variable to hold flags


static volatile long offset_amount = 0;
static volatile int zero_timer_count = 0;
static volatile int bounce_count = 0;

static void reset();

//static long take_measurement(void);

/*Called when button has been held for FIRMWARE_VERSION_TIME seconds*/
void display_firmware(){
	IE2 &= ~BTIE;                    // disable Basic Timer interrupt
	lcd_update(FIRMWARE_VERSION, overrange, 1, lcd6Flags, lcd7Flags); // display overrange while circuit discharges
	while(P1IN & BIT3);
	IE2 |= BTIE;
}

/*Check for low battery, only called after reset*/
void battery_check(){
	 SD16CCTL0 = SD16SC + SD16UNI + SD16OSR_64;
	 SD16INCTL0 = SD16INCH_3;
     while((SD16CCTL0 & SD16IFG) == 0);
     measurement = SD16MEM0;
     if(measurement <= 35000 /*40000*/){
     	 flags |= LOW_BAT_FLAG;
    	 flags |= UPDATE_SCREEN_FLAG;
     }else{
    	 if(flags |= LOW_BAT_FLAG)
    		 flags &= ~LOW_BAT_FLAG;
    	 flags |= UPDATE_SCREEN_FLAG;
     }
     SD16INCTL0 = SD16INCH_0;
     SD16CCTL0 = SD16OSR_64+SD16SC;
}


/*Check for appropriate decimal placement.
 * Called once per main loop.*/
void check_decimal_place(){
	if((P6IN & BIT7) && (decimalPos !=1)){
		//decimalPos = 2;
		decimalPos = 1;
		flags |= UPDATE_SCREEN_FLAG;
		displayVal = overrange;
		flags |= AUTO_ZERO_FLAG;
		//zero_timer_count = 0;
		//PORT_1();
		reset();
	}
	else if((P6IN & BIT6) && (decimalPos != 3)){
		//decimalPos = 4;
		decimalPos = 3;
		flags |= UPDATE_SCREEN_FLAG;
		displayVal = overrange;
		flags |= AUTO_ZERO_FLAG;
		//PORT_1();
		//zero_timer_count = 0;
		reset();
	}
	else if((P6IN & BIT5)&&(decimalPos != 2)){
		//decimalPos = 3;
		decimalPos = 2;
		flags |= UPDATE_SCREEN_FLAG;
		displayVal = overrange;
		flags |= AUTO_ZERO_FLAG;
		reset();
		//PORT_1();
		//zero_timer_count = 0;
	}
}

/**
 * @brief init_sys Initializes PEMF meter.
 *
 */
void init_sys(void){
	//sleep(10);

	displayVal = 0;
	measurement = 0;
	flags = 0;
	decimalPos = 2;
	//TODO: Delay
    // Initialize Timers
    WDTCTL = WDTPW + WDTHOLD;         // Stop WDT
    FLL_CTL0 |= XCAP14PF + DCOPLUS;   // Set load capacitance for xtal, DCO Freq X 2.
    SCFI0 |= FN_4 + FLLD_4;           // Set DCO operating range
    SCFQCTL = 28;                     // ((28+1) x 32768) x 2(DCOPlus) x 4(FLLD_4) = 7.602176 Mhz
    BTCTL = BT_ADLY_125;              // 0.25s BT Int, Set LCD freq, LCD Update .25 seconds

    /*remove the following line of code after testing*/
   // BTCTL = BT_ADLY_2000;

    P6SEL |= BIT0 + BIT1;
    SD16CTL = SD16REFON+SD16SSEL0+SD16DIV_3;
    SD16INCTL0 = SD16INCH_0;                        // Change SD16 In channel 0
    SD16AE |= BIT0;
   // SD16AE |= BIT3;
    SD16CCTL0 = SD16OSR_64+SD16SC;
    P1DIR |= BIT0;
    P1OUT |= BIT0;
    P1DIR |= BIT4;
   // P1OUT &= ~BIT4;

    //set decimal lines
    P6DIR &= ~BIT4;
    P6DIR &= ~BIT5;
    P6DIR &= ~BIT6;
    P6DIR &= ~BIT7;

    // Interrupt Activation
    /*set port reset input interruptable*/
    P1DIR &= ~BIT3;
    P1IFG &= ~BIT3;
    P1IFG = 0;
    P1IE |= BIT3;
    P1OUT |= BIT4;//short analog front end
    P1IES &= ~BIT3;
    //__enable_interrupt();
    __enable_interrupt();                         // Enable general interrupts
    IE2 |= BTIE;                      // Enable Basic Timer interrupt

   flags |= UPDATE_SCREEN_FLAG;
   displayVal = overrange;
   flags |= AUTO_ZERO_FLAG; //set flag for discharge
   lcd_initialize();
  // flags |= AUTO_ZERO_FLAG;
    return;
}

/*
 * Main Program
 */
int main(void){
	init_sys();
	//flags |= AUTO_ZERO_FLAG;
    while(1){
		check_decimal_place();
		if(!(flags & AUTO_ZERO_FLAG)){
			while((SD16CCTL0 & SD16IFG) == 0);
			/*long*/temp = SD16MEM0;

			//temp -= offset_amount;
			//measurement = (65535 - temp) - 32768;
			measurement = temp - 32768;
			//measurement = temp;
			measurement *= compressionRatio;
			//measurement *= .1; //compensating for resistor value. (.109 is old value)
			//measurement *= .1;
			if(measurement - offset_amount < 0){
				measurement = 0;
			}
			else{
				measurement -= offset_amount;
			}
			//measurement -= offset_amount;
			//measurement *= compressionRatio;
			//if(measurement < 0){
			//	measurement = 0;
			//}
			if(measurement >= 2000){
				measurement = overrange;
				flags |= UPDATE_SCREEN_FLAG;
			}
			if(measurement > displayVal){
				displayVal = measurement;
				flags |= UPDATE_SCREEN_FLAG;
			}
			/*if(displayVal != measurement){
				displayVal = measurement;
				flags |= UPDATE_SCREEN_FLAG;
			}*/
		}
    }
}

/*
 * Timer Interrupt Function
 */
#pragma vector = BASICTIMER_VECTOR
__interrupt void BT_ISR(void){
	if(flags & FIRMWARE_FLAG){
		if(P1IN & BIT3){
			firmware_counter ++;
		}else{
			flags &= ~FIRMWARE_FLAG;
			firmware_counter = 0;
		}
		if(firmware_counter >= FIRMWARE_TICKS){
			display_firmware();
			firmware_counter = 0;
			flags &= ~FIRMWARE_FLAG;
			//may need to take out
			flags |= AUTO_ZERO_FLAG;
		}
	}
	//FIXME: These aren't being initialized correctly or are being incorrectly written somewhere
	lcd6Flags = 0;
	if(flags & LOW_BAT_FLAG){
		lcd6Flags |= SCREEN_LOW_BAT;
	}else{
		lcd6Flags &= ~SCREEN_LOW_BAT;
	}
	if(flags & AUTO_ZERO_FLAG){
		if(zero_timer_count >= RESET_COUNT_TIME){
			//if enough has passed for circuit to discharge calculate offset and turn circuit back on
			zero_timer_count = 0;
			P1IE |= BIT3;//turn on re-zero interrupt


			while((SD16CCTL0 & SD16IFG) == 0);
			/*long*/ temp = SD16MEM0;
			//offset_amount = temp;
			//measurement = (65535 - temp) - 32768;
			measurement = temp - 32768;
			measurement *= compressionRatio;
			//measurement *= .109; //compensating for resistor value.
			measurement *= .1;
			offset_amount = measurement;
			displayVal = 0;
			//if(displayVal < 0)
			//    displayVal = 0;
			measurement = 0;
			flags &= ~AUTO_ZERO_FLAG;
			flags |= UPDATE_SCREEN_FLAG;
			P1OUT &= ~BIT4;// open analog reset switch


			//__delay_cycles(500000);
			P1IE |= BIT3; // re-enable auto-zero interrupt
			//offset_amount = take_measurement();
			//flags |= UPDATE_SCREEN_FLAG;
		}else{
			zero_timer_count ++;
		}
	}
	//lcd6Flags=0;
	lcd7Flags=0;
	if(flags & UPDATE_SCREEN_FLAG){
	    lcd_update(displayVal, overrange, decimalPos, lcd6Flags, lcd7Flags);          // Send displayVal variable to BCD routine
	    flags &= ~ UPDATE_SCREEN_FLAG;
	    //mustUpdateScreen = 0;
   }
	IFG2 &= ~BTIFG;                     // Clear Basic Timer int flag
}

/*
 * Re-Zero Interrupt
 */
#pragma vector = PORT1_VECTOR
__interrupt void PORT_1(void){
//	flags |= FIRMWARE_FLAG;
//	//P1IE &= ~BIT3; // turn off re-zero interrupt
//	P1OUT |= BIT4; // short analog front end
//	flags |= AUTO_ZERO_FLAG;
//	P1IE &= ~BIT3;
//	IE2 &= ~BTIE;                    // disable Basic Timer interrupt
//	//P1OUT &= ~BIT4;
//	lcd_update(overrange, overrange, decimalPos, lcd6Flags, lcd7Flags); // display overrange while circuit discharges
//	//flags |= UPDATE_SCREEN_FLAG;
//    P1IFG &= ~BIT3; /*clear interrupt flag*/
//	IE2 |= BTIE; //re-enable basic timer interrupt
	reset();
}

void reset(){
	flags |= FIRMWARE_FLAG;
	//P1IE &= ~BIT3; // turn off re-zero interrupt
	P1OUT |= BIT4; // short analog switch
	flags |= AUTO_ZERO_FLAG;
	P1IE &= ~BIT3;
	IE2 &= ~BTIE;                    // disable Basic Timer interrupt
	//P1OUT &= ~BIT4;
	lcd_update(overrange, overrange, decimalPos, lcd6Flags, lcd7Flags); // display overrange while circuit discharges
	//flags |= UPDATE_SCREEN_FLAG;
	P1IFG &= ~BIT3; /*clear interrupt flag*/
	IE2 |= BTIE; //re-enable basic timer interrupt
}

/*static long take_measurement(){
	while((SD16CCTL0 & SD16IFG) == 0);
	long temp = SD16MEM0;
	measurement = (65535 - temp) - 32768;
	measurement *= compressionRatio;
	return measurement;
}*/
