#include "SineGenerator.h"

SineGenerator::SineGenerator(float _sample_rate)
    : phase(0.0),
    sample_rate(static_cast<double>((_sample_rate)))
{

}

void SineGenerator::generate_wave(float* dest, int count, float frequency)
{
    double freq = static_cast<double>(frequency);
    double p = 2 * PI * freq * (1.0 / sample_rate);
	for (int i = 0; i < count; i++)
	{
        auto v = std::sin(phase);
        dest[i] += static_cast<float>(v);
		phase += p;
	}

	if (phase >= (2 * PI))
	{
		phase -= (2 * PI);
	}
}
