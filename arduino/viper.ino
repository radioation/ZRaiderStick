#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define DEBUG 1   

#ifdef DEBUG
  #define DEBUG_BEGIN(speed) Serial.begin(speed)
  #define DEBUG_PRINT(x)     Serial.print(x)
  #define DEBUG_PRINTF(format, ...) { \
    char _dbgBuf[64]; \
    snprintf(_dbgBuf, sizeof(_dbgBuf), format, ##__VA_ARGS__); \
    Serial.println(_dbgBuf); \
  }
  #define DEBUG_PRINTLN(x)     Serial.println(x)
#else
  #define DEBUG_BEGIN(speed)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTF(x)
  #define DEBUG_PRINTLN(x)
#endif


const uint8_t potX1pin = 3;
const uint8_t potY1pin = 4;

const uint8_t potX2pin = 2;
const uint8_t potY2pin = 5;

const uint8_t thumbstick1Xpin = A0;
const uint8_t thumbstick1Ypin = A1;
const uint8_t thumbstick2Xpin = A2;
const uint8_t thumbstick2Ypin = A3;

// Global variables to store the absolute Min and Max values found
uint16_t min1X = 1023, max1X = 0;
uint16_t min1Y = 1023, max1Y = 0;
uint16_t min2X = 1023, max2X = 0;
uint16_t min2Y = 1023, max2Y = 0;


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

        if (hline == currPot2Y) {
      // turn up y axis
      PORTD |= (1 << PD5);
    }

    if (hline == currPot2X) {
      // turn up x axis
      PORTD |= (1 << PD2);
    }
    ++hline;

  } else if (hline == FRAME_END) {
    // clear them
    PORTD &= ~((1 << PD3) | (1 << PD4) | (1 << PD2) | (1 << PD5));


    ACSR |= (1 << ACI);   // clear any pending interrupt bit
    ACSR |= (1 << ACIE);  // reactivate analog comparator
    ++hline;
  }


}

// Analog comparator, triggers when detect that pokey released POT inputs to charge
ISR(ANALOG_COMP_vect)
{
  // Keep  output pins low while POKEY is discharging the POT lines.
  PORTD &= ~((1 << PD3) | (1 << PD4) | (1 << PD2) | (1 << PD5));

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

  #ifdef DEBUG
    //  get min/max vals
    if (raw1X < min1X) min1X = raw1X;  if (raw1X > max1X) max1X = raw1X;
    if (raw1Y < min1Y) min1Y = raw1Y;  if (raw1Y > max1Y) max1Y = raw1Y;
    if (raw2X < min2X) min2X = raw2X;  if (raw2X > max2X) max2X = raw2X;
    if (raw2Y < min2Y) min2Y = raw2Y;  if (raw2Y > max2Y) max2Y = raw2Y;

    DEBUG_PRINTF("ST1 X: [%d] Min:%d Max:%d", raw1X, min1X, max1X);
    DEBUG_PRINTF("ST1 Y: [%d] Min:%d Max:%d", raw1Y, min1Y, max1Y);
    DEBUG_PRINTF("ST2 X: [%d] Min:%d Max:%d", raw2X, min2X, max2X);
    DEBUG_PRINTF("ST2 Y: [%d] Min:%d Max:%d", raw2Y, min2Y, max2Y);
    DEBUG_PRINTF("-----------------------------------");
  #endif

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

