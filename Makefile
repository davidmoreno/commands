all: commands commands-shell

CFLAGS=-O2 -Wall -Werror -D__DEBUG__ -g 

# -DPREAMBLE='"This is an example"'
# -DVERSION='"1.0"'
# -DDEFAULT_COMMANDS_PATH=\".\"

commands-shell: commands-shell.c commands.c
	${CC} commands-shell.c -o commands-shell ${CFLAGS}

commands: commands.c

clean:
	rm commands

