// NeoClock
// John Rood, 2015-01-25

// The AA borrows heavily from Mark Kriegsman's example
// https://plus.google.com/112916219338292742137/posts/2VYNQgD38Pw


#include <stdint.h>
#include <FastLED.h>
#include <Bounce2.h>

// ANIMATION (4096 / DIVISOR) = Hz
// 4096 / 127 => 32Hz
#define DIVISOR 127

#define MODEBT_PIN 2
#define TIMEBT_PIN 3
#define LDR_PIN A0

// LEDS
#define DATA_PIN    6
#define NUM_LEDS    60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

uint8_t globalBrightness = 31;
uint8_t globalSaturation = 255;


volatile int doUpdate = LOW; // animation interrupt
int repaint = 0;

uint8_t hour = 0;
uint8_t minute = 0;
uint8_t hourIndex = 0;

uint8_t myHue = 0;

uint8_t display_mode = 0; //
uint8_t old_display_mode = 0;
uint8_t inverse_mode = 0;

int F32pos = 0;
uint8_t F32frac = 0;


int secondsWidth  = 5; // width of each light bar, in whole pixels

int advanceMinute = 0;
int advanceHour = 0;
int settingTime = 0;

int buttonState;
unsigned long buttonPressTimeStamp;
unsigned long firstPressTimeStamp;
int repeatCount = 0;

// int hourStep = 0;
// int minuteStep = 0;

void drawSecond( int pos32, uint8_t frac, int width, uint8_t hue); // seconds
void drawMinute(int pos32, uint8_t frac, uint8_t hue);
void drawHour(int pos32, uint8_t frac, uint8_t hue);
void drawHourMarkers(int pos32, uint8_t frac, uint8_t hue);

void addLed(int index, CHSV value) ;

void updateBrightness(void);

void rainbow(uint8_t startHue);
CHSV rainbowHsvForPos(uint8_t pos, uint8_t startHue);

// display modes
void m_regular_colorloop(int pos32, uint8_t frac);
void m_set_time(int pos32, uint8_t frac);
void m_inverse(int pos32, uint8_t frac);
void m_rainbow_inverse(int pos32, uint8_t frac);
#define NUM_MODES 4


Bounce btMode = Bounce();
Bounce btTime = Bounce();

ISR(TIMER2_COMPA_vect) {
    doUpdate = HIGH;
}

void setup() {
    // TODO: implement auto-tuning using the 32kHz crystal
    OSCCAL = 0xAC;
    delay(1000); // wait

    pinMode(MODEBT_PIN, INPUT_PULLUP);
    pinMode(TIMEBT_PIN, INPUT_PULLUP);

    btMode.attach(MODEBT_PIN);
    btMode.interval(3);

    btTime.attach(TIMEBT_PIN);
    btTime.interval(3);

    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    updateBrightness();
    myHue = random8();

    // "random" start time
    hour = 6;
    minute = 42;
    hourIndex = hour * 5 + minute/12;
    F32pos = 13 * 32;

    TCCR2A = 0;
    TCCR2B = 0;
    TIMSK2 &= ~((1<<TOIE2)|(1<<OCIE2A)|(1<<OCIE2B)); // kill T2 intr
    ASSR |= (1<<AS2); // ext. 32kHz crystal
    OCR2A = DIVISOR;
    TCNT2 = 0;
    TCCR2A = (1<<WGM21); // CTC
    TCCR2B = ((1<<CS21)); //  div 8 => 4096Hz
    // wait 4 teh readies
    while (ASSR & ((1<<TCN2UB)|(1<<OCR2AUB)|(1<<OCR2BUB)|(1<<TCR2AUB)|(1<<TCR2BUB))) {}
    TIMSK2 |= (1<<OCIE2A);

}

void loop() {
    if (doUpdate || repaint) {
        if (F32pos >= (NUM_LEDS * 32)) {
            F32pos -= (NUM_LEDS * 32);

            advanceMinute = 1;

            if ((minute + 1) % 12 == 0) {
                advanceHour = 1;
            }
        }

        F32frac = F32pos & 0x1F;

        switch (display_mode)
        {
            case 0:
                m_regular_colorloop(F32pos, F32frac);
                break;
            case 1:
                m_inverse(F32pos, F32frac);
                break;
            case 2:
                m_rainbow_inverse(F32pos, F32frac);
                break;
            case 3:
                m_set_time(F32pos, F32frac);
                break;
            default:
                m_regular_colorloop(F32pos, F32frac);
        }

        //drawHourMarkers(F32pos, F32frac, myHue + 42);

        if (F32frac == 0x1F) {
            if (advanceMinute) {
                minute++;
                if (minute == 60) {
                    minute = 0;
                }
                advanceMinute = 0;

                myHue++;
            }

            if (advanceHour) {
                hour++;
                if (hour == 12) {
                    hour = 0;
                }
                hourIndex++;
                if (hourIndex == 60) {
                    hourIndex = 0;
                }
                advanceHour = 0;
            }

        }

        updateBrightness();

        FastLED.show();

        if (doUpdate) {
            doUpdate = LOW;
            F32pos++;
        }
        repaint = 0;
    }


    // handle buttons here

    if (!settingTime) {
        boolean modeBtChanged = btMode.update();
        int modeBtVal = btMode.read();

        if (modeBtChanged && modeBtVal == LOW) {
            display_mode++;
            if (display_mode == NUM_MODES) {
                display_mode = 0;
            }
        }
    }

    boolean timeBtChanged = btTime.update();

    if (timeBtChanged) {
        int timeBtVal = btTime.read();
        if (timeBtVal == HIGH) {
            buttonState = 0;
        }
        else {
            if (!settingTime) {
                old_display_mode = display_mode;
                display_mode = 3; // RGB
                settingTime = 1;
            }
            buttonState = 1;
            firstPressTimeStamp = millis();
            buttonPressTimeStamp = firstPressTimeStamp;
            F32pos = NUM_LEDS * 32 + 0x1F;
        }
    }

    unsigned long now = millis();
    unsigned long holdTime = (now - firstPressTimeStamp) / 1000 ;
    if (holdTime > 20) {
        holdTime = 20;
    }
    unsigned long repeatDelay = 300 - (290/20 * holdTime);

    if (buttonState == 1) {
        if (now - buttonPressTimeStamp > repeatDelay) {
            buttonPressTimeStamp = now;
            F32pos = NUM_LEDS * 32 + 0x1F; // advance 1 minute
            repaint = 1;
        }
    }
    else if (settingTime && now - buttonPressTimeStamp >= 5000) {
        display_mode = old_display_mode;
        settingTime = 0;
    }
}

// basic RGB clock
void m_set_time(int pos32, uint8_t frac)
{
    FastLED.clear();

    inverse_mode = 0;

    addLed(hourIndex, CHSV(0, globalSaturation, 255));
    addLed(minute, CHSV(85, globalSaturation, 255));
    // only show seconds if we're a selected display mode
    if (!settingTime) {
        addLed(pos32/32, CHSV(170, globalSaturation, 255));
    }
}

// fancy clock
void m_regular_colorloop(int pos32, uint8_t frac)
{
    FastLED.clear();

    inverse_mode = 0;

    drawHour(pos32, frac, myHue);
    drawMinute(pos32, frac, myHue + 25);
    drawSecond(pos32, frac, secondsWidth, myHue + 50);
}

// inverted clock
void m_inverse(int pos32, uint8_t frac)
{
    fill_solid(leds, NUM_LEDS, CHSV(myHue, globalSaturation, 255) );

    inverse_mode = 1;

    drawSecond(F32pos, F32frac, secondsWidth, myHue);
    drawMinute(F32pos, F32frac, myHue);
    drawHour(F32pos, F32frac, myHue);
}

// rainbow clock
void m_rainbow_inverse(int pos32, uint8_t frac)
{
    rainbow(myHue);

    inverse_mode = 1;

    drawSecond(F32pos, F32frac, secondsWidth, myHue);
    drawMinute(F32pos, F32frac, myHue);
    drawHour(F32pos, F32frac, myHue);
}


void drawHour(int pos32, uint8_t frac, uint8_t hue)
{
    if (advanceHour) {
        uint8_t pctStep = frac * 8;
        addLed(hourIndex - 1, CHSV(hue, globalSaturation, lerp8by8(191, 0, pctStep)));
        addLed(hourIndex,     CHSV(hue, globalSaturation, lerp8by8(255, 191, pctStep)));
        addLed(hourIndex + 1, CHSV(hue, globalSaturation, lerp8by8(191, 255, pctStep)));
        addLed(hourIndex + 2, CHSV(hue, globalSaturation, lerp8by8(0, 191, pctStep)));
    }
    else {
        addLed(hourIndex - 1, CHSV(hue, globalSaturation, 191));
        addLed(hourIndex, CHSV(hue, globalSaturation, 255));
        addLed(hourIndex + 1, CHSV(hue, globalSaturation, 191));
    }
}

void drawMinute(int pos32, uint8_t frac, uint8_t hue)
{
    if (advanceMinute) {
        uint8_t firstpixelbrightness = (255 - (frac * 8));
        uint8_t lastpixelbrightness  = (255 - firstpixelbrightness);

        addLed(minute + 1, CHSV(hue, globalSaturation, lastpixelbrightness));
        addLed(minute, CHSV(hue, globalSaturation, firstpixelbrightness));
    }
    else {
        addLed(minute, CHSV(hue, globalSaturation, 255));
    }
}


void drawSecond( int pos32, uint8_t frac, int width, uint8_t hue)
{
    int i = pos32 / 32;

    uint8_t pctStep = frac * 8;
    uint8_t firstpixelbrightness  = pctStep;

    uint8_t tail_size = width - 2;
    uint8_t tail_step = 255 / tail_size;

    uint8_t bright;
    for( int n = 0; n < width; n++) {
        if( n == 0) {
            bright = firstpixelbrightness;
        }
        else if (n == 1) {
            bright = 255; // actual second
        }
        else {
            uint8_t tail_pos = n - 2;
            uint8_t startBri = 255 - tail_pos * tail_step;
            bright = lerp8by8(startBri, startBri - tail_step, pctStep);
        }

        addLed(i, CHSV( hue, globalSaturation, bright));

        i--;
        if (i < 0) {
            i = NUM_LEDS - 1;
        }
    }
}

void drawHourMarkers(int pos32, uint8_t frac, uint8_t hue)
{
    uint8_t i;
    for (i = 0; i < 12; i++) {
        leds[i * 5] = CHSV(hue, 127, 31);
    }
}

void addLed(int index, CHSV pixelColor)
{
    if (index > 59) {
        index -= 60;
    }
    else if (index < 0) {
        index += 60;
    }

    if (display_mode == 0) {
        leds[index] += pixelColor;
    }
    else if (display_mode == 1) { // painted in myHue
        leds[index] = CHSV(myHue, globalSaturation, 255 - pixelColor.value);
    }
    else if (display_mode == 2) {
        CHSV spotColor = rainbowHsvForPos(index, myHue); // FIXME: this is a hack
        spotColor.value = 255 - pixelColor.value;
        leds[index]  = spotColor;
    }
    else {
        leds[index] += pixelColor;
    }
}

void updateBrightness(void)
{
    uint8_t bri;

    int LDRReading = analogRead(LDR_PIN);

    // somewhere between  170 - 600
    LDRReading = constrain(LDRReading, 0, 600);

    if (!inverse_mode) {
        bri = map(LDRReading, 0, 600, 64, 255);
    }
    else { // mode 1 or 2, so dim extra
        bri = map(LDRReading, 0, 600, 16, 127);
    }

    if (bri != globalBrightness) {
        FastLED.setBrightness(bri);
        globalBrightness = bri;
    }
}

void rainbow(uint8_t startHue) {
    int i;
    for (i = 0; i < 60; i++) {
        leds[i] = rainbowHsvForPos(i, startHue);
    }
}

CHSV rainbowHsvForPos(uint8_t pos, uint8_t startHue)
{
    //255*16/60
    uint8_t hue = startHue + (uint16_t(255) * uint16_t(pos)) / 60;
    return CHSV(hue, globalSaturation, 255);
}

