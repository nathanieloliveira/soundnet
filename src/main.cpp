#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <cstring>
#include <list>
#include <pa_asio.h>
#include "SineGenerator.h"
#include <array>
#include <DspFilters\Butterworth.h>
#include <fftw3.h>
#include "fsk_modulator.h"
#include "fsk_demodulator.h"
//#include "pa_win_wasapi.h"

#define SAMPLE_RATE 48000.0
#define CARRIER_FREQUENCY 7500.0f
#define OUTPUT_CHANNELS 2
#define FFT_SIZE 128
#define BITS_PER_SYMBOL 2

struct Generators
{
	std::list<SineGenerator*> generators;
};

Dsp::SimpleFilter<Dsp::Butterworth::LowPass<2>, 1> f;
SineGenerator sine(SAMPLE_RATE, CARRIER_FREQUENCY);
std::array<float, 1024> buffer;

fsk_modulator fsk_mod(SAMPLE_RATE, FFT_SIZE, BITS_PER_SYMBOL, CARRIER_FREQUENCY);
fsk_demodulator fsk_dem(SAMPLE_RATE, FFT_SIZE, BITS_PER_SYMBOL, CARRIER_FREQUENCY);

int input_stream_callback(const void* input,
	void* output,
	unsigned long frame_count,
	const PaStreamCallbackTimeInfo* time,
	PaStreamCallbackFlags flags,
	void* user_data)
{	
	static int count1 = 0;
	static int symbol = 0;
	static float temp[1024];
	const float* in = reinterpret_cast<const float*>(input);
	float* out = reinterpret_cast<float*>(output);
	Generators* gen = reinterpret_cast<Generators*>(user_data);

	memset(out, 0, 2 * frame_count * sizeof(float));
	memset(buffer.data(), 0, 2 * frame_count * sizeof(float));
	memset(temp, 0, 2 * frame_count * sizeof(float));

	//memcpy(buffer.data(), in, frame_count * sizeof(float));
	//float* b = buffer.data();
	//f.process(frame_count, &b);

	if (count1++ > 0)
	{
		count1 = 0;
		symbol++;
		if (symbol > 3)
		{
			symbol = 0;
		}
	}

	fsk_mod.modulate_symbol(symbol, buffer.data());
	//sine.generate_wave(buffer.data(), frame_count, 8250.0f);

	for (int i = 0; i < frame_count; i++)
	{
		out[i * OUTPUT_CHANNELS] = buffer[i];
		out[i * OUTPUT_CHANNELS + 1] = buffer[i];
		//printf("%f\n", in[i]);
	}

	int dem_symbol = fsk_dem.demodulate_symbol(in);
	if (count1 == 0)
	{
		printf("symbol dem: %d\n", dem_symbol);
	}

	return paContinue;
}

int output_stream_callback(const void* input,
	void* output,
	unsigned long frame_count,
	const PaStreamCallbackTimeInfo* time,
	PaStreamCallbackFlags flags,
	void* user_data)
{
	static int counter;
	static bool selector;
	static float freq1 = 10000.0f;
	static float freq2 = 12000.0f;
	static float buffer[1024];
	float* out = reinterpret_cast<float*>(output);
	float freq = 0;
	Generators* gen = reinterpret_cast<Generators*>(user_data);

	memset(output, 0, sizeof(float) * frame_count);

	if (counter++ >= 10)
	{
		selector = !selector;
		counter = 0;
	}

	/*for (auto it : gen->generators)
	{
		it->generate_wave(out, frame_count, selector ? freq1 : freq2);
	}*/

	/*float mul = 1 / (float)gen->generators.size();
	for (int i = 0; i < frame_count; i++)
	{
		out[i] *= mul;
	}*/

	for (int i = 0; i < frame_count; i++)
	{
		float v = ((float)rand() / RAND_MAX);
		out[i] = v;
	}
	return paContinue;
}

int main(int argc, char** argv)
{
	Generators gens;
	SineGenerator generator1 = SineGenerator(SAMPLE_RATE, 523.25f);
	SineGenerator generator2 = SineGenerator(SAMPLE_RATE, 659.25f);
	SineGenerator generator3 = SineGenerator(SAMPLE_RATE, 783.99f);
	gens.generators.push_back(&generator1);
	//gens.generators.push_back(&generator2);
	//gens.generators.push_back(&generator3);
	PaError error = Pa_Initialize();

	f.setup(2, SAMPLE_RATE, 2000.0);

	const int fft_size = 64;
	const int data_size = fft_size/2;
	float fft_in[fft_size];
	float spec_out[data_size];
	fftwf_complex fft_out[data_size];

	memset(fft_in, 0, sizeof(fft_in));
	memset(fft_in, 0, sizeof(spec_out));

	//generator1.generate_wave(fft_in, fft_size, 3750.0f);
	//generator2.generate_wave(fft_in, fft_size, 5250.0f);

	/*fsk_modulator fsk(SAMPLE_RATE, fft_size, 1, 12000.0f);
	fsk.modulate_symbol(1, fft_in);

	for (int i = 0; i < fft_size; i++)
	{
		fft_in[i] *= 0.1;
		fft_in[i] += (-1 + 2 * rand() / (float)RAND_MAX) * 0.5;
	}

	fsk_demodulator fsk_dem(SAMPLE_RATE, fft_size, 1, 12000.f);
	int symbol = fsk_dem.demodulate_symbol(fft_in);*/

	int devs = Pa_GetDeviceCount();
	for (int i = 0; i < devs; i++) {
		const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
		printf("n: %d -> %s\n", i, info->name);
	}

	int output_device;
	int input_device;
	int host_apis = Pa_GetHostApiCount();
	for (int i = 0; i < host_apis; i++)
	{
		const PaHostApiInfo* info = Pa_GetHostApiInfo(i);
		if (info->type == paASIO)
		{
			output_device = info->defaultOutputDevice;
			input_device = info->defaultInputDevice;
			//break;
		}
	}

	const PaDeviceInfo* output_info = Pa_GetDeviceInfo(output_device);
	printf("%s\n", output_info->name);

	PaStreamParameters output_paramaters;
	output_paramaters.channelCount = OUTPUT_CHANNELS;
	output_paramaters.device = output_device;
	output_paramaters.hostApiSpecificStreamInfo = NULL;
	output_paramaters.sampleFormat = paFloat32;
	output_paramaters.suggestedLatency = output_info->defaultLowOutputLatency;

	const PaDeviceInfo* input_info = Pa_GetDeviceInfo(input_device);
	printf("%s\n", input_info->name);

	PaStreamParameters input_parameters;
	input_parameters.channelCount = 1;
	input_parameters.device = input_device;
	input_parameters.hostApiSpecificStreamInfo = NULL;
	input_parameters.sampleFormat = paFloat32;
	input_parameters.suggestedLatency = input_info->defaultLowInputLatency;
	
	error = Pa_IsFormatSupported(&input_parameters, &output_paramaters, SAMPLE_RATE);
	if (error == paFormatIsSupported)
	{
		printf("format is supported\n");
	}
	else
	{
		goto error;
	}

	PaStream* output_stream;

	error = Pa_OpenStream(
		&output_stream,
		&input_parameters,
		&output_paramaters,
		SAMPLE_RATE,
		FFT_SIZE,
		paNoFlag,
		input_stream_callback,
		&gens);
	if (error != paNoError)
	{
		goto error;
	}

	error = Pa_StartStream(output_stream);
	if (error != paNoError)
	{
		goto error;
	}

	Pa_Sleep(5000);

	Pa_StopStream(output_stream);
	Pa_CloseStream(output_stream);

	error = Pa_Terminate();
	getchar();
	return 0;

error:
	getchar();
	return error;
}