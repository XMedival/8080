CC = gcc
CFLAGS = -Wall -Wextra -O2

asm8080: asm8080.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f asm8080 *.hex

.PHONY: clean
