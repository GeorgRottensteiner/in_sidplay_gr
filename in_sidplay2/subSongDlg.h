#ifndef __SUBSONGDLG_H
#define __SUBSONGDLG_H


class CThreadSidPlayer;

class CSubSongDlg 
{
  private:

	  HWND                m_hWnd;
	  CThreadSidPlayer*   m_pPlayer;

    static HWND                 s_HwndDlg;
    static WNDPROC              s_OriginalMainWindowProc;

    static LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


  protected:

	  void NextSubSong();
	  void PrevSubSong();


  public:

    CSubSongDlg( CThreadSidPlayer* player, HWND hWnd );
	  ~CSubSongDlg();
	  int SubSongDlgWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	  void Hide();
	  void Show();
	  void RefreshWindowTitle();
};



extern CSubSongDlg* subSongDlg;

int CALLBACK SubSongDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


#endif //__SUBSONGDLG_H