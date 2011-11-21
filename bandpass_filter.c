#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <getopt.h>
#include <assert.h>
#include <fftw3.h>
#include <math.h>
#include <time.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

const int dfl_bins = 32;

struct config {
	int bins;
	long long center_freq;
	long long bandwidth;
	int verbose;
} config;

/*
 * a happy more atomic read function so i dont have to deal with loop
 * code overhead everytime i wanna do a read of a specific length
 */

ssize_t
read2(int fd, void *buf, size_t count)
{
    size_t nleft, nread;
    ssize_t n;

    nleft = count;
    nread = 0;

    do {
        n = read(fd, (char *)buf + nread, nleft);
        if (n == 0)
            return (nread);
        else if (n == -1)
            return (-1);

        nread += n;
        nleft -= n;
    } while (nleft);

    return (nread);

}

ssize_t
write2(int fd, void *buf, size_t count)
{
    size_t nleft, nwrite;
    ssize_t n;

    nleft = count;
    nwrite = 0;

    do {
        n = write(fd, (char *)buf + nwrite, nleft);
        if (n == 0)
            return (nwrite);
        else if (n == -1)
            return (-1);

        nwrite += n;
        nleft -= n;
    } while (nleft);

    return (nwrite);

}

fftw_plan plan;
fftw_complex *fi, *fo;

void
fft_abs(fftw_complex * fft, int N)
{
    int i;

    for (i = 0; i < N; i++) {
        fft[i][0] = hypot(fft[i][0], fft[i][1]);
        fft[i][1] = 0.0;
    }
}

double
getfreq(unsigned int binno)
{

	int n = (binno + config.bins / 2) % config.bins;

	double hi_freq = (double)config.center_freq + config.bandwidth / 2.0;
	double lo_freq = (double)config.center_freq - config.bandwidth / 2.0;

	double c = (double)n / (config.bins - 1);

	double a = (1.0 - c) * lo_freq;
	double b = c * hi_freq;

	return a + b;
}

double
fft_real_weight(fftw_complex * fft, int N)
{

    double sum = 0;
    int i;

    /*
     * i skip the first bin because its the relative gain and i dont want that
     */

    for (i = 1; i * 2 < N; i++)
        sum += fft[i][0];

    /*
     * we only want half the weight due to the symmetry of the FFT
     * im not sure of we want to use i < N / 2 in the for loop or what
     */

    return sum;
}

double
fft_max(fftw_complex * fft, int N)
{
    int i;

    double max = 0.0;

    for (i = 1; i < N; i++)
        if (fft[i][0] > max)
            max = fft[i][0];

    return max;
}

void
fft_normalize(fftw_complex * fft, int N)
{

    int i;

    double max = fft_max(fft, N);

    fft[0][0] = 0.0;
    fft[0][1] = 0.0;

    if (max != 0.0) {
        for (i = 0; i < N; i++)
            fft[i][0] /= max;
        for (; i < N; i++)
            fft[i][0] = 0.0;
    }
}

void
usage(const char *arg0)
{
    printf("\nusage: %s [options] center bandwidth bins < input_samples.dat > output_samples.dat\n\n", arg0);

    printf("\tcenter       center frequency in hz or khz or mhz or ghz\n");
    printf("\tbandwidth    bandwidth in hz or khz or mhz or ghz\n");
    printf("\tbins         bins (must be a power of 2)\n");
	 printf("\t-h           show this help\n");
	 printf("\t-v           verbose\n");

	 putchar('\n');
}

char *hzstring(double v, int res, char *buf, size_t buf_sz) {

	if(v < 1000) {
		snprintf(buf, buf_sz, "%.0fhz", v);
	} else if(v < 1000000) {
		snprintf(buf, buf_sz, "%.*gkhz", res, v / 1000);
	} else if(v < 1000000000) {
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

int
main(int argc, char **argv)
{
    int i;
	 int opt;

    double hz = 3;
    double theta = 0;

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

    if (argc - optind < 3) {
        usage(*argv);
        exit(EXIT_FAILURE);
    }

	 config.center_freq = strtoull(argv[optind + 0], NULL, 0);
	 config.bandwidth = strtoull(argv[optind + 1], NULL, 0);
	 config.bins = strtoul(argv[optind + 2], NULL, 0);

	 if(config.verbose) {
	 	char buf[256];
	 	fprintf(stderr, "bins = %d\n", config.bins);
		fprintf(stderr, "bandwidth = %s\n", hzstring(config.bandwidth, 3, buf, sizeof(buf)));
		fprintf(stderr, "center_freq = %s\n", hzstring(config.center_freq, 3, buf, sizeof(buf)));
	 }

    fi = fftw_malloc(sizeof(fftw_complex) * config.bins);
    fo = fftw_malloc(sizeof(fftw_complex) * config.bins);

    plan = fftw_plan_dft_1d(config.bins, fi, fo, FFTW_FORWARD, FFTW_MEASURE);

    setvbuf(stdout, NULL, _IONBF, 0);


    for (i = 0; i < config.bins; i++) {
        fi[i][0] = cos(hz * 2.0 * M_PI * (double)(i + (theta * config.bins / 360)) / config.bins);
        fi[i][1] = 0.0;
    }

    fftw_execute(plan);

    for (i = 0; i < config.bins; i++) {
	 	char buf[256];
        double a = fo[i][0];
        double b = fo[i][1];
        printf("bin %02i %s %+5.1f %+5.1f %+5.1f %5.1f\n", i, hzstring(getfreq(i), 5, buf, sizeof(buf)), a, b, hypot(a, b), atan2(b, a) * 180.0 / M_PI);
    }

    fftw_destroy_plan(plan);

    fftw_free(fi);
    fftw_free(fo);

    exit(EXIT_SUCCESS);
}
