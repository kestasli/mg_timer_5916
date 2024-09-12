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

#define Segments 5

#define RELAY_PIN 2
#define COUNTER_DELAY 3000000  //shortest interval delay in us
//#define DISPLAY_DELAY 12
//delay pushing data to the LED matrix
unsigned long previous_time = 0;  //prev timepoint for pushing tada to the display
unsigned long current_time = 0;

unsigned long intervalStart = 0;
unsigned long intervalEnd = 1;
unsigned long timePoint = 0;
unsigned long timePointPrev = 0;

bool timerState = false;  //true- time counting, false- stopped

//TLC591x myLED(1, 11, 13, 10);  // OE pin hard-wired low (always enabled)
TLC591x myLED(Segments, 10);  //hardware
uint8_t displayArray[Segments];

int ledSegments[] = { 219, 3, 185, 171, 99, 234, 250, 131, 251, 235 };
int ledDot = 4;

void setup() {
  SPI.begin();
  pinMode(RELAY_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RELAY_PIN), relayOn, FALLING);
}

void loop() {

  if ((timePoint - timePointPrev) > COUNTER_DELAY) {
    timerState = !timerState;
    if (timerState) intervalStart = timePoint;
    if (!timerState) intervalEnd = timePoint;
    timePointPrev = timePoint;
  }


  if (timerState) showTime(micros() - intervalStart, true);
  if (!timerState) showTime(intervalEnd - intervalStart, false);

  //a[0] = ledDot | ledSegments[count / 10000 % 10];
  //a[1] = ledSegments[count / 1000 % 10];
  //a[2] = ledDot | ledSegments[count / 100 % 10];
  //a[3] = ledSegments[count / 10 % 10];
  //a[3] = ledSegments[count / 1 % 10];

  delay(10);
}


void relayOn() {
  //only grab time when relay is ON
  timePoint = micros();
}

void showTime(unsigned long interval) {
  //Max measurement interval is 9min, 59s, 99ms
  //Only update display if interval is less that capacity of display
  if (interval < 600000000) {
    //this will round up to the nearest 100ts of miliseconds
    unsigned long interval_rounded = interval + 5000;
    unsigned long minutes = interval_rounded / 1000000 / 60;
    unsigned long interval_nominutes = interval_rounded - minutes * 1000000 * 60;


    displayArray[0] = ledDot | ledSegments[minutes];
    displayArray[1] = ledSegments[interval_nominutes / 10000000 % 10];
    displayArray[2] = ledDot | ledSegments[interval_nominutes / 1000000 % 10];
    displayArray[3] = ledSegments[interval_nominutes / 100000 % 10];
    displayArray[4] = ledSegments[interval_nominutes / 10000 % 10];

  } else {
    displayArray[0] = ledDot | ledSegments[9];
    displayArray[1] = ledSegments[5];
    displayArray[2] = ledDot | ledSegments[9];
    displayArray[3] = ledSegments[9];
    displayArray[4] = ledSegments[9];
  }
  myLED.printDirect(displayArray);
}


void showTime(unsigned long interval, bool count) {
  //Max measurement interval is 9min, 59s, 99ms
  //Only update display if interval is less that capacity of display
  if (interval < 600000000) {
    //this will round up to the nearest 100ts of miliseconds
    unsigned long interval_rounded = interval + 5000;
    unsigned long minutes = interval_rounded / 1000000 / 60;
    unsigned long interval_nominutes = interval_rounded - minutes * 1000000 * 60;


    displayArray[0] = ledDot | ledSegments[minutes];
    displayArray[1] = ledSegments[interval_nominutes / 10000000 % 10];
    displayArray[2] = ledDot | ledSegments[interval_nominutes / 1000000 % 10];
    displayArray[3] = ledSegments[interval_nominutes / 100000 % 10];
    if (count) {
      displayArray[4] = ledSegments[0];
    } else {
      displayArray[4] = ledSegments[interval_nominutes / 10000 % 10];
    }


  } else {
    displayArray[0] = ledDot | ledSegments[9];
    displayArray[1] = ledSegments[5];
    displayArray[2] = ledDot | ledSegments[9];
    displayArray[3] = ledSegments[9];
    displayArray[4] = ledSegments[9];
  }
  myLED.printDirect(displayArray);
}
