/* Generating a tone using DDS
*/



// Counters
volatile double   PB3PinChangeCount  = 0;
volatile double   clkCycleCount      = 0;
volatile double   timer0IntCount     = 0;
volatile double   tempTimer0IntCount = 0;
volatile double   note_M1            = 0;
volatile double   refclk8            = 16000000;

// Flags
volatile int      flag_int0          = 0;
volatile double   inputWavlengthFlag = 0;

// Calculate word
volatile double          Note        = 500;                            // frequency of the output signal.
volatile const double    refrenceClk = 20000;                          // This is the interrupt sample rate of the dds. Even though it runs at 20khz, the signal generated is at 250khz
volatile unsigned long   tuningWord  = (pow(2,32)/ refrenceClk)*Note;  // This creats the step tuning ward

// Accumilaor Calculation
volatile unsigned long accumulator;   // This is the accumulater value.
volatile byte          icnt;          // This will indicate what value to call from the wave table.

// Table of 256 sine values / one sine period / stored in flash memory. 128(0) 255(90) 128(180) 0(270) 128(360).
PROGMEM  const char sine256[]  = {
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
  242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
  221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
  76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
  33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124
}; 







void setup() { 
  //// Setup ADC using PWM, OCR1B value varies the duty cycle, OCR1C(255) varies frequency. 64 MHz PLL / 256 = 250000hz frequency.
  PLLCSR  = 1 << PCKE  | 1 << PLLE;    // Enable PLL as Timer/Counter1 clock source, and start the PLL. 
  TIMSK   = 0;                         // Timer 1 interrupts turned OFF.  
  TCCR1   = 1 << CS10;                 // Timer clock prescale of 1.
  GTCCR   = 1 << PWM1B | 2 << COM1B0;  // Enable PWM mode based on comparator OCR1B; Clear the OC1B output line on compare match. Set when TCNT1 = $00. 
  pinMode(4, OUTPUT);                  // Enable PWM output pin.

  //// Setup for Timer/Counter0 interupt that will measure the input tone.
  GTCCR   |=   (1 << 7);      // Halt the timer while making timer changes.                        
  TCCR0A  |=   (1 << 1);      // CTC Mode; top=OCRA, update OCRA immediatly, TOV set at max.
  TCCR0B  &=  ~(1 << 0);      // 0 1 0 prescale value of 8.
  TCCR0B  |=   (1 << 1);     
  TCCR0B  &=  ~(1 << 2);     
  OCR0A    =   (49);          // 16Mhz / (49 + 1 * 8 * 2) - 20khz.
  TIMSK   |=   (1 << 4);      // Timer/Counter0 Output Compare Match A Interrupt Enable.
  GTCCR   &=  ~(1 << 7);      // Start the timer after making timer changes.

  //// Setup Input Capture on pcint3(PB3)
  GIMSK   |= 1 << 5;         // Pin Change Interrupts Enabled
  PCMSK   |= 1 << 3;         // Pin Change Interrupt Enabled on PB3
}





//// Timer/Counter0 interupt; every 20khz. Each interupt add word to the accumilator to step through sin wave.
ISR(TIMER0_COMPA_vect) {
  timer0IntCount  = timer0IntCount + 1;               // Count every time a interupt happens.
  
  accumulator  = accumulator + tuningWord;            // The accumulator
  icnt         = accumulator >> 24;                   // Top eight bits to indicate where we are stepping in the wave table
  OCR1B        = pgm_read_byte_near(sine256 + icnt);  // OCR1B is the register output for pin 4
}





//// Interrupt every time the pin changes value.
ISR(PCINT0_vect) {  
  PB3PinChangeCount++;

  // When a complete wavelength has occured.
  if (PB3PinChangeCount >= 3) {
    PB3PinChangeCount   = 1;                // Reset the PB3PinChangeCount.
    inputWavlengthFlag  = 1;                // inputWavlengthFlag is set, wavlength calculatons occur after ISR.

    clkCycleCount       = TCNT0;            // Timer count value is stored in memory.
    TCNT0               = 0;                // Reset the timer.
    tempTimer0IntCount  = timer0IntCount;   // Temporary version of timer0IntCount.
    timer0IntCount      = 0;                // Reset the timer0IntCount.
  }
}





//// Main Loop.
void loop() { 
  // Wavelength of the input signal is fully captured, this calculates a the frequency of the signal.
  if (inputWavlengthFlag >= 1) {
    inputWavlengthFlag    = 0;    // Reset flag.

    note_M1      = Note;          // Save the old Note value
    Note         = (refclk8)  / ((tempTimer0IntCount*50) + clkCycleCount);  // Calculate new note value base on the number of clock cycles that were counted durring the wavelength. 
    tuningWord   = (pow(2,32) / refrenceClk) * Note;                             // Calculate tuning word based on the new note.
  }
}























