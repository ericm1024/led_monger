// Potentiometer.h
//
// Eric Mueller, 2017
// 
// Implementation of a potentiometer reader with hysteresis.

#pragma once

class Potentiometer
{
private:
        constexpr uint8_t LOG_BIN_SIZE_ = 5;
        constexpr uint8_t ADC_BITS_ = 10;
        constexpr int HYSTERESIS_ = 8; // arbitary
        constexpr uint8_t ADC_TO_BIN_SHIFT_ = ADC_BITS_ - LOG_BIN_SIZE_;
        constexpr uint8_t BIN_SIZE_ = 1 << LOG_BIN_SIZE_;

public:
        constexpr unsigned NR_BINS = 1 << (ADC_BITS_ - BIN_BITS_);
        constexpr int NO_CHANGE = INT_MAX;

private:
        int current_bin_ = NO_CHANGE;
        const byte pin_;

public:

        int update()
        {
                int next_value = analogRead(pin_);
                int next_bin = next_value >> ADC_TO_BIN_SHIFT_;

                // is this the first time we were called?
                if (current_bin_ == NO_CHANGE) {
                        current_bin_ = next_bin;
                        return current_bin_;
                }

                // has the voltage moved into a new bin?
                if (next_bin != current_bin_) {

                        int bin_start = current_bin_ << ADC_TO_BIN_SHIFT_;
                        int bin_end = bin_start + (BIN_SIZE_ - 1);

                        // here's where the hysteresis happens. We say the bin hasn't
                        // changed until the value is at least HYSTERESIS_ code
                        // points outside of the bin boundary. This ensures that the
                        // bin doesn't spuriously change due to noise when the pot
                        // is on a bin boundary. It has to go "sufficiently far" into
                        // the next bin.
                        if (next_value < bin_start - HYSTERESIS_) {
                                current_bin_ = next_bin;
                                return next_bin;
                        } else if (next_value > bin_end + HYSTERESIS_) {
                                current_bin_ = next_bin;
                                return next_bin;
                        }
                } else {
                        return NO_CHANGE;
                }
        }
        
        Potentiometer(byte pin) : pin_{pin} {}

        // XXX: consider copy construction and assignment
};
        
