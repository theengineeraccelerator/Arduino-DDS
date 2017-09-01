/*
 Generating a tone using DDS
*/



// Counter
volatile double             count1 = 0;
volatile double              flag1 = 0;
volatile double              flag2 = 0;

volatile double          clk_count = 0;////////////////////////
volatile double          overCount = 0;////////////////////////
volatile double       temoverCount = 0;////////////////////////
volatile double             noteM1 = 0;////////////////////////
volatile double          refclk8   = 16000000;
volatile int             flag_int0 = 0;



// calculate word
volatile       double    Note     = 500;                          // about1.3hz                       // note freq. This does not acurately represent a note. 857 = Middle C
volatile const double    refclk   = 20000;                       // This is the interrupt sample rate of the dds. Even though it runs at 20khz, the signal generated is at 250khz
volatile unsigned long   tword_m  = (pow(2,32)/ refclk)*Note;  // This creats the step tuning ward


// Accumilaor calculation
volatile unsigned long Acc;                                   // This is the accumulater value. This value guides how fast you cycle through the wave table
volatile byte          icnt;                                // This will indicate where to call from the wave table

// Table of 256 sine values / one sine period / stored in flash memory    
// 128(0) 255(90) 128(180) 0(270) 128(360)
PROGMEM  const char sine256[]  = {
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
  242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
  221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
  76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
  33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124
}; 










void setup() {
  // Enable 64 MHz PLL and use as source for Timer1 8*clk =128Mhz = 500000hz signal
  PLLCSR = 1 <<PCKE | 1<<PLLE;     

  // Set up Timer/Counter1 for PWM output
  TIMSK = 0;                     // Timer interrupts OFF
  TCCR1 = 1 << CS10;               // 1:1 prescale
  GTCCR = 1 << PWM1B | 2 << COM1B0;  // PWM B, clear on match

  pinMode(4, OUTPUT);            // Enable PWM output pin


  // Set up Timer/Counter0 for 20kHz interrupt to output samples.
  //TCCR0A = 3<<WGM00;             // Fast PWM
  //TCCR0B = 1<<WGM02 | 2<<CS00;   // 1/8 prescale
  //TIMSK  = 1<<OCIE0A;             // Enable compare match, disable overflow
  //OCR0A  = 49;                    // Divide by 400


  // Set up Timer/Counter0 for interrupt  clk
  GTCCR  |=   (1 << 7);     // Halt the timer while making changes
  TCCR0A &=  ~(1 << 0);     // CTC Mode, top=OCRA, update OCRx immediatly, flag set at max
  TCCR0A |=   (1 << 1); 
  TCCR0B |=   (1 << 0);     // 0 0 1 no prescale. 2=8, 3=64, 4=256, 5=1024
  TCCR0B |=   (1 << 1);
  TCCR0B &=  ~(1 << 2);
  // TCNT0 Timer/Counter Register gives direct access, both for read and write operations,
  OCR0A     =  (49);
  //OCR0B   = (49);
  TIMSK    |= 1 << 4;        // a match.
  //TIMSK  |= 1 << 3;      // Whhen OCIE0B bit is high, This enables Match B interrupt. Interupt occurs when compare match in timer occurs
  //TIMSK  |= 1 << 1;      // Interupt ocurs when timer0 overflows 
  GTCCR    &= ~(1 << 7);     // Start the timer after making changes

  // Input Capture on pcint3(PB3)
  GIMSK   |= 1 << 5;         // Pin Change Interrupts Enabled
  PCMSK   |= 1 << 3;         // Pin Change Interrupts enabled on PB3
}









// Every 20khz
ISR(TIMER0_COMPA_vect) {
  overCount = overCount + 1;
  
  Acc          = Acc + tword_m;                         // The accumulator
  icnt         = Acc >> 24;                             // Top eight bits to indicate where we are stepping in the wave table
  OCR1B        = pgm_read_byte_near(sine256 + icnt);    // OCR1B is the register output for pin 4
}


// each chane on pin
ISR(PCINT0_vect) {  ///*
  count1++;


  if (count1 >= 3) {
    count1 = 1;
    flag1 = 1;

    clk_count    = TCNT0;        ///////////////////////
    TCNT0        = 0;                //////////////////////////////
    temoverCount = overCount;     ///////////////
    overCount    = 0;            ///////////////////////////
  }  //*/
}


//
void loop() {  ///* 
   if (flag1 >= 1) {
    flag1       = 0;

    noteM1      = Note;
    Note        = (refclk8) / ((temoverCount*50) + clk_count);////////////////////////
    tword_m     = (pow(2,32)/ refclk)*Note;
  }
//*/


}






















/*  
  // Set up Timer/Counter0 for interrupt
  GTCCR  |=  (1 << 7);     // Halt the timer while making changes
  TCCR0A &= ~(1 << 0);     // CTC Mode, top=OCRA, update OCRx immediatly, flag set at max
  TCCR0A |=  (1 << 1); 
  TCCR0B |=  (1 << 0);     // 0 0 1 no prescale. 2=8, 3=64, 4=256, 5=1024
  TCCR0B &= ~(1 << 1);
  TCCR0B &= ~(1 << 2);
  // TCNT0 Timer/Counter Register gives direct access, both for read and write operations,
  OCR0A  = (49);
  //OCR0B  = (49);
  TIMSK  |= 1 << 4;        // a match.
  //TIMSK  |= 1 << 3;      // Whhen OCIE0B bit is high, This enables Match B interrupt. Interupt occurs when compare match in timer occurs
  //TIMSK  |= 1 << 1;      // Interupt ocurs when timer0 overflows 
  GTCCR  &= ~(1 << 7);     // Start the timer after making changes
  





    // Set up Timer/Counter0 for interrupt
  GTCCR   |=  (1 << 7);     // Halt the timer while making changes
  TCCR0A  |=  (1 << 0);     //  fast pwm
  TCCR0A  |=  (1 << 1); 
  TCCR0B  |=  (1 << 3);
  
  TCCR0B  |=  (1 << 0);     // 0 0 1 no prescale. 2=8, 3=64, 4=256, 5=1024
  TCCR0B  |=  (1 << 1);
  TCCR0B  &= ~(1 << 2);
  // TCNT0 Timer/Counter Register gives direct access, both for read and write operations,
  OCR0A    = (49);
  TIMSK   |= 1 << 4;        // a match.
  GTCCR   &= ~(1 << 7);     // Start the timer after making changes
  */
























/*  
    // Set up Timer/Counter0 for 20kHz interrupt to output samples.
   //TCCR0A = 3<<WGM00;             // Fast PWM
   //TCCR0B = 1<<WGM02 | 2<<CS00;   // 1/8 prescale
   //TIMSK  = 1<<OCIE0A;             // Enable compare match, disable overflow
   //OCR0A  = 49;                    // Divide by 400
  
  
  
  
  
  
  
  
  










  
  
  
  
  
  
  // Set up Timer/Counter0 for interrupt
  GTCCR  |=  (1 << 7);     // Halt the timer while making changes
  TCCR0A &= ~(1 << 0);     // CTC Mode, top=OCRA, update OCRx immediatly, flag set at max
  TCCR0A |=  (1 << 1); 
  TCCR0B |=  (1 << 0);     // 0 0 1 no prescale. 2=8, 3=64, 4=256, 5=1024
  TCCR0B &= ~(1 << 1);
  TCCR0B &= ~(1 << 2);
  // TCNT0 Timer/Counter Register gives direct access, both for read and write operations,
  OCR0A  = (49);
  //OCR0B  = (49);
  TIMSK  |= 1 << 4;        // a match.
  //TIMSK  |= 1 << 3;      // Whhen OCIE0B bit is high, This enables Match B interrupt. Interupt occurs when compare match in timer occurs
  //TIMSK  |= 1 << 1;      // Interupt ocurs when timer0 overflows 
  GTCCR  &= ~(1 << 7);     // Start the timer after making changes
  





    // Set up Timer/Counter0 for interrupt
  GTCCR   |=  (1 << 7);     // Halt the timer while making changes
  TCCR0A  |=  (1 << 0);     //  fast pwm
  TCCR0A  |=  (1 << 1); 
  TCCR0B  |=  (1 << 3);
  
  TCCR0B  |=  (1 << 0);     // 0 0 1 no prescale. 2=8, 3=64, 4=256, 5=1024
  TCCR0B  |=  (1 << 1);
  TCCR0B  &= ~(1 << 2);
  // TCNT0 Timer/Counter Register gives direct access, both for read and write operations,
  OCR0A    = (49);
  TIMSK   |= 1 << 4;        // a match.
  GTCCR   &= ~(1 << 7);     // Start the timer after making changes
  */

