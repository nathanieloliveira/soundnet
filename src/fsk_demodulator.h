#pragma once
#include <fftw3.h>

enum demodulation_errors : int
{
	NOISE = -1,
	OUT_OF_RANGE = -2,
	OFFSET_ZERO = -3
};

class fsk_demodulator
{
	float sample_rate;
	int samples_per_symbol;
	int bits_per_symbol;
	int symbols;
	float carrier_frequency;
	float freq_step;

	int data_size;
	int carrier_bucket;

	float* fft_in;
	float* spec_density;
	float* early_gate;

	int gate_p;

	fftwf_complex* fft_out;
	fftwf_plan plan;

	fsk_demodulator() = delete;

	void calculate_spectral_density(fftwf_complex* complex, float* real, int count);
	float total_power(float* spectre, int count);
	float total_energy(fftwf_complex* dft, int count);
	int find_bucket_and_energy(float* energy_out, float* psd);
	int find_symbol(int bucket);

public:
	fsk_demodulator(float _sample_rate, int samples_per_symbol, int bits_per_symbol, float carrier_frequency);
	~fsk_demodulator();

	int demodulate_symbol(const float* buffer);
};