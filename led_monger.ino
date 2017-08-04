// led_monger.ino
//
// Eric Mueller -- eric.mueller1024@gmail.com
//
// July 2017
//
// SUMMARY

// This project uses an Arduino to control 8 RGB led strips and display
// plesant patterns on them for outdoor dinner lighting. The system has a rotary
// encoder and two potentiometers to control what is displayed on the LED strips.
// The currently running program is displayed as a decimal number on a 4 digit
// 7-segment display.
// 
// CONTROL
// 
// The system is controled by a rotary encoder and 2 potentiometers. The rotary
// encoder dictates which program controls the LED. The first potentiometer
// controls the brightness of the LEDs and the second controls the frequency
// of the currently running program.
// 
// OUTPUTS
// 
// The system controls 8 RGB LED strips, each with 144 LEDs, for a total of 1152
// LEDs. The LED strips are Adafruit DotStar LED strips, which are controled via
// SPI. These strips can be daisy chained, but due to current limitations it is
// inadvisable to daisy chain more than a couple together, so they must be
// addressed independently. Unfortunately these strips do not have chip select
// pins, so each strip gets its own clock pin. (we use bitbang SPI instead of hw
// SPI)
// 
// In addition to the LED strips, the system also displays the currently running
// program on a 4-digit 7-segment display as a decimal number.
//
// POWER
//
// The LED strips draw a significant amount of power, so the system requires a
// dedicated power supply. Adafruit suggests a budget of 8A at 5V for each strip,
// so a 100A 5V power supply was conservatively chosen. In practice, the maximum
// consumption for each strip (all white LEDs at maximum brightness) was observed
// to be roughly 6A.


#include "LedProgram.h"
#include "RotaryEncoder.h"

#include <Adafruit_DotStar.h>
#include <Adafruit_LEDBackpack.h>

using pinno_t = const uint8_t;

// we have 8 physical LED strips, each with 144 LEDs per strip, that accept
// data in blue/green/red order.
const uint16_t nr_strips = 8;
const uint16_t leds_per_strip = 144;
const uint8_t led_color_order = DOTSTAR_BGR;

// each LED strip has its own digital pins for its SPI clock and data
pinno_t led_clk_pins[nr_strips] = {
        52, 50, 48, 46, 44, 42, 40, 38
};

pinno_t led_data_pins[nr_strips] = {
        53, 51, 49, 47, 45, 43, 41, 39
};

// our LED strip. We only have one instead of nr_strips of these because we
// don't have enough ram to store nr_strips (aka 8). We could probably do some
// hacky thing where we put some of them in flash (PROGMEM), but that would
// require messing with the Adafruit_DotStar class which is a pain.
//
// All LED programs that I've prototyped so far don't modify the previous state
// of the LED buffer, they just re-generate the whole buffer at each step. Thus
// there's really no point in having 8 buffers. This decision needs to be
// revisited if this ever changes
Adafruit_DotStar strip{leds_per_strip, led_data_pins[0], led_clk_pins[0],
                led_color_order};

// analog pins for potentiometer taps
pinno_t freq_pot_pin = 1;
pinno_t brightness_pot_pin = 0;

// the display for which program we're on
Adafruit_7segment seven_seg;

void setup()
{
        seven_seg.begin(0x70);
        
        // for debugging
        Serial.begin(9600);
}

// add LED programs here
BlinkerProg blinker;
RgbBlinkerProg rgb_blinker;
SingleColorProg single_color;
ColorTempProg color_temp;

LedProgram *progs[] = {
        &blinker,
        &rgb_blinker,
        &single_color,
        &color_temp    
};

uint8_t which_prog = 0;
const uint8_t nr_progs = (sizeof progs)/(sizeof progs[0]);

// interrupt pins for rotary encoder
pinno_t rot_a_pin = 18;
pinno_t rot_b_pin = 19;

RotaryEncoder rot{rot_a_pin, rot_b_pin, nr_progs};

// switch pin for roatary encoder (currently unused)
pinno_t rot_switch_pin = 32;

void loop()
{
        unsigned long loop_start = millis();
        
        // we always want frequency to be at least 1, so add 1 to whatever we read.
        uint16_t freq = analogRead(freq_pot_pin);
        uint16_t brightness = analogRead(brightness_pot_pin);

        unsigned long interval_millis = 1000UL/(freq != 0 ? log(freq): 1);

        which_prog = rot.getPos();
        
        Serial.print("read freq=");
        Serial.print(freq);
        Serial.print(" brightness=");
        Serial.print(brightness);
        Serial.print(" prog=");
        Serial.println(which_prog);

        seven_seg.println(which_prog, DEC);
        seven_seg.writeDisplay();

        LedProgram *prog = progs[which_prog];

        for (size_t i = 0; i < nr_strips; ++i) {

                if (i == 0)
                        strip.updatePins();
                else
                        strip.updatePins(led_data_pins[i], led_clk_pins[i]);

                prog->updateStrip(strip, i, brightness, freq);

                // the DotStar class wants brighness in [0, 255]
                // this math assumes maxBrighness > 255
                strip.setBrightness(brightness/(prog->maxBrightness()/255));

                unsigned long before = micros();
                strip.show();
                unsigned long after = micros();
                Serial.print("show took ");
                Serial.print(after - before);
                Serial.println("us");
        }

        // this sleep time isn't perfect because (1), we may be taking a lot of
        // time to update the strips so we can't go at the desired frequency and
        // (2), the arduino runtime might do some stuff between calls to
        // loop().
        unsigned long loop_time = millis() - loop_start;
        Serial.print("loop_time=");
        Serial.print(loop_time);
        Serial.print(" interval_millis=");
        Serial.println(interval_millis);
        if (loop_time < interval_millis) {
                unsigned long sleep_time = interval_millis - loop_time;
                Serial.print("sleep_time=");
                Serial.println(sleep_time);
                delay(sleep_time);
        }                                      
}
