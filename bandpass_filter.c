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

struct config {
	int fft_sz;
	long long frequency;
	long long bandwidth;
	long long filter_bandwidth;
	int verbose;
} config;

fftw_plan forward, reverse;
fftw_complex *fi, *fo;
double *samples1, *samples2;

void
fft_abs(fftw_complex * fft)
{
    int i;

    for (i = 0; i < config.fft_sz; i++) {
        fft[i][0] = hypot(fft[i][0], fft[i][1]);
        fft[i][1] = 0.0;
    }
}

double
bin_freq(unsigned int binno)
{

	int n = (binno + config.fft_sz / 2) % config.fft_sz;

	double hi_freq = (double)config.frequency + config.bandwidth / 2.0;
	double lo_freq = (double)config.frequency - config.bandwidth / 2.0;

	double c = (double)n / (config.fft_sz - 1);

	double a = (1.0 - c) * lo_freq;
	double b = c * hi_freq;

	return a + b;
}

void fft_bandpass_filter(fftw_complex *fft, long long filter_bandwidth) {

	int i;

	for (i = 0; i < config.fft_sz; i++) {
		double hz = bin_freq(i);
		if( (hz < (double)(config.frequency - filter_bandwidth / 2)) || (hz > (double)(config.frequency + filter_bandwidth / 2)) ) {
			fft[i][0] = 666.0;
			fft[i][1] = 666.0;
		}
	}
}

double
fft_real_weight(fftw_complex * fft)
{

    double sum = 0;
    int i;

    /*
     * i skip the first bin because its the relative gain and i dont want that
     */

    for (i = 1; i * 2 < config.fft_sz; i++)
        sum += fft[i][0];

    /*
     * we only want half the weight due to the symmetry of the FFT
     * im not sure of we want to use i < config.fft_sz / 2 in the for loop or what
     */

    return sum;
}

double
fft_max(fftw_complex * fft)
{
    int i;

    double max = 0.0;

    for (i = 1; i < config.fft_sz; i++)
        if (fft[i][0] > max)
            max = fft[i][0];

    return max;
}

void
fft_normalize(fftw_complex * fft)
{

    int i;

    double max = fft_max(fft);

    fft[0][0] = 0.0;
    fft[0][1] = 0.0;

    if (max != 0.0) {
        for (i = 0; i < config.fft_sz; i++)
            fft[i][0] /= max;
        for (; i < config.fft_sz; i++)
            fft[i][0] = 0.0;
    }
}

void
usage(const char *arg0)
{
    printf("\nusage: %s [options] center bandwidth filter bins < input_samples.dat > output_samples.dat\n\n", arg0);

    printf("\tcenter       center frequency in hz or khz or mhz or ghz\n");
    printf("\tbandwidth    bandwidth in hz or khz or mhz or ghz\n");
    printf("\tfilter       bandpass filter bandwidth in hz or khz or mhz or ghz\n");
    printf("\tbins         fft bin count\n");
	 printf("\t-h           show this help\n");
	 printf("\t-v           verbose\n");

	 putchar('\n');
}

char *hzstring(double v, int res, char *buf, size_t buf_sz) {

	if(abs(v) < 1000) {
		snprintf(buf, buf_sz, "%.0fhz", v);
	} else if(abs(v) < 1000000) {
		snprintf(buf, buf_sz, "%.*gkhz", res, v / 1000);
	} else if(abs(v) < 1000000000) {
		snprintf(buf, buf_sz, "%.*gmhz", res, v / 1000000);
	} else {
		snprintf(buf, buf_sz, "%.*gghz", res, v / 1000000000);
	}

	return buf;
}

int bitcount(unsigned long v) {
	int a = 0;
	while(v) {
		a += (v&1);
		v >>= 1;
	}
	return a;
}

void fatal(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

long long hztoll(const char *str) {
	long long ll;
	int n;

	sscanf(str, "%Ld %n", &ll, &n);

	if(toupper(str[n]) == 'K')
		return ll * 1000;
	if(toupper(str[n]) == 'M')
		return ll * 1000000;
	if(toupper(str[n]) == 'G')
		return ll * 1000000000;
	return ll;
}

int
main(int argc, char **argv)
{
    int i, j;
	 int opt;

    double hz = 3;
    double theta = 45;

	 config.verbose = 0;

	 while ((opt = getopt(argc, argv, "vh")) != -1) {
		 switch (opt) {
			 case 'v':
				 config.verbose = 1;
				 break;
			 case 'h':
			 default: /* '?' */
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

	 if(config.verbose) {
	 	char buf[256];
	 	fprintf(stderr, "fft bin count = %d\n", config.fft_sz);
		fprintf(stderr, "center frequency = %s\n", hzstring(config.frequency, 3, buf, sizeof(buf)));
		fprintf(stderr, "singal bandwidth = %s\n", hzstring(config.bandwidth, 3, buf, sizeof(buf)));
		fprintf(stderr, "filter bandwidth = %s\n", hzstring(config.filter_bandwidth, 3, buf, sizeof(buf)));
	 }

    fi = fftw_malloc(sizeof(fftw_complex) * config.fft_sz);
    fo = fftw_malloc(sizeof(fftw_complex) * config.fft_sz);

	 forward = fftw_plan_dft_1d(config.fft_sz, fi, fo, FFTW_FORWARD, FFTW_MEASURE);
	 reverse = fftw_plan_dft_1d(config.fft_sz, fo, fi, FFTW_BACKWARD, FFTW_MEASURE);

	 samples1 = malloc(sizeof(double) * config.fft_sz);
	 samples2 = malloc(sizeof(double) * config.fft_sz);

    setvbuf(stdout, NULL, _IONBF, 0);

	 if(1) { //for(;;) {

		 for (i = 0; i < config.fft_sz; i++) {
			 fi[i][0] = cos(hz * 2.0 * M_PI * (double)(i + (theta * config.fft_sz / 360)) / config.fft_sz);
			 fi[i][1] = 0.0;
		 }

		 fftw_execute(forward);

		 fft_bandpass_filter(fo, config.filter_bandwidth);

		 fftw_execute(reverse);

		 for (j = 0; j < config.fft_sz; j++) {
			 char buf[256];
			 double a = fo[i][0];
			 double b = fo[i][1];
			 i = (j + config.fft_sz / 2) % config.fft_sz;
			 printf("bin %03i %9s %+6.1f %+6.1f %+6.1f %6.1f\n", i, hzstring(bin_freq(i), 5, buf, sizeof(buf)), a, b, hypot(a, b), atan2(b, a) * 180.0 / M_PI);
		 }
	 }

    fftw_destroy_plan(forward);
    fftw_destroy_plan(reverse);

    fftw_free(fi);
    fftw_free(fo);

    exit(EXIT_SUCCESS);
}
