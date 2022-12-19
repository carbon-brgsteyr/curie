CC = clang
CFLAGS = -Wall -pthread
LDFLAGS = -lrt -lm -lasound -ljack

PROGRAM = counter
FILES = $(PROGRAM).c libportaudio.a

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).c
	$(CC) $(CFLAGS) $(FILES) $(LDFLAGS) -o $@

run: $(PROGRAM)
	./$(PROGRAM)

.PHONY: clean
clean:
	rm -f $(PROGRAM)
