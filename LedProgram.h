// LedProgram.h
//
// Eric Mueller, 2017
// 
// Implementation of an LED strip program encapsulation base class

#pragma once

#include <Adafruit_DotStar.h>

// gamma correction table
static const byte gc_table[256] = {
0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 30, 30, 31, 32, 33, 33, 34, 35, 36, 37, 37, 38, 39, 40, 41, 42, 43, 44, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 64, 65, 66, 67, 69, 70, 71, 72, 74, 75, 76, 78, 79, 81, 82, 83, 85, 86, 88, 89, 91, 92, 94, 95, 97, 98, 100, 102, 103, 105, 107, 108, 110, 112, 114, 115, 117, 119, 121, 123, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 157, 159, 161, 163, 165, 168, 170, 172, 175, 177, 179, 182, 184, 187, 189, 192, 194, 197, 199, 202, 204, 207, 209, 212, 215, 217, 220, 223, 226, 228, 231, 234, 237, 240, 243, 246, 249, 252, 255
};

class LedProgram
{
public:
        virtual ~LedProgram() {}

        // update a strip for the next tick.
        //
        // Derriving classes *must* implement this.
        //
        // Note: the data of "strip" will be clobbered each time because there
        // is only one strip worth of buffer because of memory contraints.
        // Thus implementing classes can not rely on the strip buffer persisting
        // between calls with the same strip_nr, *however*, as an optimization,
        // implementing classes may rely on the fact that strips are always
        // updated in order, so implementing classes may do a lot of work for the
        // strip 0 and then re-use that buffer as necessary for the other strips.
        //
        // NB: the update-in-order behavior is not implemented here, it's
        // implemented in led_monger.ino, which means we can't guarentee it per
        // se, but hey, this is embedded code, get over it.
        virtual void updateStrip(Adafruit_DotStar& strip,
                                 const uint8_t strip_nr,
                                 const uint16_t brightness,
                                 const uint16_t frequency) = 0;

        // we get brighness right from an arduino ADC (10 bits)
        static constexpr uint16_t maxBrightness()
        {
                return 1 << 10;
        }

        static constexpr uint16_t maxFrequency()
        {
                return 1 << 10;
        }
};


class BlinkerProg : public LedProgram
{
private:
        bool on = false;
public:
        void updateStrip(Adafruit_DotStar& strip,
                         const uint8_t strip_nr,
                         const uint16_t brightness,
                         const uint16_t frequency)
        {
                (void)brightness;
                (void)frequency;
          
                if (strip_nr != 0)
                        return;
                
                on = !on;

                uint32_t color = on ? strip.Color(255, 255, 255) : strip.Color(0, 0, 0);
                for (size_t i = 0; i < strip.numPixels(); ++i) 
                        strip.setPixelColor(i, color);
        }
};

class RgbBlinkerProg : public LedProgram
{
private:
        bool on;
        int rgb = 0;
public:
        void updateStrip(Adafruit_DotStar& strip,
                         const uint8_t strip_nr,
                         const uint16_t brightness,
                         const uint16_t frequency)
        {
                (void)brightness;
                (void)frequency;
            
                if (strip_nr != 0)
                        return;

                on = !on;
                if (on)
                        rgb = rgb == 2 ? 0 : rgb + 1;

                uint32_t color = 0;
                switch (rgb) {
                case 0:
                        color = strip.Color(255, 0, 0);
                        break;

                case 1:
                        color = strip.Color(0, 255, 0);
                        break;

                case 2:
                default:
                        color = strip.Color(0, 0, 255);
                        break;
                }

                color = on ? color : strip.Color(0, 0, 0);                
                for (size_t i = 0; i < strip.numPixels(); ++i) 
                        strip.setPixelColor(i, color);
        }
};

class SingleColorProg : public LedProgram
{
private:
        // Input a value 0 to 255 to get a color value.
        // The colours are a transition r - g - b - back to r.
        uint32_t wheel(Adafruit_DotStar& strip, byte pos)
        {
                pos = 255 - pos;
                if(pos < 85) {
                        return strip.Color(255 - pos * 3, 0, pos * 3);
                }
                if(pos < 170) {
                        pos -= 85;
                        return strip.Color(0, pos * 3, 255 - pos * 3);
                }
                pos -= 170;
                return strip.Color(pos * 3, 255 - pos * 3, 0);
        }

public:
        void updateStrip(Adafruit_DotStar& strip,
                         const uint8_t strip_nr,
                         const uint16_t brightness,
                         const uint16_t frequency)
        {
                (void)brightness;
            
                if (strip_nr != 0)
                        return;

                uint32_t color = wheel(strip, frequency/(maxFrequency()/255));
                for (size_t i = 0; i < strip.numPixels(); ++i)
                        strip.setPixelColor(i, color);
        }
};

class ColorTempProg : public LedProgram
{
private:
uint8_t clamp(int x, int min, int max)
{
  if (x < min)
    return min;
  if (x > max)
    return max;
  return x;
}

// XXX: fix indentation
uint32_t color_temp_to_rgb(Adafruit_DotStar& strip, unsigned temp)
{
  if (temp > 40000U || temp < 1000U)
    // if someone gives us a shitty color temp, give them back a shitty color
    return strip.Color(0, 255, 0);
  
  temp /= 100;

  int red, green, blue;

  if (temp <= 66) {
    red = 255;
    green = 99.4708025861 * log(temp) - 161.1195681661;

    if (temp <= 19) {
      blue = 0;
    } else {
      blue = 138.5177312231 * log(temp - 10) - 305.0447927307;
    }
  } else {
    red = 329.698727446 * pow(temp - 60, -0.1332047592);
    green = 288.1221695283 * pow(temp - 60, -0.0755148492);
    blue = 255;
  }

  red = gc_table[clamp(red, 0, 255)];
  green = gc_table[clamp(green, 0, 255)];
  blue = gc_table[clamp(blue, 0, 255)];

  return strip.Color(red, green, blue);
}
public:
        void updateStrip(Adafruit_DotStar& strip,
                         const uint8_t strip_nr,
                         const uint16_t brightness,
                         const uint16_t frequency)
        {
                (void)brightness;
            
                if (strip_nr != 0)
                        return;

                // the constants here here are emperical aka black magic aka they
                // made the prettiest colors
                uint32_t color = color_temp_to_rgb(strip, 8*frequency + 1000);
                for (size_t i = 0; i < strip.numPixels(); ++i)
                        strip.setPixelColor(i, color);
        }
};

