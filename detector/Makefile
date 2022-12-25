CC = clang
CFLAGS = -Wall -pthread
LDFLAGS = -lrt -lm -lasound -ljack

PROGRAM = detector
SRC_FILES = $(PROGRAM).c libportaudio.a
LOG_FILES = log
COMPILATION_DB = compile_commands.json
CACHE = .cache

all: $(PROGRAM)

$(PROGRAM): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) $(LDFLAGS) -o $@

run: $(PROGRAM)
	./$(PROGRAM)

$(COMPILATION_DB): $(SRC_FILES)
	make clean
	bear -- make

.PHONY: clean
clean:
	rm -rf $(PROGRAM) $(LOG_FILES) $(COMPILATION_DB) $(CHACHE)
