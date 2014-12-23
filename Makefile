all: commands

CFLAGS=-O2 -Wall -Werror -D__DEBUG__ -g 

# -DDEFAULT_COMMANDS_PATH=\".\"

commands: commands.c

clean:
	rm commands

