CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
EXECUTABLE = proj2

all: $(EXECUTABLE)

$(EXECUTABLE): $(EXECUTABLE).c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f $(EXECUTABLE)
