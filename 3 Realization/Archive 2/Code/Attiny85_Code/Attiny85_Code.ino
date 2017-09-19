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
*/
#import <avr/pgmspace.h>


double Note = 10000; // This does not acurately represent a note. 
const double refclk =20000; // This is the interrupt sample rate of the dds. Even though it runs at 20khz, the signal generated is at 250khz
volatile unsigned long tword_m = pow(2,32)* (Note/refclk);  // This creats the step tuning ward
volatile unsigned long Acc; // This is the accumulater value. This value guides how fast you cycle through the wave table
volatile byte icnt; // This will indicate where to call from the wave table


PROGMEM  const char sine256[]  = {
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
  242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
  221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
  76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
  33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124

};  //Wave table wtih 256 values

void setup() {
  // Enable 64 MHz PLL and use as source for Timer1
  PLLCSR = 1<<PCKE | 1<<PLLE;     

  // Set up Timer/Counter1 for PWM output
  TIMSK = 0;                     // Timer interrupts OFF
  TCCR1 = 1<<CS10;               // 1:1 prescale
  GTCCR = 1<<PWM1B | 2<<COM1B0;  // PWM B, clear on match

  pinMode(4, OUTPUT);            // Enable PWM output pin

  // Set up Timer/Counter0 for 20kHz interrupt to output samples.
  TCCR0A = 3<<WGM00;             // Fast PWM
  TCCR0B = 1<<WGM02 | 2<<CS00;   // 1/8 prescale
  TIMSK = 1<<OCIE0A;             // Enable compare match, disable overflow
  OCR0A = 49;                    // Divide by 400
}

void loop() { }

ISR(TIMER0_COMPA_vect) {
  Acc = Acc + tword_m;  //The accumulator
OCR1B = (Acc >> 8) & 0x80;
} 
