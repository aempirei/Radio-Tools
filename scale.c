#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <complex.h>

#include "shared.h"

volatile struct config {
    int verbose;
	 double scalar;
} config;

void usage(const char *arg0) {

    printf("\nusage: %s [options] < input_samples.dat > output_samples.dat\n\n", arg0);
    printf("\t-h           show this help\n");
    printf("\t-v           verbose (more -v's for more verbosity)\n");
    printf("\t-s scalar    scalar multiplication factor (default: 1.0)\n");
    printf("\t-S scalar    scalar division factor (default: 1.0)\n");
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

    complex_sample_t sample;

    config.verbose = 0;
	 config.scalar = 1.0;

    while ((opt = getopt(argc, argv, "vhs:S:")) != -1) {

        switch (opt) {

		  case 's':

		  		config.scalar = strtod(optarg, NULL);
				break;

		  case 'S':

		  		config.scalar = 1.0 / strtod(optarg, NULL);
				break;


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

	 if(config.verbose) {
	 	fprintf(stderr, "scale: f(t) X %f\n", config.scalar);
	 }

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

        sample.r *= config.scalar;
        sample.i *= config.scalar;

        n = fwrite(&sample, sizeof(sample), 1, stdout);

        if (n != 1) {
            perror("fwrite()");
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
