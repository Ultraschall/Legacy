//
//  reaper.h
//  Reaper Plugin
//
//  Created by Daniel Lindenfelser on 21.06.14.
//

#ifndef __Reaper_Plugin__reaper__
#define __Reaper_Plugin__reaper__

#include <string>

#include "reaper_plugin.h"
#include "sws_rpf_wrapper.h"

#include "WDL/wdltypes.h"
#include "WDL/assocarray.h"
#include "WDL/ptrlist.h"
#include "WDL/projectcontext.h"
#include "WDL/wingui/virtwnd.h"
#include "WDL/wingui/virtwnd-controls.h"
#include "WDL/wingui/wndsize.h"

#define SWS_SEPARATOR			"SEPARATOR"

#define ULTRASCHALL_INI         "Ultraschall"

#define IDD_SOUNDBOARD 12345

#ifndef _WIN32
#define _strdup strdup
#define _strndup strndup
#define _snprintf snprintf
#define _stricmp stricmp
#define _strnicmp strnicmp
#define LVKF_ALT 1
#define LVKF_CONTROL 2
#define LVKF_SHIFT 4
#endif

#define LAST_COMMAND			((char*)(INT_PTR)-1)
#define DEFACCEL				{ 0, 0, 0 }

typedef struct COMMAND_T
{
	gaccel_register_t accel;
	const char* id;
	void (*doCommand)(COMMAND_T*);
	const char* menuText;
	INT_PTR user;
	int (*getEnabled)(COMMAND_T*);
	int uniqueSectionId;
	void(*onAction)(COMMAND_T*, int, int, int, HWND);
	bool fakeToggle;
} COMMAND_T;

extern REAPER_PLUGIN_HINSTANCE g_hInst;
extern HWND g_hwndParent;

// Command/action handling, sws_extension.cpp
int RegisterCmd(COMMAND_T* pCommand, const char* cFile, int cmdId = 0);
int RegisterCmds(COMMAND_T* pCommands, const char* cFile); // Multiple commands in a table, terminated with LAST_COMMAND
#define RegisterCommands(c) RegisterCmds(c, __FILE__)

int SWSGetCommandID(void (*cmdFunc)(COMMAND_T*), INT_PTR user = 0, const char** pMenuText = NULL);
MediaTrack* getTrackByName(char* trackName);

struct time
{
    int s, m, h = NULL;
    std::string ms = "";
};

std::string _format_time(char* aTime);

#endif /* defined(__Reaper_Plugin__reaper__) */
