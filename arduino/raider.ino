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

const uint8_t thumbXpin = A0;
const uint8_t thumbYpin = A1;

#ifdef DEBUG
// Global variables to store the absolute Min and Max values found
uint16_t minX = 1023, maxX = 0;
uint16_t minY = 1023, maxY = 0;
#endif



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
static volatile uint16_t currPotX = 115 + PULSE_DELAY;
static volatile uint16_t currPotY = 115 + PULSE_DELAY;
static volatile uint16_t nextPotX = 115 + PULSE_DELAY;
static volatile uint16_t nextPotY = 115 + PULSE_DELAY;

static volatile uint16_t hline = 0;




// Timer 2 interrupt occurs at each 64us

ISR(TIMER2_COMPA_vect)
{
  if( hline < FRAME_END ) {  

    if (hline == currPotY) {
      // turn up y axis
      PORTD |= (1 << PD4);
    }

    if (hline == currPotX) {
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
  // Keep both output pins low while POKEY is discharging the POT lines.
  PORTD &= ~((1 << PD3) | (1 << PD4));

  // update the current readings
  currPotX = nextPotX;
  currPotY = nextPotY;

  hline = 0;

  // resetting timer
  TCNT2 = 0;
  TIFR2 = (1 << OCF2A);

  ACSR &= ~(1 << ACIE); // disable comparator
}

void setup()
{
  DEBUG_BEGIN(9600);

  pinMode(potX1pin, OUTPUT);
  pinMode(potY1pin, OUTPUT);

  digitalWrite(potX1pin, LOW);
  digitalWrite(potY1pin, LOW);


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
  // Read modern thumbsticks (0 to 1023)
  const uint16_t rawX = analogRead(thumbXpin);
  const uint16_t rawY = 1023 - analogRead(thumbYpin);
#ifdef DEBUG
  //  get min/max vals
  if (rawX < minX) minX = rawX;  if (rawX > maxX) maxX = rawX;
  if (rawY < minY) minY = rawY;  if (rawY > maxY) maxY = rawY;
  DEBUG_PRINTF("ST1 X: [%d] Min:%d Max:%d", rawX, minX, maxX);
  DEBUG_PRINTF("ST1 Y: [%d] Min:%d Max:%d", rawY, minY, maxY);
  DEBUG_PRINTF("-----------------------------------");

#endif

  // map to usable range (rounded values()
  const uint16_t mappedX = MIN_RANGE +   (((uint32_t)rawX * (MAX_RANGE - MIN_RANGE) + 511) / 1023) +  PULSE_DELAY;
  const uint16_t mappedY = MIN_RANGE + (((uint32_t)rawY * (MAX_RANGE - MIN_RANGE) + 511) / 1023) + PULSE_DELAY;

  // save for next interrupt
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    nextPotX = mappedX;
    nextPotY = mappedY;
  }

  delay(10);
}
