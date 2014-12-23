all: commands

CFLAGS=-O2 -Wall -Werror

commands: commands.c

clean:
	rm commands

