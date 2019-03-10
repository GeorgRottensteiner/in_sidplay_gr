/*
** Example Winamp .RAW input plug-in
** Copyright (c) 1998, Justin Frankel/Nullsoft Inc.
**
** benski - Cinco de Mayo 2008
**  Updated for Winamp 5.5 const-correct In_Module definition 
**
*/

#include <windows.h>

#include "../Winamp/in2.h"

// avoid CRT. Evil. Big. Bloated. Only uncomment this code if you are using 
// 'ignore default libraries' in VC++. Keeps DLL size way down.
// /*
BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
// */

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2


// raw configuration.
#define NCH 2
#define SAMPLERATE 44100
#define BPS 16


In_Module mod;			// the output module (filled in near the bottom of this file)

char lastfn[MAX_PATH];	// currently playing file (used for getting info on the current file)

int file_length;		// file length, in bytes
int decode_pos_ms;		// current decoding position, in milliseconds. 
						// Used for correcting DSP plug-in pitch changes
int paused;				// are we paused?
volatile int seek_needed; // if != -1, it is the point that the decode 
						  // thread should seek to, in ms.

HANDLE input_file=INVALID_HANDLE_VALUE; // input file handle

volatile int killDecodeThread=0;			// the kill switch for the decode thread
HANDLE thread_handle=INVALID_HANDLE_VALUE;	// the handle to the decode thread


DWORD WINAPI DecodeThread(LPVOID b); // the decode thread procedure

void config(HWND hwndParent)
{
	MessageBox(hwndParent,
		"No configuration. .RAW files must be 44/16/2",
		"Configuration",MB_OK);
	// if we had a configuration box we'd want to write it here (using DialogBox, etc)
}
void about(HWND hwndParent)
{
	MessageBox(hwndParent,"Nullsoft RAW Player, by Justin Frankel",
		"About Nullsoft RAW Player",MB_OK);
}

void init() { 
	/* any one-time initialization goes here (configuration reading, etc) */ 
}

void quit() { 
	/* one-time deinit, such as memory freeing */ 
}

int isourfile(const char *fn) { 
// used for detecting URL streams.. unused here. 
// return !strncmp(fn,"http://",7); to detect HTTP streams, etc
	return 0; 
} 


// called when winamp wants to play a file
int play(const char *fn) 
{ 
	int maxlatency;
	int thread_id;

	paused=0;
	decode_pos_ms=0;
	seek_needed=-1;

	
	
	// CHANGEME! Write your own file opening code here
	input_file = CreateFile(fn,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (input_file == INVALID_HANDLE_VALUE) // error opening file
	{
		// we return error. 1 means to keep going in the playlist, -1
		// means to stop the playlist.
		return 1;
	}

	file_length=GetFileSize(input_file,NULL);


	
	strcpy(lastfn,fn);

	// -1 and -1 are to specify buffer and prebuffer lengths.
	// -1 means to use the default, which all input plug-ins should
	// really do.
	maxlatency = mod.outMod->Open(SAMPLERATE,NCH,BPS, -1,-1); 

	// maxlatency is the maxium latency between a outMod->Write() call and
	// when you hear those samples. In ms. Used primarily by the visualization
	// system.

	if (maxlatency < 0) // error opening device
	{
		CloseHandle(input_file);
		input_file=INVALID_HANDLE_VALUE;
		return 1;
	}
	// dividing by 1000 for the first parameter of setinfo makes it
	// display 'H'... for hundred.. i.e. 14H Kbps.
	mod.SetInfo((SAMPLERATE*BPS*NCH)/1000,SAMPLERATE/1000,NCH,1);

	// initialize visualization stuff
	mod.SAVSAInit(maxlatency,SAMPLERATE);
	mod.VSASetInfo(SAMPLERATE,NCH);

	// set the output plug-ins default volume.
	// volume is 0-255, -666 is a token for
	// current volume.
	mod.outMod->SetVolume(-666); 

	// launch decode thread
	killDecodeThread=0;
	thread_handle = (HANDLE) 
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) DecodeThread,NULL,0,&thread_id);
	
	return 0; 
}

// standard pause implementation
void pause() { paused=1; mod.outMod->Pause(1); }
void unpause() { paused=0; mod.outMod->Pause(0); }
int ispaused() { return paused; }


// stop playing.
void stop() { 
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killDecodeThread=1;
		if (WaitForSingleObject(thread_handle,10000) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,"error asking thread to die!\n",
				"error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}

	// close output system
	mod.outMod->Close();

	// deinitialize visualization
	mod.SAVSADeInit();
	

	// CHANGEME! Write your own file closing code here
	if (input_file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(input_file);
		input_file=INVALID_HANDLE_VALUE;
	}

}


// returns length of playing track
int getlength() {
	return (file_length*10)/(SAMPLERATE/100*NCH*(BPS/8)); 
}


// returns current output position, in ms.
// you could just use return mod.outMod->GetOutputTime(),
// but the dsp plug-ins that do tempo changing tend to make
// that wrong.
int getoutputtime() { 
	return decode_pos_ms+
		(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime()); 
}


// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
void setoutputtime(int time_in_ms) { 
	seek_needed=time_in_ms; 
}


// standard volume/pan functions
void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }

// this gets called when the use hits Alt+3 to get the file info.
// if you need more info, ask me :)

int infoDlg(const char *fn, HWND hwnd)
{
	// CHANGEME! Write your own info dialog code here
	return 0;
}


// this is an odd function. it is used to get the title and/or
// length of a track.
// if filename is either NULL or of length 0, it means you should
// return the info of lastfn. Otherwise, return the information
// for the file in filename.
// if title is NULL, no title is copied into it.
// if length_in_ms is NULL, no length is copied into it.
void getfileinfo(const char *filename, char *title, int *length_in_ms)
{
	if (!filename || !*filename)  // currently playing file
	{
		if (length_in_ms) *length_in_ms=getlength();
		if (title) // get non-path portion.of filename
		{
			char *p=lastfn+strlen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			strcpy(title,++p);
		}
	}
	else // some other file
	{
		if (length_in_ms) // calculate length
		{
			HANDLE hFile;
			hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
				OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				*length_in_ms = (GetFileSize(hFile,NULL)*10)/(SAMPLERATE/100*NCH*(BPS/8));
				CloseHandle(hFile);
			}
			else *length_in_ms=-1000; // the default is unknown file length (-1000).
		}
		if (title) // get non path portion of filename
		{
			const char *p=filename+strlen(filename);
			while (*p != '\\' && p >= filename) p--;
			strcpy(title,++p);
		}
	}
}

void eq_set(int on, char data[10], int preamp) 
{ 
	// most plug-ins can't even do an EQ anyhow.. I'm working on writing
	// a generic PCM EQ, but it looks like it'll be a little too CPU 
	// consuming to be useful :)
	// if you _CAN_ do EQ with your format, each data byte is 0-63 (+20db <-> -20db)
	// and preamp is the same. 
}


// render 576 samples into buf. 
// this function is only used by DecodeThread. 

// note that if you adjust the size of sample_buffer, for say, 1024
// sample blocks, it will still work, but some of the visualization 
// might not look as good as it could. Stick with 576 sample blocks
// if you can, and have an additional auxiliary (overflow) buffer if 
// necessary.. 
int get_576_samples(char *buf)
{
	int l;
	// CHANGEME! Write your own sample getting code here
	ReadFile(input_file,buf,576*NCH*(BPS/8),&l,NULL);
	return l;
}



DWORD WINAPI DecodeThread(LPVOID b)
{
	int done=0; // set to TRUE if decoding has finished
	while (!killDecodeThread) 
	{
		if (seek_needed != -1) // seek is needed.
		{
			int offs;
			decode_pos_ms = seek_needed;
			seek_needed=-1;
			done=0;
			mod.outMod->Flush(decode_pos_ms); // flush output device and set 
											  // output position to the seek position
			offs = MulDiv(decode_pos_ms,SAMPLERATE,1000); // decode_pos_ms*SAMPLERATE/1000
			SetFilePointer(input_file,offs*NCH*(BPS/8),NULL,FILE_BEGIN); // seek!
		}

		if (done) // done was set to TRUE during decoding, signaling eof
		{
			mod.outMod->CanWrite();		// some output drivers need CanWrite
									    // to be called on a regular basis.

			if (!mod.outMod->IsPlaying()) 
			{
				// we're done playing, so tell Winamp and quit the thread.
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				return 0;	// quit thread
			}
			Sleep(10);		// give a little CPU time back to the system.
		}
		else if (mod.outMod->CanWrite() >= ((576*NCH*(BPS/8))*(mod.dsp_isactive()?2:1)))
			// CanWrite() returns the number of bytes you can write, so we check that
			// to the block size. the reason we multiply the block size by two if 
			// mod.dsp_isactive() is that DSP plug-ins can change it by up to a 
			// factor of two (for tempo adjustment).
		{	
			int l=576*NCH*(BPS/8);			       // block length in bytes
			static char sample_buffer[576*NCH*(BPS/8)*2]; 
												   // sample buffer. twice as 
												   // big as the blocksize

			l=get_576_samples(sample_buffer);	   // retrieve samples
			if (!l)			// no samples means we're at eof
			{
				done=1;
			}
			else	// we got samples!
			{
				// give the samples to the vis subsystems
				mod.SAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);	
				mod.VSAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);
				// adjust decode position variable
				decode_pos_ms+=(576*1000)/SAMPLERATE;	

				// if we have a DSP plug-in, then call it on our samples
				if (mod.dsp_isactive()) 
					l=mod.dsp_dosamples(
						(short *)sample_buffer,l/NCH/(BPS/8),BPS,NCH,SAMPLERATE
					  ) // dsp_dosamples
					  *(NCH*(BPS/8));

				// write the pcm data to the output system
				mod.outMod->Write(sample_buffer,l);
			}
		}
		else Sleep(20); 
		// if we can't write data, wait a little bit. Otherwise, continue 
		// through the loop writing more data (without sleeping)
	}
	return 0;
}


// module definition.

In_Module mod = 
{
	IN_VER,	// defined in IN2.H
	"Nullsoft RAW Player v0.0 "
	// winamp runs on both alpha systems and x86 ones. :)
#ifdef __alpha
	"(AXP)"
#else
	"(x86)"
#endif
	,
	0,	// hMainWindow (filled in by winamp)
	0,  // hDllInstance (filled in by winamp)
	"RAW\0RAW Audio File (*.RAW)\0"
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

// exported symbol. Returns output module.

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}
