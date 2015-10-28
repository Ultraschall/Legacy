//
//  DockWindow.cpp
//  Reaper Plugin
//
//  Created by Daniel Lindenfelser on 23.06.14.
//

#include "DockWindow.h"

#include "Menus.h"

#include "localize.h"

#include "resources.h"

#define CELL_EDIT_TIMER		0x1000
#define CELL_EDIT_TIMEOUT	50
#define TOOLTIP_TIMER		0x1001
#define TOOLTIP_TIMEOUT		350
#define SWS_THEMING         false

#define SNM_FONT_NAME				"Lucia grande"
#define SNM_FONT_HEIGHT				10

// native font rendering default value
// note: not configurable on osx, optional on win (s&m.ini)
bool g_SNM_ClearType = true;

bool SWS_IsWindow(HWND hwnd)
{
#ifdef _WIN32
	return IsWindow(hwnd) ? true : false;
#else
	// ISSUE 498: OSX IsWindow is broken for docked windows!
	//   Justin recommends not using IsWindow at all, but there are a lot of cases
	//   where we rely on it for error checking.  Instead of removing all calls and
	//   doing internal window state handling I replaced all IsWindow calls with
	//   this function that checks for docked windows on OSX.  It's a bit of a
	//   hack, but less risky IMO than rewriting tons of window handling code.
	//
	//   Maybe could replace with return hwnd != NULL;
	return (bool)IsWindow(hwnd) ? true : (DockIsChildOfDock(hwnd, NULL) != -1);
#endif
}

int SWS_GetModifiers()
{
	int iKeys = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? SWS_CTRL  : 0;
	iKeys    |= GetAsyncKeyState(VK_MENU)    & 0x8000 ? SWS_ALT   : 0;
	iKeys    |= GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? SWS_SHIFT : 0;
	return iKeys;
}

WDL_DLGRET SNM_HookThemeColorsMessage(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam, bool _wantColorEdit)
{
	if (SWS_THEMING)
	{
		switch(_uMsg)
		{
#ifdef _WIN32
			case WM_INITDIALOG :
				// remove XP style on some child ctrls (cannot be themed otherwise)
				EnumChildWindows(_hwnd, EnumRemoveXPStyles, 0);
				return 0;
#endif
			case WM_CTLCOLOREDIT:
				if (!_wantColorEdit) return 0;
			case WM_CTLCOLORSCROLLBAR: // not managed yet, just in case..
			case WM_CTLCOLORLISTBOX:
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORDLG:
			case WM_CTLCOLORSTATIC:
                /* commented (custom implementations)
                 case WM_DRAWITEM:
                 */
				return SendMessage(GetMainHwnd(), _uMsg, _wParam, _lParam);
		}
	}
	return 0;
}

void* GetConfigVar(const char* cVar)
{
	int sztmp;
	void* p = NULL;
	if (int iOffset = projectconfig_var_getoffs(cVar, &sztmp))
	{
		p = projectconfig_var_addr(EnumProjects(-1, NULL, 0), iOffset);
	}
	else if (p = get_config_var(cVar, &sztmp))
	{
	}
	else
	{
		p = get_midi_config_var(cVar, &sztmp);
	}
	return p;
}

DockWindow::DockWindow(int iResource, const char* cWndTitle, const char* cId, int iCmdID)
:m_hwnd(NULL), m_iResource(iResource), m_wndTitle(cWndTitle), m_id(cId), m_bUserClosed(false), m_iCmdID(iCmdID), m_bSaveStateOnDestroy(true)
{
	if (cId && *cId) // e.g. default constructor
	{
		screenset_unregister((char*)cId);
		screenset_registerNew((char*)cId, screensetCallback, this);
	}
    
	memset(&m_state, 0, sizeof(SWS_DockWnd_State));
	*m_tooltip = '\0';
	m_ar.translateAccel = keyHandler;
	m_ar.isLocal = true;
	m_ar.user = this;
	plugin_register("accelerator", &m_ar);
}

// Init() must be called from the constructor of *every* derived class.  Unfortunately,
// you can't just call Init() from the DockWnd/base class, because the vTable isn't
// setup yet.
void DockWindow::Init()
{
	int iLen = sizeof(SWS_DockWnd_State) + SaveView(NULL, 0);
	char* cState = SWS_LoadDockWndStateBuf(m_id.Get(), iLen);
	LoadState(cState, iLen);
	delete [] cState;
}

DockWindow::~DockWindow()
{
	plugin_register("-accelerator", &m_ar);
	if (m_id.GetLength())
		screenset_unregister((char*)m_id.Get());
}

void DockWindow::Show(bool bToggle, bool bActivate)
{
	if (!SWS_IsWindow(m_hwnd))
	{
		CreateDialogParam(g_hInst, MAKEINTRESOURCE(m_iResource), g_hwndParent, DockWindow::sWndProc, (LPARAM)this);
		if (IsDocked() && bActivate)
			DockWindowActivate(m_hwnd);
#ifndef _WIN32
		// TODO see if DockWindowRefresh works here
		InvalidateRect(m_hwnd, NULL, TRUE);
#endif
	}
	else if (!IsWindowVisible(m_hwnd) || (bActivate && !bToggle))
	{
		if ((m_state.state & 2))
			DockWindowActivate(m_hwnd);
		else
			ShowWindow(m_hwnd, SW_SHOW);
		SetFocus(m_hwnd);
	}
	else if (bToggle)// If already visible close the window
		SendMessage(m_hwnd, WM_COMMAND, IDCANCEL, 0);
}

bool DockWindow::IsActive(bool bWantEdit)
{
	if (!IsValidWindow())
		return false;
    
	return GetFocus() == m_hwnd || IsChild(m_hwnd, GetFocus());
}

INT_PTR WINAPI DockWindow::sWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DockWindow* pObj = (DockWindow*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	if (!pObj && uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		pObj = (DockWindow*)lParam;
		pObj->m_hwnd = hwndDlg;
	}
	return pObj ? pObj->WndProc(uMsg, wParam, lParam) : 0;
}

INT_PTR DockWindow::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (SWS_THEMING)
	{
		// theme other ctrls
		if (INT_PTR r = SNM_HookThemeColorsMessage(m_hwnd, uMsg, wParam, lParam, false))
			return r;
	}
    
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			m_resize.init(m_hwnd);
            
			// Call derived class initialization
			OnInitDlg();
            
			if ((m_state.state & 2))
			{
				DockWindowAddEx(m_hwnd, (char*)m_wndTitle.Get(), (char*)m_id.Get(), true);
			}
			else
			{
				EnsureNotCompletelyOffscreen(&m_state.r);
				if (m_state.r.left || m_state.r.top || m_state.r.right || m_state.r.bottom)
					SetWindowPos(m_hwnd, NULL, m_state.r.left, m_state.r.top, m_state.r.right-m_state.r.left, m_state.r.bottom-m_state.r.top, SWP_NOZORDER);
				AttachWindowTopmostButton(m_hwnd);
				AttachWindowResizeGrip(m_hwnd);
				ShowWindow(m_hwnd, SW_SHOW);
			}
			break;
		}
		case WM_TIMER:
            if (wParam == TOOLTIP_TIMER)
			{
				KillTimer(m_hwnd, wParam);
                
				POINT p; GetCursorPos(&p);
				ScreenToClient(m_hwnd,&p);
				RECT r; GetClientRect(m_hwnd,&r);
				char buf[TOOLTIP_MAX_LEN] = "";
				if (PtInRect(&r,p))
					if (!m_parentVwnd.GetToolTipString(p.x,p.y,buf,sizeof(buf)) && !GetToolTipString(p.x,p.y,buf,sizeof(buf)))
						*buf='\0';
                
				if (strcmp(buf, m_tooltip))
				{
					m_tooltip_pt = p;
					lstrcpyn(m_tooltip,buf,sizeof(m_tooltip));
					InvalidateRect(m_hwnd,NULL,FALSE);
				}
			}
			else
				OnTimer(wParam);
			break;
		case WM_NOTIFY:
		{
			NMHDR* hdr = (NMHDR*)lParam;
            
			return OnNotify(wParam, lParam);
		}
		case WM_CONTEXTMENU:
		{
			KillTooltip(true);
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			
			// SWS issue 373 - removed code from here that removed all but one selection on right click on OSX
            
			bool wantDefaultItems = true;
			HMENU hMenu = OnContextMenu(x, y, &wantDefaultItems);
			if (wantDefaultItems)
			{
				if (!hMenu)
					hMenu = CreatePopupMenu();
				else
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
                
				// Add std menu items
				char str[128];
				if (_snprintf(str, sizeof(str), __LOCALIZE_VERFMT("Dock %s in Docker","sws_menu"), m_wndTitle.Get()) > 0)
					AddToMenu(hMenu, str, DOCK_MSG);
                
				// Check dock state
				if ((m_state.state & 2))
					CheckMenuItem(hMenu, DOCK_MSG, MF_BYCOMMAND | MF_CHECKED);
				AddToMenu(hMenu, __LOCALIZE("Close window","sws_menu"), IDCANCEL);
			}
			
			if (hMenu)
			{
				if (x == -1 || y == -1)
				{
					RECT r;
					GetWindowRect(m_hwnd, &r);
					x = r.left;
					y = r.top;
				}
				kbd_reprocessMenu(hMenu, NULL);
				TrackPopupMenu(hMenu, 0, x, y, 0, m_hwnd, NULL);
				DestroyMenu(hMenu);
			}
			break;
		}
		case WM_COMMAND:
			OnCommand(wParam, lParam);
			switch (wParam)
        {
            case DOCK_MSG:
                ToggleDocking();
                break;
            case IDCANCEL:
            case IDOK:
                m_bUserClosed = true;
                DestroyWindow(m_hwnd);
                break;
        }
			break;
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				static bool bRecurseCheck = false;
				if (!bRecurseCheck)
				{
					bRecurseCheck = true;
					KillTooltip();
					OnResize();
					m_resize.onResize();
					bRecurseCheck = false;
				}
			}
			break;
            // define a min size (+ fix flickering when docked)
		case WM_GETMINMAXINFO:
			if (lParam)
			{
				int w, h;
				GetMinSize(&w, &h);
				if (!IsDocked())
				{
					RECT rClient, rWnd;
					GetClientRect(m_hwnd, &rClient);
					GetWindowRect(m_hwnd, &rWnd);
					w += (rWnd.right - rWnd.left) - rClient.right;
					h += (rWnd.bottom - rWnd.top) - rClient.bottom;
				}
				LPMINMAXINFO l = (LPMINMAXINFO)lParam;
				l->ptMinTrackSize.x = w;
				l->ptMinTrackSize.y = h;
			}
			break;
		case WM_DROPFILES:
			OnDroppedFiles((HDROP)wParam);
			break;
		case WM_DESTROY:
		{
			KillTimer(m_hwnd, CELL_EDIT_TIMER);
			KillTooltip();
            
			OnDestroy();
            
			m_parentVwnd.RemoveAllChildren(false);
			m_parentVwnd.SetRealParent(NULL);
            
            
			if (m_bSaveStateOnDestroy)
			{
				char cState[4096]=""; // SDK: "will usually be 4k or greater"
				int iLen = SaveState(cState, sizeof(cState));
				if (iLen>0)
					WritePrivateProfileStruct(ULTRASCHALL_INI, m_id.Get(), cState, iLen, get_ini_file());
			}
			m_bUserClosed = false;
			m_bSaveStateOnDestroy = true;
            
			DockWindowRemove(m_hwnd); // Always safe to call even if the window isn't docked
		}
#ifdef _WIN32
			break;
		case WM_NCDESTROY:
#endif
			m_hwnd = NULL;
			RefreshToolbar(m_iCmdID);
			break;
		case WM_PAINT:
			if (!OnPaint() && m_parentVwnd.GetNumChildren())
			{
				int xo, yo; RECT r;
				GetClientRect(m_hwnd,&r);
				m_parentVwnd.SetPosition(&r);
				m_vwnd_painter.PaintBegin(m_hwnd, WDL_STYLE_GetSysColor(COLOR_WINDOW));
				if (LICE_IBitmap* bm = m_vwnd_painter.GetBuffer(&xo, &yo))
				{
					bm->resize(r.right-r.left,r.bottom-r.top);
                    
					int x=0;
					while (WDL_VWnd* w = m_parentVwnd.EnumChildren(x++))
						w->SetVisible(false); // just a setter, no redraw
                    
					int h=0;
					DrawControls(bm, &r, &h);
					m_vwnd_painter.PaintVirtWnd(&m_parentVwnd);
                    
					if (*m_tooltip)
					{
						if (!(*(int*)GetConfigVar("tooltips")&2)) // obeys the "Tooltip for UI elements" pref
						{
							POINT p = { m_tooltip_pt.x + xo, m_tooltip_pt.y + yo };
							RECT rr = { r.left+xo,r.top+yo,r.right+xo,r.bottom+yo };
							if (h>0) rr.bottom = h+yo; //JFB make sure some tooltips are not hidden (could be better but it's enough ATM..)
							DrawTooltipForPoint(bm,p,&rr,m_tooltip);
						}
						Help_Set(m_tooltip, true);
					}
				}
				m_vwnd_painter.PaintEnd();
			}
			break;
		case WM_LBUTTONDBLCLK:
			if (!OnMouseDblClick(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) &&
				!m_parentVwnd.OnMouseDblClick(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) &&
				m_parentVwnd.OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) > 0)
			{
				m_parentVwnd.OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			}
			break;
		case WM_LBUTTONDOWN:
			KillTooltip(true);
			SetFocus(m_hwnd);
			if (OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) > 0 ||
				m_parentVwnd.OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) > 0)
			{
				SetCapture(m_hwnd);
			}
			break;
		case WM_LBUTTONUP:
			if (GetCapture() == m_hwnd)
			{
				if (!OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)))
				{
					m_parentVwnd.OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				}
				ReleaseCapture();
			}
			KillTooltip(true);
			break;
		case WM_MOUSEMOVE:
			m_parentVwnd.OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)); // no capture test (hover, etc..)
			if (GetCapture() == m_hwnd)
			{
				OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			}
			else
			{
				KillTooltip(true);
				SetTimer(m_hwnd, TOOLTIP_TIMER, TOOLTIP_TIMEOUT, NULL);
#ifdef _WIN32
				// request WM_MOUSELEAVE message
				TRACKMOUSEEVENT e = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hwnd, HOVER_DEFAULT };
				TrackMouseEvent(&e);
#endif
			}
			break;
#ifdef _WIN32
            // fixes possible stuck tooltips and VWnds stuck on hover state
		case WM_MOUSELEAVE:
			KillTooltip(true);
			m_parentVwnd.OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			break;
#endif
		case WM_CTLCOLOREDIT:
			if (SWS_THEMING)
			{
				// color override for list views' cell edition
				HWND hwnd = (HWND)lParam;
				return SendMessage(GetMainHwnd(),uMsg,wParam,lParam);
			}
			return 0;
		default:
			return OnUnhandledMsg(uMsg, wParam, lParam);
	}
	return 0;
}

void DockWindow::ToggleDocking()
{
	if (!IsDocked())
		GetWindowRect(m_hwnd, &m_state.r);
    
	m_bSaveStateOnDestroy = false;
	DestroyWindow(m_hwnd);
    
	m_state.state ^= 2;
	Show(false, true);
}

// screenset support
LRESULT DockWindow::screensetCallback(int action, char *id, void *param, void *actionParm, int actionParmSize)
{
	if (DockWindow* pObj = (DockWindow*)param)
	{
		switch(action)
		{
			case SCREENSET_ACTION_GETHWND:
				return (LRESULT)pObj->m_hwnd;
			case SCREENSET_ACTION_IS_DOCKED:
				return (LRESULT)pObj->IsDocked();
			case SCREENSET_ACTION_SWITCH_DOCK:
				if (SWS_IsWindow(pObj->m_hwnd))
					pObj->ToggleDocking();
				break;
			case SCREENSET_ACTION_LOAD_STATE:
				pObj->LoadState((char*)actionParm, actionParmSize);
				break;
			case SCREENSET_ACTION_SAVE_STATE:
				return pObj->SaveState((char*)actionParm, actionParmSize);
		}
	}
	return 0;
}

int DockWindow::keyHandler(MSG* msg, accelerator_register_t* ctx)
{
	DockWindow* p = (DockWindow*)ctx->user;
	if (p && p->IsActive(true))
	{
		// Check the derived class key handler next in case they want to override anything
		int iKeys = SWS_GetModifiers();
		int iRet = p->OnKey(msg, iKeys);
		if (iRet)
			return iRet;
        
		// We don't want the key, but this window has the focus.
		// Therefore, force it to main reaper wnd (passthrough) so that main wnd actions work!
		return -666;
	}
	return 0;
}

// *Local* function to save the state of the view.  This is the window size, position & dock state,
// as well as any derived class view information from the function SaveView
int DockWindow::SaveState(char* cStateBuf, int iMaxLen)
{
	if (SWS_IsWindow(m_hwnd))
	{
		int dockIdx = DockIsChildOfDock(m_hwnd, NULL);
		if (dockIdx>=0)
			m_state.whichdock = dockIdx;
		else
			GetWindowRect(m_hwnd, &m_state.r);
	}
    
	if (!m_bUserClosed && SWS_IsWindow(m_hwnd))
		m_state.state |= 1;
	else
		m_state.state &= ~1;
    
	int iLen = sizeof(SWS_DockWnd_State);
	if (cStateBuf)
	{
		memcpy(cStateBuf, &m_state, iLen);
		iLen += SaveView(cStateBuf + iLen, iMaxLen - iLen);
        
		for (int i = 0; i < iLen / (int)sizeof(int); i++)
			REAPER_MAKELEINTMEM(&(((int*)cStateBuf)[i]));
	}
	else
		iLen += SaveView(NULL, 0);
    
	return iLen;
}

// *Local* function to restore view state.  This is the window size, position & dock state.
// Also calls the derived class method LoadView.
// if both _stateBuf and _len are NULL, hide (see SDK)
void DockWindow::LoadState(const char* cStateBuf, int iLen)
{
	bool bDocked = IsDocked();
    
	if (cStateBuf && iLen>=sizeof(SWS_DockWnd_State))
		SWS_SetDockWndState(cStateBuf, iLen, &m_state);
	else
		m_state.state &= ~1;
    
	if (iLen > sizeof(SWS_DockWnd_State))
	{
		int iViewLen = iLen - sizeof(SWS_DockWnd_State);
		char* pTemp = new char[iViewLen];
		memcpy(pTemp, cStateBuf + sizeof(SWS_DockWnd_State), iViewLen);
		LoadView(pTemp, iViewLen);
		delete [] pTemp;
	}
    
	Dock_UpdateDockID((char*)m_id.Get(), m_state.whichdock);
    
	if (m_state.state & 1)
	{
		if (SWS_IsWindow(m_hwnd) &&
			((bDocked != ((m_state.state & 2) == 2)) ||
             (bDocked && DockIsChildOfDock(m_hwnd, NULL) != m_state.whichdock)))
		{
			// If the window's already open, but the dock state or docker # has changed,
			// destroy and reopen.
			m_bSaveStateOnDestroy = false;
			DestroyWindow(m_hwnd);
		}
		Show(false, false);
        
		RECT r;
		GetWindowRect(m_hwnd, &r);
        
		// TRP 4/29/13 - Also set wnd pos/size if it's changed.
		if (!bDocked && memcmp(&r, &m_state.r, sizeof(RECT)) != 0)
			SetWindowPos(m_hwnd, NULL, m_state.r.left, m_state.r.top, m_state.r.right-m_state.r.left, m_state.r.bottom-m_state.r.top, SWP_NOZORDER);
	}
	else if (SWS_IsWindow(m_hwnd))
	{
		m_bUserClosed = true;
		DestroyWindow(m_hwnd);
	}
}

void DockWindow::KillTooltip(bool doRefresh)
{
	KillTimer(m_hwnd, TOOLTIP_TIMER);
	bool had=!!m_tooltip[0];
	*m_tooltip='\0';
	if (had && doRefresh)
		InvalidateRect(m_hwnd,NULL,FALSE);
}

// it's up to the caller to unalloc the returned value
char* SWS_LoadDockWndStateBuf(const char* _id, int _len)
{
	if (_len<=0) _len = sizeof(SWS_DockWnd_State);
	char* state = new char[_len];
	memset(state, 0, _len);
	GetPrivateProfileStruct(ULTRASCHALL_INI, _id, state, _len, get_ini_file());
	return state;
}

// if _stateBuf==NULL, read state from ini file
int SWS_GetDockWndState(const char* _id, const char* _stateBuf)
{
	SWS_DockWnd_State state;
	memset(&state, 0, sizeof(SWS_DockWnd_State));
	if (_stateBuf)
	{
		SWS_SetDockWndState(_stateBuf, sizeof(SWS_DockWnd_State), &state);
	}
	else
	{
		char* stateBuf = SWS_LoadDockWndStateBuf(_id);
		SWS_SetDockWndState(stateBuf, sizeof(SWS_DockWnd_State), &state);
		delete [] stateBuf;
	}
	return state.state;
}

void SWS_SetDockWndState(const char* _stateBuf, int _len, SWS_DockWnd_State* _state)
{
	if (_state && _stateBuf && _len>=sizeof(SWS_DockWnd_State))
		for (int i=0; i < _len / (int)sizeof(int); i++)
			((int*)_state)[i] = REAPER_MAKELEINT(*((int*)_stateBuf+i));
}

void DrawTooltipForPoint(LICE_IBitmap *bm, POINT mousePt, RECT *wndr, const char *text)
{
    if (!bm || !text || !text[0])
        return;
    
    static LICE_CachedFont tmpfont;
    if (!tmpfont.GetHFont())
    {
        //JFB mod: font size/name + optional ClearType rendering --->
        /*
         bool doOutLine = true;
         LOGFONT lf =
         {
         14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,
         #ifdef _WIN32
         "MS Shell Dlg"
         #else
         "Arial"
         #endif
         };
         tmpfont.SetFromHFont(CreateFontIndirect(&lf),LICE_FONT_FLAG_OWNS_HFONT);
         */
        LOGFONT lf = {SNM_FONT_HEIGHT,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,SNM_FONT_NAME
        };
#ifndef _SNM_SWELL_ISSUES
        tmpfont.SetFromHFont(CreateFontIndirect(&lf),LICE_FONT_FLAG_OWNS_HFONT|(g_SNM_ClearType?LICE_FONT_FLAG_FORCE_NATIVE:0));
#else
        tmpfont.SetFromHFont(CreateFontIndirect(&lf),LICE_FONT_FLAG_OWNS_HFONT); // SWELL issue: native font rendering won't draw multiple lines
#endif
        //JFB <---
    }
    tmpfont.SetBkMode(TRANSPARENT);
    LICE_pixel col1 = LICE_RGBA(0,0,0,255);
    //JFB mod: same tooltip color than REAPER --->
    //    LICE_pixel col2 = LICE_RGBA(255,255,192,255);
    LICE_pixel col2 = LICE_RGBA(255,255,225,255);
    // <---
    
    tmpfont.SetTextColor(col1);
    RECT r={0,};
    tmpfont.DrawText(bm,text,-1,&r,DT_CALCRECT);
    
    int xo = min(max(mousePt.x,wndr->left),wndr->right);
    int yo = min(max(mousePt.y + 24,wndr->top),wndr->bottom);
    
    if (yo + r.bottom > wndr->bottom-4) // too close to bottom, move up if possible
    {
        if (mousePt.y - r.bottom - 12 >= wndr->top)
            yo = mousePt.y - r.bottom - 12;
        else
            yo = wndr->bottom - 4 - r.bottom;
        
        //JFB added: (try to) prevent hidden tooltip behind the mouse pointer --->
        xo += 15;
        // <---
    }
    
    if (xo + r.right > wndr->right - 4)
        xo = wndr->right - 4 - r.right;
    
    r.left += xo;
    r.top += yo;
    r.right += xo;
    r.bottom += yo;
    
    int border = 3;
    LICE_FillRect(bm,r.left-border,r.top-border,r.right-r.left+border*2,r.bottom-r.top+border*2,col2,1.0f,LICE_BLIT_MODE_COPY);
    LICE_DrawRect(bm,r.left-border,r.top-border,r.right-r.left+border*2,r.bottom-r.top+border*2,col1,1.0f,LICE_BLIT_MODE_COPY);
    
    tmpfont.DrawText(bm,text,-1,&r,0);
}