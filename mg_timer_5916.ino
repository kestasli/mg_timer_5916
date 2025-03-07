#include <TLC591x.h>
#include <OneButton.h>
//#include <TimerOne.h>

/*
Frequency generator for arduino
https://forum.arduino.cc/t/using-digitalwrite-to-create-a-square-wave-of-a-specific-frequency/463688/4
*/

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

13(SCK)
11(MOSI)
10(SS)

*/

//#define Segments 5
#define Segments 5

#define RELAY_PIN 2
#define MODE_PIN 5
#define COUNTER_DELAY_3S 3000000  //delay in us for regular ride
#define COUNTER_DELAY_2S 2000000  //delay in us for rotations training
#define DIMM_DELAY 60000          //dimm delay in miliseconds
#define VCC_READ_DELAY 10000      //VCC read interval
#define LED_DIMMED_LEVEL 110
#define LED_NORMAL_LEVEL 0
#define BATT_LOW_LEVEL 2900

unsigned long intervalStart = 0;
unsigned long intervalEnd = 1;
unsigned long timePoint = 0;
unsigned long timePointPrev = 0;
bool timerState = false;  //true- time counting, false- stopped

unsigned long timeDim = 0;
unsigned long timeDimPrev = 0;
bool ledNormal = true;
bool battLow = false;

unsigned long timeVCC = 0;
unsigned long timeVCCPrev = 0;

//TLC591x myLED(1, 11, 13, 10);  // OE pin hard-wired low (always enabled)
//TLC591x myLED(Segments, 10);  //hardware
TLC591x myLED(Segments, 10, MODE_PIN);  //hardware
uint8_t displayArray[Segments];
int ledSegments[] = { 219, 3, 185, 171, 99, 234, 250, 131, 251, 235 };
int ledDot = 4;  //code for displaying dot

void setup() {
  Serial.begin(9600);
  myLED.displayBrightness(LED_NORMAL_LEVEL);
  ledNormal = true;
  SPI.begin();
  pinMode(RELAY_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RELAY_PIN), relayOn, FALLING);
}

void loop() {

  //timePointEmulator();

  if ((timePoint - timePointPrev) > COUNTER_DELAY_3S) {
    timerState = !timerState;
    if (timerState) intervalStart = timePoint;
    if (!timerState) intervalEnd = timePoint;
    timePointPrev = timePoint;

    myLED.displayBrightness(LED_NORMAL_LEVEL);
    ledNormal = true;
    timeDimPrev = timeDim;
  }

  if (timerState) showTime(micros() - intervalStart, true);
  if (!timerState) showTime(intervalEnd - intervalStart, false);

  timeDim = millis();
  if (timeDim - timeDimPrev > DIMM_DELAY) {
    myLED.displayBrightness(LED_DIMMED_LEVEL);
    ledNormal = false;
  }

  timeVCC = millis();
  if (timeVCC - timeVCCPrev > VCC_READ_DELAY) {
    Serial.println(readVcc());
    if (readVcc() < BATT_LOW_LEVEL) {
      showBattLow();
      Serial.println("Batt Low");
      while (true) {};
    }
    timeVCCPrev = timeVCC;
  }

  delay(51);
}

void timePointEmulator() {
  static unsigned long timeSchedulePrev = 0;
  unsigned long timeSchedule = millis();

  static unsigned int intervalSelect = 0;

  //pseudo random delays to fire timePoint, in s

  const unsigned long testSchedule[] = { 5, 47, 15, 83, 71, 100, 24, 215, 31, 90,
                                         82, 88, 40, 160, 48, 150, 43, 132, 37, 40,
                                         42, 50, 57, 111, 76, 234, 17, 214, 50, 30,
                                         90, 222, 69, 169, 41, 47, 65, 183, 14, 192,
                                         40, 118, 47, 100, 42, 129, 28, 209, 56, 128 };


  if ((timeSchedule - timeSchedulePrev) > (1000 * testSchedule[intervalSelect])) {
    //fire timepoint setup
    relayOn();
    //Serial.print(testSchedule[intervalSelect]);
    //Serial.println();
    timeSchedulePrev = timeSchedule;
    intervalSelect++;
  }

  if (intervalSelect > ((sizeof(testSchedule) / sizeof(unsigned long)) - 1)) {
    intervalSelect = 0;
  }
}

void relayOn() {
  //only grab time when relay is ON
  timePoint = micros();
}


void showBattLow() {
  displayArray[0] = 0 | ledDot;
  displayArray[1] = 0;
  displayArray[2] = 0 | ledDot;
  displayArray[3] = 0;
  displayArray[4] = 0;
  myLED.printDirect(displayArray);
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

      //myLED.displayDisable();
      myLED.printDirect(displayArray);
      //myLED.displayEnable();
    }
    interval_rounded_prev = interval_rounded;

  } else {
    displayArray[0] = ledDot | ledSegments[9];
    displayArray[1] = ledSegments[5];
    displayArray[2] = ledDot | ledSegments[9];
    displayArray[3] = ledSegments[9];
    displayArray[4] = ledSegments[9];
    myLED.printDirect(displayArray);
  }
}

long readVcc() {

  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);             // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);  // Convert
  while (bit_is_set(ADCSRA, ADSC))
    ;
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result;  // Back-calculate AVcc in mV
  return result;               //Vcc in mV
}
