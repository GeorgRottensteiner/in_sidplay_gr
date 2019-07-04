#pragma once
#include "sidplayfp/SidConfig.h"

#include <string>



enum PlayerStatus_t
{
	SP_RUNNING,
	SP_PAUSED,
	SP_STOPPED
};


struct PlayerConfig
{
	SidConfig               SidConfig;
	bool                    PlayLimitEnabled;
	int                     PlayLimitSec;
  std::string             SongLengthsFile;
  std::string             hvscDirectory;
	bool                    UseSongLengthFile;
	bool                    UseSTILfile;

	// Voice configuration which voice is enabled/disabled on which SID, first index is SID number second voice numerber
	bool                    VoiceConfig[3][3];
	bool                    PseudoStereo;
	SidConfig::sid_model_t  Sid2Model;
  std::string             PlaylistFormat;
  std::string             SubsongFormat;
  int                     NumLoopTimes;


  PlayerConfig() :
    NumLoopTimes( 0 ),
    PlayLimitSec( 0 )
  {
  }
};
