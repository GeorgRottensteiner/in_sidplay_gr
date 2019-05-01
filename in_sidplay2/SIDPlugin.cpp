#include "SIDPlugin.h"
#include "ThreadSIDPlayer.h"
#include "SubSongDlg.h"
#include "InfoDlg.h"
#include "ConfigDlg.h"
#include "AboutDlg.h"
#include "resource.h"
#include "helpers.h"

#include <string>

#include "sdk/winamp/wa_ipc.h"
#include "sdk/winamp/ipc_pe.h"



SIDPlugin             s_Plugin;



In_Module SIDPlugin::g_InModuleDefinition =
{
  IN_VER,	// defined in IN2.H
  "Winamp SIDPlayer (libsidplayfp) v1.0.0.1gr"
  // winamp runs on both alpha systems and x86 ones. :)
  /*#ifdef __alpha
  "(AXP)"
  #else
  "(x86)"
  #endif*/
  ,
  0,	// hMainWindow (filled in by winamp)
  0,  // hDllInstance (filled in by winamp)
  "SID\0Sid File (*.sid)\0"
  // this is a double-null limited list. "EXT\0Description\0EXT\0Description\0" etc.
  ,
  1,	// is_seekable
  1,	// uses output plug-in system
  SIDPlugin::PluginShowConfigDialog,
  SIDPlugin::PluginShowAboutDialog,
  SIDPlugin::PluginInit,
  SIDPlugin::PluginQuit,
  SIDPlugin::PluginGetFileInfo,
  SIDPlugin::PluginShowInfoDialog,
  SIDPlugin::PluginIsOurFile,
  SIDPlugin::PluginPlay,
  SIDPlugin::PluginPause,
  SIDPlugin::PluginUnPause,
  SIDPlugin::PluginIsPaused,
  SIDPlugin::PluginStop,

  SIDPlugin::PluginGetLength,
  SIDPlugin::PluginGetOutputTime,
  SIDPlugin::PluginSetOutputTime,

  SIDPlugin::PluginSetVolume,
  SIDPlugin::PluginSetPan,

  0,0,0,0,0,0,0,0,0, // visualization calls filled in by winamp

  0,0, // dsp calls filled in by winamp

  SIDPlugin::PluginSetEqualizer,

  NULL,		// setinfo call filled in by winamp

  0 // out_mod filled in by winamp
};



SIDPlugin::SIDPlugin() :
  m_SIDPlayer( g_InModuleDefinition )
{
}



SIDPlugin::~SIDPlugin()
{
}



// Replaces occurences of string %{...} containing tokens separated by | by checking which
//   token is empty. Example: %{sr|a} it means if %sr is empty then use %a (if artist from stil is empty use
//   artist from SID file

void SIDPlugin::conditionsReplace( std::string& formatString, const StilBlock* stilBlock, const SidTuneInfo* tuneInfo )
{
  const int     BUF_SIZE = 30;
  std::string   conditionToken;
  int           tokenBeginPos = 0;
  int           tokenEndPos = 0;
  char          toReplaceToken[BUF_SIZE];
  std::vector<std::string> tokens;


  while ( ( tokenBeginPos = formatString.find( "%{", tokenBeginPos ) ) >= 0 )
  {
    tokenEndPos = formatString.find( '}', tokenBeginPos );
    if ( tokenEndPos < 0 )
    {
      break;
    }
    conditionToken = formatString.substr( tokenBeginPos + 2, tokenEndPos - tokenBeginPos - 2 );
    sprintf_s( toReplaceToken, BUF_SIZE, "%%{%s}", conditionToken.c_str() );

    if ( !conditionToken.empty() )
    {
      tokens = split( conditionToken, '|' );
      for ( std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it )
      {

        if ( *it == "f" )
        {
          replaceAll( formatString, toReplaceToken, "%f" );
          break;
        }
        if ( ( *it == "t" )
          && ( tuneInfo->infoString( 0 ).length() > 0 ) )
        {
          replaceAll( formatString, toReplaceToken, "%t" );
          break;
        }
        if ( ( *it == "a" )
          && ( tuneInfo->infoString( 1 ).length() > 0 ) )
        {
          replaceAll( formatString, toReplaceToken, "%a" );
          break;
        }
        if ( ( *it == "r" )
          && ( tuneInfo->infoString( 2 ).length() > 0 ) )
        {
          replaceAll( formatString, toReplaceToken, "%r" );
          break;
        }
        if ( *it == "x" )
        {
          replaceAll( formatString, toReplaceToken, "%x" );
          break;
        }

        if ( ( *it == "sr" )
          && ( stilBlock != NULL )
          && ( !stilBlock->ARTIST.empty() ) )
        {
          replaceAll( formatString, toReplaceToken, "%sr" );
          break;
        }
        if ( ( *it == "st" )
          && ( stilBlock != NULL )
          && ( !stilBlock->TITLE.empty() ) )
        {
          replaceAll( formatString, toReplaceToken, "%st" );
          break;
        }
        if ( ( *it == "sa" )
          && ( stilBlock != NULL )
          && ( !stilBlock->AUTHOR.empty() ) )
        {
          replaceAll( formatString, toReplaceToken, "%sa" );
          break;
        }
        if ( ( *it == "sn" )
          && ( stilBlock != NULL )
          && ( !stilBlock->NAME.empty() ) )
        {
          replaceAll( formatString, toReplaceToken, "%sn" );
          break;
        }
      }
      //check if condition was replaced by token, if not then make final token empty
      if ( conditionToken.at( 0 ) != '%' )
      {
        conditionToken.clear();
      }
    }

    ++tokenBeginPos;
  }
}



void SIDPlugin::PluginShowConfigDialog( HWND hwndParent )
{
  s_Plugin.ShowConfigDialog( hwndParent );
}



void SIDPlugin::PluginInit()
{
  s_Plugin.Init();
}



void SIDPlugin::PluginQuit()
{
  s_Plugin.Quit();
}



// called when winamp wants to play a file
int SIDPlugin::PluginPlay( const char* Filename )
{
  return s_Plugin.Play( Filename );
}



// standard pause implementation
void SIDPlugin::PluginPause()
{
  s_Plugin.Pause();
}



void SIDPlugin::Quit()
{
  // one-time deinit, such as memory freeing
  if ( m_pSubSongDlg != NULL )
  {
    delete m_pSubSongDlg;
    m_pSubSongDlg = NULL;
  }
  if ( m_Mutex != INVALID_HANDLE_VALUE )
  {
    CloseHandle( m_Mutex );
    m_Mutex = INVALID_HANDLE_VALUE;
  }
  if ( m_UpdaterThreadHandle != INVALID_HANDLE_VALUE )
  {
    CloseHandle( m_UpdaterThreadHandle );
    m_UpdaterThreadHandle = INVALID_HANDLE_VALUE;
  }
  m_SIDPlayer.Stop();
}



int SIDPlugin::PluginIsOurFile( const char* fn )
{
  return s_Plugin.IsOurFile( fn );
}



int SIDPlugin::IsOurFile( const char* fn )
{
  // used for detecting URL streams.. unused here. 
  // return !strncmp(fn,"http://",7); to detect HTTP streams, etc
  return 0;
}



int SIDPlugin::Play( const char* Filename )
{
  /*
  if ( m_SIDPlayer. g_pSIDPlayer == NULL )
  {
    Init();
  }*/

  m_SIDPlayer.LoadTune( Filename );
  const SidTuneInfo* tuneInfo = m_SIDPlayer.GetTuneInfo();
  if ( tuneInfo == NULL )
  {
    return -1;
  }
  m_SIDPlayer.PlaySubtune( tuneInfo->startSong() );
  return 0;
}



void SIDPlugin::Pause()
{
  m_SIDPlayer.Pause();
}



void SIDPlugin::PluginUnPause()
{
  s_Plugin.Unpause();
}



void SIDPlugin::Unpause()
{
  m_SIDPlayer.Play();
}



int SIDPlugin::PluginIsPaused()
{
  return s_Plugin.IsPaused();
}



int SIDPlugin::IsPaused()
{
  return ( m_SIDPlayer.GetPlayerStatus() == SP_PAUSED ) ? 1 : 0;
}



void SIDPlugin::PluginStop()
{
  s_Plugin.Stop();
}



void SIDPlugin::Stop()
{
  m_SIDPlayer.Stop();
}



// returns length of playing track
int SIDPlugin::PluginGetLength()
{
  return s_Plugin.GetLength();
}



int SIDPlugin::GetLength()
{
  // return length as number of sub tunes
  return ( m_SIDPlayer.GetNumSubtunes() - 1 ) * 1000;
  //return sidPlayer->GetSongLength()*1000;
}



// returns current output position, in ms.
// you could just use return mod.outMod->GetOutputTime(),
// but the dsp plug-ins that do tempo changing tend to make
// that wrong.
int SIDPlugin::PluginGetOutputTime()
{
  return s_Plugin.GetOutputTime();
}



// returns current output position, in ms.
// you could just use return mod.outMod->GetOutputTime(),
// but the dsp plug-ins that do tempo changing tend to make
// that wrong.
int SIDPlugin::GetOutputTime()
{
  if ( m_SIDPlayer.CurrentSubtune() > 0 )
  {
    return ( m_SIDPlayer.CurrentSubtune() - 1 ) * 1000;
  }
  //return inmod.outMod->GetOutputTime();
  //return sidPlayer->GetPlayTime();
  return 0;
}



// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
void SIDPlugin::PluginSetOutputTime( int time_in_ms )
{
  s_Plugin.SetOutputTime( time_in_ms );
}



// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
void SIDPlugin::SetOutputTime( int time_in_ms )
{
  //GR
  m_SIDPlayer.PlaySubtune( ( time_in_ms / 1000 ) + 1 );
  //sidPlayer->SeekTo(time_in_ms);
}



// standard volume/pan functions
void SIDPlugin::PluginSetVolume( int volume )
{
  s_Plugin.SetVolume( volume );
}



// standard volume/pan functions
void SIDPlugin::SetVolume( int volume )
{
  g_InModuleDefinition.outMod->SetVolume( volume );
}



void SIDPlugin::PluginSetPan( int pan )
{
  s_Plugin.SetPan( pan );
}



void SIDPlugin::SetPan( int pan )
{
  g_InModuleDefinition.outMod->SetPan( pan );
}



// this gets called when the use hits Alt+3 to get the file info.
// if you need more info, ask me :)

int SIDPlugin::PluginShowInfoDialog( const char* Filename, HWND hwnd )
{
  return s_Plugin.ShowInfoDialog( Filename, hwnd );
}



int SIDPlugin::ShowInfoDialog( const char* Filename, HWND hwnd )
{
  const SidTuneInfo* info;
  SidTune tune( 0 );
  int i;

  std::string filename = Filename;

  i = filename.find( '}' );
  if ( i > 0 )
  {
    filename = filename.substr( i + 1 );
  }
  tune.load( filename.c_str() );
  info = tune.getInfo();
  DialogBoxParam( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_FILEINFODLG ), hwnd, InfoDlgWndProc, (LPARAM)info );
  //ShowWindow(wnd,SW_SHOW);
  return 0;
}



// this is an odd function. it is used to get the title and/or
// length of a track.
// if filename is either NULL or of length 0, it means you should
// return the info of lastfn. Otherwise, return the information
// for the file in filename.
// if title is NULL, no title is copied into it.
// if length_in_ms is NULL, no length is copied into it.
void SIDPlugin::PluginGetFileInfo( const char* Filename, char* title, int* length_in_ms )
{
  s_Plugin.GetFileInfo( Filename, title, length_in_ms );
}
  
  
  
void SIDPlugin::GetFileInfo( const char* Filename, char* title, int* length_in_ms )
{
  const SidTuneInfo*  info = NULL;
  std::string         str;
  std::string         strFilename;
  int                 length = 0;
  SidTune             tune( 0 );
  int                 subsongIndex = 1;
  char                buf[20];


  WaitForSingleObject( m_Mutex, INFINITE );
  if ( m_UpdaterThreadHandle != INVALID_HANDLE_VALUE )
  {
    CloseHandle( m_UpdaterThreadHandle );
    m_UpdaterThreadHandle = INVALID_HANDLE_VALUE;
  }

  if ( ( Filename == NULL )
    || ( strlen( Filename ) == 0 ) )
  {
    // get current song info
    info = m_SIDPlayer.GetTuneInfo();
    if ( info == NULL )
    {
      ReleaseMutex( m_Mutex );
      return;
    }
    //length = sidPlayer->GetSongLength();
    // GR - replace length by number of songs
    length = m_SIDPlayer.GetTuneInfo()->songs();
    if ( length == -1 )
    {
      ReleaseMutex( m_Mutex );
      return;
    }
    subsongIndex = info->currentSong();
    strFilename = info->path();
    strFilename += info->dataFileName();
    subsongIndex = m_SIDPlayer.CurrentSubtune();
  }
  else
  {
    subsongIndex = 1;
    strFilename = Filename;
    tune.load( strFilename.c_str() );
    info = tune.getInfo();
    if ( info == NULL )
    {
      ReleaseMutex( m_Mutex );
      return;
    }
    subsongIndex = tune.getInfo()->startSong();
    info = tune.getInfo();
    tune.selectSong( subsongIndex );
    // GR
    //length = m_SIDPlayer.GetSongLength(tune);
    length = m_SIDPlayer.GetTuneInfo()->songs();
    if ( length == -1 )
    {
      ReleaseMutex( m_Mutex );
      return;
    }
  }

  //check if we got correct tune info
  //if (info.c64dataLen == 0) return;
  if ( info->c64dataLen() == 0 )
  {
    ReleaseMutex( m_Mutex );
    return;
  }
  length *= 1000;
  if ( length < 0 )
  {
    length = 0;
  }
  if ( length_in_ms != NULL )
  {
    *length_in_ms = length;
  }

  /* build file title from template:
  %f - filename
  %t - song title from sid file
  %a - artist
  %r - release year and publisher
  %x - subsong string

  %n - subsong number in subsong string

  %sr - artist from STIL file
  %st - title from STIL file
  %sa - author from STIL file
  %sn - name from STIL file
  */
  //info->dataFileName

  std::string     fileNameOnly( strFilename );
  int cutStart = fileNameOnly.find_last_of( "\\" );
  int cutEnd = fileNameOnly.find_last_of( "." );
  fileNameOnly = fileNameOnly.substr( cutStart + 1, cutEnd - cutStart - 1 );


  // std::string titleTemplate("%f / %a / %x %sn");
  std::string titleTemplate( m_SIDPlayer.GetCurrentConfig().playlistFormat );
  std::string subsongTemplate( m_SIDPlayer.GetCurrentConfig().subsongFormat );


  // fill STIL data if necessary
  StilBlock       sb;
  if ( m_SIDPlayer.GetCurrentConfig().useSTILfile == true )
  {
    sb = m_SIDPlayer.GetSTILData2( strFilename.c_str(), subsongIndex - 1 );
  }
  conditionsReplace( titleTemplate, &sb, info );

  replaceAll( titleTemplate, "%f", fileNameOnly.c_str() );
  replaceAll( titleTemplate, "%t", info->infoString( 0 ).c_str() );
  replaceAll( titleTemplate, "%a", info->infoString( 1 ).c_str() );
  replaceAll( titleTemplate, "%r", info->infoString( 2 ).c_str() );
  if ( info->songs() > 1 )
  {
    sprintf_s( buf, sizeof( buf ), "%2d", info->songs() );
    replaceAll( subsongTemplate, "%ns", buf );

    sprintf_s( buf, sizeof( buf ), "%2d", subsongIndex );
    //itoa(subsongIndex, buf, 10);
    //replaceAll(subsongTemplate, "%n", _itoa(subsongIndex, buf, 10));
    replaceAll( subsongTemplate, "%n", buf );


    replaceAll( titleTemplate, "%x", subsongTemplate.c_str() );
  }
  else
  {
    replaceAll( titleTemplate, "%x", "" );
  }

  // fill STIL data if necessary
  replaceAll( titleTemplate, "%sr", sb.ARTIST.c_str() );
  replaceAll( titleTemplate, "%st", sb.TITLE.c_str() );
  replaceAll( titleTemplate, "%sa", sb.AUTHOR.c_str() );
  replaceAll( titleTemplate, "%sn", sb.NAME.c_str() );

  if ( title != NULL )
  {
    strcpy_s( title, GETFILEINFO_TITLE_LENGTH, titleTemplate.c_str() );
  }

  ReleaseMutex( m_Mutex );
  return;
}



void SIDPlugin::PluginSetEqualizer( int on, char data[10], int preamp )
{
  s_Plugin.SetEqualizer( on, data, preamp );
}



void SIDPlugin::SetEqualizer( int on, char data[10], int preamp )
{
  // most plug-ins can't even do an EQ anyhow.. I'm working on writing
  // a generic PCM EQ, but it looks like it'll be a little too CPU 
  // consuming to be useful :)
  // if you _CAN_ do EQ with your format, each data byte is 0-63 (+20db <-> -20db)
  // and preamp is the same. 
}



void SIDPlugin::ShowConfigDialog( HWND hwndParent )
{
  DialogBoxParam( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_CONFIG_DLG ), hwndParent, &ConfigDlgWndProc, NULL );
  SetFocus( hwndParent );
}



void SIDPlugin::PluginShowAboutDialog( HWND hwndParent )
{
  s_Plugin.ShowAboutDialog( hwndParent );
}



void SIDPlugin::ShowAboutDialog( HWND hwndParent )
{
  DialogBox( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_ABOUTDLG ), hwndParent, &AboutDlgWndProc );
  SetFocus( hwndParent );
}



void SIDPlugin::Init()
{
  m_SIDPlayer.Init();

  m_Mutex = CreateMutex( NULL, FALSE, NULL );

  //g_InModuleDefinition.hMainWindow
  HWND hwndDlg = ::CreateDialog( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_DLG_SUBSONG ), g_InModuleDefinition.hMainWindow, SubSongDlgWndProc );

  //dh::Log( "SubsongDlg %x", hwndDlg );

  m_pSubSongDlg = new CSubSongDlg( &m_SIDPlayer, hwndDlg );

  //hMainWindow
  //DialogBox( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_ABOUTDLG ), hwndParent, &AboutDlgWndProc );
}



