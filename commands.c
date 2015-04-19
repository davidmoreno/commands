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

#include "libcommands.h"

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
