# Git Style Subcommands Done Right

With commands its trivial to create a git inspired 
program that use specialized subcommand binaries to perform specific tasks.

Just compile the command.c into your custom command name, and it just works. 
Subcommands have to comply with some conventions to it all fits, but all 
convetions are very easy to comply even on bash scripts.

## Advantages of commands over other git style subcommands.

I'm well aware of hemsman (https://github.com/mattmcmanus/node-helmsman) 
and git-style-binaries(https://github.com/jashmenn/git-style-binaries) but both
are everything but unobtrusive by the minimal reason that they need a specific
interpreter (node, ruby). Maybe there are other solutions that use python
or similar, but with commands, its just one C file and conventions.

## HOWTO

Compile, change the executable name to your command name.

### How to create subcommands.

They must accept just one command line argument: --one-line-help. It must return a 
one line help line, to be print when commands is run without arguments. 

The subcommands must be in any PATH directory with name [mycommand]-[subcommand]. 
They are looked for in order.

## Custom config file.

Your commands do not need to be exactly at a $PATH file; there is a config file 
at /etc/[yourcommand], or ~/.config/[yourcommand]. All lines will be read and set as 
environment variables. There is a COMMANDS_PATH envvar, which by default is set to 
$PATH, but can be set to any other place.

The config files can contain envvar variables as in bash: $VAR and ${VAR}.

## Example subcommand

  #!/bin/bash
  
  if [ "$1" == "--one-line-help" ]; then
    echo "List all users"
    exit 1
  fi
  
  getent passwd | awk -F: '{ if ($7 == "/bin/bash") print $1; }'


