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
    int verbose;
} config;

void
usage(const char *arg0)
{
    printf("\nusage: %s [options] < input_samples.dat > output_samples.dat\n\n", arg0);

    printf("\t-h           show this help\n");
    printf("\t-v           verbose (more -v's for more verbosity)\n");

    putchar('\n');
}

void
exit_handler()
{
    if (config.verbose) {
        fputs(CURSOR_ON, stderr);
        fputs("\033[100B", stderr);
        fputs("\ngoodbye...\n", stderr);
    }
}

void
sig_handler(int signo)
{
    if (signo == SIGINT) {
        exit(EXIT_FAILURE);
    }
}

int
main(int argc, char **argv)
{
    int opt;

    double dx;

	 double sigmadx;

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

        n = read2(0, &dx, sizeof(dx));

        if (n == -1) {
            perror("read2()");
            break;
        }

        if (n != sizeof(double)) {
            if (n != 0)
                fputs("final sample not on double floating point boundary\n", stderr);
            break;
        }

		  sigmadx += dx;

        n = write2(1, &sigmadx, sizeof(sigmadx));
        if (n != sizeof(sigmadx)) {
            perror("write2()");
            break;
        }
    }

    exit(EXIT_SUCCESS);
}
