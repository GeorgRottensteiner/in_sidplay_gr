#include <windows.h>
#include "ThreadSidPlayer.h"
#include "SidInfoImpl.h"
#include "c64roms.h"

#include "SIDPlugin.h"
#include "SubSongDlg.h"



#define WM_WA_MPEG_EOF WM_USER+2



CThreadSidPlayer::CThreadSidPlayer( In_Module& inWAmod ) : 
  m_tune( 0 ), 
  m_threadHandle( INVALID_HANDLE_VALUE )
{
  m_inmod                           = &inWAmod;
  m_decodeBuf                       = NULL;
  m_decodeBufLen                    = 0;
  m_playerStatus                    = SP_STOPPED;
  m_playerConfig.PlayLimitEnabled   = false;
  m_playerConfig.PlayLimitSec       = 120;
  m_playerConfig.SongLengthsFile    = "";
  m_playerConfig.UseSongLengthFile  = false;
  m_playerConfig.UseSTILfile        = false;
  m_playerConfig.hvscDirectory      = "";
  m_playerConfig.VoiceConfig[0][0]  = true;
  m_playerConfig.VoiceConfig[0][1]  = true;
  m_playerConfig.VoiceConfig[0][2]  = true;
  m_playerConfig.VoiceConfig[1][0]  = true;
  m_playerConfig.VoiceConfig[1][1]  = true;
  m_playerConfig.VoiceConfig[1][2]  = true;
  m_playerConfig.VoiceConfig[2][0]  = true;
  m_playerConfig.VoiceConfig[2][1]  = true;
  m_playerConfig.VoiceConfig[2][2]  = true;
  m_playerConfig.PlaylistFormat     = "%t %x %sn / %a / %r %st";
  m_playerConfig.SubsongFormat      = "(Tune %n/%ns)";

  m_playerConfig.PseudoStereo       = false;
  m_playerConfig.Sid2Model          = SidConfig::sid_model_t::MOS6581;
  m_currentTuneLength               = -1;
  m_MaxLatency                      = 0;
  m_seekNeedMs                      = 0;
  m_pEngine                         = new sidplayfp();
}



CThreadSidPlayer::~CThreadSidPlayer()
{
  if ( m_decodeBufLen > 0 )
  {
    delete[] m_decodeBuf;
  }
  ClearSTILData();
  if ( m_pEngine != NULL )
  {
    if ( m_playerConfig.SidConfig.sidEmulation != NULL )
    {
      delete m_playerConfig.SidConfig.sidEmulation;
      m_playerConfig.SidConfig.sidEmulation = NULL;
    }
    delete m_pEngine;
  }
}



void CThreadSidPlayer::Init()
{
  if ( m_playerStatus != SP_STOPPED )
  {
    Stop();
  }

  m_playerConfig.SidConfig = m_pEngine->config();
  //m_playerConfig.SidConfig.sampleFormat = SID2_LITTLE_SIGNED;	

  if ( !LoadConfigFromFile( m_playerConfig ) )
  {
    //if load fails then use this default settings
    //m_playerConfig.SidConfig.precision = 16;
    SidConfig    defaultConfig;
    memcpy( ( void* )&( m_playerConfig.SidConfig ), &defaultConfig, sizeof( SidConfig ) );

    m_playerConfig.SidConfig.frequency  = 44100;
    m_playerConfig.SidConfig.playback   = SidConfig::MONO;// sid2_mono;
  }
  //m_playerConfig.SidConfig.sampleFormat = SID2_LITTLE_SIGNED;	

  SetConfig( m_playerConfig );
}



void CThreadSidPlayer::Play()
{
  int numChann;

  if ( m_playerStatus == SP_RUNNING )
  {
    return;
  }
  if ( m_playerStatus == SP_PAUSED )
  {
    m_playerStatus = SP_RUNNING;
    m_inmod->outMod->Pause( 0 );
    ResumeThread( m_threadHandle );
    return;
  }

  //if stopped then create new thread to play
  if ( m_playerStatus == SP_STOPPED )
  {
    numChann      = ( m_playerConfig.SidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
    m_MaxLatency  = m_inmod->outMod->Open( m_playerConfig.SidConfig.frequency, numChann, PLAYBACK_BIT_PRECISION, -1, -1 );

    m_inmod->SetInfo( ( m_playerConfig.SidConfig.frequency * PLAYBACK_BIT_PRECISION * numChann ) / 1000, m_playerConfig.SidConfig.frequency / 1000, numChann, 1 );
    //visualization init
    m_inmod->SAVSAInit( m_MaxLatency, m_playerConfig.SidConfig.frequency );
    m_inmod->VSASetInfo( m_playerConfig.SidConfig.frequency, numChann );
    //default volume
    m_inmod->outMod->SetVolume( -666 );
    m_playerStatus = SP_RUNNING;
    m_threadHandle = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)CThreadSidPlayer::Run, this, 0, NULL );
  }
}



PlayerStatus_t CThreadSidPlayer::GetPlayerStatus() 
{ 
  return m_playerStatus; 
}



void CThreadSidPlayer::Pause()
{
  if ( m_playerStatus == SP_RUNNING )
  {
    SuspendThread( m_threadHandle );
    m_inmod->outMod->Pause( 1 );
    m_playerStatus = SP_PAUSED;
  }
}



void CThreadSidPlayer::Stop()
{
  if ( m_playerStatus == SP_STOPPED )
  {
    return;
  }
  m_playerStatus = SP_STOPPED;
  if ( WaitForSingleObject( m_threadHandle, 3000 ) == WAIT_TIMEOUT )
  {
    TerminateThread( m_threadHandle, 0 );
  }
  m_pEngine->stop();
  CloseHandle( m_threadHandle );
  m_threadHandle = INVALID_HANDLE_VALUE;

  // close output system
  if ( m_inmod->outMod != NULL )
  {
    m_inmod->outMod->Close();
  }
  // deinitialize visualization
  m_inmod->SAVSADeInit();
}



void CThreadSidPlayer::LoadTune( const char* name )
{
  Stop();

  m_tune.load( name );
  const SidTuneInfo* tuneInfo = m_tune.getInfo();
  if ( tuneInfo == NULL )
  {
    return;
  }
  m_tune.selectSong( m_tune.getInfo()->startSong() );

  m_currentTuneLength = m_sidDatabase.length( m_tune );
  if ( ( m_playerConfig.PlayLimitEnabled ) 
  &&   ( m_currentTuneLength <= 0 ) )
  {
    m_currentTuneLength = m_playerConfig.PlayLimitSec;
  }

  if ( m_playerConfig.NumLoopTimes > 0 )
  {
    m_currentTuneLength *= m_playerConfig.NumLoopTimes;
  }

  m_pEngine->load( &m_tune );

  //mute must be applied after SID's have been created
  for ( int sid = 0; sid < 3; ++sid )
  {
    for ( int voice = 0; voice < 3; ++voice )
    {
      m_pEngine->mute( sid, voice, !m_playerConfig.VoiceConfig[sid][voice] );
    }
  }

  s_Plugin.m_pSubSongDlg->UpdateScrollBar( tuneInfo->songs(), tuneInfo->currentSong() );
}



DWORD CThreadSidPlayer::Run( void* thisparam )
{
  int desiredLen;
  int decodedLen;
  int numChn;
  int bps;
  int dspDataLen = 0;
  int freq;
  CThreadSidPlayer* playerObj = static_cast<CThreadSidPlayer*>( thisparam );
  int timeElapsed;

  playerObj->m_decodedSampleCount = 0;
  playerObj->m_playTimems = 0;
  bps = PLAYBACK_BIT_PRECISION;//playerObj->m_playerConfig.SidConfig.precision;
  numChn = ( playerObj->m_playerConfig.SidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
  freq = playerObj->m_playerConfig.SidConfig.frequency;
  desiredLen = 576 * ( PLAYBACK_BIT_PRECISION >> 3 ) * numChn * ( playerObj->m_inmod->dsp_isactive() ? 2 : 1 );


  while ( playerObj->m_playerStatus != SP_STOPPED )
  {
    if ( playerObj->m_inmod->outMod->CanWrite() >= desiredLen )
    {
      //decode music data from libsidplay object
      //pierwotnie libsidplay operowa³ na bajtach i wszystkie d³ugoœci bufora by³y w bajtach
      //libsidplayfp operuje na samplach 16 bitowych wiêc musimy odpowiednio mni¿yæ lub dzieliæ przez 2 liczbê bajtów
      decodedLen = 2 * playerObj->m_pEngine->play( reinterpret_cast<short*>( playerObj->m_decodeBuf ), desiredLen / 2 );
      //playerObj->m_decodedSampleCount += decodedLen / numChn / (bps>>3);
      //write it to vis subsystem
      playerObj->m_inmod->SAAddPCMData( playerObj->m_decodeBuf, numChn, bps, (int)playerObj->m_playTimems );
      playerObj->m_inmod->VSAAddPCMData( playerObj->m_decodeBuf, numChn, bps, (int)playerObj->m_playTimems );

      playerObj->m_decodedSampleCount += decodedLen / numChn / ( bps >> 3 );
      playerObj->m_playTimems = ( playerObj->m_decodedSampleCount * 1000 ) / playerObj->m_playerConfig.SidConfig.frequency;
      //use DSP plugin on data
      if ( playerObj->m_inmod->dsp_isactive() )
      {
        decodedLen = playerObj->m_inmod->dsp_dosamples( reinterpret_cast<short*>( playerObj->m_decodeBuf ),
                              decodedLen / numChn / ( bps >> 3 ), bps, numChn, freq );
        decodedLen *= ( numChn * ( bps >> 3 ) );
      }
      playerObj->m_inmod->outMod->Write( playerObj->m_decodeBuf, decodedLen );
    }
    else
    {
      //do we need to seek ??
      if ( playerObj->m_seekNeedMs > 0 )
      {
        playerObj->DoSeek();
      }
      else
      {
        Sleep( 20 );
      }
    }

    //timeElapsed = playerObj->GetPlayTime();
    //timeElapsed = playerObj->m_inmod->outMod->GetOutputTime();
    timeElapsed = (int)playerObj->m_playTimems;
    //if we know the song length and timer just reached it then go to next song

    if ( playerObj->GetSongLength() >= 1 )
    {
      if ( playerObj->GetSongLength() * 1000 < timeElapsed )
      {
        playerObj->m_playerStatus = SP_STOPPED;
        PostMessage( playerObj->m_inmod->hMainWindow, WM_WA_MPEG_EOF, 0, 0 );
        return 0;
      }
      //Sleep(10);
    }
    else if ( playerObj->m_playerConfig.PlayLimitEnabled )
    {
      //if we dont know song length but time limit is enabled then check it
      if ( ( playerObj->m_playerConfig.PlayLimitSec * 1000 ) < timeElapsed )
      {
        playerObj->m_playerStatus = SP_STOPPED;
        PostMessage( playerObj->m_inmod->hMainWindow, WM_WA_MPEG_EOF, 0, 0 );
        //Sleep(10);
        return 0;
      }
    }
    //no song length, and no length limit so play for infinity
  }

  return 0;
}



int CThreadSidPlayer::CurrentSubtune()
{
  if ( m_tune.getStatus() )
  {
    return m_tune.getInfo()->currentSong();
  }
  return 0;
}



int CThreadSidPlayer::GetNumSubtunes()
{
  if ( m_tune.getStatus() )
  {
    return m_tune.getInfo()->songs();
  }
  return 0;
}



void CThreadSidPlayer::PlaySubtune( int SubTune )
{
  Stop();
  m_tune.selectSong( SubTune );
  m_currentTuneLength = m_sidDatabase.length( m_tune );
  if ( ( m_playerConfig.PlayLimitEnabled ) 
  &&   ( m_currentTuneLength <= 0 ) )
  {
    m_currentTuneLength = m_playerConfig.PlayLimitSec;
  }
  m_pEngine->stop();
  m_pEngine->load( &m_tune );

  const SidTuneInfo*    pTuneInfo = m_tune.getInfo();

  if ( pTuneInfo != NULL )
  {
    s_Plugin.m_pSubSongDlg->UpdateScrollBar( pTuneInfo->songs(), pTuneInfo->currentSong() );
  }

  Play();
}



const SidTuneInfo* CThreadSidPlayer::GetTuneInfo()
{
  return ( m_tune.getStatus() ) ? m_tune.getInfo() : NULL; //SidTuneInfo();
}



int CThreadSidPlayer::GetPlayTime()
{
  return (int)( m_playTimems + ( m_inmod->outMod->GetOutputTime() - m_inmod->outMod->GetWrittenTime() ) );
}



bool CThreadSidPlayer::LoadConfigFromFile( PlayerConfig& Config )
{
  wchar_t   appDataPath[MAX_PATH];

  //try to load config from common file
  SHGetSpecialFolderPath( NULL, appDataPath, CSIDL_COMMON_APPDATA, 0 );
  wcscat_s( appDataPath, L"\\in_sidplay2.ini" );
  return LoadConfigFromFile( Config, appDataPath );
}



bool CThreadSidPlayer::LoadConfigFromFile( PlayerConfig& Config, wchar_t* fileName )
{
  char        cLine[200 + MAX_PATH];
  int         maxLen = 200 + MAX_PATH;
  std::string sLine;
  std::string token;
  std::string value;
  int         pos;
  FILE*       cfgFile = NULL;

  if ( _wfopen_s( &cfgFile, fileName, L"rb" ) )
  {
    return false;
  }
  if ( cfgFile == NULL )
  {
    return false;
  }

  while ( feof( cfgFile ) == 0 )
  {
    ReadLine( cLine, cfgFile, maxLen );
    if ( strlen( cLine ) == 0 )
    {
      continue;
    }

    sLine.assign( cLine );
    pos     = sLine.find( "=" );
    token   = sLine.substr( 0, pos );
    value   = sLine.substr( pos + 1 );

    if ( ( token.length() == 0 ) 
    ||   ( value.length() == 0 ) ) 
    {
      continue;
    }
    while ( ( value.at( 0 ) == '\"' ) 
    &&      ( value.at( value.length() - 1 ) != '\"' ) 
    &&      ( !feof( cfgFile ) ) )
    {
      ReadLine( cLine, cfgFile, maxLen );
      sLine.append( cLine );
    }
    if ( ( value.at( 0 ) == '\"' ) 
    &&   ( value.at( value.length() - 1 ) == '\"' ) )
    {
      value = value.substr( 1, value.length() - 2 );
    }
    AssignConfigValue( Config, token, value );
  }
  fclose( cfgFile );
  return true;
}



void CThreadSidPlayer::ReadLine( char* buf, FILE* file, const int maxBuf )
{
  char    c;
  size_t  readCount;
  int     pos = 0;

  do
  {
    readCount = fread( &c, 1, 1, file );
    if ( readCount == 0 )
    {
      break;
    }
    if ( ( c != '\r' ) 
    &&   ( c != '\n' ) )
    {
      buf[pos++] = c;
    }
    if ( pos == maxBuf )
    {
      break;
    }
  } 
  while ( c != '\n' );
  buf[pos] = '\0';
}



void CThreadSidPlayer::SaveConfigToFile( PlayerConfig& Config )
{
  wchar_t appDataPath[MAX_PATH];

  //try to load config from common file
  //SHGetSpecialFolderPath(NULL,appDataPath,CSIDL_COMMON_APPDATA,0);

  if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, 0, appDataPath ) ) )
  {
    wcscat_s( appDataPath, L"\\in_sidplay2.ini" );
    SaveConfigToFile( Config, appDataPath );
  }
}



void CThreadSidPlayer::SaveConfigToFile( PlayerConfig& Config, wchar_t* fileName )
{
  SidConfig* conf = &Config.SidConfig;
  std::ofstream outFile( fileName );

  outFile << "PlayFrequency=" << conf->frequency << std::endl;
  outFile << "PlayChannels=" << ( ( conf->playback == SidConfig::MONO ) ? 1 : 2 ) << std::endl;
  outFile << "C64Model=" << conf->defaultC64Model << std::endl;
  outFile << "C64ModelForced=" << conf->forceC64Model << std::endl;
  outFile << "SidModel=" << conf->defaultSidModel << std::endl;
  outFile << "SidModelForced=" << conf->forceSidModel << std::endl;
  //outFile << "Sid2ModelForced=" << conf->forceSecondSidModel << std::endl;

  outFile << "PlayLimitEnabled=" << Config.PlayLimitEnabled << std::endl;
  outFile << "PlayLimitTime=" << Config.PlayLimitSec << std::endl;
  outFile << "UseSongLengthFile=" << Config.UseSongLengthFile << std::endl;
  if ( ( !Config.UseSongLengthFile ) 
  ||   ( Config.SongLengthsFile.empty() ) ) 
  {
    outFile << "SongLengthsFile=" << "" << std::endl;
  }
  else
  {
    outFile << "SongLengthsFile=" << Config.SongLengthsFile << std::endl;
  }
  outFile << "UseSTILFile=" << Config.UseSTILfile << std::endl;
  if ( Config.hvscDirectory.empty() )
  {
    Config.UseSTILfile = false;
  }
  if ( !Config.UseSTILfile )
  {
    outFile << "HVSCDir=" << "" << std::endl;
  }
  else
  {
    outFile << "HVSCDir=" << Config.hvscDirectory << std::endl;
  }
  outFile << "UseSongLengthFile=" << Config.UseSongLengthFile << std::endl;

  outFile << "VoiceConfig=";
  for ( int sid = 0; sid < 3; ++sid )
  {
    for ( int voice = 0; voice < 3; ++voice )
    {
      outFile << Config.VoiceConfig[sid][voice];
    }
  }
  outFile << std::endl;
  outFile << "PseudoStereo=" << Config.PseudoStereo << std::endl;
  outFile << "Sid2Model=" << Config.Sid2Model << std::endl;
  outFile << "PlaylistFormat=" << Config.PlaylistFormat << std::endl;
  outFile << "SubsongFormat=" << Config.SubsongFormat << std::endl;
  outFile << "NumLoopTimes=" << Config.NumLoopTimes << std::endl;
  outFile.close();
}



void CThreadSidPlayer::AssignConfigValue( PlayerConfig& Config, const std::string& token, const std::string& value )
{
  SidConfig* conf = &Config.SidConfig;
  if ( token == "PlayFrequency" ) 
  { 
    conf->frequency = atoi( value.c_str() ); 
    return; 
  }
  if ( token == "PlayChannels" )
  {
    if ( value == "1" )
    {
      conf->playback = SidConfig::MONO;
    }
    else
    {
      conf->playback = SidConfig::STEREO;
    }
    return;
  }
  if ( token == "C64Model" )
  {
    conf->defaultC64Model = ( SidConfig::c64_model_t )atoi( value.c_str() );
    return;
  }
  if ( token == "C64ModelForced" )
  {
    conf->forceC64Model = (bool)!!atoi( value.c_str() );
    return;
  }

  if ( token == "SidModel" )
  {
    conf->defaultSidModel = ( SidConfig::sid_model_t )atoi( value.c_str() );
    return;
  }

  if ( token == "VoiceConfig" )
  {
    int digitId = 0;
    for ( int sid = 0; sid < 3; ++sid )
    {
      for ( int voice = 0; voice < 3; ++voice )
      {
        Config.VoiceConfig[sid][voice] = ( value.at( digitId++ ) == '1' );
      }
    }
    return;
  }

  if ( token == "Sid2Model" )
  {
    Config.Sid2Model = ( SidConfig::sid_model_t )atoi( value.c_str() );
    return;
  }

  if ( token == "PseudoStereo" )
  {
    Config.PseudoStereo = (bool)!!atoi( value.c_str() );
    return;
  }

  if ( token == "SidModelForced" )
  {
    conf->forceSidModel = (bool)!!atoi( value.c_str() );
    return;
  }

  /*
  if (token.compare("Sid2ModelForced") == 0)
  {
    conf->forceSecondSidModel = (bool)atoi(value.c_str());
    return;
  }*/

  if ( token == "PlayLimitEnabled" )
  {
    Config.PlayLimitEnabled = (bool)!!atoi( value.c_str() );
    return;
  }
  if ( token == "PlayLimitTime" )
  {
    Config.PlayLimitSec = atoi( value.c_str() );
    return;
  }

  if ( token == "UseSongLengthFile" )
  {
    Config.UseSongLengthFile = (bool)!!atoi( value.c_str() );
    return;
  }
  if ( token == "SongLengthsFile" )
  {
    Config.SongLengthsFile = value;
    return;
  }

  if ( token == "HVSCDir" )
  {
    Config.hvscDirectory = value;
    return;
  }
  if ( token == "UseSTILFile" )
  {
    Config.UseSTILfile = (bool)!!atoi( value.c_str() );
    return;
  }

  if ( token == "PlaylistFormat" )
  {
    Config.PlaylistFormat = value;
    return;
  }

  if ( token == "SubsongFormat" )
  {
    Config.SubsongFormat = value;
    return;
  }

  if ( token == "NumLoopTimes" )
  {
    Config.NumLoopTimes = atoi( value.c_str() );
    return;
  }
}



const PlayerConfig& CThreadSidPlayer::GetCurrentConfig()
{
  return m_playerConfig;
}



void CThreadSidPlayer::SetConfig( PlayerConfig& Config )
{
  int numChann;
  bool openRes;

  if ( m_playerStatus != SP_STOPPED )
  {
    Stop();
  }
  m_pEngine->stop();

  sidbuilder* currentBuilder = m_playerConfig.SidConfig.sidEmulation;
  if ( m_playerConfig.SidConfig.sidEmulation != NULL )
  {
    //delete m_playerConfig.SidConfig.sidEmulation;
  }
  m_playerConfig.SidConfig.sidEmulation = 0;
  m_pEngine->config( m_playerConfig.SidConfig );
  if ( currentBuilder != NULL )
  {
    delete currentBuilder;
  }

  //change assign to memcpy !
  m_playerConfig.SidConfig.frequency = Config.SidConfig.frequency;
  m_playerConfig.SidConfig.playback = Config.SidConfig.playback;
  m_playerConfig.SidConfig.defaultC64Model = Config.SidConfig.defaultC64Model;
  m_playerConfig.SidConfig.forceC64Model = Config.SidConfig.forceC64Model;
  m_playerConfig.SidConfig.defaultSidModel = Config.SidConfig.defaultSidModel;
  m_playerConfig.SidConfig.forceSidModel = Config.SidConfig.forceSidModel;
  //m_playerConfig.SidConfig.forceSecondSidModel = Config.SidConfig.forceSecondSidModel;
  //m_playerConfig.SidConfig.secondSidModel = Config.SidConfig.secondSidModel;



  m_playerConfig.PlayLimitEnabled = Config.PlayLimitEnabled;
  m_playerConfig.PlayLimitSec = Config.PlayLimitSec;
  m_playerConfig.UseSongLengthFile = Config.UseSongLengthFile;

  m_playerConfig.SidConfig.samplingMethod = SidConfig::INTERPOLATE; //RESAMPLE_INTERPOLATE

  for ( int sid = 0; sid < 3; ++sid )
  {
    for ( int voice = 0; voice < 3; ++voice )
    {
      m_playerConfig.VoiceConfig[sid][voice] = Config.VoiceConfig[sid][voice];
    }
  }
  m_playerConfig.PseudoStereo = Config.PseudoStereo;
  m_playerConfig.Sid2Model = Config.Sid2Model;



  //TODO czy trzeba drugi i trzeci adres sida??????

  //string memory cannot overlap !!!
  if ( m_playerConfig.SongLengthsFile != Config.SongLengthsFile )
  {
    m_playerConfig.SongLengthsFile = "";
    if ( !Config.SongLengthsFile.empty() )
    {
      m_playerConfig.SongLengthsFile = Config.SongLengthsFile;
    }
  }

  m_playerConfig.UseSTILfile = Config.UseSTILfile;
  if ( m_playerConfig.hvscDirectory != Config.hvscDirectory )
  {
    m_playerConfig.hvscDirectory = "";
    if ( !Config.hvscDirectory.empty() )
    {
      m_playerConfig.hvscDirectory = Config.hvscDirectory;
    }
  }

  if ( Config.PlaylistFormat != m_playerConfig.PlaylistFormat )
  {
    m_playerConfig.PlaylistFormat = Config.PlaylistFormat;
  }

  if ( Config.SubsongFormat != m_playerConfig.SubsongFormat )
  {
    m_playerConfig.SubsongFormat = "";
    m_playerConfig.SubsongFormat = Config.SubsongFormat;
  }

  m_playerConfig.NumLoopTimes = Config.NumLoopTimes;

  ReSIDfpBuilder* rs = new ReSIDfpBuilder( "ReSIDfp" );

  if ( rs )
  {
    m_playerConfig.SidConfig.sidEmulation = rs;
    rs->create( ( m_pEngine->info() ).maxsids() );


    rs->filter6581Curve( 0.5 );
    rs->filter8580Curve( (double)12500 );
    //filter always enabled
    rs->filter( true );
  }

  //TO CHANGE !!!!!!!
  if ( m_playerConfig.PseudoStereo )
  {
    m_playerConfig.SidConfig.secondSidAddress = 0xD400;
    //m_playerConfig.SidConfig.secondSidModel = m_playerConfig.sid2Model;
  }
  else
  {
    m_playerConfig.SidConfig.secondSidAddress = 0;
    //m_playerConfig.SidConfig.secondSidModel = -1;
  }
  //m_playerConfig.SidConfig.
  m_pEngine->config( m_playerConfig.SidConfig );


  //kernal,basic,chargen
  m_pEngine->setRoms( KERNAL_ROM, BASIC_ROM, CHARGEN_ROM );

  //create decode buf for 576 samples
  if ( m_decodeBufLen > 0 )
  {
    delete[] m_decodeBuf;
  }
  numChann = ( m_playerConfig.SidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
  m_decodeBufLen = 2 * 576 * ( PLAYBACK_BIT_PRECISION >> 3 ) * numChann;
  m_decodeBuf = new char[m_decodeBufLen];
  //open song length database
  if ( ( m_playerConfig.UseSongLengthFile ) && ( !m_playerConfig.SongLengthsFile.empty() ) )
  {
    openRes = m_sidDatabase.open( m_playerConfig.SongLengthsFile.c_str() );
    if ( !openRes )
    {
      std::string     message = "Error opening songlength database.\r\nDisable songlength database or choose other file.\r\nMessage was : ";
      message += m_sidDatabase.error();
      MessageBoxA( NULL, message.c_str(), "Error opening songlength database", MB_OK );
    }
  }
  //open STIL file
  if ( ( m_playerConfig.UseSTILfile ) 
  &&   ( !m_playerConfig.hvscDirectory.empty() ) )
  {
    ClearSTILData();
    FillSTILData();
    FillSTILData2();
  }
}



int CThreadSidPlayer::GetSongLength( SidTune& tune )
{
  int length;

  if ( !tune.getStatus() )
  {
    //MessageBoxA(NULL, "Tune status invalid", "Error", MB_OK);
    return -1;
  }

  length = m_sidDatabase.length( tune );
  if ( ( m_playerConfig.PlayLimitEnabled ) 
  &&   ( length <= 0 ) )
  {
    length = m_playerConfig.PlayLimitSec;
  }
  if ( m_playerConfig.NumLoopTimes > 0 )
  {
    length *= m_playerConfig.NumLoopTimes;
  }

  if ( length < 0 )
  {
    return 0;
  }
  return length;
}



int CThreadSidPlayer::GetSongLength()
{
  return m_currentTuneLength;
}



void CThreadSidPlayer::DoSeek()
{
  int bits;
  int skip_bytes;
  int bps = PLAYBACK_BIT_PRECISION;// m_playerConfig.SidConfig.precision;
  int numChn = ( m_playerConfig.SidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
  int freq = m_playerConfig.SidConfig.frequency;
  int decodedLen = 0;
  int timesek = m_seekNeedMs / 1000;
  if ( timesek == 0 ) return;

  if ( m_seekNeedMs <= m_playTimems )
  {
    timesek = m_seekNeedMs / 1000;
    if ( timesek == 0 )
    {
      return;
    }

    m_tune.selectSong( m_tune.getInfo()->currentSong() );
    //we know length of tune already
    m_pEngine->stop();
    m_pEngine->load( &m_tune );//timers are now 0
  }
  else
  {
    timesek = (int)( ( m_seekNeedMs - m_playTimems ) / 1000 );
    if ( timesek <= 0 )
    {
      return;
    }
  }

  bits = PLAYBACK_BIT_PRECISION;//m_playerConfig.SidConfig.precision;
  m_pEngine->fastForward( 3200 );
  skip_bytes = ( timesek * freq * numChn * ( bits >> 3 ) ) >> 5;
  //m_decodedSampleCount += skip_bytes / numChn / (bps>>3); //not needed
  while ( skip_bytes > m_decodeBufLen )
  {
    decodedLen = 2 * m_pEngine->play( reinterpret_cast<short*>( m_decodeBuf ), m_decodeBufLen / 2 );
    skip_bytes -= decodedLen;
  }
  /*
  if (skip_bytes >= 16)
  {
    //decodedLen = 2 * m_pEngine->play(reinterpret_cast<short*>(m_decodeBuf), skip_bytes / 2);
  }
  */
  //now take time calculationns from emulation engine and calculate other variables

  m_pEngine->time();
  m_playTimems = ( m_pEngine->time() * 1000 );// / timer->timebase();
  m_decodedSampleCount = ( m_playTimems * freq ) / 1000;
  //m_playTimems =(m_decodedSampleCount * 1000) / m_playerConfig.SidConfig.frequency;
  m_pEngine->fastForward( 100 );
  m_seekNeedMs = 0;
}



void CThreadSidPlayer::SeekTo( int timeMs )
{
  m_seekNeedMs = timeMs;
}



void CThreadSidPlayer::FillSTILData()
{
  const int     BUFLEN = 160;
  std::string   strKey;
  std::string   strInfo;
  char          buf[BUFLEN];

  m.clear();
  FILE* f = NULL;
  strcpy_s( buf, m_playerConfig.hvscDirectory.c_str() );
  strcat_s( buf, "\\documents\\stil.txt" );

  if ( ( fopen_s( &f, buf, "rb+" ) )
  ||   ( f == NULL ) )
  {
    MessageBoxA( NULL, "Error opening STIL file.\r\nDisable STIL info or choose appropriate HVSC directory", "in_sidplay2", MB_OK );
    return;
  }
  while ( feof( f ) == 0 )
  {
    ReadLine( buf, f, 160 );
    strKey.clear();
    strInfo.clear();
    if ( buf[0] == '/' ) //new file block
    {
      strKey.assign( buf );
      FixPath( strKey );//.replace("/","\\");
      ReadLine( buf, f, BUFLEN );
      while ( strlen( buf ) > 0 )
      {
        strInfo.append( buf );
        strInfo.append( "\r\n" );
        ReadLine( buf, f, BUFLEN );
      }
      m[strKey] = strInfo;
    }
  }
  fclose( f );
}



void CThreadSidPlayer::FillSTILData2()
{
  const int   BUFLEN = 160;
  const char* ARTIST = " ARTIST:";
  const char* TITLE = "  TITLE:";
  const char* COMMENT = "COMMENT:";
  const char* AUTHOR = " AUTHOR:";
  const char* NAME = "   NAME:";
  std::string strKey;
  //std::string strInfo;
  std::string tmpStr;
  char buf[BUFLEN];
  int currentSubsong;
  std::vector<StilBlock> subsongsInfo;

  m_stillMap2.clear();
  FILE* f = NULL;
  strcpy_s( buf, m_playerConfig.hvscDirectory.c_str() );
  strcat_s( buf, "\\documents\\stil.txt" );

  if ( ( fopen_s( &f, buf, "rb+" ) )
  ||   ( f == NULL ) )
  {
    MessageBoxA( NULL, "Error opening STIL file.\r\nDisable STIL info or choose appropriate HVSC directory", "in_sidplay2", MB_OK );
    return;
  }
  while ( feof( f ) == 0 )
  {
    ReadLine( buf, f, 160 );
    strKey.clear();
    if ( buf[0] == '/' ) //new file block
    {
      strKey.assign( buf );
      FixPath( strKey );//.replace("/","\\");
      currentSubsong = 0;

      ReadLine( buf, f, BUFLEN );

      StilBlock   stillBlock;


      subsongsInfo = m_stillMap2[strKey];
      subsongsInfo.push_back( StilBlock() );
      while ( strlen( buf ) > 0 )
      {
        tmpStr.assign( buf );
        //check for subsong numer
        if ( tmpStr.compare( 0, 2, "(#" ) == 0 )
        {
          int newSubsong = atoi( tmpStr.substr( 2, tmpStr.length() - 3 ).c_str() );
          //if subsong number is different than 1 then store current info and set subsong number to new value
          if ( newSubsong != 1 )
          {
            //store current subsong info
            subsongsInfo[currentSubsong] = stillBlock;
            currentSubsong = newSubsong - 1;
            //adjust vetor size to number of subsongs
            while ( (int)subsongsInfo.size() <= newSubsong )
            {
              subsongsInfo.push_back( StilBlock() );
            }
            stillBlock = StilBlock();
          }
        }
        //ARTIST
        if ( tmpStr.compare( 0, strlen( ARTIST ), ARTIST ) == 0 )
        {
          stillBlock.ARTIST = tmpStr.substr( strlen( ARTIST ) + 1 );
        }
        //TITLE
        if ( tmpStr.compare( 0, strlen( TITLE ), TITLE ) == 0 )
        {
          if ( stillBlock.TITLE.empty() )
          {
            stillBlock.TITLE = tmpStr.substr( strlen( TITLE ) + 1 );
          }
          else
          {
            stillBlock.TITLE.append( "," );
            stillBlock.TITLE.append( tmpStr.substr( strlen( TITLE ) + 1 ) );
          }
        }
        //AUTHOR
        if ( tmpStr.compare( 0, strlen( AUTHOR ), AUTHOR ) == 0 )
        {
          stillBlock.AUTHOR = tmpStr.substr( strlen( AUTHOR ) + 1 );
        }
        //NAME
        if ( tmpStr.compare( 0, strlen( NAME ), NAME ) == 0 )
        {
          stillBlock.NAME = tmpStr.substr( strlen( NAME ) + 1 );
        }
        //IGNORE COMMENT
        ReadLine( buf, f, BUFLEN );
      }
      subsongsInfo[currentSubsong] = stillBlock;

      m_stillMap2[strKey] = subsongsInfo;
    }
  }
  fclose( f );
}



void CThreadSidPlayer::FixPath( std::string& path )
{
  for ( int i = 0; i < (int)path.length(); ++i )
  {
    if ( path[i] == '/' )
    {
      path[i] = '\\';
    }
  }
}



std::string CThreadSidPlayer::GetSTILData( const char* filePath )
{
  std::map<std::string, std::string>::iterator i;
  char* stilFileName;

  if ( ( filePath == NULL ) || ( m_playerConfig.hvscDirectory.empty() ) ) return std::string();
  if ( strlen( filePath ) < m_playerConfig.hvscDirectory.length() ) return std::string();
  stilFileName = new char[strlen( filePath ) - m_playerConfig.hvscDirectory.length() + 1];
  strcpy_s( stilFileName, strlen( filePath ) - m_playerConfig.hvscDirectory.length(), &filePath[m_playerConfig.hvscDirectory.length()] );
  //i = m.find("aa\\DEMOS\\A-F\\Afterburner.sid");
  i = m.find( stilFileName );
  delete[] stilFileName;
  if ( i == m.end() )
  {
    return std::string();
  }
  return i->second;
  //if(i == NULL) return;
}


StilBlock CThreadSidPlayer::GetSTILData2( const char* filePath, int subsong )
{
  std::map<std::string, std::vector<StilBlock>>::iterator i;
  char* stilFileName;

  if ( ( filePath == NULL ) || ( m_playerConfig.hvscDirectory.empty() ) ) return StilBlock();
  if ( strlen( filePath ) < m_playerConfig.hvscDirectory.length() ) return StilBlock();
  stilFileName = new char[strlen( filePath ) - m_playerConfig.hvscDirectory.length() + 1];
  strcpy_s( stilFileName, strlen( filePath ) - m_playerConfig.hvscDirectory.length(), &filePath[m_playerConfig.hvscDirectory.length()] );
  //i = m.find("aa\\DEMOS\\A-F\\Afterburner.sid");
  i = m_stillMap2.find( stilFileName );
  delete[] stilFileName;
  if ( i == m_stillMap2.end() )
  {
    return StilBlock();
  }

  if ( subsong < (int)i->second.size() )
  {
    return i->second[subsong];
  }
  return StilBlock();
}



void CThreadSidPlayer::ClearSTILData( void )
{
  m.clear();
  m_stillMap2.clear();
}
