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
#include <fcntl.h>
#include <ctype.h>

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

void list_subcommands(){
	foreach_pathlist(getenv(COMMANDS_PATH), list_subcommands_at_dir, NULL);
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
	
	foreach_pathlist(getenv(COMMANDS_PATH), find_command_filter, &userdata);
	
	return userdata.result;
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

int main(int argc, char **argv){
#ifdef DEFAULT_COMMANDS_PATH
	setenv(COMMANDS_PATH, DEFAULT_COMMANDS_PATH, 1);
#else
	setenv(COMMANDS_PATH, secure_getenv("PATH"), 1);
#endif
	
	command_name=basename( argv[0] );
	command_name_length=strlen(command_name);
	parse_config();
	
	if (argc==1){
		printf("%s <subcommand> ...\n\n", command_name);
		printf("Known subcommands are:\n");
		list_subcommands();
		printf("\n");
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
