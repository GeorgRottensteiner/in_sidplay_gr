// in_sidplay2.cpp : Defines the exported functions for the DLL application.
//

#include <windows.h>
#include "sdk/winamp/in2.h"
#include "ThreadSidplayer.h"
#include "resource.h"
#include "aboutdlg.h"
#include "configdlg.h"
#include "infodlg.h"
//#include "subsongdlg.h"
#include "sdk/winamp/wa_ipc.h"
#include "sdk/winamp/ipc_pe.h"
#include "helpers.h"

#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#include <string>
#include <vector>



In_Module             g_InModuleDefinition;
CThreadSidPlayer*     g_pSIDPlayer = NULL;
HANDLE                gUpdaterThreadHandle = 0;
HANDLE                gMutex = 0;



void config( HWND hwndParent )
{
  DialogBoxParam( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_CONFIG_DLG ), hwndParent, &ConfigDlgWndProc, NULL );
  SetFocus( hwndParent );
}



void about( HWND hwndParent )
{
  DialogBox( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_ABOUTDLG ), hwndParent, &AboutDlgWndProc );
  SetFocus( hwndParent );
}



void init()
{
  // any one-time initialization goes here (configuration reading, etc)
  g_pSIDPlayer = new CThreadSidPlayer( g_InModuleDefinition );
  g_pSIDPlayer->Init();

  gMutex = CreateMutex( NULL, FALSE, NULL );
}



void quit() 
{
  // one-time deinit, such as memory freeing
  if ( gMutex != 0 )
  {
    CloseHandle( gMutex );
  }
  if ( gUpdaterThreadHandle != 0 )
  {
    CloseHandle( gUpdaterThreadHandle );
  }
  if ( g_pSIDPlayer != NULL )
  {
    g_pSIDPlayer->Stop();
    delete g_pSIDPlayer;
    g_pSIDPlayer = NULL;
  }
}



int isourfile( const char* fn ) 
{
  // used for detecting URL streams.. unused here. 
  // return !strncmp(fn,"http://",7); to detect HTTP streams, etc
  return 0;
}


// called when winamp wants to play a file
int play( const char* Filename )
{
  if ( g_pSIDPlayer == NULL )
  {
    init();
  }

  g_pSIDPlayer->LoadTune( Filename );
  const SidTuneInfo* tuneInfo = g_pSIDPlayer->GetTuneInfo();
  if ( tuneInfo == NULL )
  {
    return -1;
  }
  g_pSIDPlayer->PlaySubtune( tuneInfo->startSong() );
  return 0;
}



// standard pause implementation
void pause()
{
  g_pSIDPlayer->Pause();
}



void unpause()
{
  g_pSIDPlayer->Play();
}



int ispaused()
{
  return ( g_pSIDPlayer->GetPlayerStatus() == SP_PAUSED ) ? 1 : 0;
}



void stop()
{
  g_pSIDPlayer->Stop();
}



// returns length of playing track
int getlength()
{
  // return length as number of sub tunes
  return ( g_pSIDPlayer->GetNumSubtunes() - 1 ) * 1000;
  //return sidPlayer->GetSongLength()*1000;
}


// returns current output position, in ms.
// you could just use return mod.outMod->GetOutputTime(),
// but the dsp plug-ins that do tempo changing tend to make
// that wrong.
int getoutputtime()
{
  if ( g_pSIDPlayer->CurrentSubtune() > 0 )
  {
    return ( g_pSIDPlayer->CurrentSubtune() - 1 ) * 1000;
  }
  //return inmod.outMod->GetOutputTime();
  //return sidPlayer->GetPlayTime();
  return 0;
}


// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
void setoutputtime( int time_in_ms )
{
  //GR
  g_pSIDPlayer->PlaySubtune( ( time_in_ms / 1000 ) + 1 );
  //sidPlayer->SeekTo(time_in_ms);
}


// standard volume/pan functions
void setvolume( int volume ) 
{ 
  g_InModuleDefinition.outMod->SetVolume( volume ); 
}



void setpan( int pan ) 
{ 
  g_InModuleDefinition.outMod->SetPan( pan ); 
}



// this gets called when the use hits Alt+3 to get the file info.
// if you need more info, ask me :)

int infoDlg( const char *fn, HWND hwnd )
{
  const SidTuneInfo* info;
  SidTune tune( 0 );
  int i;
  std::string strfilename;

  strfilename.assign( fn );
  i = strfilename.find( '}' );
  if ( i > 0 )
  {
    strfilename = strfilename.substr( i + 1 );
  }
  tune.load( strfilename.c_str() );
  info = tune.getInfo();
  DialogBoxParam( g_InModuleDefinition.hDllInstance, MAKEINTRESOURCE( IDD_FILEINFODLG ), hwnd, InfoDlgWndProc, (LPARAM)info );
  //ShowWindow(wnd,SW_SHOW);
  return 0;
}



/**
  Replaces occurences of string %{...} containing tokens separated by | by checking which
  token is empty. Example: %{sr|a} it means if %sr is empty then use %a (if artist from stil is empty use
  artist from SID file
*/
void conditionsReplace( std::string& formatString, const StilBlock* stilBlock, const SidTuneInfo* tuneInfo )
{
  const int BUF_SIZE = 30;
  std::string conditionToken;
  int tokenBeginPos = 0;
  int tokenEndPos = 0;
  std::vector<std::string> tokens;
  char toReplaceToken[BUF_SIZE];

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

        if ( ( *it ).compare( "f" ) == 0 )
        {
          replaceAll( formatString, toReplaceToken, "%f" );
          break;
        }
        if ( ( ( *it ).compare( "t" ) == 0 ) && ( tuneInfo->infoString( 0 ).length() > 0 ) )
        {
          replaceAll( formatString, toReplaceToken, "%t" );
          break;
        }
        if ( ( ( *it ).compare( "a" ) == 0 ) && ( tuneInfo->infoString( 1 ).length() > 0 ) )
        {
          replaceAll( formatString, toReplaceToken, "%a" );
          break;
        }
        if ( ( ( *it ).compare( "r" ) == 0 ) && ( tuneInfo->infoString( 2 ).length() > 0 ) )
        {
          replaceAll( formatString, toReplaceToken, "%r" );
          break;
        }
        if ( ( *it ).compare( "x" ) == 0 )
        {
          replaceAll( formatString, toReplaceToken, "%x" );
          break;
        }

        if ( ( ( *it ).compare( "sr" ) == 0 ) && ( stilBlock != NULL ) && ( !stilBlock->ARTIST.empty() ) )
        {
          replaceAll( formatString, toReplaceToken, "%sr" );
          break;
        }
        if ( ( ( *it ).compare( "st" ) == 0 ) && ( stilBlock != NULL ) && ( !stilBlock->TITLE.empty() ) )
        {
          replaceAll( formatString, toReplaceToken, "%st" );
          break;
        }
        if ( ( ( *it ).compare( "sa" ) == 0 ) && ( stilBlock != NULL ) && ( !stilBlock->AUTHOR.empty() ) )
        {
          replaceAll( formatString, toReplaceToken, "%sa" );
          break;
        }
        if ( ( ( *it ).compare( "sn" ) == 0 ) && ( stilBlock != NULL ) && ( !stilBlock->NAME.empty() ) )
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


// this is an odd function. it is used to get the title and/or
// length of a track.
// if filename is either NULL or of length 0, it means you should
// return the info of lastfn. Otherwise, return the information
// for the file in filename.
// if title is NULL, no title is copied into it.
// if length_in_ms is NULL, no length is copied into it.
void getfileinfo( const char* filename, char* title, int* length_in_ms )
{
  const SidTuneInfo* info;
  std::string str;
  std::string strFilename;
  int length;
  SidTune tune( 0 );
  int subsongIndex = 1;
  char buf[20];

  WaitForSingleObject( gMutex, INFINITE );
  if ( gUpdaterThreadHandle != 0 )
  {
    CloseHandle( gUpdaterThreadHandle );
    gUpdaterThreadHandle = 0;
  }

  if ( ( filename == NULL ) 
  ||   ( strlen( filename ) == 0 ) )
  {
    // get current song info
    info = g_pSIDPlayer->GetTuneInfo();
    if ( info == NULL )
    {
      ReleaseMutex( gMutex );
      return;
    }
    //length = sidPlayer->GetSongLength();
    // GR - replace length by number of songs
    length = g_pSIDPlayer->GetTuneInfo()->songs();
    if ( length == -1 )
    {
      ReleaseMutex( gMutex );
      return;
    }
    subsongIndex = info->currentSong();
    strFilename = info->path();
    strFilename += info->dataFileName();
    subsongIndex = g_pSIDPlayer->CurrentSubtune();
  }
  else
  {
    subsongIndex = 1;
    strFilename.assign( filename );
    tune.load( strFilename.c_str() );
    info = tune.getInfo();
    if ( info == NULL )
    {
      ReleaseMutex( gMutex );
      return;
    }
    subsongIndex = tune.getInfo()->startSong();
    info = tune.getInfo();
    tune.selectSong( subsongIndex );
    // GR
    //length = g_pSIDPlayer->GetSongLength(tune);
    length = g_pSIDPlayer->GetTuneInfo()->songs();
    if ( length == -1 )
    {
      ReleaseMutex( gMutex );
      return;
    }
  }

  //check if we got correct tune info
  //if (info.c64dataLen == 0) return;
  if ( info->c64dataLen() == 0 )
  {
    ReleaseMutex( gMutex );
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
  int cutStart  = fileNameOnly.find_last_of( "\\" );
  int cutEnd    = fileNameOnly.find_last_of( "." );
  fileNameOnly  = fileNameOnly.substr( cutStart + 1, cutEnd - cutStart - 1 );


  // std::string titleTemplate("%f / %a / %x %sn");
  std::string titleTemplate( g_pSIDPlayer->GetCurrentConfig().playlistFormat );
  std::string subsongTemplate( g_pSIDPlayer->GetCurrentConfig().subsongFormat );


  // fill STIL data if necessary
  const StilBlock* sb = NULL;
  if ( g_pSIDPlayer->GetCurrentConfig().useSTILfile == true )
  {
    sb = g_pSIDPlayer->GetSTILData2( strFilename.c_str(), subsongIndex - 1 );
  }
  else
  {
    sb = NULL;
  }
  conditionsReplace( titleTemplate, sb, info );

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
  if ( sb == NULL )
  {
    replaceAll( titleTemplate, "%sr", "" );
    replaceAll( titleTemplate, "%sa", "" );
    replaceAll( titleTemplate, "%st", "" );
    replaceAll( titleTemplate, "%sn", "" );
  }
  else
  {
    replaceAll( titleTemplate, "%sr", sb->ARTIST.c_str() );
    replaceAll( titleTemplate, "%st", sb->TITLE.c_str() );
    replaceAll( titleTemplate, "%sa", sb->AUTHOR.c_str() );
    replaceAll( titleTemplate, "%sn", sb->NAME.c_str() );
  }

  if ( title != NULL )
  {
    strcpy_s( title, GETFILEINFO_TITLE_LENGTH, titleTemplate.c_str() );
  }

  ReleaseMutex( gMutex );
  return;
}



void eq_set( int on, char data[10], int preamp )
{
  // most plug-ins can't even do an EQ anyhow.. I'm working on writing
  // a generic PCM EQ, but it looks like it'll be a little too CPU 
  // consuming to be useful :)
  // if you _CAN_ do EQ with your format, each data byte is 0-63 (+20db <-> -20db)
  // and preamp is the same. 
}


// module definition.
extern In_Module g_InModuleDefinition =
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
  config,
  about,
  init,
  quit,
  getfileinfo,
  infoDlg,
  isourfile,
  play,
  pause,
  unpause,
  ispaused,
  stop,

  getlength,
  getoutputtime,
  setoutputtime,

  setvolume,
  setpan,

  0,0,0,0,0,0,0,0,0, // visualization calls filled in by winamp

  0,0, // dsp calls filled in by winamp

  eq_set,

  NULL,		// setinfo call filled in by winamp

  0 // out_mod filled in by winamp

};



extern "C" __declspec( dllexport )
In_Module* winampGetInModule2()
{
  return &g_InModuleDefinition;
}
