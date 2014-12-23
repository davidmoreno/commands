all: commands

CFLAGS=-O2 -Wall -Werror -D__DEBUG__ -g

commands: commands.c

clean:
	rm commands

