#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <getopt.h>
#include <assert.h>
#include <fftw3.h>
#include <math.h>
#include <time.h>
#include "shared.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CURSOR_ON "\033[?25h"
#define CURSOR_OFF "\033[?25l"
#define RETURN_HOME "\033[H"
#define CLR_SCR "\033[2J"

struct config {
    int fft_sz;
    long long frequency;
    long long bandwidth;
    long long filter_bandwidth;
    int verbose;
    int sliding;
} config;

fftw_plan forward, reverse;
fftw_complex *fi, *fo;
double *samples1, *samples2, *samples0;
size_t samples_sz;

double bin_freq(unsigned int binno) {

    int n = (binno + config.fft_sz / 2) % config.fft_sz;

    double hi_freq = (double)config.frequency + config.bandwidth / 2.0;
    double lo_freq = (double)config.frequency - config.bandwidth / 2.0;

    double c = (double)n / (config.fft_sz - 1);

    double a = (1.0 - c) * lo_freq;
    double b = c * hi_freq;

    return a + b;
}

void fft_bandpass_filter(fftw_complex * fft, long long filter_bandwidth) {

    int i;

    for (i = 0; i < config.fft_sz; i++) {
        double hz = bin_freq(i);

        if ((hz < (double)(config.frequency - filter_bandwidth / 2)) || (hz > (double)(config.frequency + filter_bandwidth / 2))) {
            fft[i][0] = 0.0;
            fft[i][1] = 0.0;
        }
    }
}

void usage(const char *arg0) {
    printf("\nusage: %s [options] center bandwidth filter bins < input_samples.dat > output_samples.dat\n\n", arg0);

    printf("\tcenter       center frequency in hz or khz or mhz or ghz\n");
    printf("\tbandwidth    bandwidth in hz or khz or mhz or ghz\n");
    printf("\tfilter       bandpass filter bandwidth in hz or khz or mhz or ghz\n");
    printf("\tbins         fft bin count\n");
    printf("\t-h           show this help\n");
    printf("\t-v           verbose (more -v's for more verbosity)\n");
    printf("\t-s           use overlapping sliding windows for smoother filtering at transform boundaries\n");

    putchar('\n');
}

void exit_handler() {

    fftw_destroy_plan(forward);
    fftw_destroy_plan(reverse);

    fftw_free(fi);
    fftw_free(fo);

    if (config.verbose) {
        fputs(CURSOR_ON, stderr);
        fputs("\033[100B", stderr);
        fputs("\ngoodbye...\n", stderr);
    }
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    int i, j, opt;
    int offset;

    config.verbose = 0;
    config.sliding = 0;

    while ((opt = getopt(argc, argv, "vhs")) != -1) {
        switch (opt) {
        case 'v':
            config.verbose++;
            break;
        case 's':
            config.sliding = 1;
            break;
        case 'h':
        default:
            usage(*argv);
            exit(EXIT_FAILURE);
        }
    }

    if (argc - optind < 4) {
        usage(*argv);
        exit(EXIT_FAILURE);
    }

    config.frequency = hztoll(argv[optind + 0]);
    config.bandwidth = hztoll(argv[optind + 1]);
    config.filter_bandwidth = hztoll(argv[optind + 2]);
    config.fft_sz = strtoul(argv[optind + 3], NULL, 0);

    if (config.verbose > 1) {
        fputs(CLR_SCR, stderr);
        fputs(RETURN_HOME, stderr);
    }

    if (config.verbose) {
        char buf[256];

        fprintf(stderr, "fft bin count: %d\n", config.fft_sz);
        fprintf(stderr, "center frequency: %s\n", hzstring(config.frequency, 3, buf, sizeof(buf)));
        fprintf(stderr, "singal bandwidth: %s\n", hzstring(config.bandwidth, 3, buf, sizeof(buf)));
        fprintf(stderr, "filter bandwidth: %s\n", hzstring(config.filter_bandwidth, 3, buf, sizeof(buf)));
        fprintf(stderr, "sliding windows: %s\n", config.sliding ? "true" : "false");
        fprintf(stderr, "verbosity: %d\n", config.verbose);
    }

    atexit(exit_handler);
    signal(SIGINT, sig_handler);

    fi = fftw_malloc(sizeof(fftw_complex) * config.fft_sz);
    fo = fftw_malloc(sizeof(fftw_complex) * config.fft_sz);

    forward = fftw_plan_dft_1d(config.fft_sz, fi, fo, FFTW_FORWARD, FFTW_MEASURE);
    reverse = fftw_plan_dft_1d(config.fft_sz, fo, fi, FFTW_BACKWARD, FFTW_MEASURE);

    samples_sz = sizeof(double) * config.fft_sz;

    samples0 = malloc(samples_sz);
    samples1 = malloc(samples_sz);
    samples2 = malloc(samples_sz);

    setvbuf(stderr, NULL, _IONBF, 0);

    for (offset = 0;; offset += config.fft_sz) {

        int n;

        memcpy(samples2, samples1, samples_sz);

        n = read2(0, samples1, samples_sz);

        if (n == -1) {
            perror("read2()");
            break;
        }

        offset += n / sizeof(double);

        if (config.verbose) {
            fputs(CURSOR_OFF, stderr);
            fprintf(stderr, "sample offset: %d\n", offset);
        }

        if (n < samples_sz) {
            if (n != 0)
                fprintf(stderr, "\nfinal samples short of a complete FFT (%d of %d)\n", n / sizeof(double), config.fft_sz);
            break;
        }

        for (i = 0; i < config.fft_sz; i++) {
            fi[i][0] = samples1[i] * 1e19;
            fi[i][1] = 0.0;
        }

        fftw_execute(forward);

        fft_bandpass_filter(fo, config.filter_bandwidth);

        fftw_execute(reverse);

        for (i = 0; i < config.fft_sz; i++) {
            samples0[i] = fo[i][0];
        }

        n = write2(1, samples0, samples_sz);
        if (n != samples_sz) {
            perror("write2()");
            break;
        }

        if (config.verbose) {
            if (config.verbose > 1) {
                fprintf(stderr, "%3s %9s %8s %8s %8s %6s\n", "bin", "frequency", "Re", "Img", "|x|", "angle");
                for (j = 0; j < config.fft_sz; j++) {
                    i = (j + config.fft_sz / 2) % config.fft_sz;
                    char buf[256];
                    double a = fo[i][0];
                    double b = fo[i][1];

                    fprintf(stderr, "%03i %9s %+8.2g %+8.2g %+8.2g %6.1f\n", i, hzstring(bin_freq(i), 5, buf, sizeof(buf)), a, b,
                            hypot(b, a), atan2(b, a) * 180.0 / M_PI);
                }
                fprintf(stderr, "\033[%dA", config.fft_sz + 1);
            }
            fputs("\033[1A", stderr);
            fputs(CURSOR_ON, stderr);
        }
    }

    exit(EXIT_SUCCESS);
}
