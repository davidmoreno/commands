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
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char *command_name=NULL;
size_t command_name_length=0;
const char *COMMANDS_PATH="COMMANDS_PATH";

int scandir_startswith_command_name(const struct dirent *d){
// 	printf("Check %s %d %s %ld\n",d->d_name, strncmp(d->d_name, command_name, command_name_length), command_name, command_name_length);
	return 
		(strncmp(d->d_name, command_name, command_name_length)==0) && 
		d->d_name[command_name_length]=='-' &&
		d->d_type&0111;
}

void list_subcommands_at_dir(void *_, const char *dirname){
	struct dirent **namelist;

	int n=scandir(dirname, &namelist, scandir_startswith_command_name, alphasort);
	if (n<0){
		perror("Cant scan dir in path: ");
	}
	char tmp[1024];
	while (n--) {
		const char *subcommand=namelist[n]->d_name;
		snprintf(tmp, sizeof(tmp), "%s/%s --one-line-help", dirname, subcommand);
		printf("  %s - ", subcommand+command_name_length+1);
		fflush(stdout);
		system(tmp);
		
		free(namelist[n]);
	}
	free(namelist);
}

int foreach_commands_path(void (*feach)(void *, const char *path), void *userdata){
	char *paths=strdup(secure_getenv(COMMANDS_PATH));
	char *path=paths;
	while(*path){ // While some path left
		char *next=strchr(path, ':');
		if (next){ // Set to 0, and go to next char.
			*next='\0';
			next++;
		}

		feach(userdata, path);
		
		path=next;
		if (!next)
			break; // Normal exit point. 
	}
	
	free(paths);
	return 0;
}

void list_subcommands(){
	foreach_commands_path(list_subcommands_at_dir, NULL);
}

struct find_command_t{
	const char *subcommand;
	char *result;
};

void find_command_filter(void *_, const char *path){
	struct find_command_t *res=_;
	if (res->result)
		return;
	struct stat st;
	char tmp[1024];
	snprintf(tmp, sizeof(tmp),"%s/%s-%s", path, command_name, res->subcommand);
	printf("%s", tmp);
	if (stat(tmp, &st)==0)
		res->result=strdup(tmp);
}

char *find_command(const char *subcommand){
	struct find_command_t userdata;
	
	userdata.subcommand=subcommand;
	userdata.result=NULL;
	
	foreach_commands_path( find_command_filter, &userdata);
	
	return userdata.result;
}

int main(int argc, char **argv){
	setenv(COMMANDS_PATH, secure_getenv("PATH"), 1);
	
	command_name=basename( argv[0] );
	command_name_length=strlen(command_name);
	
	if (argc==1){
		printf("%s <subcommand>\n", command_name);
		printf("Known subcommands are:\n\n");
		list_subcommands();
	}
	else{
		const char *subcommand=argv[1];
		argc--; argv++;
		char *subcommand_path=find_command(subcommand);
		execv(subcommand_path, argv);
		free(subcommand_path); // Dont say I'm not clean.
		perror("Could not execute subcommand: ");
	}
	
	
	return 0;
}
