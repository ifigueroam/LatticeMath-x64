CFLAGS=-O3 -march=native -fopenmp -Wall -Wextra -Wpedantic -std=c99 \
	-Wundef -Wshadow -Wcast-align -Wpointer-arith -Wmissing-prototypes \
	-fstrict-aliasing -fno-common -pipe -Wno-unused-function -I./BaseLib

SRCDIR=CoreLib
SCRIPTDIR=Scripts
TESTDIR=Testing

SOURCES=$(SRCDIR)/randombytes.c $(SRCDIR)/poly.c $(SRCDIR)/common.c $(SRCDIR)/zq.c $(SRCDIR)/poly_api.c
HEADERS=BaseLib/randombytes.h BaseLib/poly.h BaseLib/common.h BaseLib/zq.h

TESTS=$(TESTDIR)/test_01schoolbook $(TESTDIR)/test_02karatsuba $(TESTDIR)/test_03toom $(TESTDIR)/test_04ntt $(TESTDIR)/test_05benchmark

.PHONY: all clean format
all: $(TESTS)

format:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i
	@for f in DEVLOG.md README.md GEMINI.md TRACKLOG.md input_config Makefile; do \
		if [ -f "$f" ]; then \
			fold -s -w 105 "$f" > "$f.tmp" && mv "$f.tmp" "$f"; \
		fi; \
	done


$(TESTDIR)/test_01schoolbook: $(SCRIPTDIR)/01schoolbook.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_02karatsuba: $(SCRIPTDIR)/02karatsuba.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_03toom: $(SCRIPTDIR)/03toom.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_04ntt: $(SCRIPTDIR)/04ntt.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_05benchmark: $(SCRIPTDIR)/05benchmark.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SCRIPTDIR)/05benchmark.c $(SCRIPTDIR)/02karatsuba.c $(SCRIPTDIR)/03toom.c $(SCRIPTDIR)/04ntt.c $(SOURCES) -D_16BIT_COEFFICIENTS -DBENCHMARK -lm

clean:
	$(RM) $(TESTS)
