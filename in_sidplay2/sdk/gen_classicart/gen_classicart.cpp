/* gen_classicart
  version 0.2, February 27th, 2008

  Copyright (C) 2008 Will Fisher

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Will Fisher will.fisher@gmail.com

*/

#include <windows.h>
#include <shlwapi.h>
#include "../winamp/gen.h"
#include "../winamp/wa_ipc.h"
#define WA_DLG_IMPLEMENT
#include "../winamp/wa_dlg.h"
#include "api.h"
#include "resource.h"

#define PLUGIN_NAME "Album Art Viewer v0.2"

int init();
void quit();
void config();

winampGeneralPurposePlugin plugin = {GPPHDR_VER,PLUGIN_NAME,init,config,quit};
extern "C" __declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {return &plugin;}

static api_memmgr* WASABI_API_MEMMGR;
static api_service* WASABI_API_SVC;
static api_albumart* AGAVE_API_ALBUMART;

#define WINAMP_ARTVIEW_MENUID 0xa1ba
#define WINAMP_ARTVIEW_TEXT L"Album Art"

embedWindowState myWndState={0};
HWND myWnd=NULL, myWndChild=NULL;
HMENU menu, context_menu;
WNDPROC oldWndProc;
BOOL fUnicode;
char * INI_FILE;
static INT_PTR CALLBACK art_dlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void showArtWindow();
void hideArtWindow();

int artwindow_open = 0;
int lockAspect=1;
int autoHide=0;

// {8B9052B2-2782-4ac8-BA8E-E3DEDBF0BDB5}
static const GUID ArtViewerGUID = 
{ 0x8b9052b2, 0x2782, 0x4ac8, { 0xba, 0x8e, 0xe3, 0xde, 0xdb, 0xf0, 0xbd, 0xb5 } };

int init() {
	WASABI_API_SVC = (api_service*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
	if(WASABI_API_SVC == (api_service*)1 || WASABI_API_SVC == NULL) return 1;

	INI_FILE = (char*)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETINIFILE);

	waServiceFactory *sf = WASABI_API_SVC->service_getServiceByGuid(memMgrApiServiceGuid);
	WASABI_API_MEMMGR = reinterpret_cast<api_memmgr *>(sf->getInterface());

	sf = WASABI_API_SVC->service_getServiceByGuid(albumArtGUID);
	AGAVE_API_ALBUMART = reinterpret_cast<api_albumart *>(sf->getInterface());
	

	WADlg_init(plugin.hwndParent);

	// subclass main window
	fUnicode = IsWindowUnicode(plugin.hwndParent);
	oldWndProc = (WNDPROC) ((fUnicode) ? SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc) : 
											SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)SubclassProc));

	// add our menu option
	menu = (HMENU)SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)0,IPC_GET_HMENU);
	MENUITEMINFO i = {sizeof(i), MIIM_ID | MIIM_STATE | MIIM_TYPE, MFT_STRING, MFS_UNCHECKED, WINAMP_ARTVIEW_MENUID};
	i.dwTypeData = WINAMP_ARTVIEW_TEXT;
	InsertMenuItem(menu, 10 + (int)SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)0,IPC_ADJUST_OPTIONSMENUPOS), TRUE, &i);
	SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)1,IPC_ADJUST_OPTIONSMENUPOS);

	// load values from ini file
	lockAspect = GetPrivateProfileIntA("gen_classicart","wnd_lock_aspect",lockAspect,INI_FILE);
	myWndState.r.top = GetPrivateProfileIntA("gen_classicart","wnd_top",0,INI_FILE);
	myWndState.r.bottom = GetPrivateProfileIntA("gen_classicart","wnd_bottom",0,INI_FILE);
	myWndState.r.left = GetPrivateProfileIntA("gen_classicart","wnd_left",0,INI_FILE);
	myWndState.r.right = GetPrivateProfileIntA("gen_classicart","wnd_right",0,INI_FILE);
	artwindow_open = GetPrivateProfileIntA("gen_classicart","wnd_open",artwindow_open,INI_FILE);
	autoHide = GetPrivateProfileIntA("gen_classicart","wnd_auto_hide",autoHide,INI_FILE);
	
	// create window
	myWndState.flags = EMBED_FLAGS_NOWINDOWMENU;
	myWnd = (HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)&myWndState,IPC_GET_EMBEDIF);
	myWndChild = CreateDialogParam(plugin.hDllInstance,MAKEINTRESOURCE(IDD_DIALOG),myWnd,art_dlgproc,0);
	SET_EMBED_GUID((&myWndState),ArtViewerGUID);
	SetWindowText(myWnd,WINAMP_ARTVIEW_TEXT);
	if(artwindow_open) showArtWindow();

	context_menu = LoadMenu(plugin.hDllInstance,MAKEINTRESOURCE(IDR_MENU1));
	return 0;
}

void quit() {
	if(fUnicode) SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
	else SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

	// save window state
#define WritePrivateProfileInt(app, key, val, ini) { char zzval[10]; wsprintfA(zzval,"%d",val); WritePrivateProfileStringA(app,key,zzval,ini); }
	GetWindowRect(myWnd,&myWndState.r);
	WritePrivateProfileInt("gen_classicart","wnd_top",myWndState.r.top,INI_FILE);
	WritePrivateProfileInt("gen_classicart","wnd_bottom",myWndState.r.bottom,INI_FILE);
	WritePrivateProfileInt("gen_classicart","wnd_left",myWndState.r.left,INI_FILE);
	WritePrivateProfileInt("gen_classicart","wnd_right",myWndState.r.right,INI_FILE);
	WritePrivateProfileInt("gen_classicart","wnd_open",artwindow_open,INI_FILE);
	WritePrivateProfileInt("gen_classicart","wnd_lock_aspect",lockAspect,INI_FILE);
	WritePrivateProfileInt("gen_classicart","wnd_auto_hide",autoHide,INI_FILE);
	DestroyWindow(myWnd);
	WADlg_close();
}

void config() {
	MessageBoxA(plugin.hwndParent,PLUGIN_NAME "\nBy Will Fisher, (C) 2008",PLUGIN_NAME,0);
}

static LRESULT WINAPI SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_COMMAND && LOWORD(wParam) == WINAMP_ARTVIEW_MENUID)
	{
		if(artwindow_open) PostMessage(myWndChild,WM_CLOSE,0,0);
		else PostMessage(myWndChild,WM_USER+1,0,0);
	}
	else if(msg == WM_WA_IPC && lParam == IPC_CB_MISC && (wParam == IPC_CB_MISC_TITLE || wParam == IPC_CB_MISC_TITLE_RATING))
	{
		// art change
		PostMessage(myWndChild,WM_USER,0,0);
	}
	return (fUnicode) ? CallWindowProcW(oldWndProc, hwnd, msg, wParam, lParam) : CallWindowProcA(oldWndProc, hwnd, msg, wParam, lParam);
}

ARGB32 * loadImg(const void * data, int len, int *w, int *h, bool ldata=false) {
	FOURCC imgload = svc_imageLoader::getServiceType();
	int n = WASABI_API_SVC->service_getNumServices(imgload);
	for(int i=0; i<n; i++) {
		waServiceFactory *sf = WASABI_API_SVC->service_enumService(imgload,i);
		if(sf) {
			svc_imageLoader * l = (svc_imageLoader*)sf->getInterface();
			if(l) {
				if(l->testData(data,len)) {
					ARGB32* ret;
					if(ldata) ret = l->loadImageData(data,len,w,h);
					else ret = l->loadImage(data,len,w,h);
					sf->releaseInterface(l);
					return ret;
				}
				sf->releaseInterface(l);
			}
		}
	}
	return NULL;
}

ARGB32 * loadRrc(int id, char * sec, int *w, int *h, bool data=false)
{
	DWORD size = 0;
	
	HGLOBAL resourceHandle=NULL;
	HRSRC rsrc = FindResourceA(plugin.hDllInstance, MAKEINTRESOURCEA(id), sec);
	if(rsrc)
	{
		resourceHandle = LoadResource(plugin.hDllInstance, rsrc);
		size = SizeofResource(plugin.hDllInstance, rsrc);
		resourceHandle = LockResource(resourceHandle);
	}

	//HGLOBAL resourceHandle = WASABI_API_LOADRESFROMFILE((LPCTSTR)sec, (LPCTSTR)MAKEINTRESOURCEA(id), &size);
	if(resourceHandle)
	{
		ARGB32* ret = loadImg(resourceHandle,size,w,h,data);
		UnlockResource(resourceHandle);
		return ret;
	}
	return NULL;
}

void adjustbmp(ARGB32 * p, int len, COLORREF fg)
{
	ARGB32 * end = p+len;
	while (p < end)
	{
		int a = (*p>>24)&0xff ;
		int b = a*((*p&0xff) * (fg&0xff)) / (0xff*0xff);
		int g = a*(((*p>>8)&0xff) * ((fg>>8)&0xff)) / (0xff*0xff);
		int r = a*(((*p>>16)&0xff) * ((fg>>16)&0xff)) / (0xff*0xff);
		*p = (a<<24) | (r&0xff) | ((g&0xff)<<8) | ((b&0xff)<<16);
		p++;
	}
}

RECT lastWnd={0};
HDC cacheDC = NULL;
COLORREF bgcolour = RGB(0,0,0);

void DrawArt(HDC dc, HWND hwndDlg, ARGB32 * cur_image, int cur_w, int cur_h)
{
	RECT dst, wnd;
	GetWindowRect(hwndDlg,&wnd);
	wnd.right = wnd.right - wnd.left;
	wnd.left = 0;
	wnd.bottom = wnd.bottom - wnd.top;
	wnd.top = 0;

	if(!memcmp(&lastWnd,&wnd,sizeof(RECT)) && cacheDC) {
		BitBlt(dc,0,0,wnd.right,wnd.bottom,cacheDC,0,0,SRCCOPY);
		return;
	}

	// create cacheDC
	if(cacheDC) DeleteDC(cacheDC);
	cacheDC = CreateCompatibleDC(dc);
	HBITMAP hbm = CreateCompatibleBitmap(dc,wnd.right,wnd.bottom);
	SelectObject(cacheDC,hbm);
	DeleteObject(hbm);
	lastWnd = wnd;

	if(!lockAspect) dst = wnd;
	else 
	{
		// maintain 'square' stretching, fill in dst
		double aspX = (double)(wnd.right)/(double)cur_w;
		double aspY = (double)(wnd.bottom)/(double)cur_h;
		double asp = min(aspX, aspY);
		int newW = (int)(cur_w*asp);
		int newH = (int)(cur_h*asp);
		dst.left = (wnd.right - newW)/2;
		dst.top = (wnd.bottom - newH)/2;
		dst.right = dst.left + newW;
		dst.bottom = dst.top + newH;
	}

	// fill the background to black
	HBRUSH brush = CreateSolidBrush(bgcolour);
	FillRect(cacheDC,&wnd,brush);
	DeleteObject(brush);

	//SkinBitmap(cur_image, cur_w, cur_h).stretchToRect(&DCCanvas(cacheDC), &dst);
	HDC srcDC = CreateCompatibleDC(dc);
		BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof bmi);
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = cur_w;
	bmi.bmiHeader.biHeight = -cur_h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	void *bits = 0;
	HBITMAP srcBMP = CreateDIBSection(srcDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
	memcpy(bits, cur_image, cur_w*cur_h*4);
	
	HBITMAP oldSrcBM = (HBITMAP)SelectObject(srcDC,srcBMP);
	
			BLENDFUNCTION blendFn;
		blendFn.BlendOp = AC_SRC_OVER;
		blendFn.BlendFlags  = 0;
		blendFn.SourceConstantAlpha  = 255;
		blendFn.AlphaFormat = AC_SRC_ALPHA;

		AlphaBlend(cacheDC,
		           dst.left, dst.top,
		           dst.right-dst.left, dst.bottom-dst.top,
		           srcDC,
		           0, 0,
		           cur_w, cur_h,
		           blendFn);		

	BitBlt(dc,0,0,wnd.right,wnd.bottom,cacheDC,0,0,SRCCOPY);
	SelectObject(srcDC,oldSrcBM);
	DeleteObject(srcBMP);
	DeleteDC(srcDC);
}

static INT_PTR CALLBACK art_dlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static ARGB32 * cur_image;
	static int cur_w, cur_h;
	static bool closed;
	switch(msg)
	{
	case WM_INITDIALOG:
		closed = 0;
		cur_image = 0;
		PostMessage(hwndDlg,WM_USER,0,0);
		break;
	case WM_USER+1:
		showArtWindow();
		closed=0;
		break;
	case WM_DISPLAYCHANGE:
		WADlg_init(plugin.hwndParent);
	case WM_USER:
		{
			wchar_t *filename = (wchar_t *)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_PLAYING_FILENAME);
			if(cur_image) WASABI_API_MEMMGR->sysFree(cur_image); cur_image = 0;
			if (AGAVE_API_ALBUMART->GetAlbumArt(filename, L"cover", &cur_w, &cur_h, &cur_image) != ALBUMART_SUCCESS)
			{/*
				SkinBitmap b(L"winamp.cover.notfound");
				if(!b.isInvalid())
				{
					cur_w = b.getWidth();
					cur_h = b.getHeight();
					cur_image = (ARGB32*)WASABI_API_MEMMGR->sysMalloc(cur_w * cur_h * sizeof(ARGB32));
					memcpy(cur_image,b.getBits(),cur_w * cur_h * sizeof(ARGB32));
				}
				else*/
				{
					cur_image = loadRrc(IDR_IMAGE_NOTFOUND,"PNG",&cur_w, &cur_h,true);
					if(cur_image) adjustbmp(cur_image, cur_w*cur_h, WADlg_getColor(WADLG_ITEMFG));
				}

				bgcolour = WADlg_getColor(WADLG_ITEMBG);
				if(autoHide && !closed && msg != WM_DISPLAYCHANGE)
					hideArtWindow();
			}
			else
			{
				bgcolour = RGB(0,0,0);
				if(autoHide && !closed && msg != WM_DISPLAYCHANGE)
					showArtWindow();
			}
			if(cacheDC) DeleteDC(cacheDC); cacheDC = NULL;
			InvalidateRect(hwndDlg,NULL,TRUE);
		}
		break;
	case WM_DESTROY:
		if(cur_image) WASABI_API_MEMMGR->sysFree(cur_image); cur_image = 0;
		if(cacheDC) DeleteDC(cacheDC); cacheDC = NULL;
		break;
	case WM_PAINT:
		{
			if (cur_image)
			{
				PAINTSTRUCT psPaint={0};
				HDC dc = BeginPaint(hwndDlg, &psPaint);
				DrawArt(dc,hwndDlg,cur_image,cur_w,cur_h);
				EndPaint(hwndDlg, &psPaint);
			}
		}
		break;
	case WM_ERASEBKGND:
		{
			if (cur_image)
			{
				HDC dc = (HDC)wParam;
				DrawArt(dc,hwndDlg,cur_image,cur_w,cur_h);
				return 1;
			}
		}
		break;
	case WM_SIZE:
		if(cacheDC) DeleteDC(cacheDC); cacheDC = NULL;
		InvalidateRect(hwndDlg,NULL,TRUE);
		break;
	case WM_CLOSE:
		closed=1;
		hideArtWindow();
		break;
	case WM_RBUTTONDOWN:
		{
			HMENU menu = GetSubMenu(context_menu,0);
			POINT p;
			GetCursorPos(&p);
			CheckMenuItem(menu,ID_CONTEXTMENU_LOCKASPECTRATIO,(lockAspect?MF_CHECKED:MF_UNCHECKED)|MF_BYCOMMAND);
			CheckMenuItem(menu,ID_CONTEXTMENU_AUTOHIDE,(autoHide?MF_CHECKED:MF_UNCHECKED)|MF_BYCOMMAND);
			TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_LEFTBUTTON|TPM_NONOTIFY,p.x,p.y,0,hwndDlg,NULL);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_CONTEXTMENU_GETALBUMART:
			{
				wchar_t *filename = (wchar_t *)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_PLAYING_FILENAME);
				if(filename && *filename) {
					wchar_t artist[1024],album[1024];
					extendedFileInfoStructW a = {filename,L"artist",artist,1024};
					SendMessage(plugin.hwndParent,WM_WA_IPC,(LPARAM)&a,IPC_GET_EXTENDED_FILE_INFOW);
					a.metadata = L"album";
					a.ret = album;
					SendMessage(plugin.hwndParent,WM_WA_IPC,(LPARAM)&a,IPC_GET_EXTENDED_FILE_INFOW);
					artFetchData d = {sizeof(d),hwndDlg,artist,album,0};
					int r = (int)SendMessage(plugin.hwndParent,WM_WA_IPC,(LPARAM)&d,IPC_FETCH_ALBUMART);
					if(r == 0 && d.imgData && d.imgDataLen) // success, save art in correct location
					{
						AGAVE_API_ALBUMART->SetAlbumArt(filename,L"cover",0,0,d.imgData,d.imgDataLen,d.type);
						WASABI_API_MEMMGR->sysFree(d.imgData);
						SendMessage(hwndDlg,WM_USER,0,0);	
					}
				}
			}
			break;
		case ID_CONTEXTMENU_LOCKASPECTRATIO:
			lockAspect = (!lockAspect);
			if(cacheDC) DeleteDC(cacheDC); cacheDC = NULL;
			InvalidateRect(hwndDlg,NULL,TRUE);
			break;
		case ID_CONTEXTMENU_REFRESH:
			SendMessage(hwndDlg,WM_USER,0,0);
			break;
		case ID_CONTEXTMENU_OPENFOLDER:
			{
				wchar_t *filename = (wchar_t *)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_PLAYING_FILENAME);
				if(filename && *filename) {
					wchar_t fn[MAX_PATH];
					lstrcpynW(fn,filename,MAX_PATH);
					PathRemoveFileSpecW(fn);
					ShellExecuteW(NULL,L"open",fn,NULL,NULL,SW_SHOW);
				}
			}
			break;
		case ID_CONTEXTMENU_AUTOHIDE:
			autoHide = (!autoHide);
			break;
		}
		break;
	}
	return 0;
}

void showArtWindow()
{
	artwindow_open=1;
	MENUITEMINFO i = {sizeof(i), MIIM_STATE , MFT_STRING, MFS_CHECKED, WINAMP_ARTVIEW_MENUID};
	SetMenuItemInfo(menu, WINAMP_ARTVIEW_MENUID, FALSE, &i);
	ShowWindow(myWnd,SW_SHOW);
}

void hideArtWindow()
{
	artwindow_open=0;
	MENUITEMINFO i = {sizeof(i), MIIM_STATE , MFT_STRING, MFS_UNCHECKED, WINAMP_ARTVIEW_MENUID};
	SetMenuItemInfo(menu, WINAMP_ARTVIEW_MENUID, FALSE, &i);
	ShowWindow(myWnd,SW_HIDE);
}