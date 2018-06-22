#include "SineGenerator.h"

SineGenerator::SineGenerator(float _sample_rate, float _wave_frequency)
	: phase(0.0f),
	sample_rate(_sample_rate),
	wave_frequency(_wave_frequency)
{
	period = 2 * PI * wave_frequency * (1 / sample_rate);
}

SineGenerator::~SineGenerator()
{
}

void SineGenerator::generate_wave(float* dest, int count, float frequency)
{
	double p = 2 * PI * frequency * (1 / sample_rate);
	for (int i = 0; i < count; i++)
	{
		double v = sin(phase);
		dest[i] += v;
		phase += p;
	}

	if (phase >= (2 * PI))
	{
		phase -= (2 * PI);
	}
}
