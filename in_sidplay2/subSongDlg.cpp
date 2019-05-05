#include <windows.h>
#include <Windowsx.h>

#include "resource.h"
#include "typesdefs.h"
#include "subsongdlg.h"

#include "threadsidplayer.h"
#include "SIDPlugin.h"



CSubSongDlg*          s_pSubSongDlg = NULL;

extern SIDPlugin      s_Plugin;



CSubSongDlg::CSubSongDlg( CThreadSidPlayer* pPlayer ) :
  m_hWnd( NULL ),
  m_pPlayer( pPlayer ),
  m_NumberOfSongs( 0 ),
  m_CurrentSong( 0 ),
  m_DraggedSliderPos( 0 ),
  m_DraggingSlider( false )
{ 
  s_pSubSongDlg = this;

  m_hWnd = ::CreateDialogParam( SIDPlugin::g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_DLG_SUBSONG ), SIDPlugin::g_InModuleDefinition.hMainWindow, StaticSubSongDlgWndProc, (LPARAM)this );
}



CSubSongDlg::~CSubSongDlg()
{
  s_pSubSongDlg = NULL;
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
}



void CSubSongDlg::PrevSubSong()
{
	const SidTuneInfo* pTuneInfo = m_pPlayer->GetTuneInfo();

  if ( ( pTuneInfo->currentSong() - 1 ) < 1 )
  {
    return;
  }
  m_pPlayer->PlaySubtune( pTuneInfo->currentSong() - 1 );
}



RECT CSubSongDlg::GetSliderRect()
{
  RECT    rc;

  SetRectEmpty( &rc );

  if ( m_NumberOfSongs > 1 )
  {
    GetClientRect( m_hWnd, &rc );

    InflateRect( &rc, -10, -10 );

    int     sliderWidth = 40;
    int     sliderRange = ( rc.right - rc.left - sliderWidth ) / ( m_NumberOfSongs - 1 );

    rc.left = rc.left + ( rc.right - rc.left - sliderWidth ) * ( m_CurrentSong - 1 ) / ( m_NumberOfSongs - 1 );
    rc.right = rc.left + sliderWidth;
  }
  return rc;
}



int CSubSongDlg::SubSongDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
    case WM_NCHITTEST:
      {
        /*
        RECT    rcSlider = GetSliderRect();

        POINT   pt;

        pt.x = GET_X_LPARAM( lParam );
        pt.y = GET_Y_LPARAM( lParam );

        ScreenToClient( hWnd, &pt );

        if ( !PtInRect( &rcSlider, pt ) )
        {
          //SetWindowLong( hWnd, DWL_MSGRESULT, HTCAPTION );
        }
        else*/
        {
          SetWindowLong( hWnd, DWL_MSGRESULT, HTCLIENT );
        }
      }
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
    case WM_DRAWITEM:
      {
        if ( wParam == IDC_STATIC_DISPLAY )
        {
          DRAWITEMSTRUCT&   dis( *(DRAWITEMSTRUCT*)lParam );

          HBRUSH brushBlack = CreateSolidBrush( RGB( 0, 0, 0 ) );

          RECT    rc;

          GetClientRect( dis.hwndItem, &rc );

          FillRect( dis.hDC, &rc, brushBlack );

          InflateRect( &rc, -10, -10 );

          DrawEdge( dis.hDC, &rc, BDR_SUNKENINNER, BF_RECT );

          if ( m_NumberOfSongs > 1 )
          {
            int     displayPos = m_CurrentSong;

            if ( m_DraggingSlider )
            {
              displayPos = m_DraggedSliderPos;
            }

            int     sliderWidth = 40;

            rc.left = rc.left + ( rc.right - rc.left - sliderWidth ) * ( displayPos - 1 ) / ( m_NumberOfSongs - 1 );
            rc.right = rc.left + sliderWidth;

            DrawFrameControl( dis.hDC, &rc, DFC_BUTTON, DFCS_BUTTONPUSH );
          }
          else if ( m_NumberOfSongs == 1 )
          {
            int     displayPos = 0;
            int     sliderWidth = 40;

            rc.right = rc.left + sliderWidth;

            DrawFrameControl( dis.hDC, &rc, DFC_BUTTON, DFCS_BUTTONPUSH );
          }

          DeleteObject( brushBlack );

          return TRUE;
        }
      }
      break;
    case WM_MOVE:
    case WM_SIZE:
      {
        RECT    rc;

        GetWindowRect( hWnd, &rc );
        SetWindowPos( m_StaticDisplay, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER );
      }
      break;
    case WM_LBUTTONDOWN:
      {
        RECT    rcSlider = GetSliderRect();

        POINT   pt;

        pt.x = GET_X_LPARAM( lParam );
        pt.y = GET_Y_LPARAM( lParam );

        if ( PtInRect( &rcSlider, pt ) )
        {
          m_DraggingSlider = true;
          m_DraggedSliderPos = m_CurrentSong;
          SetCapture( hWnd );
        }
        SetFocus( hWnd );
      }
      break;
    case WM_LBUTTONUP:
      if ( m_DraggingSlider )
      {
        ReleaseCapture();
        m_DraggingSlider = false;

        if ( m_DraggedSliderPos != m_CurrentSong )
        {
          const SidTuneInfo* pTuneInfo = m_pPlayer->GetTuneInfo();

          m_pPlayer->PlaySubtune( m_DraggedSliderPos );

          if ( pTuneInfo != NULL )
          {
            UpdateScrollBar( pTuneInfo->songs(), m_DraggedSliderPos );
          }
          else
          {
            UpdateScrollBar( 0, 0 );
          }
        }
      }
      break;
    case WM_MOUSEMOVE:
      if ( ( m_DraggingSlider )
      &&   ( wParam & MK_LBUTTON ) )
      {
        if ( m_NumberOfSongs >= 2 )
        {
          RECT    rc;
          POINT   pt;

          pt.x = GET_X_LPARAM( lParam );
          pt.y = GET_Y_LPARAM( lParam );

          GetClientRect( hWnd, &rc );

          InflateRect( &rc, -10, -10 );

          int     sliderWidth = 40;
          int     sliderRange = ( rc.right - rc.left - sliderWidth ) / ( m_NumberOfSongs - 1 );

          int     songNo = pt.x / sliderRange;
          ++songNo;
          if ( songNo < 1 )
          {
            songNo = 1;
          }
          if ( songNo > m_NumberOfSongs )
          {
            songNo = m_NumberOfSongs;
          }

          if ( songNo != m_DraggedSliderPos )
          {
            m_DraggedSliderPos = songNo;
            InvalidateRect( m_StaticDisplay, NULL, TRUE );
          }
        }
      }
      break;
	  case WM_INITDIALOG:
      {
        m_OriginalMainWindowProc = (WNDPROC)SetWindowLongPtr( SIDPlugin::g_InModuleDefinition.hMainWindow, GWLP_WNDPROC, (LONG_PTR)StaticMainWndProc );

        m_StaticDisplay = GetDlgItem( hWnd, IDC_STATIC_DISPLAY );
        SetWindowLong( m_StaticDisplay, GWL_STYLE, GetWindowLong( m_StaticDisplay, GWL_STYLE ) | SS_OWNERDRAW );
        InvalidateRect( m_StaticDisplay, NULL, TRUE );

        AdjustSizeToParent();
      }
      return TRUE;
	}
  return FALSE;
}



LRESULT CALLBACK CSubSongDlg::StaticMainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  if ( s_pSubSongDlg != NULL )
  {
    return s_pSubSongDlg->MainWndProc( hWnd, uMsg, wParam, lParam );
  }
  // Booo!
  return DefWindowProc( hWnd, uMsg, wParam, lParam );
}



void CSubSongDlg::AdjustSizeToParent()
{
  RECT    rc;

  GetWindowRect( SIDPlugin::g_InModuleDefinition.hMainWindow, &rc );
  rc.top    = rc.bottom;
  rc.bottom = rc.top + 30;

  SetWindowPos( m_hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER );
  SetWindowPos( m_StaticDisplay, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER );
  InvalidateRect( m_StaticDisplay, NULL, TRUE );
}



LRESULT CALLBACK CSubSongDlg::MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_MOVE:
    case WM_SIZE:
      {
        LRESULT res = CallWindowProc( m_OriginalMainWindowProc, hWnd, uMsg, wParam, lParam );
        /*
        xPos = (int)(short)LOWORD( lParam );   // horizontal position 
        yPos = (int)(short)HIWORD( lParam );   // vertical position
        */
        AdjustSizeToParent();

        return res;
      }
      break;
  }

  return CallWindowProc( m_OriginalMainWindowProc, hWnd, uMsg, wParam, lParam );
}



void CSubSongDlg::Hide()
{
	ShowWindow( m_hWnd, SW_HIDE );
}



void CSubSongDlg::Show()
{
	ShowWindow( m_hWnd, SW_SHOW );
}



int CALLBACK CSubSongDlg::StaticSubSongDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  if ( uMsg == WM_INITDIALOG )
  {
    s_pSubSongDlg = (CSubSongDlg*)lParam;
  }
  if ( s_pSubSongDlg != NULL )
  {
    return s_pSubSongDlg->SubSongDlgWndProc( hWnd, uMsg, wParam, lParam );
  }
  return FALSE;
}



void CSubSongDlg::UpdateScrollBar( int NumberOfSongs, int CurrentSong )
{
  m_NumberOfSongs     = NumberOfSongs;
  m_CurrentSong       = CurrentSong;
  m_DraggedSliderPos  = CurrentSong;

  InvalidateRect( m_StaticDisplay, NULL, TRUE );
}