CC = clang
CFLAGS = -Wall -pthread
LDFLAGS = -lrt -lm -lasound -ljack

PROGRAM = detector
FILES = $(PROGRAM).c libportaudio.a

all: $(PROGRAM)

$(PROGRAM): $(FILES)
	$(CC) $(CFLAGS) $(FILES) $(LDFLAGS) -o $@

run: $(PROGRAM)
	./$(PROGRAM)

compile_commands.json: $(FILES)
	make clean
	bear -- make

.PHONY: clean
clean:
	rm -f $(PROGRAM)
