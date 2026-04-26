CFLAGS=-O3 -march=native -fopenmp -Wall -Wextra -Wpedantic -std=c99 \
	-Wundef -Wshadow -Wcast-align -Wpointer-arith -Wmissing-prototypes \
	-fstrict-aliasing -fno-common -pipe -Wno-unused-function -I./BaseLib

SRCDIR=CoreLib
SCRIPTDIR=Scripts
TESTDIR=Testing

SOURCES=$(SRCDIR)/randombytes.c $(SRCDIR)/poly.c $(SRCDIR)/common.c $(SRCDIR)/zq.c $(SRCDIR)/poly_api.c
HEADERS=BaseLib/randombytes.h BaseLib/poly.h BaseLib/common.h BaseLib/zq.h

TESTS=$(TESTDIR)/test_01-schoolbook $(TESTDIR)/test_02-karatsuba $(TESTDIR)/test_03-toom \
      $(TESTDIR)/test_04-ntt $(TESTDIR)/test_05-winograd $(TESTDIR)/test_06-monomial $(TESTDIR)/test_00-benchmark

.PHONY: all clean format
all: $(TESTS)

format:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i
	@for f in Docs/DEVLOG.md README.md GEMINI.md Docs/TRACKLOG.md input_config Makefile; do \
		if [ -f "$f" ]; then \
			fold -s -w 105 "$f" > "$f.tmp" && mv "$f.tmp" "$f"; \
		fi; \
	done


$(TESTDIR)/test_01-schoolbook: $(SCRIPTDIR)/01-schoolbook.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_02-karatsuba: $(SCRIPTDIR)/02-karatsuba.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_03-toom: $(SCRIPTDIR)/03-toom.c $(SCRIPTDIR)/02-karatsuba.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -c $(SCRIPTDIR)/02-karatsuba.c -o $(TESTDIR)/02-karatsuba.o -DBENCHMARK -D_16BIT_COEFFICIENTS
	$(CC) $(CFLAGS) -o $@ $(SCRIPTDIR)/03-toom.c $(TESTDIR)/02-karatsuba.o $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_04-ntt: $(SCRIPTDIR)/04-ntt.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_05-winograd: $(SCRIPTDIR)/05-winograd.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_06-monomial: $(SCRIPTDIR)/06-monomial.c $(SCRIPTDIR)/02-karatsuba.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -c $(SCRIPTDIR)/02-karatsuba.c -o $(TESTDIR)/02-karatsuba.o -DBENCHMARK -D_16BIT_COEFFICIENTS
	$(CC) $(CFLAGS) -o $@ $(SCRIPTDIR)/06-monomial.c $(TESTDIR)/02-karatsuba.o $(SOURCES) -D_16BIT_COEFFICIENTS -lm

$(TESTDIR)/test_00-benchmark: $(SCRIPTDIR)/00-benchmark.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SCRIPTDIR)/00-benchmark.c $(SCRIPTDIR)/02-karatsuba.c \
	$(SCRIPTDIR)/03-toom.c $(SCRIPTDIR)/04-ntt.c $(SCRIPTDIR)/05-winograd.c $(SCRIPTDIR)/06-monomial.c $(SOURCES) \
	-D_16BIT_COEFFICIENTS -DBENCHMARK -lm
clean:
	$(RM) $(TESTS)
