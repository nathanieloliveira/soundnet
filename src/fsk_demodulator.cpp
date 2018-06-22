#include "fsk_demodulator.h" 
#include <cstring>
#include <math.h>
#include <algorithm>

fsk_demodulator::fsk_demodulator(float _sample_rate, int _samples_per_symbol, int _bits_per_symbol, float _carrier_frequency)
	: sample_rate(_sample_rate),
	samples_per_symbol(_samples_per_symbol),
	bits_per_symbol(_bits_per_symbol),
	symbols((1 << _bits_per_symbol)),
	carrier_frequency(_carrier_frequency)
{
	// basically facilitates the fft demodulation
	freq_step = sample_rate / samples_per_symbol;
	data_size = samples_per_symbol / 2;

	carrier_bucket = roundf(carrier_frequency / (sample_rate / samples_per_symbol));

	// initial value for early gate pointer
	gate_p = samples_per_symbol - 1;

	fft_in = fftwf_alloc_real(samples_per_symbol);
	spec_density = fftwf_alloc_real(data_size);
	early_gate = fftwf_alloc_real(samples_per_symbol * 2);
	fft_out = fftwf_alloc_complex(data_size);
	plan = fftwf_plan_dft_r2c_1d(samples_per_symbol, fft_in, fft_out, FFTW_ESTIMATE);

	memset(early_gate, 0, samples_per_symbol * 2 * sizeof(float));
}


fsk_demodulator::~fsk_demodulator()
{
	fftwf_free(fft_in);
	fftwf_free(spec_density);
	fftwf_free(fft_out);
	fftwf_free(early_gate);
	fftwf_destroy_plan(plan);
}

void fsk_demodulator::calculate_spectral_density(fftwf_complex* source, float* dest, int count)
{
	for (int i = 0; i < count; i++)
	{
		float real = source[i][0];
		float imag = source[i][1];

		real /= count * 2;
		imag /= count * 2;

		dest[i] = 2 * sqrtf(powf(real, 2) + powf(imag, 2));
	}
}

float fsk_demodulator::total_energy(fftwf_complex* dft, int count)
{
	float mul = 1 / (float) data_size;
	float acc = 0;
	for (int i = 0; i < data_size; i++)
	{
		float real = dft[i][0];
		float imag = dft[i][1];

		real /= count * 2;
		imag /= count * 2;

		acc += powf(2 * sqrtf(powf(real, 2) + powf(imag, 2)), 2);
	}
	return acc * mul;
}

float fsk_demodulator::total_power(float* spectre, int count)
{
	float total_power = 0;
	float freq_step = sample_rate / (count * 2);
	for (int i = 1; i < data_size; i++)
	{
		total_power += spectre[i] * i * freq_step;
	}
	return total_power;
}

int fsk_demodulator::find_bucket_and_energy(float* energy_out, float* psd)
{
	int bucket = 0;
	float max = 0;
	int low_symbol_bucket = carrier_bucket - symbols / 2;
	int high_symbol_bucket = carrier_bucket + 1 + symbols / 2;
	for (int i = low_symbol_bucket; i < high_symbol_bucket; i++)
	{
		float v = psd[i];
		if (v > max)
		{
			max = v;
			bucket = i;
		}
	}
	*energy_out = max;
	return bucket;
}

int fsk_demodulator::find_symbol(int bucket)
{
	int symbol = -1;
	int offset = bucket - carrier_bucket;
	if (offset == 0)
	{
		return OFFSET_ZERO;
	}

	if (offset > 0)
	{
		symbol = (offset * 2) - 1;
	}
	else
	{
		symbol = abs(offset * 2) - 2;
	}
	if (symbol >= symbols)
	{
		symbol = OUT_OF_RANGE;
	}
	return symbol;
}

int fsk_demodulator::demodulate_symbol(const float* buffer)
{
	// calculate late fft
	std::copy(buffer, buffer + samples_per_symbol, fft_in);
	fftwf_execute(plan);
	calculate_spectral_density(fft_out, spec_density, data_size);

	// find bucket with max energy with late samples
	float max = 0;
	int bucket = find_bucket_and_energy(&max, spec_density);

	float max_energy = powf(max, 2);
	float avg_energy = total_energy(fft_out, data_size);

	// calculate early fft
	for (int i = 0; i < gate_p; i++)
	{
		early_gate[gate_p + i] = buffer[i];
	}
	//std::copy(buffer, buffer + gate_p, early_gate + gate_p);
	std::copy(early_gate, early_gate + samples_per_symbol, fft_in);
	fftwf_execute(plan);
	calculate_spectral_density(fft_out, spec_density, data_size);

	// find early bucket and max energy
	float max_early = 0;
	int bucket_early = find_bucket_and_energy(&max_early, spec_density);

	float max_energy_early = powf(max_early, 2);
	float avg_energy_early = total_energy(fft_out, data_size);

	// test threshold
	if (max_energy_early < (avg_energy_early * 3))
	{
		// reset gate position
		gate_p = samples_per_symbol;

		// fill early gate
		const float* buffer_end = buffer + samples_per_symbol;
		std::copy(buffer_end - gate_p, buffer_end, early_gate);
		return NOISE;
	}

	// adjust gate p if needed
	if (bucket_early != bucket)
	{
		if (abs(max - max_early) < max_early * 0.05)
		{
			// do nothing
		}
		else if (max > max_early)
		{
			gate_p += 2;
		}
		else
		{
			gate_p -= 2;
		}

		if (gate_p > samples_per_symbol)
		{
			gate_p = samples_per_symbol - samples_per_symbol / 2;
		}

		if (gate_p < 0)
		{
			gate_p = samples_per_symbol / 2;
		}
		printf("                 gate_p: %d\n", gate_p);
	}

	//for (int i = 0; i < gate_p; i++)
	//{
	//	early_gate[i] = *(buffer + samples_per_symbol - gate_p + i);
	//}

	// fill early gate
	const float* buffer_end = buffer + samples_per_symbol;
	std::copy(buffer_end - gate_p, buffer_end, early_gate);
	//memcpy(early_gate, buffer_end - gate_p, samples_per_symbol - gate_p);

	return find_symbol(bucket_early);
}
