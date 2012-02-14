#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <complex.h>

#include "shared.h"

volatile struct config {
    int verbose;
} config;

void usage(const char *arg0) {

    printf("\nusage: %s [options] < input_samples.dat\n\n", arg0);
    printf("\t-h           show this help\n");
    printf("\t-v           verbose (more -v's for more verbosity)\n");
    putchar('\n');

}

void exit_handler() {
    if (config.verbose) {
        fputs("\ngoodbye...\n", stderr);
    }
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {

    int opt;
	 unsigned long k = 0;

    complex_sample_t sample;

	 double peakr=0.0;
	 double peaki=0.0;
	 double peak=0.0;
	 double rmsr=0.0;
	 double rmsi=0.0;

    config.verbose = 0;

    while ((opt = getopt(argc, argv, "vh")) != -1) {

        switch (opt) {

        case 'v':

            config.verbose++;
            break;

        case 'h':
        default:

            usage(*argv);
            exit(EXIT_FAILURE);
        }
    }

    if (argc - optind < 0) {
        usage(*argv);
        exit(EXIT_FAILURE);
    }

    atexit(exit_handler);

    signal(SIGINT, sig_handler);

	 for (;;) {

		 int n;

		 n = fread(&sample, sizeof(sample), 1, stdin);

		 if (n != 1) {
			 if (feof(stdin)) {
				 break;
			 } else {
				 perror("fread()");
				 exit(EXIT_FAILURE);
			 }
		 }

		 k++;

		 if(fabs(sample.r) > peakr) peakr = sample.r;
		 if(fabs(sample.i) > peaki) peaki = sample.i;

		 rmsr += sample.r * sample.r;
		 rmsi += sample.i * sample.i;
	 }

	 if(fabs(peakr) > fabs(peaki)) {
	 	peak = peakr;
	 } else {
	 	peak = peaki;
	 }

	 printf("peakr %f peaki %f peak %f rmsr %f rmsi %f rms %f\n", peakr, peaki, peak, sqrt(rmsr / k), sqrt(rmsi / k), sqrt((rmsi + rmsr) / (2.0 * k)));

    exit(EXIT_SUCCESS);
}
