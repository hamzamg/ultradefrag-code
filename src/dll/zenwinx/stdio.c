/*
 *  ZenWINX - WIndows Native eXtended library.
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

/**
 * @file stdio.c
 * @brief Console I/O code.
 * @addtogroup Console
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

#define INTERNAL_BUFFER_SIZE 2048

int __stdcall kb_read(PKEYBOARD_INPUT_DATA pKID,int msec_timeout);
void IntTranslateKey(PKEYBOARD_INPUT_DATA InputData, KBD_RECORD *kbd_rec);

/**
 * @brief Displays ANSI string on the screen.
 * @param[in] string the string to be displayed.
 * @note Internal use only.
 */
void winx_print(char *string)
{
	ANSI_STRING aStr;
	UNICODE_STRING uStr;
	int i, len;

	/* never call winx_dbg_print_ex() here */
	if(!string) return;
	RtlInitAnsiString(&aStr,string);
	if(RtlAnsiStringToUnicodeString(&uStr,&aStr,TRUE) == STATUS_SUCCESS){
		NtDisplayString(&uStr);
		RtlFreeUnicodeString(&uStr);
	} else {
		len = strlen(string);
		for(i = 0; i < len; i ++)
			winx_putch(string[i]);
	}

}

/**
 * @brief putch() native equivalent.
 * @bug Does not recognize special characters
 *      such as 'backspace'.
 */
int __cdecl winx_putch(int ch)
{
	UNICODE_STRING uStr;
	short s[2];

	/* never call winx_dbg_print_ex() here */
	s[0] = (short)ch; s[1] = 0;
	RtlInitUnicodeString(&uStr,s);
	NtDisplayString(&uStr);
	return ch;
}

/**
 * @brief puts() native equivalent.
 * @bug Does not recognize special characters
 *      such as 'backspace'.
 */
int __cdecl winx_puts(const char *string)
{
	if(!string) return (-1);
	return winx_printf("%s\n",string) ? 0 : (-1);
}

/**
 * @brief printf() native equivalent.
 * @bug Does not recognize special characters
 *      such as 'backspace'.
 */
int __cdecl winx_printf(const char *format, ...)
{
	va_list arg;
	int done;
	char *small_buffer;
	char *big_buffer = NULL;
	int size = INTERNAL_BUFFER_SIZE * 2;
	
	/* never call winx_dbg_print_ex() here */

	if(!format) return 0;
	
	small_buffer = winx_heap_alloc(INTERNAL_BUFFER_SIZE);
	if(!small_buffer){
		DebugPrint("Not enough memory for winx_printf()!\n");
		winx_print("\nNot enough memory for winx_printf()!\n");
		return 0;
	}

	/* prepare for _vsnprintf call */
	va_start(arg,format);
	memset(small_buffer,0,INTERNAL_BUFFER_SIZE);
	done = _vsnprintf(small_buffer,INTERNAL_BUFFER_SIZE,format,arg);
	if(done == -1 || done == INTERNAL_BUFFER_SIZE){
		/* buffer is too small; try to allocate two times larger */
		do {
			big_buffer = winx_heap_alloc((SIZE_T)size);
			if(!big_buffer){
				DebugPrint("Not enough memory for winx_printf()!\n");
				winx_print("\nNot enough memory for winx_printf()!\n");
				va_end(arg);
				winx_heap_free(small_buffer);
				return 0;
			}
			memset(big_buffer,0,size);
			done = _vsnprintf(big_buffer,size,format,arg);
			if(done != -1 && done != size) break;
			winx_heap_free(big_buffer);
			size = size << 1;
			if(!size){
				DebugPrint("winx_printf() failed!\n");
				winx_print("\nwinx_printf() failed!\n");
				va_end(arg);
				winx_heap_free(small_buffer);
				return 0;
			}
		} while(1);
	}
	if(big_buffer){
		winx_print(big_buffer);
		winx_heap_free(big_buffer);
	} else {
		winx_print(small_buffer);
	}
	va_end(arg);
	winx_heap_free(small_buffer);
	return done;
}

/**
 * @brief Checks the console for keyboard input.
 * @details Waits for an input during the specified time interval.
 * @param[in] msec the time interval, in milliseconds.
 * @return If any key was pressed, the return value is
 *         the ascii character or zero for the control keys.
 *         Negative value indicates failure.
 * @note
 * - If an INFINITE time constant is passed, the
 *   time-out interval never elapses.
 * - This call may terminate the program if NtCancelIoFile() 
 *   fails for one of the existing keyboard devices.
 */
int __cdecl winx_kbhit(int msec)
{
	KBD_RECORD kbd_rec;

	return winx_kb_read(&kbd_rec,msec);
}

/**
 * @brief Low level winx_kbhit() equivalent.
 * @param[in] kbd_rec pointer to structure receiving key information.
 * @param[in] msec the timeout interval, in milliseconds.
 * @return If any key was pressed, the return value is
 *         the ascii character or zero for the control keys.
 *         Negative value indicates failure.
 * @note
 * - If an INFINITE time constant is passed, the
 *   time-out interval never elapses.
 * - This call may terminate the program if NtCancelIoFile() 
 *   fails for one of the existing keyboard devices.
 */
int __cdecl winx_kb_read(KBD_RECORD *kbd_rec,int msec)
{
	KEYBOARD_INPUT_DATA kbd;

	if(kb_read(&kbd,msec) < 0) return (-1);
	IntTranslateKey(&kbd,kbd_rec);
	if(!kbd_rec->bKeyDown){
		/*winx_printf("\nwinx_kb_read(): The key was released!\n");*/
		return (-1);
	}
	return (int)kbd_rec->AsciiChar;
}

/**
 * @brief Checks the console for the 'Break' character on 
 *        the keyboard input.
 * @details Waits for an input during the specified time interval.
 * @param[in] msec the time interval, in milliseconds.
 * @return If the 'Break' key was pressed, the return value is zero.
 *         Otherwise it returns negative value.
 * @note
 * - If an INFINITE time constant is passed, the
 *   time-out interval never elapses.
 * - This call may terminate the program if NtCancelIoFile() 
 *   fails for one of the existing keyboard devices.
 */
int __cdecl winx_breakhit(int msec)
{
	KEYBOARD_INPUT_DATA kbd;

	if(kb_read(&kbd,msec) < 0) return (-1);
	if((kbd.Flags & KEY_E1) && (kbd.MakeCode == 0x1d)) return 0;
	/*winx_printf("\nwinx_breakhit(): Other key was pressed.\n");*/
	return (-1);
}

/**
 * @brief getch() native equivalent.
 * @note This call may terminate the program if NtCancelIoFile() 
 *       fails for one of the existing keyboard devices.
 */
int __cdecl winx_getch(void)
{
	KEYBOARD_INPUT_DATA kbd;
	KBD_RECORD kbd_rec;

	do{
		if(kb_read(&kbd,INFINITE) < 0) return (-1);
		IntTranslateKey(&kbd,&kbd_rec);
	} while(!kbd_rec.bKeyDown);
	return (int)kbd_rec.AsciiChar;
}

/**
 * @brief getche() native equivalent.
 * @note This call may terminate the program if NtCancelIoFile() 
 *       fails for one of the existing keyboard devices.
 * @bug Does not recognize special characters
 *      such as 'backspace'.
 */
int __cdecl winx_getche(void)
{
	int ch;

	ch = winx_getch();
	if(ch != -1 && ch != 0 && ch != 0x08) /* skip backspace */
		winx_putch(ch);
	return ch;
}

/**
 * @brief gets() native equivalent with limited number of 
 *        characters to read.
 * @param[out] string the storage for the input string.
 * @param[in] n the maximum number of characters to read.
 * @return Number of characters read including terminal zero.
 *         Negative value indicates failure.
 * @note This call may terminate the program if NtCancelIoFile() 
 *       fails for one of the existing keyboard devices.
 * @bug Does not recognize special characters
 *      such as 'backspace'.
 */
int __cdecl winx_gets(char *string,int n)
{
	int i;
	int ch;

	if(!string){
		winx_printf("\nwinx_gets() invalid string!\n");
		return (-1);
	}
	if(n <= 0){
		winx_printf("\nwinx_gets() invalid string length %d!\n",n);
		return (-1);
	}
	
	for(i = 0; i < n; i ++){
		do {
			ch = winx_getche();
			if(ch == -1){
				string[i] = 0;
				return (-1);
			}
		} while(ch == 0 || ch == 0x08); /* skip backspace */
		if(ch == 13){
			winx_putch('\n');
			string[i] = 0;
			return (i+1);
		}
		string[i] = (char)ch;
	}
	winx_printf("\nwinx_gets() buffer overflow!\n");
	string[n-1] = 0;
	return n;
}

/**
 * @brief Initializes commands history.
 * @param[in] h pointer to structure holding
 * the commands history.
 */
void __cdecl winx_init_history(winx_history *h)
{
	if(h == NULL){
		DebugPrint("winx_init_history(): h = NULL!");
		return;
	}
	h->head = h->current = NULL;
	h->n_entries = 0;
}

/**
 * @brief Destroys commands history.
 * @param[in] h pointer to structure holding
 * the commands history.
 */
void __cdecl winx_destroy_history(winx_history *h)
{
	winx_history_entry *entry;
	
	if(h == NULL){
		DebugPrint("winx_destroy_history(): h = NULL!");
		return;
	}

	for(entry = h->head; entry != NULL; entry = entry->next_ptr){
		if(entry->string) winx_heap_free(entry->string);
		if(entry->next_ptr == h->head) break;
	}
	winx_list_destroy((list_entry **)(void *)&h->head);
	h->current = NULL;
	h->n_entries = 0;
}

/**
 * @brief Adds an entry to commands history list.
 * @note Internal use only.
 */
static void winx_add_history_entry(winx_history *h,char *string)
{
	winx_history_entry *entry, *last_entry = NULL;
	int length;
	
	if(h == NULL || string == NULL) return;
	
	if(h->head) last_entry = h->head->prev_ptr;
	entry = (winx_history_entry *)winx_list_insert_item((list_entry **)(void *)&h->head,
		(list_entry *)last_entry,sizeof(winx_history_entry));
	if(entry == NULL){
		DebugPrint("Not enough memory for winx_add_winx_history_entry()!");
		winx_printf("\nNot enough memory for winx_add_winx_history_entry()!\n");
		return;
	}
	
	length = strlen(string) + 1;
	entry->string = (char *)winx_heap_alloc(length);
	if(entry->string == NULL){
		DebugPrint("Cannot allocate %u bytes of memory for winx_add_winx_history_entry()!",length);
		winx_printf("\nCannot allocate %u bytes of memory for winx_add_winx_history_entry()!\n",length);
		winx_list_remove_item((list_entry **)(void *)&h->head,(list_entry *)entry);
	} else {
		strcpy(entry->string,string);
		h->n_entries ++;
		h->current = entry;
	}
}

/**
 * @brief Displays prompt on the screen and waits for
 * the user input. When user presses the return key
 * fills the string pointed by the second parameter 
 * by characters read.
 * @param[in] prompt the string to be printed as prompt.
 * @param[out] string the storage for the input string.
 * @param[in] n the maximum number of characters to read.
 * @param[in,out] h pointer to structure holding
 * the commands history. May be NULL.
 * @return Number of characters read including terminal zero.
 * Negative value indicates failure.
 * @note
 * - Recognizes properly both backslash and escape keys.
 * - The sentence above works fine only when user input
 * stands in a single line of the screen.
 * - Recognizes arrow keys to walk through commands history.
 * - This call may terminate the program if NtCancelIoFile() 
 * fails for one of the existing keyboard devices.
 */
int __cdecl winx_prompt_ex(char *prompt,char *string,int n,winx_history *h)
{
	KEYBOARD_INPUT_DATA kbd;
	KBD_RECORD kbd_rec;
	char *buffer;
	int buffer_length;
	char format[16];
	int i, ch, line_length;
	int history_listed_to_the_last_entry = 0;

	if(!string){
		winx_printf("\nwinx_prompt_ex() invalid string!\n");
		return (-1);
	}
	if(n <= 0){
		winx_printf("\nwinx_prompt_ex() invalid string length %d!\n",n);
		return (-1);
	}
	
	if(!prompt)	prompt = "";
	buffer_length = strlen(prompt) + n;
	buffer = winx_heap_alloc(buffer_length);
	if(buffer == NULL){
		winx_printf("\nNot enough memory for winx_prompt_ex()!\n");
		return (-1);
	}
	
	winx_printf("%s",prompt);
	
	/* keep string always null terminated */
	RtlZeroMemory(string,n);
	for(i = 0; i < (n - 1); i ++){
		/* read keyboard until an ordinary character appearance */
		do {
			do{
				if(kb_read(&kbd,INFINITE) < 0) goto fail;
				IntTranslateKey(&kbd,&kbd_rec);
			} while(!kbd_rec.bKeyDown);
			ch = (int)kbd_rec.AsciiChar;
			/*
			* Truncate the string if either backspace or escape pressed.
			* Walk through history if one of arrow keys pressed.
			*/
			if(ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1 || \
			  kbd_rec.wVirtualScanCode == 0x48 || kbd_rec.wVirtualScanCode == 0x50){
				line_length = strlen(prompt) + strlen(string);
				/* handle escape key */
				if(kbd_rec.wVirtualScanCode == 0x1){
					/* truncate the string if escape pressed */
					RtlZeroMemory(string,n);
					i = 0;
				}
				/* handle backspace key */
				if(ch == 0x08){
					/*
					* make the string one character shorter
					* if backspace pressed
					*/
					if(i > 0){
						i--;
						string[i] = 0;
					}
				}
				/* handle arrow keys */
				if(h){
					if(h->head && h->current){
						if(kbd_rec.wVirtualScanCode == 0x48){
							/* list history back */
							if(h->current == h->head->prev_ptr && \
							  !history_listed_to_the_last_entry){
								/* set the flag and don't list back */
								history_listed_to_the_last_entry = 1;
							} else {
								if(h->current != h->head)
									h->current = h->current->prev_ptr;
								history_listed_to_the_last_entry = 0;
							}
							if(h->current->string){
								RtlZeroMemory(string,n);
								strcpy(string,h->current->string);
								i = strlen(string);
							}
						} else if(kbd_rec.wVirtualScanCode == 0x50){
							/* list history forward */
							if(h->current->next_ptr != h->head){
								h->current = h->current->next_ptr;
								if(h->current->string){
									RtlZeroMemory(string,n);
									strcpy(string,h->current->string);
									i = strlen(string);
								}
								if(h->current == h->head->prev_ptr)
									history_listed_to_the_last_entry = 1;
								else
									history_listed_to_the_last_entry = 0;
							}
						}
					}
				}
				
				/* clear history_listed_to_the_last_entry flag */
				if(ch != 0 || kbd_rec.wVirtualScanCode == 0x1)
					history_listed_to_the_last_entry = 0;
				
				/* redraw the prompt */
				_snprintf(buffer,buffer_length,"%s%s",prompt,string);
				buffer[buffer_length - 1] = 0;
				_snprintf(format,sizeof(format),"\r%%-%us",line_length);
				format[sizeof(format) - 1] = 0;
				winx_printf(format,buffer);
				/*
				* redraw the prompt again to set carriage position
				* exactly behind the string printed
				*/
				line_length = strlen(prompt) + strlen(string);
				_snprintf(format,sizeof(format),"\r%%-%us",line_length);
				format[sizeof(format) - 1] = 0;
				winx_printf(format,buffer);
				continue;
			}
		} while(ch == 0 || ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1 || \
			  kbd_rec.wVirtualScanCode == 0x48 || kbd_rec.wVirtualScanCode == 0x50);
		
		/* print a character read */
		winx_putch(ch);

		/* return when \r character appears */
		if(ch == '\r'){
			winx_putch('\n');
			goto done;
		}

		/* we have an ordinary character, append it to the string */
		string[i] = (char)ch;
	}
	winx_printf("\nwinx_prompt_ex() buffer overflow!\n");

done:
	winx_heap_free(buffer);
	/* add nonempty strings to the history */
	if(string[0]){
		winx_add_history_entry(h,string);
	}
	return (i+1);
	
fail:
	winx_heap_free(buffer);
	return (-1);
}

/**
 * @brief Simplified analog of winx_prompt_ex() call.
 * Has no support of commands history.
 */
int __cdecl winx_prompt(char *prompt,char *string,int n)
{
	return winx_prompt_ex(prompt,string,n,NULL);
}

/** @} */
