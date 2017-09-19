/*
DDS Tone Shield Rev 01
Author: Douglas Bahr
Date: 11/11/2016
Project: Create an arduino shield to generate a sine wave to drive a speaker. The shield usses a DDS to create a sine wave with the code offloaded onto an ATtiny85

History: This code is generated using the combination of projects found on these pages.
http://www.technoblogy.com/show?QVN
http://interface.khm.de/index.php/lab/interfaces-advanced/arduino-dds-sinewave-generator/
Generating a tone using DDS to be programmed onto an attiny85
How to program an attiny85
http://highlowtech.org/?p=1695

Current issues to be resolved: 

     x) 'Note' doesn't drive the program liniarly with with the indicated frequency
     x) As part of DDS Tone shield, there isn't anything in this program to communicate with the arduino tone() function to modify the frequency
x) There isn't a state switch to change from arduino input to a 0-5v output of a 5k pot
x) Ther isn't an LED state to corrospond to manually driven vs arduino tone() driven 





Notes
The first section turns on the 64MHz Phase-Locked Loop (PLL), and specifies this as the clock source for Timer/Counter1.

Timer/Counter1 is then set up in PWM mode, to make it act as an analogue-to-digital converter.
    Using the value in OCR1B to vary the duty cycle. 
    The frequency of the square wave is specified by OCR1C
        we leave it at its default value, 255, which divides the clock by 256, giving a 250kHz square wave.

Timer/Counter0 is used to generate an interrupt to output the samples. 
    The rate of this interrupt is the 8MHz system clock divided by a prescaler of 8
        and a value in OCR0A of 49+1, giving 20kHz. 

The interrupt calls an Interrupt Service Routine ISR(TIMER0_COMPA_vect) which calculates and outputs the samples.

As an example, here's the calculation showing what note you get for the specified value of Note, 857. 
    The Interrupt Service Routine is called once every 20kHz,
    and each time Note is added into the 16-bit phase accumulator, Acc.
    The top bit of Acc will therefore change with a frequency of 20000/(65536/857) or 261.5 Hz, middle C.

*/



#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#import <avr/pgmspace.h>




// calculate word
int                    Note     = 10;                          // about1.3hz                       // note freq. This does not acurately represent a note. 857 = Middle C
const int              refclk   = 10000;                       // This is the interrupt sample rate of the dds. Even though it runs at 20khz, the signal generated is at 250khz
volatile unsigned long tword_m  =        (Note*pow(2,32)) / refclk;  // This creats the step tuning ward
volatile int          flag_int0 = 0;

// Accumilaor calculation
volatile unsigned long Acc;                                   // This is the accumulater value. This value guides how fast you cycle through the wave table
volatile byte          icnt;                                // This will indicate where to call from the wave table

// Counters
volatile int count_20khz = 0;
volatile int temp_20khz  = 0;

// table of 256 sine values / one sine period / stored in flash memory    
// 128(0) 255(90) 128(180) 0(270) 128(360)
PROGMEM  const char sine256[]  = {
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
  242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
  221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
  76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
  33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124
};  




// Note: ditch the timer 0 and make it directly measure the uinput at all times and if none for a certin time set a dumb ass value
// have the timer count and stop at every time a input signal comes in.
void setup() {
  // Timer 1: DAC
  PLLCSR   = 1 << PCKE | 1 << PLLE;    // PCKE = Enable PCK clock as Timer/Counter1 clock source, PLLE = PLL is started and if needed internal RC-oscillator  PLL generates freq that is 8X input. 64Mhz 
  TIMSK    = 0;                        // Turn timer interrupts OFF // Set up Timer/Counter1 for PWM output
  TCCR1   |=   1 << CS10;              // prescale = 1 = clk; 
  GTCCR   |=  (1 << 6) | (1 << 5);     // PWM based on comparator OCR1B; Clear the OC1B output line on match
  //OCR1B   |=  10000000;                // 8 bit
  
  pinMode(4, OUTPUT);                  // Enable pin 4(PB$) as the signal output.


  // Timer 0: Input frequency sample width adjusment/ Every 20khz interupt tiggers and the signal is updated. 
  TCCR0A = 3 << WGM00;             // Fast PWM
  TCCR0B = 1 << WGM02 | 2<<CS00;   // 1/8 prescale
  TIMSK  = 1 << OCIE0A;            // Enable compare match, disable overflow
  OCR0A  = 49;                     // Divide by 400




  // Timer0 atah reads every input signal as fast as possible
  /*GCTCCR |= 1 << 7;   // Halt the timer while making changes
  TCCR0A |= 


  TCCR0B |= 


  OCR0B  |= 11111111; // Output Compare Register B contains an 8-bit value that is continuously compared with the counter value
  TIMSK  |= 1 << 3;   // Whhen OCIE0B bit is high, This enables Match B interrupt. Interupt occurs when compare match in timer occurs
  TIMSK  |= 1 << 1;   // Interupt ocurs when timer0 overflows 
  GCTCCR &= 1 << 7;   // Start the timer after making changes
*/

  // Input Capture on pcint3(PB3)
  GIMSK |= 1 << 5;      // Pin Change Interrupts Enabled
  PCMSK |= 1 << 5;      // Pin Change Interrupts enabled on PB3
  DDRB  |= 1 << 5;

  // Testing
  DDRB   |= 1 << 1;         // enable PB0 as an outputs
  PORTB  ^= 1 << 1;         // Toggle its Value
}




/* 
 * Recalculate the signal frequency of the sine wave
 *  
 */
void loop() { 
  if(flag_int0 == 3) {  // When the input capture has completed a wavelenth.
    flag_int0 = 1;  
    
    Note        = refclk / temp_20khz;         // Input frequency is the refrence clock frequency over number of refrence clock sycles. Ex. 20khz/10cycles durring the input capture eriod = 2khz input signal.     
    tword_m     = pow(2,32) * (Note / refclk); // Ex. 2khz/20khz = .1 * 2^32, it is essentially a fraction of the 2^32 value?

    //PORTB ^= 1 << PB0; // Change the let value for testing purpose
    }
}




/* 
 * Interupt Routine for the 20khz signal
 *  
 *  Every 20khz this happens
 */
ISR(TIMER0_COMPA_vect) {
  Acc          = Acc + tword_m;                         // The accumulator
  icnt         = Acc >> 24;                             // Top eight bits to indicate where we are stepping in the wave table
  OCR1B        = pgm_read_byte_near(sine256 + icnt);    // OCR1B is the register output for pin 4

  count_20khz  = count_20khz + 1;
} 





/* 
 *  Interupt routine for the input capture
 *  
 *  Interupt every time the signal changes.
 */
ISR(PCINT0_vect) {    
  PORTB ^= 1 << 1; // Change the let value for testing purpose
 
}

































/* Leftover
 *  Interupt routine for the input capture
 *  
 *  Interupt every time the signal goes high
 *
ISR(INT0_vect) {
  flag_int0    = 1;

  temp_20khz   = count_20khz;
  count_20khz  = 0;
}*/




/*  
  //TCCR0A  = 1 << 0;              // Fast PWM, top(OCRA), Update OCRx(Bottom), TOV set(TOP)
  //TCCR0A  = 1 << 1;
  //TCCR0B  = 1 << 3;
  //TCCR0B  = 1 << 1;              // 1/8 prescale. Clk_io  / 8 (From prescaler)
  //TIMSK   = 1 << 4;              // Enable compare match interrupt, disable overflow
  //OCR0A   = 0;                   // Divi  de by 400
  // Interupt Input
  // GIMSK |= (1 << PCIE);       // pin change interrupt enable
  // PCMSK |= (1 << PCINT3);     // pin change interrupt enabled for PCINT4
  // sei();                      // enable interrupts TIMSK

  // Input Pin Interupt
  // into
  //GIMSK |=  (1 << 6);            // The external pin interrupt is enabled
  //MCUCR |=  (1 << 1);            // The rising edge of INT0 generates an interrupt request
  //MCUCR |=  (1 << 0);
*/















