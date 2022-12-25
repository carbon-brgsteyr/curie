#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "portaudio.h"

// #define SAMPLE_RATE  (17932) // Test failure to open with this value.
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (512)
#define NUM_SECONDS         (60)
#define TIME_LIMIT          (false)
// #define DITHER_FLAG         (paDitherOff)
#define DITHER_FLAG         (0)

#define LOG_PATH            "./log"

// Select sample format.
#if 1
#define PA_SAMPLE_TYPE  paFloat32
#define CPP_SAMPLE_TYPE float
#define SAMPLE_SIZE     (4)
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 0
#define PA_SAMPLE_TYPE  paInt16
#define CPP_SAMPLE_TYPE int16_t
#define SAMPLE_SIZE     (2)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
// #define PA_SAMPLE_TYPE  paInt24
// #define SAMPLE_SIZE     (3)
// #define SAMPLE_SILENCE  (0)
// #define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
#define CPP_SAMPLE_TYPE int8_t
#define SAMPLE_SIZE     (1)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
#define CPP_SAMPLE_TYPE uint8_t
#define SAMPLE_SIZE     (1)
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif

FILE *fp = NULL;
PaStream *stream = NULL;
char *sample_block = NULL;

void end_stream(PaStream *stream) {
    if (stream) {
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}

void print_device_info(const PaDeviceInfo* info) {
    fprintf(fp, "    Name: %s\n", info->name);
    fprintf(fp, "      LL: %g s\n", info->defaultLowInputLatency);
    fprintf(fp, "      HL: %g s\n", info->defaultHighInputLatency);
}

void sig_handler(int signo) {
    if (signo != SIGINT) return;
    free(sample_block);
    end_stream(stream);
    fprintf(fp, "Stop: %lu\n", (uint64_t)time(NULL));
    fflush(fp);
    fclose(fp);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");

    int opt;

    bool debug = false;
    bool echo = false;

    while ((opt = getopt(argc, argv, ":de")) != -1) {
        switch (opt) {
            case 'd':
                debug = true;
                break;
            case 'e':
                echo = true;
                break;
        }
    }

    fp = fopen(LOG_PATH, "a");
 
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) goto error2;

    PaStreamParameters input_parameters;
    input_parameters.device = Pa_GetDefaultInputDevice();    // default input device
    
    const PaDeviceInfo* input_info;
    input_info = Pa_GetDeviceInfo(input_parameters.device);

    PaStreamParameters output_parameters;
    output_parameters.device = Pa_GetDefaultOutputDevice();  // default output device
    
    const PaDeviceInfo* output_info;
    output_info = Pa_GetDeviceInfo(output_parameters.device);

    fprintf(fp, "=========================\n");
    
    fprintf(fp, "Input device: %d\n", input_parameters.device);
    print_device_info(input_info);

    fprintf(fp, "Output device: %d\n", output_parameters.device);
    print_device_info(output_info);
    
    size_t num_channels;
    // num_channels = input_info->maxInputChannels < output_info->maxOutputChannels
    //    ? input_info->maxInputChannels
    //    : output_info->maxOutputChannels;
    num_channels = 2;

    fprintf(fp, "Num channels: %zu\n", num_channels);

    input_parameters.channelCount = num_channels;
    input_parameters.sampleFormat = PA_SAMPLE_TYPE;
    input_parameters.suggestedLatency = input_info->defaultHighInputLatency ;
    input_parameters.hostApiSpecificStreamInfo = NULL;

    output_parameters.channelCount = num_channels;
    output_parameters.sampleFormat = PA_SAMPLE_TYPE;
    output_parameters.suggestedLatency = output_info->defaultHighOutputLatency;
    output_parameters.hostApiSpecificStreamInfo = NULL;

    // -- setup --
    err = Pa_OpenStream(
        &stream,
        &input_parameters,
        &output_parameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,          // we won't output out of range samples so don't bother clipping them
        NULL,               // no callback, use blocking API
        NULL);              // no callback, so no callback userData
    if (err != paNoError) goto error2;

    size_t num_bytes;
    num_bytes = FRAMES_PER_BUFFER * num_channels * SAMPLE_SIZE;
    sample_block = (char *)malloc(num_bytes);
    if (sample_block == NULL) {
        fprintf(fp, "Could not allocate record array.\n");
        goto error1;
    }
    memset(sample_block, SAMPLE_SILENCE, num_bytes);

    float *floatBlock = (float *)sample_block;

    err = Pa_StartStream(stream);
    if (err != paNoError) goto error1;
    
    fprintf(fp, "Start: %lu\n", (uint64_t)time(NULL));
    if (TIME_LIMIT) {
        fprintf(fp, "Will run %d seconds\n", NUM_SECONDS);
    }
    else {
        fprintf(fp, "Will run indefinitely\n");
    }
    fflush(fp);

    // recorded channel
    size_t channel = 0;
    
    uint64_t count = 0;
    bool state = false;
    float positive = 0.5;
    float negative = -0.5;

    for (size_t i = 0; !TIME_LIMIT || i < (NUM_SECONDS * SAMPLE_RATE) / FRAMES_PER_BUFFER; ++i) {
        // You may get underruns or overruns if the output is not primed by PortAudio.
        if (echo) {
            err = Pa_WriteStream(stream, sample_block, FRAMES_PER_BUFFER);
            if (err) goto xrun;
        }
        err = Pa_ReadStream(stream, sample_block, FRAMES_PER_BUFFER);
        if (err) goto xrun;
        for (size_t j = 0; j < FRAMES_PER_BUFFER; j++) {
            float f = floatBlock[j * num_channels + channel];
            if (!state && f > positive) state = true;
            if (state && f < negative) {
                state = false;
                count++;
                if (debug) 
                    printf("\r%lu", count);
                else
                    printf("\n");
                fflush(stdout);
            }
        }
    }

    fprintf(fp, "Wire off.\n");
    fflush(fp);

    err = Pa_StopStream(stream);
    if (err != paNoError) goto error1;

    free(sample_block);
    Pa_Terminate();

    fclose(fp);
    return 0;

xrun:
    fprintf(fp, "err = %d\n", err);
    free(sample_block);
    end_stream(stream);
    if (err & paInputOverflow)
        fprintf(fp, "Input Overflow.\n");
    if (err & paOutputUnderflow)
        fprintf(fp, "Output Underflow.\n");
    fflush(fp);
    fclose(fp);
    return -2;
error1:
    free(sample_block);
error2:
    end_stream(stream);
    fprintf(fp, "An error occurred while using the portaudio stream\n");
    fprintf(fp, "Error number: %d\n", err);
    fprintf(fp, "Error message: %s\n", Pa_GetErrorText(err));
    fflush(fp);
    fclose(fp);
    return -1;
}
