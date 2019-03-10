#ifndef NULLSOFT_MAINH
#define NULLSOFT_MAINH
#include <windows.h>
extern IDispatch *winampExternal;
INT_PTR CALLBACK MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Navigate(char *s);
void RefreshBrowser(int defurl);

#include "../gen_ml/ml.h"
extern winampMediaLibraryPlugin SampleHTTP;



/* dialog skinning helper functions 
see ML_IPC_SKIN_WADLG_GETFUNC in gen_ml/ml.h for details */

/* function pointer for letting winamp get first dibs on dialog messages */
typedef int (*HookDialogFunc)(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern HookDialogFunc ml_hook_dialog_msg;

/* function pointer for drawing dialog items in winamp style */
typedef void (*DrawFunc)(HWND hwndDlg, int *tab, int tabsize); 
extern DrawFunc ml_draw;

/* function pointer to retrieve winamp skin colors */
typedef int (*ColorFunc)(int idx);
extern ColorFunc ml_color;

/* quick function to change a COLORREF into something we can use in sprintf("#%06X") */
COLORREF GetHTMLColor(int color);

#endif