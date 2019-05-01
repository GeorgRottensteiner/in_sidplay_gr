#pragma once

#include "sdk/winamp/in2.h"

#include "ThreadSidPlayer.h"

#include <string>


class CSubSongDlg;
struct StilBlock;
class SidTuneInfo;



class SIDPlugin
{
  public:

    static In_Module      g_InModuleDefinition;

    CThreadSidPlayer      m_SIDPlayer;

    CSubSongDlg*          m_pSubSongDlg = NULL;

    HANDLE                m_Mutex = INVALID_HANDLE_VALUE;
    HANDLE                m_UpdaterThreadHandle = INVALID_HANDLE_VALUE;



    SIDPlugin();
    ~SIDPlugin();


    static void     PluginShowConfigDialog( HWND hwndParent );
    static void     PluginShowAboutDialog( HWND hwndParent );
    static void     PluginInit();
    static void     PluginQuit();
    static int      PluginPlay( const char* Filename );
    static void     PluginPause();
    static int      PluginIsOurFile( const char* fn );
    static void     PluginUnPause();
    static int      PluginIsPaused();
    static void     PluginStop();
    static int      PluginGetLength();
    static int      PluginGetOutputTime();
    static void     PluginSetOutputTime( int time_in_ms );
    static void     PluginSetVolume( int volume );
    static void     PluginSetPan( int pan );
    static int      PluginShowInfoDialog( const char* Filename, HWND hwnd );
    static void     PluginGetFileInfo( const char* Filename, char* title, int* length_in_ms );
    static void     PluginSetEqualizer( int on, char data[10], int preamp );


  protected:


    void            Init();
    void            Quit();

    int             Play( const char* Filename );

    void            ShowConfigDialog( HWND hwndParent );
    void            ShowAboutDialog( HWND hwndParent );
    void            Pause();
    int             IsOurFile( const char* fn );
    void            Unpause();
    int             IsPaused();
    void            Stop();
    int             GetLength();
    int             GetOutputTime();
    void            SetOutputTime( int time_in_ms );
    void            SetVolume( int volume );
    void            SetPan( int pan );
    int             ShowInfoDialog( const char* Filename, HWND hwnd );
    void            GetFileInfo( const char* Filename, char* title, int* length_in_ms );
    void            SetEqualizer( int on, char data[10], int preamp );

    void            conditionsReplace( std::string& formatString, const StilBlock* stilBlock, const SidTuneInfo* tuneInfo );
                    
};

extern SIDPlugin             s_Plugin;
