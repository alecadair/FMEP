
#include "dc_measurement.h"
#include "msp430f4250.h"
//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

/*
//---------- Found in dc_measurement.c ---------//
long dc_measurement(unsigned char settings, long temperature, long offset_value);
*/

//------------------------------------------------------------------------------
// Declarations
//------------------------------------------------------------------------------

//FIXME: Are these in the right order?
//(rangeCeiling - rangeFloor) + 100

//const long HALF_SIZE[2]=
//{
//  200100,        // Standard offset (Two's Compliment)       ((+200000) - (-200000)) + 100
//  80100          // High Stability offset (Two's Compliment) ((+80000) - (-80000)) + 100
//};
//
//const double COMP_RATIO[2]=
//{
//  6.106660563, // Standard Compression (200100/65535)*2 + gain pot fudge factor
//  2.444495308  // High Stability
//
//	//0.122222900,       	// Standard Compression
//	//0.305328369,     		// High Stability
//
//};

float lastMeasurement=0;


//------------------------------------------------------------------------------
// DC Function (forced average routine)
//------------------------------------------------------------------------------

long dc_measurement(unsigned char settings, long offset_value)
{  
	//This can be used to measure latency using oscilloscope on P1.1
	//P1OUT|=BIT1;
	//IE2 &= ~BTIE;
    //long temp_value = 0;                // SD16CH2 individual result
//    float sample[5];                     // SD16CH2 individual result
    float result = 0;                    // Temp register for calcs
    int i;
    long innerBuff=0;
    while((SD16CCTL0 & SD16IFG) == 0);
    innerBuff = SD16MEM0;
	//result = innerBuff*.305;
//    if(result - lastMeasurement < 10 && result - lastMeasurement > -10){
//
//    	result=(result + lastMeasurement)*0.5;
//    }
//    lastMeasurement=result;
 //   result -= offset_value;
    //P1OUT&=~BIT1;
  //  P1IE &= ~(POWER | AUTO_ZERO);
	//IE2 |= BTIE;
   return innerBuff;
}
