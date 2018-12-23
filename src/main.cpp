#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <cstring>
#include <list>
#include <pa_asio.h>
#include "SineGenerator.h"
#include <array>
#include <DspFilters/Butterworth.h>
#include <fftw3.h>
#include "fsk_modulator.h"
#include "fsk_demodulator.h"
#include "media_access.h"
//#include "pa_win_wasapi.h"

#define SAMPLE_RATE 48000.0
#define CARRIER_FREQUENCY 7500.0f
#define OUTPUT_CHANNELS 2
#define FFT_SIZE 64
#define BITS_PER_SYMBOL 4

struct Generators
{
	std::list<SineGenerator*> generators;
};

static Dsp::SimpleFilter<Dsp::Butterworth::LowPass<2>, 1> f;
static SineGenerator sine(SAMPLE_RATE);
static std::array<float, 1024> buffer;

static fsk_modulator fsk_mod(SAMPLE_RATE, FFT_SIZE, BITS_PER_SYMBOL, CARRIER_FREQUENCY);
static fsk_demodulator fsk_dem(SAMPLE_RATE, FFT_SIZE, BITS_PER_SYMBOL, CARRIER_FREQUENCY);

static media_access media(SAMPLE_RATE, FFT_SIZE, BITS_PER_SYMBOL, CARRIER_FREQUENCY);

int stream_callback(const void* input,
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

	memset(out, 0, 2 * frame_count * sizeof(float));
	memset(buffer.data(), 0, 2 * frame_count * sizeof(float));
	memset(temp, 0, 2 * frame_count * sizeof(float));

	//memcpy(buffer.data(), in, frame_count * sizeof(float));
	//float* b = buffer.data();
	//f.process(frame_count, &b);

    symbol = symbol == 0 ? 1 : 0;

    fsk_mod.modulate_symbol(symbol, buffer.data());

    for (unsigned long i = 0; i < frame_count; i++)
	{
		out[i * OUTPUT_CHANNELS] = buffer[i];
		out[i * OUTPUT_CHANNELS + 1] = buffer[i];
		//printf("%f\n", in[i]);
	}

	int dem_symbol = fsk_dem.demodulate_symbol(in);
//    printf("symbol dem: %d\n", dem_symbol);

    media.process(in, out, frame_count);

	return paContinue;
}

void print_device_names() {
    int devs = Pa_GetDeviceCount();
    for (int i = 0; i < devs; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        printf("n: %d -> %s\n", i, info->name);
    }
}

int main(int argc, char** argv)
{
	PaError error = Pa_Initialize();
	f.setup(2, SAMPLE_RATE, 2000.0);

    int output_device = Pa_GetDefaultOutputDevice();
    int input_device = Pa_GetDefaultInputDevice();

    int device_count = Pa_GetDeviceCount();
    for (int i = 0; i < device_count; i++)
    {
        const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
        printf("Device %d: %s\n", i, device_info->name);
        printf("Inputs: %d. Outputs: %d\n", device_info->maxInputChannels, device_info->maxOutputChannels);
        printf("HostApi: %s\n", Pa_GetHostApiInfo(device_info->hostApi)->name);
    }

    output_device = input_device;

    const PaDeviceInfo* output_info = Pa_GetDeviceInfo(output_device);
    printf("%s\n", output_info->name);

    PaStreamParameters output_paramaters;
    output_paramaters.channelCount = OUTPUT_CHANNELS;
    output_paramaters.device = output_device;
    output_paramaters.hostApiSpecificStreamInfo = nullptr;
    output_paramaters.sampleFormat = paFloat32;
    output_paramaters.suggestedLatency = output_info->defaultLowOutputLatency;

    const PaDeviceInfo* input_info = Pa_GetDeviceInfo(input_device);
    printf("%s\n", input_info->name);

    PaStreamParameters input_parameters;
    input_parameters.channelCount = 1;
    input_parameters.device = input_device;
    input_parameters.hostApiSpecificStreamInfo = nullptr;
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
        stream_callback,
        nullptr);
	if (error != paNoError)
	{
		goto error;
	}

	error = Pa_StartStream(output_stream);
	if (error != paNoError)
	{
		goto error;
	}

    media.set_packet_callback([](mac_packet& packet){
        printf("got packet\n");
    });

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
