//
//  DockWindow.h
//  Reaper Plugin
//
//  Created by Daniel Lindenfelser on 23.06.14.
//

#ifndef __Reaper_Plugin__DockWindow__
#define __Reaper_Plugin__DockWindow__

#include <iostream>

#include "reaper.h"

#define TOOLTIP_MAX_LEN					512
#define MIN_DOCKWND_WIDTH				147
#define MIN_DOCKWND_HEIGHT				100

// Aliases to keys
#define SWS_ALT		LVKF_ALT		// 1
#define SWS_CTRL	LVKF_CONTROL	// 2
#define SWS_SHIFT	LVKF_SHIFT		// 4

bool SWS_IsWindow(HWND hwnd);
int SWS_GetModifiers();

#pragma pack(push, 4)
typedef struct SWS_DockWnd_State // Converted to little endian on store
{
	RECT r;
	int state;
	int whichdock;
} SWS_DockWnd_State;
#pragma pack(pop)

class DockWindow
{
public:
	// Unless you need the default contructor (new SWS_DockWnd()), you must provide all parameters
	DockWindow(int iResource=0, const char* cWndTitle="", const char* cId="", int iCmdID=0);
	virtual ~DockWindow();
    
	virtual bool IsActive(bool bWantEdit = false);
	bool IsDocked() { return (m_state.state & 2) == 2; }
	bool IsValidWindow() { return SWS_IsWindow(m_hwnd) ? true : false; }
	HWND GetHWND() { return m_hwnd; }
	WDL_VWnd* GetParentVWnd() { return &m_parentVwnd; }
	WDL_VWnd_Painter* GetVWndPainter() { return &m_vwnd_painter; }
    
	void Show(bool bToggle, bool bActivate);
	void ToggleDocking();
	int SaveState(char* cStateBuf, int iMaxLen);
	void LoadState(const char* cStateBuf, int iLen);
	virtual void OnCommand(WPARAM wParam, LPARAM lParam) {}
    
	static LRESULT screensetCallback(int action, char *id, void *param, void *actionParm, int actionParmSize);
    
	static const int DOCK_MSG = 0xFF0000;
    
protected:
    
	virtual INT_PTR WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Init(); // call from derived constructor!!
	virtual void GetMinSize(int* w, int* h) { *w=MIN_DOCKWND_WIDTH; *h=MIN_DOCKWND_HEIGHT; }
    
	virtual void OnInitDlg() {}
	virtual int OnNotify(WPARAM wParam, LPARAM lParam) { return 0; }
	virtual HMENU OnContextMenu(int x, int y, bool* wantDefaultItems) { return NULL; }
	virtual bool OnPaint() { return false; } // return true if implemented
	virtual void OnResize() {}
	virtual void OnDestroy() {}
	virtual void OnTimer(WPARAM wParam=0) {}
	virtual void OnDroppedFiles(HDROP h) {}
	virtual int OnKey(MSG* msg, int iKeyState) { return 0; } // return 1 for "processed key"
	virtual int OnMouseDown(int xpos, int ypos) { return 0; } // return -1 to eat, >0 to capture
	virtual bool OnMouseDblClick(int xpos, int ypos) { return false; }
	virtual bool OnMouseMove(int xpos, int ypos) { return false; }
	virtual bool OnMouseUp(int xpos, int ypos) { return false; }
	virtual INT_PTR OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }
    
	// Functions for derived classes to load/save some view information (for startup/screensets)
	virtual int SaveView(char* cViewBuf, int iLen) { return 0; } // return num of chars in state (if cViewBuf == NULL, ret # of bytes needed)
	virtual void LoadView(const char* cViewBuf, int iLen) {}
    
	// Functions for WDL_VWnd-based GUIs
	virtual void DrawControls(LICE_IBitmap* bm, const RECT* r, int* tooltipHeight = NULL) {}
	virtual bool GetToolTipString(int xpos, int ypos, char* bufOut, int bufOutSz) { return false; }
	virtual void KillTooltip(bool doRefresh=false);
    
	HWND m_hwnd;
	int m_iCmdID;
	int m_iResource;
	WDL_FastString m_wndTitle;
	WDL_FastString m_id;
	accelerator_register_t m_ar;
	SWS_DockWnd_State m_state;
	bool m_bUserClosed;
	bool m_bSaveStateOnDestroy;
	WDL_WndSizer m_resize;
	WDL_VWnd_Painter m_vwnd_painter;
	WDL_VWnd m_parentVwnd; // owns all children VWnds
	char m_tooltip[TOOLTIP_MAX_LEN];
	POINT m_tooltip_pt;
    
private:
	static INT_PTR WINAPI sWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static int keyHandler(MSG *msg, accelerator_register_t *ctx);
};

char* SWS_LoadDockWndStateBuf(const char* _id, int _len = -1);
int SWS_GetDockWndState(const char* _id, const char* _stateBuf = NULL);
void SWS_SetDockWndState(const char* _stateBuf, int _len, SWS_DockWnd_State* _stateOut);

void DrawTooltipForPoint(LICE_IBitmap *bm, POINT mousePt, RECT *wndr, const char *text);

#endif /* defined(__Reaper_Plugin__DockWindow__) */
