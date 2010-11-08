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
 * @file reg.c
 * @brief System registry.
 * @addtogroup Registry
 * @{
 */

/*
* Use standard RtlXxx functions instead of writing 
* specialized versions of them.
*/

#include "ntndk.h"
#include "zenwinx.h"

static int __stdcall open_smss_key(HANDLE *pKey);
static int __stdcall read_boot_exec_value(HANDLE hKey,void **data,DWORD *size);
static int __stdcall write_boot_exec_value(HANDLE hKey,void *data,DWORD size);
static void __stdcall flush_smss_key(HANDLE hKey);

/* The following two functions replaces bootexctrl in native mode. */

/**
 * @brief Registers command to be executed
 * during the Windows boot process.
 * @param[in] command the name of the command's
 * executable, without an extension.
 * @return Zero for success, negative value otherwise.
 * @note Command's executable must be placed inside 
 * a system32 directory to be executed successfully.
 */
int __stdcall winx_register_boot_exec_command(short *command)
{
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *data;
	DWORD size, value_size;
	short *value, *pos;
	DWORD length, i, len;
	
	DbgCheck1(command,"winx_register_boot_exec_command",-1);
	
	if(open_smss_key(&hKey) < 0) return (-1);
	size = (wcslen(command) + 1) * sizeof(short);
	if(read_boot_exec_value(hKey,(void **)(void *)&data,&size) < 0){
		NtCloseSafe(hKey);
		return (-1);
	}
	
	if(data->Type != REG_MULTI_SZ){
		DebugPrint("BootExecute value has wrong type 0x%x!",
				data->Type);
		winx_heap_free((void *)data);
		NtCloseSafe(hKey);
		return (-1);
	}
	
	value = (short *)(data->Data);
	length = (data->DataLength >> 1) - 1;
	for(i = 0; i < length;){
		pos = value + i;
		//DebugPrint("%ws",pos);
		len = wcslen(pos) + 1;
		if(!wcscmp(pos,command)) goto done;
		i += len;
	}
	wcscpy(value + i,command);
	value[i + wcslen(command) + 1] = 0;

	value_size = (i + wcslen(command) + 1 + 1) * sizeof(short);
	if(write_boot_exec_value(hKey,(void *)(data->Data),value_size) < 0){
		winx_heap_free((void *)data);
		NtCloseSafe(hKey);
		return (-1);
	}

done:	
	winx_heap_free((void *)data);
	flush_smss_key(hKey);
	NtCloseSafe(hKey);
	return 0;
}

/**
 * @brief Deregisters command from being executed
 * during the Windows boot process.
 * @param[in] command the name of the command's
 * executable, without an extension.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_unregister_boot_exec_command(short *command)
{
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *data;
	DWORD size;
	short *value, *pos;
	DWORD length, i, len;
	short *new_value;
	DWORD new_value_size;
	DWORD new_length;
	
	DbgCheck1(command,"winx_unregister_boot_exec_command",-1);
	
	if(open_smss_key(&hKey) < 0) return (-1);
	size = (wcslen(command) + 1) * sizeof(short);
	if(read_boot_exec_value(hKey,(void **)(void *)&data,&size) < 0){
		NtCloseSafe(hKey);
		return (-1);
	}
	
	if(data->Type != REG_MULTI_SZ){
		DebugPrint("BootExecute value has wrong type 0x%x!",
				data->Type);
		winx_heap_free((void *)data);
		NtCloseSafe(hKey);
		return (-1);
	}
	
	value = (short *)(data->Data);
	length = (data->DataLength >> 1) - 1;
	
	new_value_size = (length + 1) << 1;
	new_value = winx_heap_alloc(new_value_size);
	if(!new_value){
		DebugPrint("Cannot allocate %u bytes of memory"
						 "for new BootExecute value!",new_value_size);
		winx_heap_free((void *)data);
		NtCloseSafe(hKey);
		return (-1);
	}

	memset((void *)new_value,0,new_value_size);
	new_length = 0;
	for(i = 0; i < length;){
		pos = value + i;
		//DebugPrint("%ws",pos);
		len = wcslen(pos) + 1;
		if(wcscmp(pos,command)){
			wcscpy(new_value + new_length,pos);
			new_length += len;
		}
		i += len;
	}
	new_value[new_length] = 0;
	
	if(write_boot_exec_value(hKey,(void *)new_value,
	  (new_length + 1) * sizeof(short)) < 0){
		winx_heap_free((void *)new_value);
		winx_heap_free((void *)data);
		NtCloseSafe(hKey);
		return (-1);
	}

	winx_heap_free((void *)new_value);
	winx_heap_free((void *)data);
	flush_smss_key(hKey); /* required by native app before shutdown */
	NtCloseSafe(hKey);
	return 0;
}

/**
 * @brief Opens the SMSS registry key.
 * @param[out] pKey pointer to the key handle.
 * @return Zero for success, negative value otherwise.
 * @note Internal use only.
 */
static int __stdcall open_smss_key(HANDLE *pKey)
{
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status;
	
	RtlInitUnicodeString(&us,L"\\Registry\\Machine\\SYSTEM\\"
							 L"CurrentControlSet\\Control\\Session Manager");
	InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
	status = NtOpenKey(pKey,KEY_QUERY_VALUE | KEY_SET_VALUE,&oa);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot open %ws",us.Buffer);
		return (-1);
	}
	return 0;
}

/**
 * @brief Queries the BootExecute
 * value of the SMSS registry key.
 * @param[in] hKey the key handle.
 * @param[out] data pointer to a buffer that receives the value.
 * @param[in,out] size This paramenter must contain before the call
 * a number of bytes which must be allocated additionally to the size
 * of the BootExecute value. After the call this parameter contains
 * size of the allocated buffer containing the queried value, in bytes.
 * @return Zero for success, negative value otherwise.
 * @note Internal use only.
 */
static int __stdcall read_boot_exec_value(HANDLE hKey,void **data,DWORD *size)
{
	void *data_buffer = NULL;
	DWORD data_size = 0;
	DWORD data_size2 = 0;
	DWORD additional_space_size = *size;
	UNICODE_STRING us;
	NTSTATUS status;
	
	RtlInitUnicodeString(&us,L"BootExecute");
	status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
			NULL,0,&data_size);
	if(status != STATUS_BUFFER_TOO_SMALL){
		DebugPrintEx(status,"Cannot query BootExecute value size");
		return (-1);
	}
	data_size += additional_space_size;
	data_buffer = winx_heap_alloc(data_size);
	if(data_buffer == NULL){
		DebugPrint("Cannot allocate %u bytes of memory for read_boot_exec_value()!",
				data_size);
		return (-1);
	}
	
	RtlZeroMemory(data_buffer,data_size);
	status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
			data_buffer,data_size,&data_size2);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot query BootExecute value");
		winx_heap_free(data_buffer);
		return (-1);
	}
	
	*data = data_buffer;
	*size = data_size;
	return 0;
}

/**
 * @brief Sets the BootExecute value of the SMSS registry key.
 * @param[in] hKey the key handle.
 * @param[in] data pointer to a buffer containing the value.
 * @param[in] size the size of buffer, in bytes.
 * @return Zero for success, negative value otherwise.
 * @note Internal use only.
 */
static int __stdcall write_boot_exec_value(HANDLE hKey,void *data,DWORD size)
{
	UNICODE_STRING us;
	NTSTATUS status;
	
	RtlInitUnicodeString(&us,L"BootExecute");
	status = NtSetValueKey(hKey,&us,0,REG_MULTI_SZ,data,size);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot set BootExecute value");
		return (-1);
	}
	
	return 0;
}

/**
 * @brief Flushes the SMSS registry key.
 * @param[in] hKey the key handle.
 * @note
 * - Internal use only.
 * - Takes no effect in native apps before 
 * the system shutdown/reboot :-)
 */
static void __stdcall flush_smss_key(HANDLE hKey)
{
	NTSTATUS status;
	
	status = NtFlushKey(hKey);
	if(status != STATUS_SUCCESS)
		DebugPrintEx(status,"Cannot update Session Manager registry key on disk");
}

/** @} */
