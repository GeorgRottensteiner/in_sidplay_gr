#include <windows.h>
#include "resource.h"
#include "typesdefs.h"
#include "subsongdlg.h"

#include "threadsidplayer.h"
#include "SIDPlugin.h"



CSubSongDlg* subSongDlg;

extern SIDPlugin      s_Plugin;


WNDPROC CSubSongDlg::s_OriginalMainWindowProc;
HWND    CSubSongDlg::s_HwndDlg;



CSubSongDlg::CSubSongDlg( CThreadSidPlayer* pPlayer, HWND hWnd ) :
  m_hWnd( hWnd ),
  m_pPlayer( pPlayer )
{ 
  s_HwndDlg = m_hWnd;
}



CSubSongDlg::~CSubSongDlg()
{
  if ( m_hWnd != NULL )
  {
    DestroyWindow( m_hWnd );
  }
}



void CSubSongDlg::NextSubSong()
{
	const SidTuneInfo* pTuneInfo = m_pPlayer->GetTuneInfo();
  if ( ( pTuneInfo->currentSong() + 1 ) > pTuneInfo->songs() )
  {
    return;
  }
  m_pPlayer->PlaySubtune( pTuneInfo->currentSong() + 1 );
	RefreshWindowTitle();
}



void CSubSongDlg::PrevSubSong()
{
	const SidTuneInfo* pTuneInfo = m_pPlayer->GetTuneInfo();

  if ( ( pTuneInfo->currentSong() - 1 ) < 1 )
  {
    return;
  }
  m_pPlayer->PlaySubtune( pTuneInfo->currentSong() - 1 );
	RefreshWindowTitle();
}



int CSubSongDlg::SubSongDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
    case WM_NCHITTEST:
      SetWindowLong( hWnd, DWL_MSGRESULT, HTCAPTION );
      return TRUE;
	  case WM_COMMAND:
		  {
			  int wmId = LOWORD( wParam );			
			  switch ( wmId )
			  {
			    case IDCANCEL:
				    Hide();
				    break;
			    case IDC_PREV:
				    PrevSubSong();
				    break;
			    case IDC_NEXT:
				    NextSubSong();
				    break;
			  }
		  }
      return TRUE;
	  case WM_DESTROY:
      return TRUE;
	  case WM_INITDIALOG:
      {
        HWND    hwndParent = SIDPlugin::g_InModuleDefinition.hMainWindow;
        RECT    rc;

        GetWindowRect( hwndParent, &rc );
        rc.top = rc.bottom;
        rc.bottom = rc.top + 100;

        SetWindowPos( hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER );

        s_OriginalMainWindowProc = (WNDPROC)SetWindowLongPtr( hwndParent, GWLP_WNDPROC, (LONG_PTR)MainWndProc );
      }
      return TRUE;
	}
  return FALSE;
}



LRESULT CALLBACK CSubSongDlg::MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_MOVE:
      {
        LRESULT res = CallWindowProc( s_OriginalMainWindowProc, hWnd, uMsg, wParam, lParam );
        /*
        xPos = (int)(short)LOWORD( lParam );   // horizontal position 
        yPos = (int)(short)HIWORD( lParam );   // vertical position
        */
        RECT    rc;

        GetWindowRect( hWnd, &rc );
        rc.top = rc.bottom;
        rc.bottom = rc.top + 100;

        SetWindowPos( s_HwndDlg, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER );

        return res;
      }
      break;
  }

  return CallWindowProc( s_OriginalMainWindowProc, hWnd, uMsg, wParam, lParam );
}



void CSubSongDlg::Hide()
{
	ShowWindow( m_hWnd, SW_HIDE );
}



void CSubSongDlg::Show()
{
	ShowWindow( m_hWnd, SW_SHOW );
}



void CSubSongDlg::RefreshWindowTitle()
{
	char                buf[30];
	const SidTuneInfo*  pTuneInfo= m_pPlayer->GetTuneInfo();

	sprintf_s( buf, sizeof( buf ), "Subtune %d of %d ", pTuneInfo->currentSong(), pTuneInfo->songs() );
	SetWindowTextA( m_hWnd,buf );
}



int CALLBACK SubSongDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return subSongDlg->SubSongDlgWndProc( hWnd,uMsg,wParam,lParam );
}
