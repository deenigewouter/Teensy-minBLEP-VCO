/*
=============================================================================
MinBlepper and interpolateLinear are derived from on VCV Rack's source code, licensed under GPLv3.
VCV Rack Free source code and binaries are copyright © 2016-2023 VCV.
=============================================================================
The TeenBLEPVCO class is derivded from the Fundamental VCO, licensed under GPLv3.
Source code is copyright © 2016-2022 Andrew Belt.
This program is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 3 of the License, or (at your option) any later 
version.
=============================================================================
* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

//-----------------------------------------------------------
 // AudioTeenBLEPVCO for Teensy Audio Library, v0.1
 // Based on VCV Rack and VCV Fundamental source code.  
 // Wouter van Schendel, March 22, 2023
 // 
 // This is a work-in-progress, improvements will be made.
 // Pull requests and/or feedback are welcome :). Have fun
 // 
 // Missing features / roadmap:
 //     - Modulation
 //     - Hardsync 
 //     - Oversampling (maybe)
 //     - Analogy waveshaping
 //     - Oscillator drift
 //     - volume compensation
 //     - proper waveform labeling
 // 
 // Issues
 //     - DC offset on sawtooth. Increases with octaves
 //     - probably more
 // 
 // If the license notice above is incorrect, please let me know. 
 //-----------------------------------------------------------



#ifndef _SYNTH_TEENBLEP_h
#define _SYNTH_TEENBLEP_h


#include <Arduino.h>
#include "AudioStream.h"
#include "minblepTable.h"

inline float fclampf(float in, float min, float max);

inline float interpolateLinear(const float* p, float x);

// Maintains the minBLEP buffer. The included 2049 sample minBLEP 
// is 16 zerocrossings, 64 oversampling + 1 padding.
// Probably much bigger than necessary.
class MinBlepper {

public:
    void insertDiscontinuity(float phase, float x);
    float process();

private:
    static const int _zeroCrossings = 16;
    static const int _overSampling = 64;
    const float *_minblepTable = minblep_Table;
    int _pos = 0;
    float _buf[2 * _zeroCrossings] = {};
};


class AudioTeenBLEPVCO : public AudioStream
{
public:
    AudioTeenBLEPVCO(void) : AudioStream(0, NULL) {}
    virtual void update(void);
    void frequency(float freq) {
        if (freq < 0.0f) {
            freq = 0.0;
        }
        else if (freq > AUDIO_SAMPLE_RATE_EXACT / 2.0f) {
            freq = AUDIO_SAMPLE_RATE_EXACT / 2.0f;
        }
        _frequency = freq;
    }
    void waveform(int wave)
    {
        _waveform = wave;
    }
    void pulseWidth(float n) {	// 0.0 to 1.0
        if (n < 0.05) {
            n = 0.05;
        }
        else if (n > 0.95f) {
            n = 0.95f;
        }
        _pulseWidth = n;
    }
    void begin(int wave)
    {
        _sampleTime = 1.f / AUDIO_SAMPLE_RATE_EXACT;
        _phase = 0.1f;
        frequency(440.f);
        pulseWidth(0.5f);
        waveform(wave);
    }
private:
    float _sampleTime;
    float _phase;
    float _pulseWidth;
    float _frequency;
    int _waveform;
    MinBlepper _minBlepper;
};

#endif

