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


//#define UPDATE_SCREEN_FLAG 0x01
//#define LOW_BAT_FLAG 0x02
#define SCREEN_LOW_BAT 0x01
//#define RESET_COUNT_FLAG 0x04
//#define AUTO_ZERO_FLAG 0x08
//#define SYS_INIT_FLAG 0x10
//#define BOUNCE_FLAG 0x40
//#define FIRMWARE_FLAG 0x20

#define FIRMWARE_VERSION_TIME 5 //time in seconds for button hold to show firmware version
#define FIRMWARE_VERSION 17		//removed offset and oversampling calculation
#define FIRMWARE_TICKS 32		//this number divided by eight is the amount of time in seconds reset must be held to show firmware

#define RESET_COUNT_TIME 6 // this number divided by 8 is the amount of seconds to wait on discharge

struct{
	unsigned int update_screen : 1;
	unsigned int low_battery : 1;
	unsigned int reset_mode : 1;
	unsigned int display_firmware : 1;
	unsigned int reset_bounce : 1;

}flag;

struct flag;

static const long OVER_RANGE = 200000;

static volatile unsigned char decimal_position;	//current position of decimal from the right of screen - indicates which range meter is in
static volatile unsigned char lcd6Flags=0;	//flags for display driver (In poll's voice "JOOOOSSSHHHH")
static volatile unsigned char lcd7Flags=0;
static volatile long measurement = 0;		//last valued measured by the ADC
static volatile long display_value = 0;		//current value that should be written to the display


static volatile long temp = 0; //delete

static double compressionRatio = .06251; // = ((40000 + .025(40000))/65536) * .1;

static volatile unsigned int firmware_counter = 0;
//static volatile unsigned char flags = 0;//variable to hold flags


static volatile long offset_amount = 0;
static volatile int zero_timer_count = 0;
static volatile int bounce_count = 0;

static void reset();


/*Called when button has been held for FIRMWARE_VERSION_TIME seconds*/
void display_firmware(){
	IE2 &= ~BTIE;                    // disable Basic Timer interrupt
	lcd_update(FIRMWARE_VERSION, OVER_RANGE, 1, lcd6Flags, lcd7Flags); // display overrange while circuit discharges
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
    	 flag.low_battery = 1;
     }else{
    	 if(flag.low_battery)
    		 flag.low_battery = 0;
     }
     flag.update_screen = 1;
     SD16INCTL0 = SD16INCH_0;
     SD16CCTL0 = SD16OSR_64+SD16SC;
}


/*Check for appropriate decimal placement.
 * Called once per main loop.*/
void check_decimal_place(){
	if((P6IN & BIT7) && (decimal_position !=1)){
		//decimal_position = 2;
		decimal_position = 1;
		flag.update_screen = 1;
		display_value = OVER_RANGE;
		flag.reset_mode = 1;
		reset();
	}
	else if((P6IN & BIT6) && (decimal_position != 3)){
		decimal_position = 3;
		flag.update_screen =1;
		display_value = OVER_RANGE;
		flag.reset_mode = 1;
		reset();
	}
	else if((P6IN & BIT5) && (decimal_position != 2)){
		decimal_position= 2;
		flag.update_screen = 1;
		display_value = OVER_RANGE;
		flag.reset_mode = 1;
		reset();
	}
}

/**
 * @brief init_sys Initializes PEMF meter.
 *
 */
void init_sys(void){
	display_value = 0;
	measurement = 0;
	flag.display_firmware = 0;
	flag.low_battery = 0;
	flag.reset_mode = 0;
	flag.update_screen = 0;

	decimal_position = 2;

    WDTCTL = WDTPW + WDTHOLD;         // Stop WDT
    FLL_CTL0 |= XCAP14PF + DCOPLUS;   // Set load capacitance for xtal, DCO Freq X 2.
    SCFI0 |= FN_4 + FLLD_4;           // Set DCO operating range
    SCFQCTL = 28;                     // ((28+1) x 32768) x 2(DCOPlus) x 4(FLLD_4) = 7.602176 Mhz
    BTCTL = BT_ADLY_125;              // 0.25s BT Int, Set LCD freq, LCD Update .25 seconds


    P6SEL |= BIT0 + BIT1;
    SD16CTL = SD16REFON+SD16SSEL0+SD16DIV_3;
    SD16INCTL0 = SD16INCH_0;                        // Change SD16 In channel 0
    SD16AE |= BIT0;
    SD16CCTL0 = SD16OSR_64+SD16SC;

    P1DIR |= BIT0;
    P1OUT |= BIT0;
    P1DIR |= BIT4;

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
    P1OUT |= BIT4;			//short analog front end
    P1IES &= ~BIT3;

    __enable_interrupt();	// Enable general interrupts
    IE2 |= BTIE;			// Enable Basic Timer interrupt

   /*fix flags and delete following lines of code*/
   flag.update_screen = 1;
   //flags |= UPDATE_SCREEN_FLAG;
   display_value = OVER_RANGE;
   flag.reset_mode = 1;
   lcd_initialize();
   return;
}

/*
 * Main Program
 */
int main(void){
	init_sys();
    while(1){
		check_decimal_place();
		if(!flag.reset_mode){
			while((SD16CCTL0 & SD16IFG) == 0);
			temp = SD16MEM0;
			measurement = temp - 32768;
			measurement *= compressionRatio;
			if(measurement - offset_amount < 0){
				measurement = 0;
			}
			else{
				measurement -= offset_amount;
			}
			if(measurement >= 2000){
				measurement = OVER_RANGE;
				flag.update_screen = 1;
			}
			if(measurement > display_value){
				display_value = measurement;
				flag.update_screen = 1;
			}
		}
    }
}

/*
 * Timer Interrupt Function
 */
#pragma vector = BASICTIMER_VECTOR
__interrupt void BT_ISR(void){
	if(flag.display_firmware){
		if(P1IN & BIT3){
			firmware_counter ++;
		}else{
			flag.display_firmware = 0;
			firmware_counter = 0;
		}
		if(firmware_counter >= FIRMWARE_TICKS){
			display_firmware();
			firmware_counter = 0;
			flag.display_firmware = 0;
			flag.reset_mode = 1;
		}
	}
	lcd6Flags = 0;
	if(flag.low_battery){
		lcd6Flags |= SCREEN_LOW_BAT;
	}else{
		lcd6Flags &= ~SCREEN_LOW_BAT;
	}
	if(flag.reset_mode){
		P1IE &= ~BIT3;//disable reset interrupt
		if(zero_timer_count >= RESET_COUNT_TIME){
			//if enough has passed for circuit to discharge calculate offset and turn circuit back on
			if(!flag.reset_bounce){
				while((SD16CCTL0 & SD16IFG) == 0);
				temp = SD16MEM0;
				measurement = temp - 32768;
				measurement *= compressionRatio;
				offset_amount = measurement;
				display_value = 0;
				measurement = 0;
				P1OUT &= ~BIT4;// open analog reset switch
				flag.reset_bounce = 1;
			}else{
				if(bounce_count >= 4){
					zero_timer_count = 0;
					bounce_count = 0;
					//while((SD16CCTL0 & SD16IFG) == 0);
					//temp = SD16MEM0;
					//measurement = temp - 32768;
					//measurement *= compressionRatio;
					//offset_amount = measurement;
					display_value = 0;
					measurement = 0;
					flag.reset_mode = 0;
					flag.reset_bounce = 0;
					P1IE |= BIT3; // re-enable auto-zero interrupt
					flag.update_screen = 1;
				}else{
					bounce_count ++;
				}
			}
		}else{
			zero_timer_count ++;
		}
	}
	lcd7Flags=0;
	if(flag.update_screen){
	    lcd_update(display_value, OVER_RANGE, decimal_position, lcd6Flags, lcd7Flags);          // Send display_value variable to BCD routine
	    flag.update_screen = 0;
   }
	IFG2 &= ~BTIFG;                     // Clear Basic Timer int flag
}

/*
 * Re-Zero Interrupt
 */
#pragma vector = PORT1_VECTOR
__interrupt void PORT_1(void){
	reset();
}

void reset(){
	flag.display_firmware = 1;
	P1OUT |= BIT4; // short analog switch
	flag.reset_mode = 1;
	P1IE &= ~BIT3;					// disable reset interrupt
	IE2 &= ~BTIE;                    // disable Basic Timer interrupt
	lcd_update(OVER_RANGE, OVER_RANGE, decimal_position, lcd6Flags, lcd7Flags); // display overrange while circuit discharges
	P1IFG &= ~BIT3; /*clear interrupt flag*/
	IE2 |= BTIE; //re-enable basic timer interrupt
}
