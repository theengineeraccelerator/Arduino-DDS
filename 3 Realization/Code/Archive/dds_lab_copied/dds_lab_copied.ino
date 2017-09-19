/*
Arduino Sine wave Generator using the direct digital synthesis Method

Here we describe how to generate sine waves with an Arduino board in a very accurate way. 
Almost no additional hardware is required. 
The frequency range reaches form zero to 16 KHz with a resolution of a millionth part of one Hertz! 
Distortions can be kept less than one percent on frequencies up to 3 KHz. 
This technique is not only useful for music and sound generation another range of application is test equipment or measurement instrumentation. 
Also in telecommunication the DDS Method is useful for instance in frequency of phase modulation (FSK PSK).



The DDS Method (digital direct synthesis)
To implement the DDS Method in software we need four components.

A accumulator              - A long integer variable
A tuning word              - A long integer variable
A sinewave table           - A list of numerical values of one sine period stored as constants.
A digital analog converter - which is provided by the PWM (analogWrite) unit
a reference clock derived by a internal hardware timer in the atmega. 


To the accumulator, the tuning word is added, 
the most significant byte of the accu is taken as address of the sinetable where the value is fetched and outputted as analog value bye the PWM unit. 

The whole process is cycle timed by an interrupt process which acts as the reference clock. 


Software implementation

To run this software on an Arduino Diecimila or Duemilenove connect a potentiometer to  +5Volt and Ground and the wiper to analog 0. 
The frequency appears on pin 11 where you can connect active speakers or an output filter described later.

*/



/*
 *
 * DDS Sine Generator mit ATMEGS 168
 * Timer2 generates the  31250 KHz Clock Interrupt
 *
 * KHM 2009 /  Martin Nawrath
 * Kunsthochschule fuer Medien Koeln
 * Academy of Media Arts Cologne

 */
int  ledPin  = 13;                 // LED pin 7
int  testPin = 7;
int  t2Pin   = 6;
byte bb;





#include "avr/pgmspace.h"

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))


// table of 256 sine values / one sine period / stored in flash memory
// 128(0) 255(90) 128(180) 0(270) 128(360)
PROGMEM  prog_uchar sine256[]  = {
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
  242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
  221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
  76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
  33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124

};

const double refclk = 31376.6;          // 31372.549  = 16MHz / 510. from timer2
double       dfreq;

// variables used inside interrupt service declared as voilatile
volatile byte icnt;              // var inside interrupt
volatile byte icnt1;             // var inside interrupt
volatile byte c4ms;              // counter incremented all 4ms
volatile unsigned long phaccu;   // pahse accumulator
volatile unsigned long tword_m;  // dds tuning word m




void setup()  {
  // Print the name of the project as the app starts.
  pinMode(ledPin, OUTPUT);      // Sets the digital pin as output
  Serial.begin(115200);         // Connect to the serial port, Sets the data rate in bits per second (baud) for serial data transmission
  Serial.println("DDS Test");   // Send message along the output pin

  // Set the output pins
  pinMode(6, OUTPUT);      // sets the digital pin as output
  pinMode(7, OUTPUT);      // sets the digital pin as output
  pinMode(11, OUTPUT);     // pin11 = PWM  output / frequency output

  // Setup PWM
  Setup_timer2();

  // Interupt setup
  cbi (TIMSK0,TOIE0);              // Disable Timer0!!! delay() is now not available
  sbi (TIMSK2,TOIE2);              // enable Timer2 Interrupt. Timer/Counter2 Overflow interrupt is enabled.

  // how is it calculated
  dfreq   = 1000.0;                    // initial output frequency = 1000.0Hz, freq that we want the sie wave to be at
  tword_m = pow(2,32) * dfreq/refclk;  // calulate DDS new tuning word. (2^32) * 1000/31376.6. 136,887,024.98
}

void loop() {
  while(1) {
     if (c4ms > 250) {                   // timer / wait fou a full second
      c4ms   = 0;
      dfreq  = analogRead(0);            // read Poti voltage on analog pin 0 to adjust output value from 0..1023 Hz which is the frequency

      cbi (TIMSK2,TOIE2);                // disble Timer2 Interrupt
      tword_m = pow(2,32) * dfreq/refclk;  // calulate DDS new tuning word
      sbi (TIMSK2,TOIE2);                // enable Timer2 Interrupt 

      Serial.print(dfreq);
      Serial.print("  ");
      Serial.println(tword_m);
    }

   sbi(PORTD,6); // Test / set PORTD,7 high to observe timing with a scope
   cbi(PORTD,6); // Test /reset PORTD,7 high to observe timing with a scope
  }
 }

//******************************************************************
// timer2 setup. pin 11
// Prscaler set to 1. PWM mode to phase correct PWM.
// The PWM frequency for the output when using phase correct PWM is calculated: (f_clkIO / Pre_Scale * 510). 16,000,000 / 510 = 31,372.55Hz
void Setup_timer2() {
  // Timer2 clk T2S /(No prescaling)
  sbi (TCCR2B, CS20);
  cbi (TCCR2B, CS21);
  cbi (TCCR2B, CS22);

  // Set the COM2A to 10 which will clear OC2A on Compare Match
  // For ensuring compatibility with future devices, this bit must be set to zero when TCCR2B is written when operating in PWM mode
  // Setting the COM2x1:0 bits to two will produce a non-inverted PWM
  cbi (TCCR2A, COM2A0);  
  sbi (TCCR2A, COM2A1);  

  // Waveform Generation Mode 1 - Phase Correct PWM. Update of OCRx at TOP, TOV Flag Set on BOTTOM.
  sbi (TCCR2A, WGM20);  
  cbi (TCCR2A, WGM21);
  cbi (TCCR2B, WGM22);
}


//******************************************************************
// Timer2 Interrupt Service at 31,372,550 KHz = 32uSec. 
// This is the timebase REFCLOCK for the DDS generator
// FOUT = (M (REFCLK)) / (2 exp 32)
// runtime : 8 microseconds ( inclusive push and pop)
ISR(TIMER2_OVF_vect) {
  // Test location to observe timing with a oscope
  sbi(PORTD,7);                     // Test / Set PORTD pin 7 high 

  // 
  phaccu   =   phaccu  +  tword_m;                  // soft DDS, phase accu with 32 bits. phaccu accumilates the word every itteration
  icnt     =   phaccu >> 24;                        // use upper 8 bits for phase accu as frequency information          

  // The Output Compare Register A contains an 8-bit value that is continuously compared with the counter value (TCNT2). A match can be used to generate an Output Comp are interrupt, or to generate a waveform output on the OC2A pin.
  // BOTTOM The counter reaches the BOTTOM when it becomes zero (0x00).MAX The counter reaches its MAXimum when it becomes 0xFF (decimal 255). 
  // TOPThe counter reaches the TOP when it becomes equal to the highest value in the count sequence. 
  // The TOP value can be assigned to be the fixed value 0xFF (MAX) or the value stored in the OCR2A Register. The assignment is dependent on the mode of operation.
  OCR2A    =   pgm_read_byte_near(sine256 + icnt);  // read value from ROM sine table and send to PWM DAC. sine256[icnt]

  if(icnt1++ == 125) {                              // increment variable c4ms all 4 milliseconds
    c4ms++;
    icnt1=0;
   }   

  // Test location
  cbi(PORTD,7);           // reset  PORTD pin 7 low 

}



















