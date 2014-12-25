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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>

char *command_name=NULL;
size_t command_name_length=0;
const char *COMMANDS_PATH="COMMANDS_PATH";

typedef struct subcommand_t{
	char *name;
	char *fullpath;
}subcommand_t;

typedef struct subcommand_list_t{
	subcommand_t *list;
	size_t count;
	size_t size;
}subcommand_list_t;

int foreach_pathlist(const char *search_paths, void (*feach)(void *, const char *path), void *userdata);
subcommand_t *subcommand_list_add(subcommand_list_t *scl, const char *name, const char *fullpath);
void scandir_add_to_subcommandlist(void *l, const char *dirname);

subcommand_list_t *subcommand_list_new(){
	subcommand_list_t *scl=malloc(sizeof(subcommand_list_t));
	scl->count=0;
	scl->size=8;
	scl->list=malloc(sizeof(subcommand_t)*scl->size);
	
	foreach_pathlist(getenv(COMMANDS_PATH), scandir_add_to_subcommandlist, scl);
	return scl;
}

void subcommand_list_free(subcommand_list_t *scl){
	int i;
	for(i=0;i<scl->count;i++){
		free(scl->list[i].name);
		free(scl->list[i].fullpath);
	}
	free(scl->list);
	free(scl);
}

/// Adds a subcommand to the list. Avoid dups based on name.
subcommand_t *subcommand_list_add(subcommand_list_t *scl, const char *name, const char *fullpath){
	subcommand_t *I=scl->list, *endI=scl->list+scl->count;
	for(;I!=endI;++I){
		if (strcmp(I->name, name)==0)
			return I;
	}
	if (scl->size < scl->count+1){
		scl->size+=8;
		scl->list=realloc(scl->list, sizeof(subcommand_t)*scl->size);
	}
	I=scl->list+scl->count;
	I->name=strdup(name);
	I->fullpath=strdup(fullpath);
	scl->count++;
	
	return I;
}

int scandir_startswith_command_name(const struct dirent *d){
// 	printf("Check %s %d %s %ld\n",d->d_name, strncmp(d->d_name, command_name, command_name_length), command_name, command_name_length);
	if (d->d_name[strlen(d->d_name)-1]=='~')
		return 0;
	return 
		(strncmp(d->d_name, command_name, command_name_length)==0) && 
		d->d_name[command_name_length]=='-' &&
		d->d_type&0111;
}

void scandir_add_to_subcommandlist(void *l, const char *dirname){
	subcommand_list_t *scl=l;
	struct dirent **namelist;

	int n=scandir(dirname, &namelist, scandir_startswith_command_name, alphasort);
	if (n<0){
		perror("Cant scan dir in path: ");
	}
	char tmp[1024];
	struct stat st;
	while (n--) {
		snprintf(tmp, sizeof(tmp), "%s/%s", dirname, namelist[n]->d_name);
		if (stat(tmp, &st) >= 0){
			if (st.st_mode & 0111) // Excutable for anybody.
				subcommand_list_add(scl, namelist[n]->d_name+command_name_length+1, tmp);
		}
		free(namelist[n]);
	}
	free(namelist);
}

int foreach_pathlist(const char *search_paths, void (*feach)(void *, const char *path), void *userdata){
	char *paths=strdup(search_paths);;
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

void list_subcommands_list(){
	subcommand_list_t *sbl=subcommand_list_new();
	subcommand_t *I=sbl->list;
	subcommand_t *endI=sbl->list+sbl->count;
	for(;I!=endI;++I){
		printf("%s ", I->name);
	}
	subcommand_list_free(sbl);
}

void list_subcommands_one_line_help(){
	subcommand_list_t *sbl=subcommand_list_new();
	char tmp[1024];
	subcommand_t *I=sbl->list;
	subcommand_t *endI=sbl->list+sbl->count;
	for(;I!=endI;++I){
		snprintf(tmp, sizeof(tmp), "%s --one-line-help", I->fullpath);
		printf("  %8s - ", I->name);
		fflush(stdout);
		system(tmp);
	}
	
	subcommand_list_free(sbl);
}


const char *string_trim(char *str){
	// Trim start
	while (*str && isspace(*str)){
		str++;
	}
	
	// Trim end
	char *end=str+strlen(str);
	while ( end>str && isspace(*end) ){
		end--;
	}
	*end=0;
	
	return str;
}

char *string_envformat(const char *str){
	size_t ressize=strlen(str)+512;
	char *begin_res=malloc(ressize); // An estimation.. might be way wrong.
	char *res=begin_res;
	char *resend=res+ressize-1;
	char tvarname[256];
	char *varname=NULL;
	
	while(*str){
		if (res>=resend){
			goto end;
		}
		if (*str=='$'){
			str++;
			if (*str=='$'){
				*res++='$';
			}
			else{
				varname=tvarname;
				*varname=*str;
			}
		}
		if (varname){
			if (isalnum(*str))
				*varname++=*str;
			else{
				*varname=0;
				char *env=getenv(tvarname);
				strncpy(res, env, resend-res);
				size_t lenv=strlen(env);
				if (res+lenv>resend)
					goto end;
				res+=lenv;
				varname=NULL;
				*res++=*str;
			}
		}
		else
			*res++=*str;
		str++;
	}
	if (varname){
		*varname=0;
		char *env=getenv(tvarname);
		strncpy(res, env, resend-res);
		res+=strlen(env);
	}
end:
	*res=0;
	return begin_res;
}

int parse_configline(char *line){
	char *comments=strchr(line, '#');
	if (comments)
		*comments=0;
	if (strlen(string_trim(line))==0)
		return 0;
	char *eq=strchr(line,'=');
	if (!eq)
		return 1;
	*eq=0;

	const char *key=string_trim(line);
	const char *val=string_trim(eq+1);
	char *final_val=string_envformat(val);	
	
// 	printf("<%s>=<%s>\n", key, final_val);
	setenv(key, final_val, 1);
	free(final_val);
	return 0;
}

int parse_configfile(const char *filename){
	int fd=open(filename, O_RDONLY);
	if (fd<0)
		return fd;
	char tmp[1025];
	char line[1024];
	int nr;
	int lineno=0;
	while ( (nr=read(fd, tmp, sizeof(tmp)-1)) > 0){
		tmp[nr]=0;
// 		printf("Read block: <%s>\n", tmp);
		if (nr<sizeof(tmp))
			tmp[nr]=0; // EOF
		char *where=tmp;
		char *where_next=NULL;
		*line=0;
		while ( (where_next=strchr(where, '\n')) ){
			lineno++;
			*where_next='\0';
			strncat(line, where, sizeof(line)-1);
			
			// Process the line
			if (parse_configline(line)!=0)
				goto error;
			*line=0;
			
			where=where_next+1;
		}
		strncpy(line, where, tmp + sizeof(tmp) - where);
		if (nr<sizeof(tmp)){
			// Process the line
			if (parse_configline(line)!=0)
				goto error;
		}
	}
	
	close(fd);
	return 0;
error:
	fprintf(stderr, "Error parsing config file %s:%d", filename, lineno);
	close(fd);
	return 1;
}

void parse_config(){
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "/etc/%s", command_name);
	parse_configfile(tmp);
	snprintf(tmp, sizeof(tmp), "%s/.config/%s", getenv("HOME"), command_name);
	parse_configfile(tmp);

#ifdef __DEBUG__
	snprintf(tmp, sizeof(tmp), "./%src", command_name);
	parse_configfile(tmp);
#endif

}

char *find_command(const char *name){
	char *ret=NULL;
	subcommand_list_t *scl=subcommand_list_new();
	
	subcommand_t *I=scl->list, *endI=scl->list+scl->count;
	for(;I!=endI;++I){
		if (strcmp(I->name, name)==0){
			ret=strdup(I->fullpath);
			break;
		}
	}
	
	subcommand_list_free(scl);
	return ret;
}

/**** PUBLIC API ****/

/**
 * @short Shows the list of subcommands with the one line help of each.
 */
void help(){
	printf("%s <subcommand> ...\n\n", command_name);
#ifdef PREAMBLE
	printf("%s\n\n", PREAMBLE);
#endif
	printf("Known subcommands are:\n");
	list_subcommands_one_line_help();
	printf("\n");
}

/**
 * @short Prints all known subcommands.
 */
void list(){
	list_subcommands_list();
	printf("\n");
}

/**
 * @short Runs a specific subcommand, replacing current process (exec).
 */
int run_command(const char *subcommand, int argc, char **argv){
	char *subcommand_path=find_command(subcommand);
	if (!subcommand_path){
		fprintf(stderr,"Command %s not found. Check available running %s without arguments.\n", subcommand, command_name);
		return 1;
	}
	execv(subcommand_path, argv);
	free(subcommand_path); // Dont say I'm not clean.
	perror("Could not execute subcommand: ");
	return 1;
}

/**
 * @short Performs the default main. 
 * 
 * Its is usefull as another function, so it can be easily customized to allow more command line
 * options, and call this for default ones.
 */
int commands_main(int argc, char **argv){
#ifdef DEFAULT_COMMANDS_PATH
	setenv(COMMANDS_PATH, DEFAULT_COMMANDS_PATH, 1);
#else
	setenv(COMMANDS_PATH, secure_getenv("PATH"), 1);
#endif
	
	command_name=basename( argv[0] );
	command_name_length=strlen(command_name);
	
	setenv("COMMANDS_NAME", command_name, 1);
	
	parse_config();
	
	if (argc==1){
		help();
		return 0;
	}
	else{
#ifdef ONE_LINE_HELP
		if (strcmp(argv[1], "--one-line-help")==0){
			printf("%s\n", ONE_LINE_HELP);
			return 0;
		}
#endif
#ifdef VERSION
		if (strcmp(argv[1], "--version")==0 || strcmp(argv[1], "-v")==0){
			printf("%s %s\n", command_name, VERSION);
			return 0;
		}
#endif
		if (strcmp(argv[1], "--list")==0){
			list();
			return 0;
		}
		if (strcmp(argv[1], "--help")==0){
			help();
			return 0;
		}
		return run_command(argv[1], argc-1, argv+1);
	}
	
	
	return 0;
}

#ifndef NO_MAIN
/**
 * @short main is kept as simple as possile, to allow easy customization.
 * 
 * If you are customizing commands main, I recomend you create another file, with your command name, with some content as
 * 
 *   #define NO_MAIN
 *   #include "commands.c"
 * 
 *   int main(int argc, char **argv){
 *     ...
 *     commands_main(argc, argv);
 *   }
 * 
 * Or calling the one_line_help(), list() or run_command(commandname, argc-1, argv+1) at your will.
 */
int main(int argc, char **argv){
	return commands_main(argc, argv);
}
#endif
