/**
 * Flake encoder for Winamp
 * Copyright (c) 2006 Will Fisher
 * The Flake library is Copyright (c) 2006 Justin Ruggles,
 * see flake.h for more details
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "AudioCoderFlake.h"

// wasabi based services for localisation support
#include <api/service/waServiceFactory.h>
#include "../Agave/Language/api_language.h"
#include "../winamp/wa_ipc.h"

#include <strsafe.h>

HWND winampwnd = 0;
api_service *WASABI_API_SVC = 0;
api_language *WASABI_API_LNG = 0;
HINSTANCE WASABI_API_LNG_HINST = 0, WASABI_API_ORIG_HINST = 0;

typedef struct
{
	configtype cfg;
	char configfile[MAX_PATH];
}
configwndrec;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

void readconfig(char *configfile, configtype *cfg)
{
  cfg->compression = 8;
	if (configfile) GetPrivateProfileStruct("audio_flake", "conf", cfg, sizeof(configtype), configfile);
}

void writeconfig(char *configfile, configtype *cfg)
{
	if (configfile) WritePrivateProfileStruct("audio_flake", "conf", cfg, sizeof(configtype), configfile);
}

static HINSTANCE GetMyInstance()
{
	MEMORY_BASIC_INFORMATION mbi = {0};
	if(VirtualQuery(GetMyInstance, &mbi, sizeof(mbi)))
		return (HINSTANCE)mbi.AllocationBase;
	return NULL;
}

void GetLocalisationApiService(void)
{
	if(!WASABI_API_LNG)
	{
		// loader so that we can get the localisation service api for use
		if(!WASABI_API_SVC)
		{
			WASABI_API_SVC = (api_service*)SendMessage(winampwnd, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
			if (WASABI_API_SVC == (api_service*)1)
			{
				WASABI_API_SVC = NULL;
				return;
			}
		}

		if(!WASABI_API_LNG)
		{
			waServiceFactory *sf;
			sf = WASABI_API_SVC->service_getServiceByGuid(languageApiGUID);
			if (sf) WASABI_API_LNG = reinterpret_cast<api_language*>(sf->getInterface());
		}

		// need to have this initialised before we try to do anything with localisation features
		WASABI_API_START_LANG(GetMyInstance(),EncFlakeLangGUID);
	}
}

extern "C"
{
	unsigned int __declspec(dllexport) GetAudioTypes3(int idx, char *desc)
	{
		if (idx == 0)
		{
			GetLocalisationApiService();
			StringCchPrintf(desc, 1024, WASABI_API_LNGSTRING(IDS_ENC_FLAC_DESC), FLAKE_IDENT);
			return mmioFOURCC('F', 'L', 'A', 'k');
		}
		return 0;
	}

	AudioCoder __declspec(dllexport) *CreateAudio3(int nch, int srate, int bps, unsigned int srct, unsigned int *outt, char *configfile)
	{
		if (srct == mmioFOURCC('P', 'C', 'M', ' ') && *outt == mmioFOURCC('F', 'L', 'A', 'k'))
		{
			configtype cfg;
			readconfig(configfile, &cfg);
			*outt = mmioFOURCC('F', 'L', 'A', 'k');
			AudioCoderFlake *t=0;
			t = new AudioCoderFlake(nch, srate, bps, &cfg);
			if (t->GetLastError())
			{
				delete t;
				return NULL;
			}
			return t;
		}
		return NULL;
	}

	void __declspec(dllexport) FinishAudio3(const char *filename, AudioCoder *coder)
	{
		((AudioCoderFlake*)coder)->FinishAudio(filename);
	}

	void __declspec(dllexport) FinishAudio3W(const wchar_t *filename, AudioCoder *coder)
	{
		((AudioCoderFlake*)coder)->FinishAudio(filename);
	}

	void __declspec(dllexport) PrepareToFinish(const char *filename, AudioCoder *coder)
	{
		((AudioCoderFlake*)coder)->PrepareToFinish();
	}

	void __declspec(dllexport) PrepareToFinishW(const wchar_t *filename, AudioCoder *coder)
	{
		((AudioCoderFlake*)coder)->PrepareToFinish();
	}

	BOOL CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		static configwndrec *wr;
		switch(uMsg)
		{
			case WM_INITDIALOG:
				wr = (configwndrec *)lParam;
				SendMessage(GetDlgItem(hwndDlg,IDC_COMPRESSIONSLIDER),TBM_SETRANGE,TRUE,MAKELONG(0,12));
				SendMessage(GetDlgItem(hwndDlg,IDC_COMPRESSIONSLIDER),TBM_SETPOS,TRUE,wr->cfg.compression);
			break;

			case WM_NOTIFY:
				if(wParam == IDC_COMPRESSIONSLIDER)
				{
					LPNMHDR l = (LPNMHDR)lParam;
					if(l->idFrom == IDC_COMPRESSIONSLIDER)
					wr->cfg.compression = SendMessage(GetDlgItem(hwndDlg,IDC_COMPRESSIONSLIDER),TBM_GETPOS,0,0);
				}
			break;

			case WM_DESTROY:
				writeconfig(wr->configfile,&wr->cfg);
				free(wr); wr=NULL;
				break;
		}
		return 0;
	}

	HWND __declspec(dllexport) ConfigAudio3(HWND hwndParent, HINSTANCE hinst, unsigned int outt, char *configfile)
	{
		if (outt == mmioFOURCC('F', 'L', 'A', 'k'))
		{
			configwndrec *wr = (configwndrec*)malloc(sizeof(configwndrec));
			if (configfile) lstrcpyn(wr->configfile, configfile, MAX_PATH);
			else wr->configfile[0] = 0;
			readconfig(configfile, &wr->cfg);
			GetLocalisationApiService();
			return WASABI_API_CREATEDIALOGPARAM(IDD_CONFIG, hwndParent, DlgProc, (LPARAM)wr);
		}
		return NULL;
	}

	int __declspec(dllexport) SetConfigItem(unsigned int outt, char *item, char *data, char *configfile)
	{
		// nothing yet
		return 0;
	}

	int __declspec(dllexport) GetConfigItem(unsigned int outt, char *item, char *data, int len, char *configfile)
	{
		if (outt == mmioFOURCC('F', 'L', 'A', 'k'))
		{
			configtype cfg;
			readconfig(configfile, &cfg);
			if (!lstrcmpi(item, "bitrate"))  lstrcpynA(data,"755",len); // FUCKO: this is ment to be an estimate for approximations of output filesize (used by ml_pmp). Improve this.
			else if (!lstrcmpi(item,"extension")) lstrcpynA(data,"flac",len);
			return 1;
		}
		return 0;
	}

	void __declspec(dllexport) SetWinampHWND(HWND hwnd)
	{
		winampwnd = hwnd;
	}
};
