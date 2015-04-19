all: commands commands-shell

CFLAGS=-O2 -Wall -Werror -DVERSION='"1.0"'
#-DDEFAULT_COMMANDS_PATH=\".\" -DPREAMBLE='"This is an example"' -DONE_LINE_HELP='"Help"'

#
#
# 

commands-shell: commands-shell.c libcommands.c
	${CC} commands-shell.c libcommands.c -o commands-shell ${CFLAGS}

commands: commands.c libcommands.c

clean:
	rm commands

