// RotaryEncoder.h
//
// Eric Mueller, 2017
// 
// Implementation of an interrupt-based rotary encoder reader. This class
// requires two interrupt pins, and does not support multiple instances
// because the ISR has no way of telling which instance its opperating on.

#pragma once

#include <Adafruit_LEDBackpack.h>

class RotaryEncoder
{
private:
        // the display we're writing the number to
        Adafruit_7segment& seven_seg_;
        
        const byte pin_a_;
        const byte pin_b_;

        uint8_t enc_prev_pos_ = 0;
        uint8_t enc_flags_ = 0;

        // the integer position encoded by the encoder
        volatile uint8_t rotary_index_ = 0;
        const uint8_t max_index_;

        static RotaryEncoder *instance_;

        static void pin_isr()
        {
                RotaryEncoder *enc = instance_;

                bool change = false;

                // note: for better performance, the code will now use
                // direct port access techniques
                // http://www.arduino.cc/en/Reference/PortManipulation
                uint8_t enc_cur_pos = 0;
                // read in the encoder state first
                if (digitalRead(enc->pin_a_) == LOW) {
                        enc_cur_pos |= (1 << 0);
                }
                if (digitalRead(enc->pin_b_) == LOW) {
                        enc_cur_pos |= (1 << 1);
                }
 
                // if any rotation at all
                if (enc_cur_pos != enc->enc_prev_pos_)
                {
                        if (enc->enc_prev_pos_ == 0x00)
                        {
                                // this is the first edge
                                if (enc_cur_pos == 0x01) {
                                        enc->enc_flags_ |= (1 << 0);
                                }
                                else if (enc_cur_pos == 0x02) {
                                        enc->enc_flags_ |= (1 << 1);
                                }
                        }
 
                        if (enc_cur_pos == 0x03)
                        {
                                // this is when the encoder is in the middle of a "step"
                                enc->enc_flags_ |= (1 << 4);
                        }
                        else if (enc_cur_pos == 0x00)
                        {
                                // this is the final edge
                                if (enc->enc_prev_pos_ == 0x02) {
                                        enc->enc_flags_ |= (1 << 2);
                                }
                                else if (enc->enc_prev_pos_ == 0x01) {
                                        enc->enc_flags_ |= (1 << 3);
                                }
 
                                // check the first and last edge
                                // or maybe one edge is missing, if missing then require the middle state
                                // this will reject bounces and false movements
                                if (bit_is_set(enc->enc_flags_, 0) && (bit_is_set(enc->enc_flags_, 2) || bit_is_set(enc->enc_flags_, 4))) {
                                        ++enc->rotary_index_;
                                        change = true;
                                }
                                else if (bit_is_set(enc->enc_flags_, 1) && (bit_is_set(enc->enc_flags_, 3) || bit_is_set(enc->enc_flags_, 4))) {
                                        --enc->rotary_index_;
                                        change = true;
                                }
 
                                enc->enc_flags_ = 0; // reset for next time
                        }
                }
 
                enc->enc_prev_pos_ = enc_cur_pos;

                if (change) {
                        // XXX: don't do this. If things deadlock, look here first.
                        interrupts();
                        enc->seven_seg_.print(enc->rotary_index_
                                              % enc->max_index_, DEC);
                        enc->seven_seg_.writeDisplay();
                        noInterrupts();
                }
        }
                
public:
        RotaryEncoder(const byte pin_a, const byte pin_b,
                      const uint8_t max_index, Adafruit_7segment& seven_seg)
                : seven_seg_{seven_seg}, pin_a_{pin_a}, pin_b_{pin_b},
                  max_index_{max_index}
        {
                pinMode(pin_a_, INPUT_PULLUP);
                pinMode(pin_b_, INPUT_PULLUP);
                
                // this initial setup needs to happen before attachInterrupt so we
                // don't race to update this value
                if (digitalRead(pin_a_) == LOW) {
                        enc_prev_pos_ |= 0x1;
                }
                if (digitalRead(pin_b_) == LOW) {
                        enc_prev_pos_ |= 0x2;
                }

                attachInterrupt(digitalPinToInterrupt(pin_a_), pin_isr, CHANGE);
                attachInterrupt(digitalPinToInterrupt(pin_b_), pin_isr, CHANGE);

                if (!instance_)
                        instance_ = this;
        }

        ~RotaryEncoder()
        {
                detachInterrupt(digitalPinToInterrupt(pin_a_));
                detachInterrupt(digitalPinToInterrupt(pin_b_));
        }
        
        // XXX: consider copy construction and assignment

        uint8_t getIndex()
        {
                uint8_t index;
                noInterrupts();
                index = rotary_index_;
                interrupts();
                return index % max_index_;
        }
};

RotaryEncoder *RotaryEncoder::instance_ = NULL;
