#include <TLC591x.h>

//TLC591x myLED(1, SDI_pin, CLK_pin, LE_pin);          // OE pin hard-wired low (always enabled)
/*
To control individual LEDs, or directly control LED segments in a 7-segment display, use:
void printDirect(const uint8_t* b)
b is an array of size of at least num_chips. The values are shifted out right to left (i.e., b[0] is shifed out last).
*/

/*
TLC591x myLED(num_chips, SDI_pin, CLK_pin, LE_pin, OE_pin);  // OE pin controlled by library
TLC591x myLED(num_chips, SDI_pin, CLK_pin, LE_pin);          // OE pin hard-wired low (always enabled)
*/

/*
TLC591x myLED(num_chips, LE_pin, OE_pin);  // OE pin controlled by library
TLC591x myLED(num_chips, LE_pin);          // OE pin hard-wired low (always enabled)
*/

/*
Arduino Mini Pro -- TCL5916

13(SCK)   -- 3(CLK)
11(MOSI)  -- 2(SDI)
10(SS)    -- 4(LE)
*/

//#define Segments 5
#define Segments 5

#define RELAY_PIN 2
#define MODE_PIN 3
#define COUNTER_DELAY_3S 3000000  //delay in us for regular ride
#define COUNTER_DELAY_2S 2000000  //delay in us for rotations training
#define DIMM_DELAY 120000         //deimm delay in miliseconds
#define LED_DIMMED 127
#define LED_NORMAL 0

unsigned long intervalStart = 0;
unsigned long intervalEnd = 1;
unsigned long timePoint = 0;
unsigned long timePointPrev = 0;

unsigned long timeDim = 0;
unsigned long timeDimPrev = 0;
bool dimEnabled = false;  //true- full brightness, false- dimmed

bool timerState = false;  //true- time counting, false- stopped

//TLC591x myLED(1, 11, 13, 10);  // OE pin hard-wired low (always enabled)
//TLC591x myLED(Segments, 10);  //hardware
TLC591x myLED(Segments, 10, 5);  //hardware
uint8_t displayArray[Segments];
int ledSegments[] = { 219, 3, 185, 171, 99, 234, 250, 131, 251, 235 };
int ledDot = 4;  //code for displaying dot
unsigned int dimmLevel = LED_NORMAL;

void setup() {
  myLED.displayBrightness(getDimLevel(dimEnabled));
  SPI.begin();
  pinMode(RELAY_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RELAY_PIN), relayOn, FALLING);
}

void loop() {

  if ((timePoint - timePointPrev) > COUNTER_DELAY_3S) {
    timerState = !timerState;
    if (timerState) intervalStart = timePoint;
    if (!timerState) intervalEnd = timePoint;
    timePointPrev = timePoint;
    timeDimPrev = timeDim;
  }

  if (timerState) showTime(micros() - intervalStart, true);
  if (!timerState) showTime(intervalEnd - intervalStart, false);

  timeDimm = millis();
  if (timeDim - timeDimPrev > DIMM_DELAY) {
    myLED.displayBrightness(getDimLevel(true));
  }

  delay(10);
}


void relayOn() {
  //only grab time when relay is ON
  timePoint = micros();
}

unsigned int getDimLevel(bool dimmStatus) {
  if (dimmStatus) {
    return LED_NORMAL;
  } else {
    return LED_DIMMED;
  }
}

void showTime(unsigned long interval, bool count) {

  /*
A static variable is a special kind of variable; it is allocated memory 'statically'.
Its lifetime is the entire run of the program. It is specific to a function, i.e., only the function that defined it can access it.
However, it doesn't get destroyed after the function call ends.
It preserves its value between successive function calls. It is created and initialized the first time a function is called.
In the next function call, it is not created again. It just exists.
*/

  unsigned long interval_rounded = interval + 5000;
  static unsigned long interval_rounded_prev = 0;

  unsigned long minutes = interval_rounded / 1000000 / 60;
  unsigned long interval_nominutes = interval_rounded - minutes * 1000000 * 60;

  unsigned long digit0 = minutes;
  unsigned long digit1 = interval_nominutes / 10000000 % 10;
  unsigned long digit2 = interval_nominutes / 1000000 % 10;
  unsigned long digit3 = interval_nominutes / 100000 % 10;
  unsigned long digit4 = interval_nominutes / 10000 % 10;

  //Max measurement interval is 9min, 59s, 99ms
  //Only update display if interval is less that capacity of display
  if (interval < 600000000) {
    //this will round up to the nearest 100ts of miliseconds

    if (interval_rounded > interval_rounded_prev) {
      displayArray[0] = ledSegments[digit0] | ledDot;
      displayArray[1] = ledSegments[digit1];
      displayArray[2] = ledSegments[digit2] | ledDot;
      displayArray[3] = ledSegments[digit3];

      if (count) {
        displayArray[4] = ledSegments[0];
      } else {
        displayArray[4] = ledSegments[digit4];
      }

      myLED.printDirect(displayArray);
    }
    interval_rounded_prev = interval_rounded;

  } else {
    displayArray[0] = ledDot | ledSegments[9];
    displayArray[1] = ledSegments[5];
    displayArray[2] = ledDot | ledSegments[9];
    displayArray[3] = ledSegments[9];
    displayArray[4] = ledSegments[9];
  }
}
