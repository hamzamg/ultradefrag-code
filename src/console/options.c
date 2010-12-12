/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
* UltraDefrag console interface - command line parsing code.
*/

#include "udefrag.h"
#include "../include/getopt.h"

/* forward declarations */
void search_for_paths(void);

int __cdecl printf_stub(const char *format,...)
{
	return 0;
}

void show_help(void)
{
	printf(
		"===============================================================================\n"
		VERSIONINTITLE " - Powerful disk defragmentation tool for Windows NT\n"
		"Copyright (c) Dmitri Arkhangelski, 2007-2010.\n"
		"\n"
		"===============================================================================\n"
		"This program is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU General Public License\n"
		"as published by the Free Software Foundation; either version 2\n"
		"of the License, or (at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program; if not, write to the Free Software\n"
		"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n"
		"===============================================================================\n"
		"\n"
		"Usage: udefrag [command] [options] [volumeletter:] [path(s)]\n"
		"\n"
		"  The default action is to display this help message.\n"
		"\n"
		"Commands:\n"
		"  -a,  --analyze                      Analyze volume\n"
		"       --defragment                   Defragment volume\n"
		"  -o,  --optimize                     Optimize volume space\n"
		"  -l,  --list-available-volumes       List volumes available\n"
		"                                      for defragmentation,\n"
		"                                      except removable media\n"
		"  -la, --list-available-volumes=all   List all available volumes\n"
		"  -h,  --help                         Show this help screen\n"
		"  -?                                  Show this help screen\n"
		"\n"
		"  If command is not specified it will defragment volume.\n"
		"\n"
		"Options:\n"
		"  -b,  --use-system-color-scheme      Use system (usually black/white)\n"
		"                                      color scheme instead of the green color.\n"
		"  -p,  --suppress-progress-indicator  Hide progress indicator.\n"
		"  -v,  --show-volume-information      Show volume information after a job.\n"
		"  -m,  --show-cluster-map             Show map representing clusters\n"
		"                                      on the volume.\n"
		"       --map-border-color=color       Set cluster map border color.\n"
		"                                      Available color values: black, white,\n"
		"                                      red, green, blue, yellow, magenta, cyan,\n"
		"                                      darkred, darkgreen, darkblue, darkyellow,\n"
		"                                      darkmagenta, darkcyan, gray.\n"
		"                                      Yellow color is used by default.\n"
		"       --map-symbol=x                 Set a character used for the map drawing.\n"
		"                                      There are two accepted formats:\n"
		"                                      a character may be typed directly,\n"
		"                                      or its hexadecimal number may be used.\n"
		"                                      For example, --map-symbol=0x1 forces\n"
		"                                      UltraDefrag to use a smile character\n"
		"                                      for the map drawing.\n"
		"                                      Valid numbers are in range: 0x1 - 0xFF\n"
		"                                      \'%%\' symbol is used by default.\n"
		"       --map-rows=n                   Number of rows in cluster map.\n"
		"                                      Default value is 10.\n"
		"       --map-symbols-per-line=n       Number of map symbols\n"
		"                                      containing in each row of the map.\n"
		"                                      Default value is 68.\n"
		"       --use-entire-window            Expand cluster map to use entire\n"
		"                                      console window.\n" 
		"       --wait                         Wait until already running instance\n"
		"                                      of udefrag.exe tool completes before\n"
		"                                      starting the job (useful for\n"
		"                                      the scheduled defragmentation).\n"
		"       --shellex                      This option forces to list objects\n"
		"                                      to be processed and display a prompt\n"
		"                                      to hit any key after a job completion.\n"
		"                                      It is intended for use in Explorer\'s\n"
		"                                      context menu handler.\n"
		"\n"
		"Volume letter:\n"
		"  It is possible to specify multiple volume letters, like this:\n\n"
		"  udefrag c: d: x:\n\n"
		"  Also the following keys can be used instead of the volume letter:\n\n"
		"  --all                               Process all available volumes.\n"
		"  --all-fixed                         Process all volumes except removable.\n"
		"\n"
		"Path:\n"
		"  It is possible to specify multiple paths, like this:\n\n"
		"  udefrag \"C:\\Documents and Settings\" C:\\WINDOWS\\WindowsUpdate.log\n\n"
		"  Paths including spaces must be enclosed in double quotes (\").\n"
		"  Relative and absolute paths are supported.\n"
		"  Short paths (like C:\\PROGRA~1\\SOMEFI~1.TXT) aren\'t accepted on NT4.\n"
		"\n"
		"Accepted environment variables:\n"
		"\n"
		"  UD_IN_FILTER                        List of files to be included\n"
		"                                      in defragmentation process. Patterns\n"
		"                                      must be separated by semicolons.\n"
		"\n"
		"  UD_EX_FILTER                        List of files to be excluded from\n"
		"                                      defragmentation process. Patterns\n"
		"                                      must be separated by semicolons.\n"
		"\n"
		"  UD_SIZELIMIT                        Exclude all files larger than specified.\n"
		"                                      The following size suffixes are accepted:\n"
		"                                      Kb, Mb, Gb, Tb, Pb, Eb.\n"
		"\n"
		"  UD_FRAGMENTS_THRESHOLD              Exclude all files which have less number\n"
		"                                      of fragments than specified.\n"
		"\n"
		"  UD_TIME_LIMIT                       When the specified time interval elapses\n"
		"                                      the job will be stopped automatically.\n"
		"                                      The following time format is accepted:\n"
		"                                      Ay Bd Ch Dm Es. Here A,B,C,D,E represent\n"
		"                                      any integer numbers, y,d,h,m,s - suffixes\n"
		"                                      used for years, days, hours, minutes\n"
		"                                      and seconds.\n"
		"\n"
		"  UD_REFRESH_INTERVAL                 Progress refresh interval,\n"
		"                                      in milliseconds.\n"
		"                                      The default value is 100.\n"
		"\n"
		"  UD_DISABLE_REPORTS                  If this environment variable is set\n"
		"                                      to 1 (one), no file fragmentation\n"
		"                                      reports will be generated.\n"
		"\n"
		"  UD_DBGPRINT_LEVEL                   Control amount of the debugging output.\n"
		"                                      NORMAL is used by default, DETAILED\n"
		"                                      may be used to collect information for\n"
		"                                      the bug report, PARANOID turns on\n"
		"                                      a really huge amount of debugging\n"
		"                                      information.\n"
		"\n"
		"  UD_LOG_FILE_PATH                    If this variable is set, it should\n"
		"                                      contain the path (including filename)\n"
		"                                      to the log file to save debugging output\n"
		"                                      into. If the variable is not set, no logging\n"
		"                                      to the file will be performed.\n"
		"\n"
		"  UD_DRY_RUN                          If this environment variable is set\n"
		"                                      to 1 (one) the files are not physically\n"
		"                                      moved.\n"
		"                                      This allows testing the algorithm without\n"
		"                                      changing the content of the volume.\n"
		);
}

static struct option long_options_[] = {
	/*
	* Disk defragmenting options.
	*/
	{ "analyze",                     no_argument,       0, 'a' },
	{ "defragment",                  no_argument,       0,  0  },
	{ "optimize",                    no_argument,       0, 'o' },
	{ "all",                         no_argument,       0,  0  },
	{ "all-fixed",                   no_argument,       0,  0  },
	
	/*
	* Volume listing options.
	*/
	{ "list-available-volumes",      optional_argument, 0, 'l' },
	
	/*
	* Progress indicators options.
	*/
	{ "suppress-progress-indicator", no_argument,       0, 'p' },
	{ "show-volume-information",     no_argument,       0, 'v' },
	{ "show-cluster-map",            no_argument,       0, 'm' },
	
	/*
	* Colors and decoration.
	*/
	{ "use-system-color-scheme",     no_argument,       0, 'b' },
	{ "map-border-color",            required_argument, 0,  0  },
	{ "map-symbol",                  required_argument, 0,  0  },
	{ "map-rows",                    required_argument, 0,  0  },
	{ "map-symbols-per-line",        required_argument, 0,  0  },
	{ "use-entire-window",           no_argument,       0,  0  },
	
	/*
	* Help.
	*/
	{ "help",                        no_argument,       0, 'h' },
	
	/*
	* Screensaver options.
	*/
	{ "screensaver",                 no_argument,       0,  0  },
	
	/*
	* Miscellaneous options.
	*/
	{ "wait",                        no_argument,       0,  0  },
	{ "shellex",                     no_argument,       0,  0  },
	
	{ 0,                             0,                 0,  0  }
};

char short_options_[] = "aol::pvmbh?iesd";

/* new code based on GNU getopt() function */
void parse_cmdline(int argc, char **argv)
{
	int c;
	int option_index = 0;
	const char *long_option_name;
	int dark_color_flag = 0;
	int map_symbol_number = 0;
	int rows = 0, symbols_per_line = 0;
	int letter_index;
	char ch;
	int length;
	
	memset(&letters,0,sizeof(letters));
	letter_index = 0;
	
	if(argc < 2) h_flag = 1;
	while(1){
		option_index = 0;
		c = getopt_long(argc,argv,short_options_,
			long_options_,&option_index);
		if(c == -1) break;
		switch(c){
		case 0:
			//printf("option %s", long_options_[option_index].name);
			//if(optarg) printf(" with arg %s", optarg);
			//printf("\n");
			long_option_name = long_options_[option_index].name;
			if(!strcmp(long_option_name,"defragment")) { /* do nothing here */ }
			else if(!strcmp(long_option_name,"map-border-color")){
				if(!optarg) break;
				if(!strcmp(optarg,"black")){
					map_border_color = 0x0; break;
				}
				if(!strcmp(optarg,"white")){
					map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
						FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
				}
				if(!strcmp(optarg,"gray")){
					map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
						FOREGROUND_BLUE; break;
				}
				if(strstr(optarg,"dark")) dark_color_flag = 1;

				if(strstr(optarg,"red")){
					map_border_color = FOREGROUND_RED;
				}
				else if(strstr(optarg,"green")){
					map_border_color = FOREGROUND_GREEN;
				}
				else if(strstr(optarg,"blue")){
					map_border_color = FOREGROUND_BLUE;
				}
				else if(strstr(optarg,"yellow")){
					map_border_color = FOREGROUND_RED | FOREGROUND_GREEN;
				}
				else if(strstr(optarg,"magenta")){
					map_border_color = FOREGROUND_RED | FOREGROUND_BLUE;
				}
				else if(strstr(optarg,"cyan")){
					map_border_color = FOREGROUND_GREEN | FOREGROUND_BLUE;
				}
				
				if(!dark_color_flag) map_border_color |= FOREGROUND_INTENSITY;
			}
			else if(!strcmp(long_option_name,"map-symbol")){
				if(!optarg) break;
				if(strstr(optarg,"0x") == optarg){
					/* decode hexadecimal number */
					(void)sscanf(optarg,"%x",&map_symbol_number);
					if(map_symbol_number > 0 && map_symbol_number < 256)
						map_symbol = (char)map_symbol_number;
				} else {
					if(optarg[0]) map_symbol = optarg[0];
				}
			}
			else if(!strcmp(long_option_name,"screensaver")){
				screensaver_mode = 1;
			}
			else if(!strcmp(long_option_name,"wait")){
				wait_flag = 1;
			}
			else if(!strcmp(long_option_name,"shellex")){
				shellex_flag = 1;
			}
			else if(!strcmp(long_option_name,"use-entire-window")){
				use_entire_window = 1;
			}
			else if(!strcmp(long_option_name,"map-rows")){
				if(!optarg) break;
				rows = atoi(optarg);
				if(rows > 0) map_rows = rows;
			}
			else if(!strcmp(long_option_name,"map-symbols-per-line")){
				if(!optarg) break;
				symbols_per_line = atoi(optarg);
				if(symbols_per_line > 0) map_symbols_per_line = symbols_per_line;
			}
			else if(!strcmp(long_option_name,"all")){
				all_flag = 1;
			}
			else if(!strcmp(long_option_name,"all-fixed")){
				all_fixed_flag = 1;
			}
			break;
		case 'a':
			a_flag = 1;
			break;
		case 'o':
			o_flag = 1;
			break;
		case 'l':
			l_flag = 1;
			if(optarg){
				if(!strcmp(optarg,"a")) la_flag = 1;
				if(!strcmp(optarg,"all")) la_flag = 1;
			}
			break;
		case 'p':
			p_flag = 1;
			break;
		case 'v':
			v_flag = 1;
			break;
		case 'm':
			m_flag = 1;
			break;
		case 'b':
			b_flag = 1;
			break;
		case 'h':
			h_flag = 1;
			break;
		case 'i':
		case 'e':
		case 's':
		case 'd':
			obsolete_option = 1;
			break;
		case '?': /* invalid option or -? option */
			if(optopt == '?') h_flag = 1;
			break;
		default:
			printf("?? getopt returned character code 0%o ??\n", c);
		}
	}
	
	dbg_print("command line: %ls\n",GetCommandLineW());
	
	/* scan for individual volume letters */
	if(optind < argc){
		dbg_print("non-option ARGV-elements: ");
		while(optind < argc){
			dbg_print("%s ", argv[optind]);
			length = strlen(argv[optind]);
			if(length == 2){
				if(argv[optind][1] == ':'){
					ch = argv[optind][0];
					if(letter_index > (MAX_DOS_DRIVES - 1)){
						printf("Too many letters specified on the command line.\n");
					} else {
						letters[letter_index] = ch;
						letter_index ++;
					}
				}
			}
			optind++;
			if(optind < argc)
				dbg_print("; ");
		}
		dbg_print("\n");
	}
	
	/* scan for paths of objects to be processed */
	search_for_paths();

	/* --all-fixed flag has more precedence */
	if(all_fixed_flag) all_flag = 0;
	
	if(!l_flag && !all_flag && !all_fixed_flag && !letters[0] && !paths) h_flag = 1;
	
	/* calculate map dimensions if --use-entire-window flag is set */
	if(use_entire_window) CalculateClusterMapDimensions();
}

typedef DWORD (WINAPI *GET_LONG_PATH_NAME_W_PROC)(LPCWSTR,LPWSTR,DWORD);
wchar_t long_path[MAX_LONG_PATH + 1];
wchar_t full_path[MAX_LONG_PATH + 1];

/*
* Paths may be either in short or in long format,
* either ANSI or Unicode, either full or relative.
* This is not safe to assume something concrete.
*/
void search_for_paths(void)
{
	wchar_t *cmdline, *cmdline_copy;
	wchar_t **xargv;
	int i, j, xargc;
	int length;
	DWORD result;
	HMODULE hKernel32Dll = NULL;
	GET_LONG_PATH_NAME_W_PROC pGetLongPathNameW = NULL;
	
	cmdline = GetCommandLineW();
	
	/*
	* CommandLineToArgvW has one documented bug -
	* it doesn't accept backslash + quotation mark sequence.
	* So, we're adding a dot after each trailing backslash
	* in quoted paths. This dot will be removed by GetFullPathName.
	* http://msdn.microsoft.com/en-us/library/bb776391%28VS.85%29.aspx
	*/
	cmdline_copy = malloc(wcslen(cmdline) * sizeof(short) * 2 + sizeof(short));
	if(cmdline_copy == NULL){
		display_error("search_for_paths: not enough memory!");
		return;
	}
	length = wcslen(cmdline);
	for(i = 0, j = 0; i < length; i++){
		cmdline_copy[j] = cmdline[i];
		j ++;
		if(cmdline[i] == '\\' && i != (length - 1)){
			if(cmdline[i + 1] == '"'){
				/* trailing backslash in a quoted path detected */
				cmdline_copy[j] = '.';
				j ++;
			}
		}
	}
	cmdline_copy[j] = 0;
	//printf("command line copy: %ls\n",cmdline_copy);
	
	xargv = CommandLineToArgvW(cmdline_copy,&xargc);
	free(cmdline_copy);
	if(xargv == NULL){
		display_last_error("CommandLineToArgvW failed!");
		return;
	}
	
	hKernel32Dll = LoadLibrary("kernel32.dll");
	if(hKernel32Dll == NULL){
		WgxDbgPrintLastError("search_for_paths: cannot load kernel32.dll");
	} else {
		pGetLongPathNameW = (GET_LONG_PATH_NAME_W_PROC)GetProcAddress(hKernel32Dll,"GetLongPathNameW");
		if(pGetLongPathNameW == NULL)
			WgxDbgPrintLastError("search_for_paths: GetLongPathNameW not found in kernel32.dll");
	}
	
	for(i = 1; i < xargc; i++){
		if(xargv[i][0] == 0) continue;   /* skip empty strings */
		if(xargv[i][0] == '-') continue; /* skip options */
		if(wcslen(xargv[i]) == 2){       /* skip individual volume letters */
			if(xargv[i][1] == ':')
				continue;
		}
		//printf("path detected: arg[%i] = %ls\n",i,xargv[i]);
		/* convert path to the long file name format (on w2k+) */
		if(pGetLongPathNameW){
			result = pGetLongPathNameW(xargv[i],long_path,MAX_LONG_PATH + 1);
			if(result == 0){
				WgxDbgPrintLastError("search_for_paths: GetLongPathNameW failed");
				goto use_short_path;
			} else if(result > MAX_LONG_PATH + 1){
				printf("search_for_paths: long path of \'%ls\' is too long!",xargv[i]);
				goto use_short_path;
			}
		} else {
use_short_path:
			wcsncpy(long_path,xargv[i],MAX_LONG_PATH);
		}
		long_path[MAX_LONG_PATH] = 0;
		/* convert path to the full path */
		result = GetFullPathNameW(long_path,MAX_LONG_PATH + 1,full_path,NULL);
		if(result == 0){
			WgxDbgPrintLastError("search_for_paths: GetFullPathNameW failed");
			wcscpy(full_path,long_path);
		} else if(result > MAX_LONG_PATH + 1){
			printf("search_for_paths: full path of \'%ls\' is too long!",long_path);
			wcscpy(full_path,long_path);
		}
		full_path[MAX_LONG_PATH] = 0;
		/* add path to the list */
		insert_path(full_path);
	}
	
	GlobalFree(xargv);
}

int insert_path(wchar_t *path)
{
	object_path *new_item, *last_item;
	
	if(path == NULL)
		return (-1);
	
	new_item = malloc(sizeof(object_path));
	if(new_item == NULL){
		display_error("insert_path: not enough memory!");
		return (-1);
	}
	
	if(paths == NULL){
		paths = new_item;
		new_item->prev = new_item->next = new_item;
		goto done;
	}
	
	last_item = paths->prev;
	last_item->next = new_item;
	new_item->prev = last_item;
	new_item->next = paths;
	paths->prev = new_item;
	
done:
	new_item->processed = 0;
	wcsncpy(new_item->path,path,MAX_LONG_PATH);
	new_item->path[MAX_LONG_PATH] = 0;
	return 0;
}

void destroy_paths(void)
{
	object_path *item, *next, *head;
	
	if(paths == NULL)
		return;
	
	item = head = paths;
	do {
		next = item->next;
		free(item);
		item = next;
	} while (next != head);
	
	paths = NULL;
}
