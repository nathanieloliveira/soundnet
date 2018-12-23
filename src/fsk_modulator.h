#pragma once
#include "SineGenerator.h"

class fsk_modulator
{
	float sample_rate;
    float carrier_frequency;
    float freq_step;

	int samples_per_symbol;
	int bits_per_symbol;
	int symbols;
    int ramp_units;

	SineGenerator generator;

	fsk_modulator() = delete;

public:
	fsk_modulator(float _sample_rate, int samples_per_symbol, int bits_per_symbol, float carrier_frequency);
    ~fsk_modulator() = default;

    void modulate_symbol(int symbol, float* bufffer);

};
