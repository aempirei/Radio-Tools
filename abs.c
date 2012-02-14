#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <complex.h>

#include "shared.h"

struct config {
    int verbose;
} config;

void usage(const char *arg0) {

    printf("\nusage: %s [options] < input_samples.dat > output_samples.dat\n\n", arg0);
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

    complex_sample_t sample;

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

        sample.r = fabs(sample.r);
        sample.i = fabs(sample.i);

        n = fwrite(&sample, sizeof(sample), 1, stdout);

        if (n != 1) {
            perror("fwrite()");
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
