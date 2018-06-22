#include "fsk_modulator.h"

fsk_modulator::fsk_modulator(float _sample_rate, int _samples_per_symbol, int _bits_per_symbol, float _carrier_frequency)
	: sample_rate(_sample_rate),
	samples_per_symbol(_samples_per_symbol),
	bits_per_symbol(_bits_per_symbol),
	symbols((1 << _bits_per_symbol)),
	carrier_frequency(_carrier_frequency),
	generator(_sample_rate, _carrier_frequency)
{
	// basically facilitates the fft demodulation
	freq_step = sample_rate / samples_per_symbol;
}


fsk_modulator::~fsk_modulator()
{
}


void fsk_modulator::modulate_symbol(int symbol, float* buffer)
{
	float symbol_freq = carrier_frequency;
	if (symbol % 2 == 0) {
		symbol_freq += (-1 - ceilf(symbol / 2.0f)) * freq_step;
	}
	else
	{
		symbol_freq += ceilf(symbol / 2.0f) * freq_step;
	}
	generator.generate_wave(buffer, samples_per_symbol, symbol_freq);
}

void fsk_modulator::modulate_ending(float* bufffer)
{

}
