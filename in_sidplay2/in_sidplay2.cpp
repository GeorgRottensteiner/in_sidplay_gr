// in_sidplay2.cpp : Defines the exported functions for the DLL application.
//

#include <windows.h>
#include "ThreadSidplayer.h"
#include "resource.h"
#include "aboutdlg.h"
#include "configdlg.h"
#include "infodlg.h"
#include "subsongdlg.h"
#include "sdk/winamp/wa_ipc.h"
#include "sdk/winamp/ipc_pe.h"
#include "helpers.h"

#include "SIDPlugin.h"

#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#include <string>
#include <vector>



HANDLE                gUpdaterThreadHandle = 0;



//#include <Debug/debugclient.h>

extern "C" __declspec( dllexport ) In_Module* winampGetInModule2()
{
  return &s_Plugin.g_InModuleDefinition;
}
