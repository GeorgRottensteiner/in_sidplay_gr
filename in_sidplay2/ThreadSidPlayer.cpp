#include <windows.h>
#include "ThreadSidPlayer.h"
#include "SidInfoImpl.h"
#include "c64roms.h"



CThreadSidPlayer::CThreadSidPlayer( In_Module& inWAmod ) : 
  m_tune( 0 ), 
  m_threadHandle( INVALID_HANDLE_VALUE )
{
  m_inmod                           = &inWAmod;
  m_decodeBuf                       = NULL;
  m_decodeBufLen                    = 0;
  m_playerStatus                    = SP_STOPPED;
  m_playerConfig.playLimitEnabled   = false;
  m_playerConfig.playLimitSec       = 120;
  m_playerConfig.songLengthsFile    = "";
  m_playerConfig.useSongLengthFile  = false;
  m_playerConfig.useSTILfile        = false;
  m_playerConfig.hvscDirectory      = "";
  m_playerConfig.voiceConfig[0][0]  = true;
  m_playerConfig.voiceConfig[0][1]  = true;
  m_playerConfig.voiceConfig[0][2]  = true;
  m_playerConfig.voiceConfig[1][0]  = true;
  m_playerConfig.voiceConfig[1][1]  = true;
  m_playerConfig.voiceConfig[1][2]  = true;
  m_playerConfig.voiceConfig[2][0]  = true;
  m_playerConfig.voiceConfig[2][1]  = true;
  m_playerConfig.voiceConfig[2][2]  = true;
  m_playerConfig.playlistFormat     = "%t %x %sn / %a / %r %st";
  m_playerConfig.subsongFormat      = "(Tune %n/%ns)";

  m_playerConfig.pseudoStereo       = false;
  m_playerConfig.sid2Model          = SidConfig::sid_model_t::MOS6581;
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
    if ( m_playerConfig.sidConfig.sidEmulation != NULL )
    {
      delete m_playerConfig.sidConfig.sidEmulation;
      m_playerConfig.sidConfig.sidEmulation = NULL;
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

  m_playerConfig.sidConfig = m_pEngine->config();
  //m_playerConfig.sidConfig.sampleFormat = SID2_LITTLE_SIGNED;	

  if ( !LoadConfigFromFile( &m_playerConfig ) )
  {
    //if load fails then use this default settings
    //m_playerConfig.sidConfig.precision = 16;
    SidConfig* defaultConf = new SidConfig();
    memcpy( ( void* )&( m_playerConfig.sidConfig ), defaultConf, sizeof( SidConfig ) );

    delete defaultConf;
    m_playerConfig.sidConfig.frequency  = 44100;
    m_playerConfig.sidConfig.playback   = SidConfig::MONO;// sid2_mono;
  }
  //m_playerConfig.sidConfig.sampleFormat = SID2_LITTLE_SIGNED;	

  SetConfig( &m_playerConfig );
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
    numChann      = ( m_playerConfig.sidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
    m_MaxLatency  = m_inmod->outMod->Open( m_playerConfig.sidConfig.frequency, numChann, PLAYBACK_BIT_PRECISION, -1, -1 );

    m_inmod->SetInfo( ( m_playerConfig.sidConfig.frequency * PLAYBACK_BIT_PRECISION * numChann ) / 1000, m_playerConfig.sidConfig.frequency / 1000, numChann, 1 );
    //visualization init
    m_inmod->SAVSAInit( m_MaxLatency, m_playerConfig.sidConfig.frequency );
    m_inmod->VSASetInfo( m_playerConfig.sidConfig.frequency, numChann );
    //default volume
    m_inmod->outMod->SetVolume( -666 );
    m_playerStatus = SP_RUNNING;
    m_threadHandle = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)CThreadSidPlayer::Run, this, 0, NULL );
  }
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
  if ( ( m_playerConfig.playLimitEnabled ) && ( m_currentTuneLength <= 0 ) )
  {
    m_currentTuneLength = m_playerConfig.playLimitSec;
  }
  m_pEngine->load( &m_tune );

  //mute must be applied after SID's have been created
  for ( int sid = 0; sid < 3; ++sid )
  {
    for ( int voice = 0; voice < 3; ++voice )
    {
      m_pEngine->mute( sid, voice, !m_playerConfig.voiceConfig[sid][voice] );
    }
  }
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
  bps = PLAYBACK_BIT_PRECISION;//playerObj->m_playerConfig.sidConfig.precision;
  numChn = ( playerObj->m_playerConfig.sidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
  freq = playerObj->m_playerConfig.sidConfig.frequency;
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
      playerObj->m_playTimems = ( playerObj->m_decodedSampleCount * 1000 ) / playerObj->m_playerConfig.sidConfig.frequency;
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
    else if ( playerObj->m_playerConfig.playLimitEnabled )
    {
      //if we dont know song length but time limit is enabled then check it
      if ( ( playerObj->m_playerConfig.playLimitSec * 1000 ) < timeElapsed )
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
  if ( ( m_playerConfig.playLimitEnabled ) 
  &&   ( m_currentTuneLength <= 0 ) )
  {
    m_currentTuneLength = m_playerConfig.playLimitSec;
  }
  m_pEngine->stop();
  m_pEngine->load( &m_tune );
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



bool CThreadSidPlayer::LoadConfigFromFile( PlayerConfig *conf )
{
  wchar_t   appDataPath[MAX_PATH];

  if ( conf == NULL )
  {
    return false;
  }

  //try to load config from common file
  SHGetSpecialFolderPath( NULL, appDataPath, CSIDL_COMMON_APPDATA, 0 );
  wcscat_s( appDataPath, L"\\in_sidplay2.ini" );
  return LoadConfigFromFile( conf, appDataPath );
}



bool CThreadSidPlayer::LoadConfigFromFile( PlayerConfig *conf, wchar_t* fileName )
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
    AssignConfigValue( conf, token, value );
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



void CThreadSidPlayer::SaveConfigToFile( PlayerConfig* conf )
{
  wchar_t appDataPath[MAX_PATH];

  //try to load config from common file
  //SHGetSpecialFolderPath(NULL,appDataPath,CSIDL_COMMON_APPDATA,0);

  if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, 0, appDataPath ) ) )
  {
    wcscat_s( appDataPath, L"\\in_sidplay2.ini" );
    SaveConfigToFile( conf, appDataPath );
  }
}



void CThreadSidPlayer::SaveConfigToFile( PlayerConfig* plconf, wchar_t* fileName )
{
  SidConfig* conf = &plconf->sidConfig;
  std::ofstream outFile( fileName );

  outFile << "PlayFrequency=" << conf->frequency << std::endl;
  outFile << "PlayChannels=" << ( ( conf->playback == SidConfig::MONO ) ? 1 : 2 ) << std::endl;
  outFile << "C64Model=" << conf->defaultC64Model << std::endl;
  outFile << "C64ModelForced=" << conf->forceC64Model << std::endl;
  outFile << "SidModel=" << conf->defaultSidModel << std::endl;
  outFile << "SidModelForced=" << conf->forceSidModel << std::endl;
  //outFile << "Sid2ModelForced=" << conf->forceSecondSidModel << std::endl;

  outFile << "PlayLimitEnabled=" << plconf->playLimitEnabled << std::endl;
  outFile << "PlayLimitTime=" << plconf->playLimitSec << std::endl;
  outFile << "UseSongLengthFile=" << plconf->useSongLengthFile << std::endl;
  if ( ( !plconf->useSongLengthFile ) 
  ||   ( plconf->songLengthsFile.empty() ) ) 
  {
    outFile << "SongLengthsFile=" << "" << std::endl;
  }
  else
  {
    outFile << "SongLengthsFile=" << plconf->songLengthsFile << std::endl;
  }
  outFile << "UseSTILFile=" << plconf->useSTILfile << std::endl;
  if ( plconf->hvscDirectory.empty() )
  {
    plconf->useSTILfile = false;
  }
  if ( !plconf->useSTILfile )
  {
    outFile << "HVSCDir=" << "" << std::endl;
  }
  else
  {
    outFile << "HVSCDir=" << plconf->hvscDirectory << std::endl;
  }
  outFile << "UseSongLengthFile=" << plconf->useSongLengthFile << std::endl;

  outFile << "VoiceConfig=";
  for ( int sid = 0; sid < 3; ++sid )
  {
    for ( int voice = 0; voice < 3; ++voice )
    {
      outFile << plconf->voiceConfig[sid][voice];
    }
  }
  outFile << std::endl;
  outFile << "PseudoStereo=" << plconf->pseudoStereo << std::endl;
  outFile << "Sid2Model=" << plconf->sid2Model << std::endl;
  outFile << "PlaylistFormat=" << plconf->playlistFormat << std::endl;
  outFile << "SubsongFormat=" << plconf->subsongFormat << std::endl;
  outFile.close();
}



void CThreadSidPlayer::AssignConfigValue( PlayerConfig* plconf, std::string token, std::string value )
{
  SidConfig* conf = &plconf->sidConfig;
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
        plconf->voiceConfig[sid][voice] = ( value.at( digitId++ ) == '1' );
      }
    }
    return;
  }

  if ( token == "Sid2Model" )
  {
    plconf->sid2Model = ( SidConfig::sid_model_t )atoi( value.c_str() );
    return;
  }

  if ( token == "PseudoStereo" )
  {
    plconf->pseudoStereo = (bool)!!atoi( value.c_str() );
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
    plconf->playLimitEnabled = (bool)!!atoi( value.c_str() );
    return;
  }
  if ( token == "PlayLimitTime" )
  {
    plconf->playLimitSec = atoi( value.c_str() );
    return;
  }

  if ( token == "UseSongLengthFile" )
  {
    plconf->useSongLengthFile = (bool)!!atoi( value.c_str() );
    return;
  }
  if ( token == "SongLengthsFile" )
  {
    plconf->songLengthsFile = value;
    return;
  }

  if ( token == "HVSCDir" )
  {
    plconf->hvscDirectory = value;
    return;
  }
  if ( token == "UseSTILFile" )
  {
    plconf->useSTILfile = (bool)!!atoi( value.c_str() );
    return;
  }

  if ( token == "PlaylistFormat" )
  {
    plconf->playlistFormat = value;
    return;
  }

  if ( token == "SubsongFormat" )
  {
    plconf->subsongFormat = value;
    return;
  }
}



const PlayerConfig& CThreadSidPlayer::GetCurrentConfig()
{
  return m_playerConfig;
}



void CThreadSidPlayer::SetConfig( PlayerConfig* newConfig )
{
  int numChann;
  bool openRes;

  if ( m_playerStatus != SP_STOPPED )
  {
    Stop();
  }
  m_pEngine->stop();

  sidbuilder* currentBuilder = m_playerConfig.sidConfig.sidEmulation;
  if ( m_playerConfig.sidConfig.sidEmulation != NULL )
  {
    //delete m_playerConfig.sidConfig.sidEmulation;
  }
  m_playerConfig.sidConfig.sidEmulation = 0;
  m_pEngine->config( m_playerConfig.sidConfig );
  if ( currentBuilder != NULL )
  {
    delete currentBuilder;
  }

  //change assign to memcpy !
  m_playerConfig.sidConfig.frequency = newConfig->sidConfig.frequency;
  m_playerConfig.sidConfig.playback = newConfig->sidConfig.playback;
  m_playerConfig.sidConfig.defaultC64Model = newConfig->sidConfig.defaultC64Model;
  m_playerConfig.sidConfig.forceC64Model = newConfig->sidConfig.forceC64Model;
  m_playerConfig.sidConfig.defaultSidModel = newConfig->sidConfig.defaultSidModel;
  m_playerConfig.sidConfig.forceSidModel = newConfig->sidConfig.forceSidModel;
  //m_playerConfig.sidConfig.forceSecondSidModel = newConfig->sidConfig.forceSecondSidModel;
  //m_playerConfig.sidConfig.secondSidModel = newConfig->sidConfig.secondSidModel;



  m_playerConfig.playLimitEnabled = newConfig->playLimitEnabled;
  m_playerConfig.playLimitSec = newConfig->playLimitSec;
  m_playerConfig.useSongLengthFile = newConfig->useSongLengthFile;

  m_playerConfig.sidConfig.samplingMethod = SidConfig::INTERPOLATE; //RESAMPLE_INTERPOLATE

  for ( int sid = 0; sid < 3; ++sid )
  {
    for ( int voice = 0; voice < 3; ++voice )
    {
      m_playerConfig.voiceConfig[sid][voice] = newConfig->voiceConfig[sid][voice];
    }
  }
  m_playerConfig.pseudoStereo = newConfig->pseudoStereo;
  m_playerConfig.sid2Model = newConfig->sid2Model;



  //TODO czy trzeba drugi i trzeci adres sida??????

  //string memory cannot overlap !!!
  if ( m_playerConfig.songLengthsFile != newConfig->songLengthsFile )
  {
    m_playerConfig.songLengthsFile = "";
    if ( !newConfig->songLengthsFile.empty() )
    {
      m_playerConfig.songLengthsFile = newConfig->songLengthsFile;
    }
  }

  m_playerConfig.useSTILfile = newConfig->useSTILfile;
  if ( m_playerConfig.hvscDirectory != newConfig->hvscDirectory )
  {
    m_playerConfig.hvscDirectory = "";
    if ( !newConfig->hvscDirectory.empty() )
    {
      m_playerConfig.hvscDirectory = newConfig->hvscDirectory;
    }
  }

  if ( newConfig->playlistFormat != m_playerConfig.playlistFormat )
  {
    m_playerConfig.playlistFormat = newConfig->playlistFormat;
  }

  if ( newConfig->subsongFormat != m_playerConfig.subsongFormat )
  {
    m_playerConfig.subsongFormat = "";
    m_playerConfig.subsongFormat = newConfig->subsongFormat;
  }

  //m_sidBuilder = ReSIDBuilderCreate("");
    //SidLazyIPtr<IReSIDBuilder> rs(m_sidBuilder);

  ReSIDfpBuilder* rs = new ReSIDfpBuilder( "ReSIDfp" );

  if ( rs )
  {

    //const SidInfoImpl* si = reinterpret_cast<const SidInfoImpl*>(&m_pEngine->info());

    m_playerConfig.sidConfig.sidEmulation = rs;
    rs->create( ( m_pEngine->info() ).maxsids() );


    rs->filter6581Curve( 0.5 );
    rs->filter8580Curve( (double)12500 );
    //filter always enabled
    rs->filter( true );
  }

  //TO CHANGE !!!!!!!
  if ( m_playerConfig.pseudoStereo )
  {
    m_playerConfig.sidConfig.secondSidAddress = 0xD400;
    //m_playerConfig.sidConfig.secondSidModel = m_playerConfig.sid2Model;
  }
  else
  {
    m_playerConfig.sidConfig.secondSidAddress = 0;
    //m_playerConfig.sidConfig.secondSidModel = -1;
  }
  //m_playerConfig.sidConfig.
  m_pEngine->config( m_playerConfig.sidConfig );


  //kernal,basic,chargen
  m_pEngine->setRoms( KERNAL_ROM, BASIC_ROM, CHARGEN_ROM );

  //create decode buf for 576 samples
  if ( m_decodeBufLen > 0 )
  {
    delete[] m_decodeBuf;
  }
  numChann = ( m_playerConfig.sidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
  m_decodeBufLen = 2 * 576 * ( PLAYBACK_BIT_PRECISION >> 3 ) * numChann;
  m_decodeBuf = new char[m_decodeBufLen];
  //open song length database
  if ( ( m_playerConfig.useSongLengthFile ) && ( !m_playerConfig.songLengthsFile.empty() ) )
  {
    openRes = m_sidDatabase.open( m_playerConfig.songLengthsFile.c_str() );
    if ( openRes != 0 )
    {
      MessageBoxA( NULL, "Error opening songlength database.\r\nDisable songlength databse or choose other file", "in_sidplay2", MB_OK );
    }
  }
  //open STIL file
  if ( ( m_playerConfig.useSTILfile ) 
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
  if ( ( m_playerConfig.playLimitEnabled ) 
  &&   ( length <= 0 ) )
  {
    length = m_playerConfig.playLimitSec;
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
  int bps = PLAYBACK_BIT_PRECISION;// m_playerConfig.sidConfig.precision;
  int numChn = ( m_playerConfig.sidConfig.playback == SidConfig::STEREO ) ? 2 : 1;
  int freq = m_playerConfig.sidConfig.frequency;
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

  bits = PLAYBACK_BIT_PRECISION;//m_playerConfig.sidConfig.precision;
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
  //m_playTimems =(m_decodedSampleCount * 1000) / m_playerConfig.sidConfig.frequency;
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

  if ( ( filePath == NULL ) || ( m_playerConfig.hvscDirectory.empty() ) ) return NULL;
  if ( strlen( filePath ) < m_playerConfig.hvscDirectory.length() ) return NULL;
  stilFileName = new char[strlen( filePath ) - m_playerConfig.hvscDirectory.length() + 1];
  strcpy_s( stilFileName, strlen( filePath ) - m_playerConfig.hvscDirectory.length(), &filePath[m_playerConfig.hvscDirectory.length()] );
  //i = m.find("aa\\DEMOS\\A-F\\Afterburner.sid");
  i = m.find( stilFileName );
  delete[] stilFileName;
  if ( i == m.end() )
  {
    return NULL;
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
