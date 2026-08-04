#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <EEPROM.h>

//#define DEBUG 1

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
  #define DEBUG_PRINTF(format, ...)
  #define DEBUG_PRINTLN(x)
#endif

void (*runLoop)();  

const uint8_t potX1pin = 3;
const uint8_t potY1pin = 4;

const uint8_t potX2pin = 2;
const uint8_t potY2pin = 5;
const uint8_t calibratePin = 8;


const uint8_t thumb1Xpin = A0;
const uint8_t thumb1Ypin = A1;
const uint8_t thumb2Xpin = A2;
const uint8_t thumb2Ypin = A3;


// Struct to store the min/max/middle values found in calibration.
struct CalibrationData {
    uint16_t min1X;
    uint16_t max1X;
    uint16_t mid1X;
    uint16_t min1Y;
    uint16_t max1Y;
    uint16_t mid1Y;

    uint16_t min2X;
    uint16_t max2X;
    uint16_t mid2X;
    uint16_t min2Y;
    uint16_t max2Y;
    uint16_t mid2Y;
};
CalibrationData cal;

#define CALIBRATION_DURATION_MS 10000
#define CAL_BUTTON_LOCKOUT_MS 1000
#define MID_SAMPLE_COUNT 10
unsigned long lockoutStart = 0;
unsigned long calStart = 0;
#define CAL_WRITTEN_FLAG_ADDR 32
const uint8_t WRITTEN_FLAG_VAL = 0xAB;

void saveCalibration() {
    // *IMPORTANT* EEPROM has limited writes, **don't** call repeatedly in a fast loop
    EEPROM.put(0, cal);
    EEPROM.write( CAL_WRITTEN_FLAG_ADDR, WRITTEN_FLAG_VAL);
}

void loadCalibration() {
    EEPROM.get(0, cal);
}


// check for hline getting stuck 
unsigned int hlineStuckCounter = 0; 
const unsigned int HLINE_STUCK_LIMIT = 5;  // just a guess

// Early observation with single axis: Range seems to be 0 to 222 but I see 1-225 in PADDLE() reads. 
// * I did see some strange behavior with 2-axis with values above 210 on my XL and maybe at 0 or 1.  
//   A stable range seems to be 6-210 on my 800XL, this may need to change as I test more units.
//   
// Adding a short delay (rouglhly 31 from observations) as we're not using Stingray's 10k resistor
// and a rising edge to detect delay start
#define PULSE_DELAY 31
#define MIN_RANGE   6
#define MAX_RANGE   210
#define FRAME_END   257


// Use current/next values to updating x/y in 
// the 115 + PULSE_DELAY default is used to check signal stability *without* analogreads.
static volatile uint16_t currPot1X = 115 + PULSE_DELAY;
static volatile uint16_t currPot1Y = 115 + PULSE_DELAY;
static volatile uint16_t nextPot1X = 115 + PULSE_DELAY;
static volatile uint16_t nextPot1Y = 115 + PULSE_DELAY;

static volatile uint16_t currPot2X = 115 + PULSE_DELAY;
static volatile uint16_t currPot2Y = 115 + PULSE_DELAY;
static volatile uint16_t nextPot2X = 115 + PULSE_DELAY;
static volatile uint16_t nextPot2Y = 115 + PULSE_DELAY;

static volatile uint16_t hline = 0;




// Timer 2 interrupt occurs at ~64us
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

        // check comparator output bit if it's low
        if( !( ACSR & (1 << ACO))) {
            // line is already low, reset hline
            ACSR &= ~(1 << ACIE);
            hline = 0;    
        }
        ++hline;
    }


}

// Analog comparator, triggers when detect that pokey released POT inputs to charge
ISR(ANALOG_COMP_vect)
{
    // Keep output pins low while POKEY is discharging the POT lines.
    PORTD &= ~((1 << PD3) | (1 << PD4) | (1 << PD2) | (1 << PD5));

    // update the current readings
    currPot1X = nextPot1X;
    currPot1Y = nextPot1Y;
    currPot2X = nextPot2X;
    currPot2Y = nextPot2Y;

    hline = 0;

    // resetting timer (seemes unnecessary)
    ACSR &= ~(1 << ACIE); // disable comparator
}

void restartComparator() { 
    ACSR &= ~(1 << ACIE); // disable comparator

    delayMicroseconds(50); 
    hline= 0;
    ACSR |= (1 << ACI);   // clear any pending interrupt bit
    ACSR |= (1 << ACIE);  // reactivate analog comparator
}


void setup()
{
    DEBUG_BEGIN(9600);

    pinMode(potX1pin, OUTPUT);
    pinMode(potY1pin, OUTPUT);
    pinMode(potX2pin, OUTPUT);
    pinMode(potY2pin, OUTPUT);

    digitalWrite(potX1pin, LOW);
    digitalWrite(potY1pin, LOW);
    digitalWrite(potX2pin, LOW);
    digitalWrite(potY2pin, LOW);

    // setup analog comparator as input
    pinMode(6, INPUT);
    pinMode(7, INPUT);

    // switch
    pinMode( calibratePin, INPUT_PULLUP);


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

    if( EEPROM.read( CAL_WRITTEN_FLAG_ADDR) == WRITTEN_FLAG_VAL ) {
        loadCalibration();

    } else {
        // assume whole range.
        cal.max1X = 1023;
        cal.min1X = 0;
        cal.mid1X = 511;
        cal.max1Y = 1023;
        cal.min1Y = 0;
        cal.mid1Y = 511;
        cal.max2X = 1023;
        cal.min2X = 0;
        cal.mid2X = 511;
        cal.max2Y = 1023;
        cal.min2Y = 0;
        cal.mid2Y = 511;
    }

    runLoop = mainLoop;
}

bool buttonPressed() {
    if (millis() < CAL_BUTTON_LOCKOUT_MS  + lockoutStart) return false;   // ignore during lockout
    static bool lastState = HIGH;
    bool state = digitalRead(calibratePin);
    bool pressed = (lastState == HIGH && state == LOW);  // falling edge
    lastState = state;
    if (pressed) lockoutStart = millis();
    return pressed;
}

void mainLoop()
{

    // Read modern thumb1sticks (0 to 1023)
    uint16_t raw1X = analogRead(thumb1Xpin);
    uint16_t raw1Y = 1023 - analogRead(thumb1Ypin);
    uint16_t raw2X = analogRead(thumb2Xpin);
    uint16_t raw2Y = 1023 - analogRead(thumb2Ypin);

    if( raw1X < cal.min1X ) raw1X = cal.min1X;
    if( raw1X > cal.max1X ) raw1X = cal.max1X;
    if( raw1Y < cal.min1Y ) raw1Y = cal.min1Y;
    if( raw1Y > cal.max1Y ) raw1Y = cal.max1Y;

    if( raw2X < cal.min2X ) raw2X = cal.min2X;
    if( raw2X > cal.max2X ) raw2X = cal.max2X;
    if( raw2Y < cal.min2Y ) raw2Y = cal.min2Y;
    if( raw2Y > cal.max2Y ) raw2Y = cal.max2Y;

    DEBUG_PRINTF("ST1 X: [%d] Min:%d Max:%d ST1 Y: [%d] Min:%d Max:%d ST2 X: [%d] Min:%d Max:%d ST2 Y: [%d] Min:%d Max:%d", raw1X, min1X, max1X, raw1Y, min1Y, max1Y, raw2X, min2X, max2X, raw2Y, min2Y, max2Y);


    // map  pot ranges  to usable range  TODO: Consider a non-linear mapping for this.
    const uint16_t mapped1X = MIN_RANGE +   (((uint32_t)(raw1X - cal.min1X) * (MAX_RANGE - MIN_RANGE) + (cal.mid1X-cal.min1X)) / (cal.max1X-cal.min1X)) +  PULSE_DELAY;
    const uint16_t mapped1Y = MIN_RANGE + (((uint32_t)(raw1Y - cal.min1Y)* (MAX_RANGE - MIN_RANGE) + (cal.mid1Y-cal.min1Y)) / (cal.max1Y-cal.min1Y)) + PULSE_DELAY;
    const uint16_t mapped2X = MIN_RANGE +   (((uint32_t)(raw2X - cal.min2X) * (MAX_RANGE - MIN_RANGE) + (cal.mid2X-cal.min2X)) / (cal.max2X-cal.min2X)) +  PULSE_DELAY;
    const uint16_t mapped2Y = MIN_RANGE + (((uint32_t)(raw2Y - cal.min2Y)* (MAX_RANGE - MIN_RANGE) + (cal.mid2Y-cal.min2Y)) / (cal.max2Y-cal.min2Y)) + PULSE_DELAY;
 
    // save for next interrupt
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        nextPot1X = mapped1X;
        nextPot1Y = mapped1Y;
        nextPot2X = mapped2X;
        nextPot2Y = mapped2Y;
    }

    //are we stuck?
    if (hline >= 258) {
        hlineStuckCounter++;
        if( hlineStuckCounter > HLINE_STUCK_LIMIT ) {
            restartComparator();
            hlineStuckCounter = 0;
        }
    } else {
        hlineStuckCounter = 0; 
    }

    if( buttonPressed()){
        startCalibration();
    }
    delay(3);
}

void startCalibration() {

    DEBUG_PRINT("START CAL\n");

    // get a few readings for center.
    uint32_t sum1X = 0;
    uint32_t sum1Y = 0;
    uint32_t sum2X = 0;
    uint32_t sum2Y = 0;
    for( int i=0; i <MID_SAMPLE_COUNT ; i++ ) {
        sum1X +=  analogRead(thumb1Xpin);
        sum1Y += 1023 - analogRead(thumb1Ypin);
        sum2X +=  analogRead(thumb2Xpin);
        sum2Y += 1023 - analogRead(thumb2Ypin);
        delay(10);
    }
    cal.mid1X = sum1X / MID_SAMPLE_COUNT;
    cal.mid1Y = sum1Y / MID_SAMPLE_COUNT;
    cal.min1X = 1023; cal.max1X = 0;
    cal.min1Y = 1023; cal.max1Y = 0; 

    cal.mid2X = sum2X / MID_SAMPLE_COUNT;
    cal.mid2Y = sum2Y / MID_SAMPLE_COUNT;
    cal.min2X = 1023; cal.max2X = 0;
    cal.min2Y = 1023; cal.max2Y = 0; 
    calStart = millis();
    runLoop = calibrationLoop;

}

void calibrationLoop() {
    const uint16_t raw1X = analogRead(thumb1Xpin);
    const uint16_t raw1Y = 1023 - analogRead(thumb1Ypin);
    const uint16_t raw2X = analogRead(thumb2Xpin);
    const uint16_t raw2Y = 1023 - analogRead(thumb2Ypin);

    //  get min/max vals
    if (raw1X < cal.min1X) cal.min1X = raw1X;  
    if (raw1X > cal.max1X) cal.max1X = raw1X;
    if (raw1Y < cal.min1Y) cal.min1Y = raw1Y;  
    if (raw1Y > cal.max1Y) cal.max1Y = raw1Y;

    if (raw2X < cal.min2X) cal.min2X = raw2X;  
    if (raw2X > cal.max2X) cal.max2X = raw2X;
    if (raw2Y < cal.min2Y) cal.min2Y = raw2Y;  
    if (raw2Y > cal.max2Y) cal.max2Y = raw2Y;


    bool timedOut = ( millis() >  calStart +CALIBRATION_DURATION_MS );
    bool userQuit = buttonPressed();

    if( timedOut || userQuit ) {
        runLoop = mainLoop; // exit calibration loop by switching to mainLoop
        saveCalibration(); // eeprom has limited life only call when exiting calibration

        DEBUG_PRINTF("1 MID X: [%d] Min: %d Max: %d MID Y: [%d] Min: %d Max: %d", cal.mid1X, cal.min1X, cal.max1X,   cal.mid1Y, cal.min1Y, cal.max1Y);
        DEBUG_PRINTF("2 MID X: [%d] Min: %d Max: %d MID Y: [%d] Min: %d Max: %d", cal.mid2X, cal.min2X, cal.max2X,   cal.mid2Y, cal.min2Y, cal.max2Y);
    }
}

void loop() {
    runLoop();
}


