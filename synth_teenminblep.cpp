/*
=============================================================================
The MinBlepper class is based on VCV Rack's source code, licensed under GPLv3.
VCV Rack Free source code and binaries are copyright © 2016-2023 VCV.
=============================================================================
The TeenBLEPVCO class is based on the Fundamental VCO, licensed under GPLv3.
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

#include "synth_teenminblep.h"

inline float interpolateLinear(const float* mbTable, float x)
{
    int xi = x;
    float xf = x - xi;
    float crossfade = mbTable[xi] + (mbTable[xi + 1] - mbTable[xi]) * xf;
    return crossfade;
}

inline float fclampf(float x, float a = 0.f, float b = 1.f) {
    return fmaxf(fminf(x, b), a);
}

void MinBlepper::insertDiscontinuity(float phase, float amount)
{
    if (!(-1 < phase && phase <= 0))
        return;
    for (int j = 0; j < 2 * _zeroCrossings; j++) {
        float minBlepIndex = ((float)j - phase) * _overSampling;
        int index = (_pos + j) % (2 * _zeroCrossings);
        _buf[index] += amount * (-1.f + interpolateLinear(_minblepTable, minBlepIndex));
    }
}

float MinBlepper::process()
{
    float v = _buf[_pos];
    _buf[_pos] = float(0);
    _pos = (_pos + 1) % (2 * _zeroCrossings);
    return v;
}


void AudioTeenBLEPVCO::update(void)
{
    audio_block_t* out;
    uint32_t i;

    out = allocate();
    for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float deltaPhase = fclampf(_frequency * _sampleTime, 0.f, 0.35f);

        _phase += deltaPhase;
        _phase -= floorf(_phase);

        // Detect discontinuities and insert BLEPS
        switch (_waveform) {
            case 0:
            {
                float halfCrossing = (0.5f - (_phase - deltaPhase)) / deltaPhase;
                bool jump = (0 < halfCrossing) & (halfCrossing <= 1.f);
                if (jump) {
                    _minBlepper.insertDiscontinuity(halfCrossing - 1.f, -2.f);
                }
                break;
            }
            case 1:
            {
                // Jump sqr when crossing 0
                float wrapCrossing = (0 - (_phase - deltaPhase)) / deltaPhase;
                bool jumpWrap = (0 < wrapCrossing) & (wrapCrossing <= 1.f);
                if (jumpWrap) {
                    _minBlepper.insertDiscontinuity(wrapCrossing - 1.f, 2.f);
                }
                // Jump sqr when crossing `pulseWidth`
                float pulseCrossing = (_pulseWidth - (_phase - deltaPhase)) / deltaPhase;
                bool jumpPulse = (0 < pulseCrossing) & (pulseCrossing <= 1.f);
                if (jumpPulse) {
                    _minBlepper.insertDiscontinuity(pulseCrossing - 1.f, -2.f);
                }
                break;
            
            }
        }

        // hardsync discontinuity stuff goes here

        // construct the waveforms
        float oscValue;
        switch (_waveform) {
            case 0:
            {
                float x = _phase + 0.5f;
                x -= truncf(x);
                oscValue = 2.f * x - 1.f;
                oscValue += _minBlepper.process();
                break;
            }
            case 1:
            {
                oscValue = (_phase < _pulseWidth) ? 1.f : -1.f;
                oscValue += _minBlepper.process();
                break;
            }
            case 2:
            {
                oscValue = 1 - 4 * fminf(fabsf(_phase - 0.25f), fabsf(_phase - 1.25f));
                break;
            }
        }
        // Teensy Audio library uses 16bit integers. Attenuate before conversion to avoid overflows.
        float sample = oscValue * 0.3f;
        out->data[i] = (int16_t)(sample * 32768.0f);
    }
    transmit(out, 0);
    release(out);
}
