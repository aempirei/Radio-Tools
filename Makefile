CC = gcc
CCFLAGS = -Wall -I. -lm -lfftw3 -g -ggdb
CFLAGS = -Wall -I.  -g -ggdb
TARGETS = bandpass_filter

.PHONY: all clean wipe

all: $(TARGETS) $(MODULES) $(SCRIPTS)

bandpass_filter: bandpass_filter.o shared.o
	$(CC) $(CCFLAGS) -o $@ $^

clean:
	rm -f *.o *~

wipe: clean
	rm -f $(TARGETS)
