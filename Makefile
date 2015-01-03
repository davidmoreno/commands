all: commands commands-shell

CFLAGS=-O2 -Wall -Werror -D__DEBUG__ -g  -DVERSION='"1.0"'
#-DDEFAULT_COMMANDS_PATH=\".\" -DPREAMBLE='"This is an example"' -DONE_LINE_HELP='"Help"'

#
#
# 

commands-shell: commands-shell.c commands.c
	${CC} commands-shell.c -o commands-shell ${CFLAGS}

commands: commands.c

clean:
	rm commands

