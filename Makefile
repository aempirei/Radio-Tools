CC = gcc
CCFLAGS = -Wall -I. -lm -lfftw3 -g -ggdb
CFLAGS = -Wall -I.  -g -ggdb
TARGETS = bandpass_filter differentiate integrate

.PHONY: all clean wipe

all: $(TARGETS) $(MODULES) $(SCRIPTS)

integrate: integrate.o shared.o
	$(CC) $(CCFLAGS) -o $@ $^

differentiate: differentiate.o shared.o
	$(CC) $(CCFLAGS) -o $@ $^

bandpass_filter: bandpass_filter.o shared.o
	$(CC) $(CCFLAGS) -o $@ $^

clean:
	rm -f *.o *~

wipe: clean
	rm -f $(TARGETS)
