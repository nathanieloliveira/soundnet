#pragma once
#include "SineGenerator.h"

class fsk_modulator
{
	float sample_rate;
	int samples_per_symbol;
	int bits_per_symbol;
	int symbols;
	float carrier_frequency;
	float freq_step;

	SineGenerator generator;

	fsk_modulator() = delete;

public:
	fsk_modulator(float _sample_rate, int samples_per_symbol, int bits_per_symbol, float carrier_frequency);
	~fsk_modulator();

	void modulate_symbol(int symbol, float* bufffer);
	void modulate_ending(float* bufffer);

};