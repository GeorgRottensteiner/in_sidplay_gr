#include "HTMLControl.h"
#include "../gen_ml/ml_ipc_0313.h"
#include "../Winamp/wa_dlg.h"
#include "main.h"
#include <map>
#include <mshtml.h>
#include <winbase.h>
#include "resource.h"
#include <strsafe.h>

HANDLE threadEvent  = 0;
extern winampMediaLibraryPlugin wRemote;
HTMLControl *htmlControl = 0;

static long activeTree = 0;
extern DWORD threadStorage;

HWND g_mainHWND=NULL;

#define ID_FILE_SHOWLIBRARY             40379
HANDLE ieThread=0;

#define IDC_EMBEDDEDBROWSER	0x1000

#define	WNDPROP_SCCTRLW		L"SCCTRL"

static bool bShowOnNavigate = true;

static void GoToPage(int TreeID)
{
	HTMLControl *thisControl = (HTMLControl *)TlsGetValue(threadStorage);

	thisControl->NavigateToName("http://www.winamp.com/media/music");

}

/* quick function to change a COLORREF into something we can use in sprintf("#%06X") */
COLORREF GetHTMLColor(int color)
{
	return ((color >> 16)&0xff|(color&0xff00)|((color<<16)&0xff0000));
}

static void OnSkinChanged(HTMLControl *pCtrl)
{
	wchar_t css[128];
	if (!pCtrl) return;
	if (!ml_color) return; // make sure we have the function to get colors first!
	return;  // lets not do this for now...
	if (S_OK ==StringCchPrintfW(css, 128, L"BODY { background-color: #%06X; color:#%06X }",
	                            GetHTMLColor(ml_color(WADLG_ITEMBG)),
	                            GetHTMLColor(ml_color(WADLG_ITEMFG))))
	{
		pCtrl->SetHostCSS(css);
	}
}
static void OnBrowserSize(HTMLControl *pCtrl)
{
	RECT rc;
	if (pCtrl && pCtrl->m_hwnd && GetClientRect(pCtrl->m_hwnd, &rc))
	{
		pCtrl->setLocation(rc.left, rc.top + 2, rc.right - rc.left - 3, rc.bottom - rc.top - 3);
		InvalidateRect(pCtrl->m_hwnd, NULL, TRUE);
	}
}
void HTMLControl::OnNavigateComplete()
{
	if (bShowOnNavigate)
	{
		HWND hwndLock, hwndHost;
		bShowOnNavigate = false;
		hwndLock = m_hwnd;
		if (hwndLock) SendMessage(hwndLock, WM_SETREDRAW, FALSE, 0L);

		setVisible(TRUE);
		OnBrowserSize(this);

		hwndHost = GetHostHWND();
		if (hwndHost) SetWindowLongPtr(hwndHost, GWLP_ID, IDC_EMBEDDEDBROWSER);

		if (hwndLock)
		{
			SendMessage(hwndLock, WM_SETREDRAW, TRUE, 0L);
			InvalidateRect(hwndLock, NULL, TRUE);
		}
	}
}

static LRESULT HostWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC fnOldProc;
	fnOldProc = (WNDPROC)GetPropW(hwnd, WNDPROP_SCCTRLW);
	if (!fnOldProc) return DefWindowProcW(hwnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_NCDESTROY: // detach
		RemovePropW(hwnd, WNDPROP_SCCTRLW);
		CallWindowProcW(fnOldProc, hwnd, uMsg, wParam, lParam);
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)fnOldProc);
		return 0;
	case WM_ERASEBKGND:
		if (wParam)
		{
			RECT rc;
			if (GetClientRect(hwnd, &rc))
			{
				SetBkColor((HDC)wParam, ml_color(WADLG_WNDBG));
				ExtTextOutW((HDC)wParam, 0, 0, ETO_OPAQUE, &rc, L"", 0, 0);
			}
		}
		return 1;
	case WM_SETFOCUS:
		if (htmlControl)
		{
			htmlControl->setFocus(TRUE); return 0;
		}
		break;
	}

	return CallWindowProcW(fnOldProc, hwnd, uMsg, wParam, lParam);
}

static BOOL SubclassHost(HWND hwndHost)
{
	if (!hwndHost || !IsWindow(hwndHost)) return FALSE;
	WNDPROC fnOldProc;
	fnOldProc = (WNDPROC)SetWindowLongPtrW(hwndHost, GWLP_WNDPROC, (LONG_PTR)HostWndProc);
	if (!fnOldProc || !SetPropW(hwndHost, WNDPROP_SCCTRLW, fnOldProc))
	{
		if (fnOldProc) SetWindowLongPtrW(hwndHost, GWLP_WNDPROC, (LONG_PTR)fnOldProc);
		return FALSE;
	}
	return TRUE;
}


static DWORD CALLBACK IEThread(LPVOID param)
{
	HWND parent, hwndHost;

	parent = (HWND)param;
	OleInitialize(0);
	bShowOnNavigate = true;

	HTMLControl *thisControl = new HTMLControl();
	htmlControl= thisControl;
	TlsSetValue(threadStorage, thisControl);
	thisControl->CreateHWND(parent);
	SetEvent(threadEvent);

	OnSkinChanged(thisControl);

	hwndHost = thisControl->GetHostHWND();
	if (hwndHost)
	{
		SetWindowPos(hwndHost, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		SubclassHost(hwndHost);
	}


	while (1)
	{
		DWORD dwStatus = MsgWaitForMultipleObjectsEx(0, NULL,
		                 INFINITE, QS_ALLINPUT,
		                 MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
		if (dwStatus == WAIT_OBJECT_0)
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					OleUninitialize();
					return 0;
				}
				else if (msg.message == WM_USER+1111)
				{
					if (thisControl->m_pweb)
						thisControl->m_pweb->Stop();
					thisControl->remove();
					thisControl->close();
					thisControl->Release();
					thisControl=0;
					continue;
				}

				if (WM_KEYFIRST > msg.message || WM_KEYLAST < msg.message || !thisControl || !thisControl->translateKey(&msg))
				{
					if (!IsDialogMessage(parent, &msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
		}
	}
}


static VOID CALLBACK KillAPC(ULONG_PTR param)
{
	HTMLControl *thisControl = (HTMLControl *)TlsGetValue(threadStorage);
	if (thisControl)
	{
	}

}

static VOID CALLBACK GoToPageURL(ULONG_PTR param)
{
	HTMLControl *thisControl = (HTMLControl *)TlsGetValue(threadStorage);
	if (thisControl)
	{
		GoToPage(param);
		thisControl->Release();
	}
}

VOID CALLBACK OnBrowserSizeAPC(ULONG_PTR param)
{
	HTMLControl *thisControl = (HTMLControl *)TlsGetValue(threadStorage);
	if (thisControl) OnBrowserSize(thisControl);
}
DWORD threadId;

static VOID CALLBACK OnSkinChangedAPC(ULONG_PTR param)
{
	HTMLControl *thisControl = (HTMLControl *)TlsGetValue(threadStorage);
	if (thisControl)
	{
		OnSkinChanged(thisControl);
		if (thisControl->m_pweb)
		{
			// if you want to update page uncomment this
			//	thisControl->m_pweb->Stop();
			//	thisControl->m_pweb->Refresh();
		}
		thisControl->Release();
	}
}

INT_PTR CALLBACK MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
		/* first, ask the dialog skinning system if it wants to do anything with the message 
	   the function pointer gets set during WM_INITDIALOG so might be NULL for the first few messages
		 in theory we could grab it right here if we don't have it, but it's not necessary
		 and I wanted to put all the function pointer gathering code in the same place for this example	*/
	if (ml_hook_dialog_msg) 
	{
		INT_PTR a = ml_hook_dialog_msg(hwndDlg, uMsg, wParam, lParam);
		if (a)
			return a;
	}


	switch (uMsg)
	{
	case WM_INITDIALOG:

			/* skin dialog */
	MLSKINWINDOW sw;
	sw.skinType = SKINNEDWND_TYPE_DIALOG;
	sw.style = SWS_USESKINCOLORS | SWS_USESKINCURSORS | SWS_USESKINFONT;
	sw.hwndToSkin = hwndDlg;
	MLSkinWindow(SampleHTTP.hwndLibraryParent, &sw);


		g_mainHWND = hwndDlg;
		activeTree = lParam;

		threadEvent = CreateEvent(0, TRUE, FALSE, 0);

		ieThread = CreateThread(NULL, 0, IEThread, (LPVOID)hwndDlg, 0, &threadId);
		WaitForSingleObject(threadEvent, INFINITE);
		htmlControl->AddRef();
		QueueUserAPC(GoToPageURL, ieThread, (LONG_PTR)activeTree);
		CloseHandle(threadEvent);
		return FALSE;
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
//			childSizer.Resize(hwndDlg, discover_rlist, sizeof(discover_rlist) / sizeof(discover_rlist[0]));
			QueueUserAPC(OnBrowserSizeAPC, ieThread, (ULONG_PTR)0);
		}
		break;

	case WM_DISPLAYCHANGE:
		if (ieThread)
		{
			htmlControl->AddRef();
			QueueUserAPC(OnSkinChangedAPC, ieThread, 0);
		}
		break;
	case WM_ERASEBKGND:
		SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
		return 1;
	case WM_PAINT:
	{
		static int tab[] = { IDC_EMBEDDEDBROWSER | DCW_SUNKENBORDER };
		ml_draw(hwndDlg, tab, sizeof(tab) / sizeof(tab[0]));
	}
	return 0;
	case WM_DESTROY:
		htmlControl = 0;
		activeTree = 0;
		PostThreadMessage(threadId, WM_USER+1111, 0, 0);
		CloseHandle(ieThread);
		ieThread =0;
		break;

	}
	return 0;
}
