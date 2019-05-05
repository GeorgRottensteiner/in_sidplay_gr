#ifndef __SUBSONGDLG_H
#define __SUBSONGDLG_H


class CThreadSidPlayer;

class CSubSongDlg 
{
  private:

    int                 m_DraggedSliderPos;
    int                 m_CurrentSong;
    int                 m_NumberOfSongs;
    bool                m_DraggingSlider;

	  HWND                m_hWnd;
	  CThreadSidPlayer*   m_pPlayer;

    HWND                m_StaticDisplay;

    WNDPROC             m_OriginalMainWindowProc;

    LRESULT CALLBACK    MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    static int CALLBACK       StaticSubSongDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static LRESULT CALLBACK   StaticMainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    void                AdjustSizeToParent();

    RECT                GetSliderRect();



  protected:

	  void                NextSubSong();
	  void                PrevSubSong();


  public:

    CSubSongDlg( CThreadSidPlayer* player );
	  ~CSubSongDlg();
	  int SubSongDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	  void Hide();
	  void Show();


    void                UpdateScrollBar( int NumberOfSongs, int CurrentSong );
};



extern CSubSongDlg* s_pSubSongDlg;


#endif //__SUBSONGDLG_H