#include "configDlg.h"
#include "resource.h"
#include "typesdefs.h"
#include <shlobj.h>

#include "helpers.h"
#include "SIDPlugin.h"

#include "threadsidplayer.h"



extern SIDPlugin             s_Plugin;

PlayerConfig                  currentConfig;



void ConfigDlgInitDialog( HWND hWnd )
{
  int val;

  currentConfig = s_Plugin.m_SIDPlayer.GetCurrentConfig();

  //we dont need current value coz we will load it from file
  currentConfig.SongLengthsFile = "";

  //now load "real" settings from file (they may differ from those above)
  s_Plugin.m_SIDPlayer.LoadConfigFromFile( currentConfig );


  //FREQ
  SendDlgItemMessage( hWnd, IDC_FREQUENCY, CB_ADDSTRING, 0, (LPARAM)L"48000" );
  SendDlgItemMessage( hWnd, IDC_FREQUENCY, CB_ADDSTRING, 0, (LPARAM)L"44100" );
  SendDlgItemMessage( hWnd, IDC_FREQUENCY, CB_ADDSTRING, 0, (LPARAM)L"22050" );
  SendDlgItemMessage( hWnd, IDC_FREQUENCY, CB_ADDSTRING, 0, (LPARAM)L"11025" );
  //channels
  SendDlgItemMessage( hWnd, IDC_CHANNELS, CB_ADDSTRING, 0, (LPARAM)L"Mono" );
  SendDlgItemMessage( hWnd, IDC_CHANNELS, CB_ADDSTRING, 0, (LPARAM)L"Stereo" );
  //C64 model
  SendDlgItemMessage( hWnd, IDC_C64MODEL, CB_ADDSTRING, 0, (LPARAM)L"PAL" );
  SendDlgItemMessage( hWnd, IDC_C64MODEL, CB_ADDSTRING, 0, (LPARAM)L"NTSC" );
  SendDlgItemMessage( hWnd, IDC_C64MODEL, CB_ADDSTRING, 0, (LPARAM)L"Old NTSC" );
  SendDlgItemMessage( hWnd, IDC_C64MODEL, CB_ADDSTRING, 0, (LPARAM)L"DREAN" );

  //sid model
  SendDlgItemMessage( hWnd, IDC_SIDMODEL, CB_ADDSTRING, 0, (LPARAM)L"MOS-6581" );
  SendDlgItemMessage( hWnd, IDC_SIDMODEL, CB_ADDSTRING, 0, (LPARAM)L"MOS-8580" );

  //sid2 model
  SendDlgItemMessage( hWnd, IDC_SID2MODEL, CB_ADDSTRING, 0, (LPARAM)L"MOS-6581" );
  SendDlgItemMessage( hWnd, IDC_SID2MODEL, CB_ADDSTRING, 0, (LPARAM)L"MOS-8580" );

  //set values:
  //freq
  switch ( currentConfig.SidConfig.frequency )
  {
    case 48000:
      val = 0;
      break;
    case 44100:
      val = 1;
      break;
    case 22050:
      val = 2;
      break;
    case 11025:
      val = 3;
      break;
  }
  SendDlgItemMessage( hWnd, IDC_FREQUENCY, CB_SETCURSEL, (WPARAM)val, 0 );

  //channel
  val = ( currentConfig.SidConfig.playback == SidConfig::MONO ) ? val = 0 : val = 1;
  SendDlgItemMessage( hWnd, IDC_CHANNELS, CB_SETCURSEL, (WPARAM)val, 0 );
  //C64 model
  SendDlgItemMessage( hWnd, IDC_C64MODEL, CB_SETCURSEL, (WPARAM)currentConfig.SidConfig.defaultC64Model, 0 );
  //force c64 model
  if ( currentConfig.SidConfig.forceC64Model )
  {
    CheckDlgButton( hWnd, IDC_FORCE_C64MODEL, BST_CHECKED );
  }
  else
  {
    CheckDlgButton( hWnd, IDC_FORCE_C64MODEL, BST_UNCHECKED );
  }

  //sid model
  SendDlgItemMessage( hWnd, IDC_SIDMODEL, CB_SETCURSEL, (WPARAM)currentConfig.SidConfig.defaultSidModel, 0 );
  //force SID model
  if ( currentConfig.SidConfig.forceSidModel )
  {
    CheckDlgButton( hWnd, IDC_FORCE_SID_MODEL, BST_CHECKED );
  }
  else
  {
    CheckDlgButton( hWnd, IDC_FORCE_SID_MODEL, BST_UNCHECKED );
  }

  /*
  if (playerConfig->sidConfig.forceSecondSidModel) CheckDlgButton(hWnd, IDC_FORCE_SID2_MODEL, BST_CHECKED);
  else */
  CheckDlgButton( hWnd, IDC_FORCE_SID2_MODEL, BST_UNCHECKED );

  if ( currentConfig.PlayLimitEnabled )
  {
    CheckDlgButton( hWnd, IDC_PLAYLIMIT_CHK, BST_CHECKED );
  }
  else
  {
    CheckDlgButton( hWnd, IDC_PLAYLIMIT_CHK, BST_UNCHECKED );
  }
  SetDlgItemTextA( hWnd, IDC_PLAYLIMITTIME, NumberToString( currentConfig.PlayLimitSec ).c_str() );

  if ( currentConfig.UseSongLengthFile )
  {
    CheckDlgButton( hWnd, IDC_ENABLESONGLENDB, BST_CHECKED );
  }
  else
  {
    CheckDlgButton( hWnd, IDC_ENABLESONGLENDB, BST_UNCHECKED );
  }
  if ( !currentConfig.SongLengthsFile.empty() )
  {
    SetDlgItemTextA( hWnd, IDC_SONGLENGTHFILE, currentConfig.SongLengthsFile.c_str() );
  }
  CheckDlgButton( hWnd, IDC_ENABLESTIL, ( ( currentConfig.UseSTILfile ) ? BST_CHECKED : BST_UNCHECKED ) );
  SetDlgItemTextA( hWnd, IDC_HVSCDIR, currentConfig.hvscDirectory.c_str() );

  //pseudo stereo
  CheckDlgButton( hWnd, IDC_PSEUDOSTEREO, ( ( currentConfig.PseudoStereo ) ? BST_CHECKED : BST_UNCHECKED ) );
  //second sid model (pseudo stereo)
  SendDlgItemMessage( hWnd, IDC_SID2MODEL, CB_SETCURSEL, (WPARAM)currentConfig.Sid2Model, 0 );

  if ( !currentConfig.PlaylistFormat.empty() )
  {
    SetDlgItemTextA( hWnd, IDC_PLAYLIST_FORMAT, currentConfig.PlaylistFormat.c_str() );
  }
  else
  {
    SetDlgItemTextA( hWnd, IDC_PLAYLIST_FORMAT, "%t %x / %a / %r / %sn" );
  }

  if ( !currentConfig.SubsongFormat.empty() )
  {
    SetDlgItemTextA( hWnd, IDC_SUBSONG_FORMAT, currentConfig.SubsongFormat.c_str() );
  }
  else
  {
    SetDlgItemTextA( hWnd, IDC_SUBSONG_FORMAT, "(Tune %n)" );
  }

  char    temp[2048];
  if ( currentConfig.NumLoopTimes > 0 )
  {
    sprintf( temp, "%d", currentConfig.NumLoopTimes );
    SetDlgItemTextA( hWnd, IDC_EDIT_LOOP_TIMES, temp );
  }

  CheckDlgButton( hWnd, IDC_VOICE00, ( currentConfig.VoiceConfig[0][0] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE01, ( currentConfig.VoiceConfig[0][1] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE02, ( currentConfig.VoiceConfig[0][2] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE10, ( currentConfig.VoiceConfig[1][0] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE11, ( currentConfig.VoiceConfig[1][1] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE12, ( currentConfig.VoiceConfig[1][2] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE20, ( currentConfig.VoiceConfig[2][0] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE21, ( currentConfig.VoiceConfig[2][1] == false ) ? BST_UNCHECKED : BST_CHECKED );
  CheckDlgButton( hWnd, IDC_VOICE22, ( currentConfig.VoiceConfig[2][2] == false ) ? BST_UNCHECKED : BST_CHECKED );
}



void UpdateConfig( HWND hWnd )
{
  const int MAX_BUFFER_SIZE = 100;
  int val;
  char buf[MAX_BUFFER_SIZE];


  val = SendDlgItemMessage( hWnd, IDC_FREQUENCY, CB_GETCURSEL, 0, 0 );
  switch ( val )
  {
    case 0:
      currentConfig.SidConfig.frequency = 48000;
      break;
    case 1:
      currentConfig.SidConfig.frequency = 44100;
      break;
    case 2:
      currentConfig.SidConfig.frequency = 22050;
      break;
    case 3:
      currentConfig.SidConfig.frequency = 11025;
      break;
  }

  //playback channels
  val = SendDlgItemMessage( hWnd, IDC_CHANNELS, CB_GETCURSEL, 0, 0 );
  currentConfig.SidConfig.playback = ( val == 0 ) ? SidConfig::MONO : SidConfig::STEREO;

  //C64 model
  val = SendDlgItemMessage( hWnd, IDC_C64MODEL, CB_GETCURSEL, 0, 0 );
  currentConfig.SidConfig.defaultC64Model = ( SidConfig::c64_model_t )val;
  if ( IsDlgButtonChecked( hWnd, IDC_FORCE_C64MODEL ) == BST_CHECKED )
  {
    currentConfig.SidConfig.forceC64Model = true;
  }
  else
  {
    currentConfig.SidConfig.forceC64Model = false;
  }

  //SID model
  val = SendDlgItemMessage( hWnd, IDC_SIDMODEL, CB_GETCURSEL, 0, 0 );
  currentConfig.SidConfig.defaultSidModel = ( SidConfig::sid_model_t ) val;
  if ( IsDlgButtonChecked( hWnd, IDC_FORCE_SID_MODEL ) == BST_CHECKED )
  {
    currentConfig.SidConfig.forceSidModel = true;
  }
  else
  {
    currentConfig.SidConfig.forceSidModel = false;
  }

  //currentConfig.sidConfig.forceSecondSidModel = (IsDlgButtonChecked(hWnd, IDC_FORCE_SID2_MODEL) == BST_CHECKED) ? true : false;

  if ( IsDlgButtonChecked( hWnd, IDC_PLAYLIMIT_CHK ) == BST_CHECKED )
  {
    GetDlgItemTextA( hWnd, IDC_PLAYLIMITTIME, buf, 20 );
    currentConfig.PlayLimitEnabled = true;
    currentConfig.PlayLimitSec = atoi( buf );
  }
  else
  {
    currentConfig.PlayLimitEnabled = false;
  }

  if ( IsDlgButtonChecked( hWnd, IDC_ENABLESONGLENDB ) == BST_CHECKED )
  {
    currentConfig.UseSongLengthFile = true;
  }
  else
  {
    currentConfig.UseSongLengthFile = false;
  }

  currentConfig.UseSTILfile = ( IsDlgButtonChecked( hWnd, IDC_ENABLESTIL ) == BST_CHECKED ) ? true : false;

  //voice configuration
  currentConfig.VoiceConfig[0][0] = ( IsDlgButtonChecked( hWnd, IDC_VOICE00 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[0][1] = ( IsDlgButtonChecked( hWnd, IDC_VOICE01 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[0][2] = ( IsDlgButtonChecked( hWnd, IDC_VOICE02 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[1][0] = ( IsDlgButtonChecked( hWnd, IDC_VOICE10 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[1][1] = ( IsDlgButtonChecked( hWnd, IDC_VOICE11 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[1][2] = ( IsDlgButtonChecked( hWnd, IDC_VOICE12 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[2][0] = ( IsDlgButtonChecked( hWnd, IDC_VOICE20 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[2][1] = ( IsDlgButtonChecked( hWnd, IDC_VOICE21 ) == BST_CHECKED ) ? true : false;
  currentConfig.VoiceConfig[2][2] = ( IsDlgButtonChecked( hWnd, IDC_VOICE22 ) == BST_CHECKED ) ? true : false;

  //pseudostereo
  currentConfig.PseudoStereo = ( IsDlgButtonChecked( hWnd, IDC_PSEUDOSTEREO ) == BST_CHECKED ) ? true : false;

  //SID model
  val = SendDlgItemMessage( hWnd, IDC_SID2MODEL, CB_GETCURSEL, 0, 0 );
  currentConfig.Sid2Model = ( SidConfig::sid_model_t ) val;

  //playlist format
  GetDlgItemTextA( hWnd, IDC_PLAYLIST_FORMAT, buf, MAX_BUFFER_SIZE );
  currentConfig.PlaylistFormat = buf;

  //subsong format
  GetDlgItemTextA( hWnd, IDC_SUBSONG_FORMAT, buf, MAX_BUFFER_SIZE );
  currentConfig.SubsongFormat = buf;

  // loop times
  GetDlgItemTextA( hWnd, IDC_EDIT_LOOP_TIMES, buf, MAX_BUFFER_SIZE );
  currentConfig.NumLoopTimes = atoi( buf );
  if ( currentConfig.NumLoopTimes < 0 )
  {
    currentConfig.NumLoopTimes = 0;
  }
}



void SelectHvscFile( HWND hWnd )
{
  wchar_t path[MAX_PATH];
  size_t pathLen;


  if ( GetFileNameFromBrowse( hWnd, path, MAX_PATH, L"c:\\", L"txt", L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0",
                              L"Select song length db file" ) != TRUE ) 
  {
    return;
  }
  pathLen = wcslen( path ) + 1;

  char*     pDummy = new char[pathLen];

  size_t    numBytesConverted = 0;
  
  if ( wcstombs_s( &numBytesConverted, pDummy, pathLen, path, pathLen ) )
  {
    // do what?
    delete[] pDummy;
    return;
  }

  currentConfig.SongLengthsFile = pDummy;
  delete[] pDummy;

  SetDlgItemTextA( hWnd, IDC_SONGLENGTHFILE, currentConfig.SongLengthsFile.c_str() );
}



void SelectHvscDirectory( HWND hWnd )
{
  wchar_t       path[MAX_PATH];
  size_t        pathLen;
  LPITEMIDLIST  idlRoot = NULL;

  BROWSEINFO bi = { 0 };

  bi.lpszTitle = L"Select HVSC directory";

  LPITEMIDLIST pidl = SHBrowseForFolder( &bi );

  if ( pidl != NULL )
  {
    // get the name of the folder and put it in path
    SHGetPathFromIDList( pidl, path );
    IMalloc* imalloc = 0;
    if ( SUCCEEDED( SHGetMalloc( &imalloc ) ) )
    {
      imalloc->Free( pidl );
      imalloc->Release();
    }

    pathLen = wcslen( path ) + 1;

    char*     pDummy = new char[pathLen];

    size_t    numBytesConverted = 0;

    if ( wcstombs_s( &numBytesConverted, pDummy, pathLen, path, pathLen ) )
    {
      // do what?
      delete[] pDummy;
      return;
    }

    currentConfig.hvscDirectory = pDummy;
    delete[] pDummy;

    SetDlgItemTextA( hWnd, IDC_HVSCDIR, currentConfig.hvscDirectory.c_str() );
  }
}



int CALLBACK ConfigDlgWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    case WM_COMMAND:
      {
        int wmId = LOWORD( wParam );
        switch ( wmId )
        {
          case IDOK:
            UpdateConfig( hWnd );
            s_Plugin.m_SIDPlayer.SaveConfigToFile( currentConfig );
            s_Plugin.m_SIDPlayer.SetConfig( currentConfig );
            EndDialog( hWnd, wmId );
            break;
          case IDC_BROWSE_BTN:
            SelectHvscFile( hWnd );
            break;
          case IDC_BROWSE_HVSC:
            SelectHvscDirectory( hWnd );
            break;
          case IDCANCEL:
            EndDialog( hWnd, wmId );
            break;
        }
        if ( wmId == IDCANCEL )
        {
          EndDialog( hWnd, wmId );
        }
      }
      return TRUE;
    case WM_DESTROY:
      return TRUE;
    case WM_INITDIALOG:
      ConfigDlgInitDialog( hWnd );
      return TRUE;
  }

  return FALSE;
}
