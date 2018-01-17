/******************************************************************************
/ Menus.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS), Jeffos
/ https://code.google.com/p/sws-extension
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

// Utility functions for manipulating menus, as well as the main menu creation function
// for the SWS extension.

#include "reaper.h"

#include "Menus.h"

#include "localize.h"

bool IsLocalized() {
#ifdef _SWS_LOCALIZATION
	return (GetLangPack()->GetLength() > 0);
#else
	return false;
#endif
}

#ifndef _WIN32
int GetMenuString(HMENU hMenu, UINT uIDItem, char* lpString, int nMaxCount, UINT uFlag)
{
	if (hMenu && lpString)
	{
		MENUITEMINFO xInfo;
		xInfo.cbSize = sizeof(MENUITEMINFO);
        /* On Win OS
         xInfo.fMask = MIIM_STRING;
         */
		xInfo.fMask = MIIM_TYPE; // ok on OSX/SWELL
		xInfo.fType = MFT_STRING;
		xInfo.fState = 0;
		xInfo.wID = 0;
		xInfo.hSubMenu = NULL;
		xInfo.hbmpChecked = NULL;
		xInfo.hbmpUnchecked = NULL;
		xInfo.dwItemData = 0;
		xInfo.dwTypeData = lpString;
		xInfo.cch = nMaxCount;
		if (GetMenuItemInfo(hMenu, uIDItem, (uFlag&MF_BYPOSITION) == MF_BYPOSITION, &xInfo))
			return strlen(lpString);
	}
	return 0;
}
#endif

// *************************** UTILITY FUNCTIONS ***************************

void AddToMenuOld(HMENU hMenu, const char* text, int id, int iInsertAfter, bool bPos, UINT uiSate)
{
	if (!text)
		return;

	int iPos = GetMenuItemCount(hMenu);
	if (bPos)
		iPos = iInsertAfter;
	else
	{
		if (iInsertAfter < 0)
			iPos += iInsertAfter + 1;
		else
		{
			HMENU h = FindMenuItem(hMenu, iInsertAfter, &iPos);
			if (h)
			{
				hMenu = h;
				iPos++;
			}
		}
	}
	
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	if (strcmp(text, SWS_SEPARATOR) == 0)
	{
		mi.fType = MFT_SEPARATOR;
		mi.fMask = MIIM_TYPE;
		InsertMenuItem(hMenu, iPos, true, &mi);
	}
	else
	{
		mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mi.fState = uiSate;
		mi.fType = MFT_STRING;
		mi.dwTypeData = (char*)text;
		mi.wID = id;
		InsertMenuItem(hMenu, iPos, true, &mi);
	}
}

// This version "auto sort" menu items
// Note: could be used as default AddToMenu (when no insert position is
// requested) but this func is only used when the extension is localized ATM..
void AddToMenu(HMENU hMenu, const char* text, int id, int iInsertAfter, bool bPos, UINT uiSate)
{
	if (!text)
		return;

	if(!IsLocalized() || bPos || iInsertAfter != -1) {
		AddToMenuOld(hMenu, text, id, iInsertAfter, bPos, uiSate);
		return;
	}

	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	if (strcmp(text, SWS_SEPARATOR) == 0)
	{
		mi.fType = MFT_SEPARATOR;
		mi.fMask = MIIM_TYPE;
		InsertMenuItem(hMenu, GetMenuItemCount(hMenu), true, &mi);
	}
	else
	{
		mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mi.fState = uiSate;
		mi.fType = MFT_STRING;
		mi.dwTypeData = (char*)text;
		mi.wID = id;
		InsertMenuItem(hMenu, FindSortedPos(hMenu, text), true, &mi);
	}
}

void AddSubMenuOld(HMENU hMenu, HMENU subMenu, const char* text, int iInsertAfter, UINT uiSate)
{
	int iPos = GetMenuItemCount(hMenu);
	if (iInsertAfter < 0)
	{
		iPos += iInsertAfter + 1;
		if (iPos < 0)
			iPos = 0;
	}
	else
	{
		HMENU h = FindMenuItem(hMenu, iInsertAfter, &iPos);
		if (h)
		{
			hMenu = h;
			iPos++;
		}
	}

	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
	mi.fState = uiSate;
	mi.fType = MFT_STRING;
	mi.hSubMenu = subMenu;
	mi.dwTypeData = (LPSTR)text;
	InsertMenuItem(hMenu, iPos, true, &mi);
}

// This version "auto sort" sub menu items
// Note: could be used as default AddSubMenu (when no insert position is
// requested) but this func is only used when the extension is localized ATM..
void AddSubMenu(HMENU hMenu, HMENU subMenu, const char* text, int iInsertAfter, UINT uiSate)
{
	if(!IsLocalized() || iInsertAfter != -1) {
		AddSubMenuOld(hMenu, subMenu, text, iInsertAfter, uiSate);
		return;
	}

	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
	mi.fState = uiSate;
	mi.fType = MFT_STRING;
	mi.hSubMenu = subMenu;
	mi.dwTypeData = (LPSTR)text;
	InsertMenuItem(hMenu, FindSortedPos(hMenu, text), true, &mi);
}

// find sorted position after the last separator
int FindSortedPos(HMENU hMenu, const char* text)
{
	int pos = -1, nbItems = GetMenuItemCount(hMenu);
#ifdef _WIN32
	wchar_t widetext[4096], widebuf[4096];
	MultiByteToWideChar(CP_UTF8, 0, text, -1, widetext, 4096);
	_locale_t locale = _create_locale(LC_ALL, "");
#else
	char buf[4096] = "";
#endif
	MENUITEMINFO mi = {sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_TYPE;

	for (int i=nbItems-1; i>=0 ; i--)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.fType == MFT_SEPARATOR)
			break;
#ifdef _WIN32
		GetMenuStringW(hMenu, i, widebuf, 4096, MF_BYPOSITION);
		if (_wcsnicoll_l(widetext, widebuf, 4096, locale) < 0) // setLocale() can break things (atof and comma as a decimal mark) so use temporary locale object
			pos = i;
#else
		GetMenuString(hMenu, i, buf, sizeof(buf), MF_BYPOSITION);
		if (strcasecmp(text, buf) < 0) // not as good as on Win OS, e.g. French "Sélectionner" vs "Supprimer"
			pos = i;
#endif
	}
#ifdef _WIN32	
	_free_locale(locale);
#endif	
	return pos<0 ? nbItems : pos;
}

HMENU FindMenuItem(HMENU hMenu, int iCmd, int* iPos)
{
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_ID | MIIM_SUBMENU;
	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.hSubMenu)
		{
			HMENU hSubMenu = FindMenuItem(mi.hSubMenu, iCmd, iPos);
			if (hSubMenu)
				return hSubMenu;
		}
		if (mi.wID == iCmd)
		{
			*iPos = i;
			return hMenu;
		}
	}
	return NULL;
}

void SWSSetMenuText(HMENU hMenu, int iCmd, const char* cText)
{
	int iPos;
	hMenu = FindMenuItem(hMenu, iCmd, &iPos);
	if (hMenu)
	{
		MENUITEMINFO mi={sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_TYPE;
		mi.fType = MFT_STRING;
		mi.dwTypeData = (char*)cText;
		SetMenuItemInfo(hMenu, iPos, true, &mi);
	}
}

int SWSGetMenuPosFromID(HMENU hMenu, UINT id)
{	// Replacement for deprecated windows func GetMenuPosFromID
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_ID;
	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.wID == id)
			return i;
	}
	return -1;
}
