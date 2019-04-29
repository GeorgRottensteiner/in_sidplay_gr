#include <windows.h>
#include "resource.h"
#include "typesdefs.h"
#include "subsongdlg.h"

#include "threadsidplayer.h"



CSubSongDlg* subSongDlg;



CSubSongDlg::CSubSongDlg( CThreadSidPlayer* pPlayer, HWND hWnd ) :
  m_hWnd( hWnd ),
  m_pPlayer( pPlayer )
{ 
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



int CSubSongDlg::SubSongDlgWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg )
	{
	  //case WM_REFRESH_TUNE_INFO:		
	  //	break;
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
		  break;
	  case WM_DESTROY:
		  break;
	  case WM_INITDIALOG:
		  break;
	  default:
		  return FALSE;
	}
	return TRUE;
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
