#pragma once
#include <cmath>

class SineGenerator
{
    static constexpr double PI = 3.1415926535;
    double phase;
    double sample_rate;

public:
    SineGenerator(float _sample_rate);
    ~SineGenerator() = default;

	void generate_wave(float* dest, int count, float freq);
};

