#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

const uint8_t potX1pin = 3;
const uint8_t potY1pin = 4;

const uint8_t potX1pin = 8;
const uint8_t potY1pin = 9;

const uint8_t thumbstick1Xpin = A0;
const uint8_t thumbstick1Ypin = A1;
const uint8_t thumbstick2Xpin = A2;
const uint8_t thumbstick2Ypin = A3;


// Observed single axis input range seems to be 0 to 222 but I see 1-225 in PADDLE() reads. 
// * I did see some strange behavior with 2-axis with values above 210 on my XL and maybe at 0 or 1.  
//   A stable range seems to be 6-210 on my 800XL, this may need to change as I test more units.
//   
// Adding a short delay (rouglhly 31 from observations) as we're not using Stingray's 10k resistor
// and a rising edge to detect delay start
#define PULSE_DELAY 31
#define MIN_RANGE   6
#define MAX_RANGE   210
#define FRAME_END   257


// adding current/next values as updating x/y in main loop seemed to cause 
// some random problems with timing
static volatile uint16_t currPot1X = 115 + PULSE_DELAY;
static volatile uint16_t currPot1Y = 115 + PULSE_DELAY;
static volatile uint16_t nextPot1X = 115 + PULSE_DELAY;
static volatile uint16_t nextPot1Y = 115 + PULSE_DELAY;

static volatile uint16_t currPot2X = 115 + PULSE_DELAY;
static volatile uint16_t currPot2Y = 115 + PULSE_DELAY;
static volatile uint16_t nextPot2X = 115 + PULSE_DELAY;
static volatile uint16_t nextPot2Y = 115 + PULSE_DELAY;


static volatile uint16_t hline = 0;




// Timer 2 interrupt occurs at each 64us

ISR(TIMER2_COMPA_vect)
{
  if( hline < FRAME_END ) {  

    if (hline == currPot1Y) {
      // turn up y axis
      PORTD |= (1 << PD4);
    }

    if (hline == currPot1X) {
      // turn up x axis
      PORTD |= (1 << PD3);
    }
    ++hline;

  } else if (hline == FRAME_END) {
    // clear them
    PORTD &= ~((1 << PD3) | (1 << PD4));


    ACSR |= (1 << ACI);   // clear any pending interrupt bit
    ACSR |= (1 << ACIE);  // reactivate analog comparator
    ++hline;
  }


}

// Analog comparator, triggers when detect that pokey released POT inputs to charge
ISR(ANALOG_COMP_vect)
{
  // Keep  output pins low while POKEY is discharging the POT lines.
  PORTD &= ~((1 << PD3) | (1 << PD4) | (1 << PD8) | (1 << PD9));

  // update the current readings
  currPot1X = nextPot1X;
  currPot1Y = nextPot1Y;
  currPot2X = nextPot2X;
  currPot2Y = nextPot2Y;

  hline = 0;

  // resetting timer
  TCNT2 = 0;
  TIFR2 = (1 << OCF2A);

  ACSR &= ~(1 << ACIE); // disable comparator
}

void setup()
{

  pinMode(potX1pin, OUTPUT);
  pinMode(potY1pin, OUTPUT);
  pinMode(potX2pin, OUTPUT);
  pinMode(potY2pin, OUTPUT);

  digitalWrite(potX1pin, LOW);
  digitalWrite(potY1pin, LOW);
  digitalWrite(potX2pin, LOW);
  digitalWrite(potY2pin, LOW);


  // Setup Timer2
  TCCR2A = (0 << WGM20) // WGM[2..0] = 010 CTC mode, counts up overflow on OCR2A
    | (1 << WGM21); //
  TCCR2B = (0 << WGM22) //
    | (0 << CS20) // CS[2..0] = 010 Prescaler clock/8
    | (1 << CS21) //
    | (0 << CS22); //
  OCR2A = 127;          // reload value for 15748Hz (63.5us)

  TIMSK2 = (0 << OCIE2B) //  Enable interrupt on match
    | (1 << OCIE2A) //
    | (0 << TOIE2); //


  // Setup Analog Comparator
  ADCSRB = 0;             // (Disable) ACME: Analog Comparator Multiplexer Enable
  ACSR = (1 << ACI)     // (Clear) Analog Comparator Interrupt Flag
    | (1 << ACIE)    // Analog Comparator Interrupt Enable
    | (1 << ACIS1);  // Unlike Stingray/5200, I trigger on FALLING edge (when POKEY clamps to 0V)

  sei();
}

void loop()
{
  // Read modern thumbstick1sticks (0 to 1023)
  const uint16_t raw1X = analogRead(thumbstick1Xpin);
  const uint16_t raw1Y = 1023 - analogRead(thumbstick1Ypin);
  const uint16_t raw2X = analogRead(thumbstick2Xpin);
  const uint16_t raw2Y = 1023 - analogRead(thumbstick2Ypin);

  // map to usable range (rounded values()
  const uint16_t mapped1X = MIN_RANGE + (((uint32_t)raw1X * (MAX_RANGE - MIN_RANGE) + 511) / 1023) + PULSE_DELAY;
  const uint16_t mapped1Y = MIN_RANGE + (((uint32_t)raw1Y * (MAX_RANGE - MIN_RANGE) + 511) / 1023) + PULSE_DELAY;
  const uint16_t mapped2X = MIN_RANGE + (((uint32_t)raw2X * (MAX_RANGE - MIN_RANGE) + 511) / 1023) + PULSE_DELAY;
  const uint16_t mapped2Y = MIN_RANGE + (((uint32_t)raw2Y * (MAX_RANGE - MIN_RANGE) + 511) / 1023) + PULSE_DELAY;

  // save for next interrupt
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    nextPot1X = mapped1X;
    nextPot1Y = mapped1Y;
    nextPot2X = mapped2X;
    nextPot2Y = mapped2Y;
  }

  delay(5);
}
