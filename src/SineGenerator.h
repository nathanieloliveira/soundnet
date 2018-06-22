#pragma once
#include <cmath>

#define PI 3.1415926535

class SineGenerator
{
	double phase;
	float period;
	float sample_rate;
	float wave_frequency;
public:
	SineGenerator(float _sample_rate, float _wave_frequency);
	~SineGenerator();

	void generate_wave(float* dest, int count, float freq);
};

