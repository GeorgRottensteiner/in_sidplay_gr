#include <windows.h>
#include "../Winamp/in2.h"
#include <shlwapi.h>

/*
This example project is compiled with the UNICODE_INPUT_PLUGIN preprocessor symbol turned on
which makes this a unicode input plugin rather than an ANSI one
Keep this in mind if you copy/paste this into your own project
*/

HMODULE in_mp3_lib = 0;
In_Module *in_mp3 = 0;
extern In_Module plugin; // our plugin's definition and data struct

void SyncPluginFlags()
{
	// sync seekable state and plugin flags (field is named "UsesOutputPlug" but is actually a flags field)
	// this is the real challenging part ... we really need to make sure that these things match at all times.
	// this is problematic because it can change on the fly on a background thread and we don't have a good way of knowing when it happened
	// we're going to override our chained plugin's "SetInfo" function as a way to have a good opportunity to sync
	// these flags right at the real playback start
	if (in_mp3)
	{
		plugin.is_seekable = in_mp3->is_seekable;
		plugin.UsesOutputPlug = in_mp3->UsesOutputPlug;
	}
}

void SetInfoHook(int bitrate, int srate, int stereo, int synched)
{
	if (in_mp3) // we shouldn't even get called if this is null, but it's worth checking anyway
	{
		SyncPluginFlags();
		plugin.SetInfo(bitrate, srate, stereo, synched); // call the real one
	}
}


void Config(HWND hwndParent)
{
	// empty for this example
}

void About(HWND hwndParent)
{
	// empty for this example
}

typedef In_Module *(*PluginGetter)();
void Init()
{
	// on older versions of Winamp (5.1 and prior, I believe)
	// plugin.hMainWindow will be NULL during init

	wchar_t path[MAX_PATH];
	GetModuleFileNameW(plugin.hDllInstance, path, MAX_PATH);
	PathRemoveFileSpecW(path);
	PathAppendW(path, L"in_mp3.dll");

	in_mp3_lib = LoadLibraryW(path);
	if (in_mp3_lib)
	{
		PluginGetter pluginGetter = (PluginGetter)GetProcAddress(in_mp3_lib, "winampGetInModule2");
		if (pluginGetter)
		{
			in_mp3 = pluginGetter();
		}
	}
}

void Quit()
{
	if (in_mp3_lib)
		FreeLibrary(in_mp3_lib);
	in_mp3_lib=0;
	in_mp3=0;
}

void GetFileInfo(const wchar_t *file, wchar_t *title, int *length_in_ms)
{
	if (in_mp3) 
	{
		if (in_mp3->version & IN_UNICODE)
		{
			in_mp3->GetFileInfo(file, title, length_in_ms);
		}
		else
		{
			// if our chained plugin is ANSI, 
			// we'll have to convert the filename to ANSI
			// and convert the returned title back to Unicode
			char file_ansi[1024];
			WideCharToMultiByte(CP_ACP, 0, file, -1, file_ansi, 1023, 0, 0);
			file_ansi[1023]=0; // MultiByteToWideChar doesn't like to null terminate when the buffer is too long

			char title_ansi[GETFILEINFO_TITLE_LENGTH]="";
			// we have to cast the pointers because our typedefs
			// differ from the chained plugin's typedefs
			in_mp3->GetFileInfo((const in_char *)file_ansi, (in_char *)title_ansi, length_in_ms);
			MultiByteToWideChar(CP_ACP, 0, title_ansi, -1, title, GETFILEINFO_TITLE_LENGTH);
		}
	}
	else // no chained plugin
	{
		// just fill in something default
		// if you are making your own plugin
		// you will might have a way to fill this out
		if (title)
			*title=0;
		if (length_in_ms)
			*length_in_ms=-1000;
	}
}

int InfoBox(const wchar_t *file, HWND hwndParent)
{
	if (in_mp3)
	{
		if (in_mp3->version & IN_UNICODE)
		{
			return in_mp3->InfoBox(file, hwndParent);
		}
		else
		{
			// if our chained plugin is ANSI, 
			// we'll have to convert the filename to ANSI
			char file_ansi[1024];
			WideCharToMultiByte(CP_ACP, 0, file, -1, file_ansi, 1023, 0, 0);
			file_ansi[1023]=0; // MultiByteToWideChar doesn't like to null terminate when the buffer is too long

			// we have to cast the pointer because our typedef
			// differ from the chained plugin's typedef
			return in_mp3->InfoBox((const in_char *)file_ansi, hwndParent);
		}
	}
	else
	{
		return INFOBOX_UNCHANGED;
	}
}

int IsOurFile(const wchar_t *file)
{
	// we're going to cheat and steal the MP3 file extension here
	// so we don't have to worry about loading order getting in the way
	// this is not the recommended way to do this in a normal input plugin, though!!!
	return !wcsicmp(L".mp3", PathFindExtension(file));
}

int Play(const wchar_t *file)
{
	if (in_mp3)
	{
		int res;
		in_mp3->SetInfo = SetInfoHook; // hook the SetInfo function so we can use it as an opportunity to sync plugin state
		in_mp3->outMod = plugin.outMod;
		if (in_mp3->version & IN_UNICODE)
		{
			res = in_mp3->Play(file);
		}
		else
		{
			// if our chained plugin is ANSI, 
			// we'll have to convert the filename to ANSI
			char file_ansi[1024];
			WideCharToMultiByte(CP_ACP, 0, file, -1, file_ansi, 1023, 0, 0);
			file_ansi[1023]=0; // MultiByteToWideChar doesn't like to null terminate when the buffer is too long

			// we have to cast the pointer because our typedef
			// differ from the chained plugin's typedef
			res = in_mp3->Play((const in_char *)file_ansi);
		}
		SyncPluginFlags();
		return res;
	}
	else
	{
		return 1;
	}
}

void Pause()
{
	if (in_mp3)
		in_mp3->Pause();
}

void UnPause()
{
	if (in_mp3)
		in_mp3->UnPause();
}

int IsPaused()
{
	if (in_mp3)
		return in_mp3->IsPaused();
	else
		return 0;
}

void Stop()
{
	if (in_mp3)
	{
		in_mp3->Stop();
		in_mp3->SetInfo = plugin.SetInfo; // unhook
	}
}

int GetLength()
{
	if (in_mp3)
		return in_mp3->GetLength();
	else
		return 0;
}

int GetOutputTime()
{
	if (in_mp3)
		return in_mp3->GetOutputTime();
	else
		return 0;
}

void SetOutputTime(int time_in_ms)
{
	if (in_mp3)
		in_mp3->SetOutputTime(time_in_ms);
}

void SetVolume(int volume)
{
	if (in_mp3)
	{
		in_mp3->outMod = plugin.outMod;
		in_mp3->SetVolume(volume);
	}
}

void SetPan(int pan)
{
	if (in_mp3)
	{
		in_mp3->outMod = plugin.outMod;
		in_mp3->SetPan(pan);
	}
}

void EQSet(int on, char data[10], int preamp)
{
	if (in_mp3 && in_mp3->EQSet)
		in_mp3->EQSet(on, data, preamp);
}

In_Module plugin =
{
	IN_VER,	// defined in IN2.H
	"Input Plugin Chaining Example",
	0,	// hMainWindow (filled in by winamp)
	0,  // hDllInstance (filled in by winamp)
	"\0",	// this is a double-null limited list. "EXT\0Description\0EXT\0Description\0" etc.
	1,	// is_seekable
	1,	// uses output plug-in system
	Config,
	About,
	Init,
	Quit,
	GetFileInfo,
	InfoBox,
	IsOurFile,
	Play,
	Pause,
	UnPause,
	IsPaused,
	Stop,

	GetLength,
	GetOutputTime,
	SetOutputTime,

	SetVolume,
	SetPan,

	0,0,0,0,0,0,0,0,0, // visualization calls filled in by winamp

	0,0, // dsp calls filled in by winamp

	EQSet,

	NULL,		// setinfo call filled in by winamp

	0, // out_mod filled in by winamp
};

extern "C" __declspec(dllexport) In_Module *winampGetInModule2()
{
	return &plugin;
}