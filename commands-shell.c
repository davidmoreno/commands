/**
 * Copyright 2014-2015 David Moreno
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Originally distributed at https://github.com/davidmoreno/commands . 
 * Change at free will.
 */

#define NO_MAIN

#include "commands.c"
#include <sys/wait.h>


/**
 * @short Runs a specific subcommand, NOT replacing current process.
 */
int run_command_no_exec(const char *subcommand, int argc, char **argv){
	pid_t pid;
	if ( (pid=fork()) == 0){
		run_command(subcommand, argc, argv);
	}
	int status=0;
	waitpid(pid,&status,0);
	if (!WIFEXITED(status)){
		fprintf(stderr, "Error executing command %s.\n", subcommand);
	}
	return WEXITSTATUS(status);
}

int main(int argc, char **argv){
	if (argc==2 && strcmp(argv[1],"--one-line-help")==0){
		printf("Executes a restricted shell\n");
		return 0;
	}
#ifdef DEFAULT_COMMANDS_PATH
	setenv(COMMANDS_PATH, DEFAULT_COMMANDS_PATH, 1);
#else
	setenv(COMMANDS_PATH, secure_getenv("PATH"), 1);
#endif
	
	// Must be set at COMMANDS_NAME envvar.
	command_name=getenv("COMMANDS_NAME");
	if (!command_name){
		char *tmp=strdup(argv[0]);
		command_name=strdup(basename(tmp));
		free(tmp);
		// Use first part of my own name
		char *dash=strchr(command_name,'-');
		if (dash)
			*dash=0;
	}
	else
		command_name=strdup(command_name);
	command_name_length=strlen(command_name);
	config_parse();
	
	int running=1;
	char line[2048];
	memset(line, 0, sizeof(line));
	char *margv[256];
	while(running){
		printf("%s> ", command_name);
		fflush(stdout);
		
		memset(line, 0, sizeof(line));
		if ( !fgets(line, sizeof(line), stdin) ){
			running=0;
			break;
		}
		if (!strchr(line,'\0')){
			fprintf(stderr,"Statement too long. Ignoring.");
		}
		else{
			char *I=line, *endI=line+strnlen(line, sizeof(line))-1;
			*(endI)=0;
			int margc=1;
			margv[0]=I;
			for(;I!=endI && *I;++I){
				if (isspace(*I)){
					*I++=0;
					while (isspace(*I)) ++I;
					margv[margc++]=I;
				}
			}
			margv[margc]=NULL;
			if (strcmp(margv[0],"exit")==0)
				running=0;
			else
				run_command_no_exec(margv[0], margc, margv);
		}
	}
	printf("\n");
	subcommand_list_free();
	free(command_name);
	return 0;
}
