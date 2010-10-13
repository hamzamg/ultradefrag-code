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

/**
 * @file toolbar.c
 * @brief Toolbar.
 * @addtogroup Toolbar
 * @{
 */

#include "main.h"

HWND hToolbar = NULL;
HIMAGELIST hToolbarImgList = NULL;
HIMAGELIST hToolbarImgListD = NULL;
HIMAGELIST hToolbarImgListH = NULL;

struct toolbar_button {
	int bitmap;
    int command;
    int state;
    int style;
};

struct toolbar_button buttons[] = {
	{0, IDM_ANALYZE,          TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{1, IDM_DEFRAG,           TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{2, IDM_OPTIMIZE,         TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{3, IDM_STOP,             TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, 0,                    TBSTATE_ENABLED, TBSTYLE_SEP   },
	{4, IDM_SHOW_REPORT,      TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, 0,                    TBSTATE_ENABLED, TBSTYLE_SEP   },
	{5, IDM_CFG_GUI_SETTINGS, TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, 0,                    TBSTATE_ENABLED, TBSTYLE_SEP   },
	{6, IDM_CFG_BOOT_ENABLE,  TBSTATE_ENABLED, TBSTYLE_CHECK },
	{7, IDM_CFG_BOOT_SCRIPT,  TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, 0,                    TBSTATE_ENABLED, TBSTYLE_SEP   },
	{8, IDM_CONTENTS,         TBSTATE_ENABLED, TBSTYLE_BUTTON},
};

#define N_BUTTONS (sizeof(buttons)/sizeof(struct toolbar_button))

TBBUTTON tb_buttons[N_BUTTONS] = {{0}};

/**
 * @brief Creates toolbar for main window.
 * @return Zero for success, negative value otherwise.
 */
int CreateToolbar(void)
{
	HDC hdc;
	int bpp = 32;
	int id, idd, idh;
	int i;
	
	/* create window */
	hToolbar = CreateWindowEx(0,
		TOOLBARCLASSNAME, "",
		WS_CHILD | WS_VISIBLE | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT,
		0, 0, 10, 10,
		hWindow, NULL, hInstance, NULL);
	if(hToolbar == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot create main toolbar!");
		return (-1);
	}
	
	/* set buttons */
	for(i = 0; i < N_BUTTONS; i++){
		tb_buttons[i].iBitmap = buttons[i].bitmap;
		tb_buttons[i].idCommand = buttons[i].command;
		tb_buttons[i].fsState = (BYTE)buttons[i].state;
		tb_buttons[i].fsStyle = (BYTE)buttons[i].style;
	}
	SendMessage(hToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
	SendMessage(hToolbar,TB_ADDBUTTONS,N_BUTTONS,(LPARAM)&tb_buttons);
	
	/* assign images to buttons */
	hdc = GetDC(hWindow);
	if(hdc){
		bpp = GetDeviceCaps(hdc,BITSPIXEL);
		ReleaseDC(hWindow,hdc);
	}
	switch(bpp){
	case 1:
	case 2:
	case 4:
	case 8:
		/* nt4 etc */
		id = IDB_TOOLBAR_8_BIT;
		idd = IDB_TOOLBAR_DISABLED_8_BIT;
		idh = IDB_TOOLBAR_HIGHLIGHTED_8_BIT;
		break;
	case 16:
		/* w2k etc */
		id = IDB_TOOLBAR_16_BIT;
		idd = IDB_TOOLBAR_DISABLED_16_BIT;
		idh = IDB_TOOLBAR_HIGHLIGHTED_16_BIT;
		break;
	default:
		/* xp etc */
		id = IDB_TOOLBAR;
		idd = IDB_TOOLBAR_DISABLED;
		idh = IDB_TOOLBAR_HIGHLIGHTED;
		break;
	}
	hToolbarImgList = ImageList_LoadImage(hInstance,MAKEINTRESOURCE(id),
		16,0,RGB(255,0,255),IMAGE_BITMAP,LR_CREATEDIBSECTION);
	if(hToolbarImgList == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot load main images for the main toolbar!");
		return (-1);
	}
	hToolbarImgListD = ImageList_LoadImage(hInstance,MAKEINTRESOURCE(idd),
		16,0,RGB(255,0,255),IMAGE_BITMAP,LR_CREATEDIBSECTION);
	if(hToolbarImgListD == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot load grayed images for the main toolbar!");
		return (-1);
	}
	hToolbarImgListH = ImageList_LoadImage(hInstance,MAKEINTRESOURCE(idh),
		16,0,RGB(255,0,255),IMAGE_BITMAP,LR_CREATEDIBSECTION);
	if(hToolbarImgListH == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot load highlighted images for the main toolbar!");
		return (-1);
	}
	SendMessage(hToolbar,TB_SETIMAGELIST,0,(LPARAM)hToolbarImgList);
	SendMessage(hToolbar,TB_SETDISABLEDIMAGELIST,0,(LPARAM)hToolbarImgListD);
	SendMessage(hToolbar,TB_SETHOTIMAGELIST,0,(LPARAM)hToolbarImgListH);
	
	/* set state of checks */
#ifndef UDEFRAG_PORTABLE
	if(IsBootTimeDefragEnabled()){
		boot_time_defrag_enabled = 1;
		SendMessage(hToolbar,TB_CHECKBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(TRUE,0));
	} else {
		SendMessage(hToolbar,TB_CHECKBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(FALSE,0));
	}
#else
	SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(FALSE,0));
	SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_CFG_BOOT_SCRIPT,MAKELONG(FALSE,0));
#endif

	return 0;
}

/** @} */
