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

#ifndef _AUDIOCODERFLAKE_H_
#define _AUDIOCODERFLAKE_H_

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include "../nsv/enc_if.h"
#include "resource.h"
extern "C" {
#include "flake/libflake/flake.h"
}

typedef struct {
  int compression; // 0-12
} configtype;

class AudioCoderFlake : public AudioCoder {
public:
	AudioCoderFlake(int nch, int srate, int bps, configtype * cfg);
	/* AudioCoder implementation */  
  virtual int Encode(int framepos, void *in, int in_avail, int *in_used, void *out, int out_avail); //returns bytes in out
  virtual ~AudioCoderFlake();

	/* internal public functions */
  void PrepareToFinish();
  void FinishAudio(const wchar_t *filename);
	void FinishAudio(const char *filename);
  int GetLastError() { return error; }
private:
  int error;
  FlakeContext s;
  configtype * cfg;
  int samplecount;
  int header_size, written_header;
  int frame_size, written_frame;
  BYTE * frame;
  BYTE * block;
  int blocklen;
  int blockfilled;
  int lastblock;
};

#endif //_AUDIOCODERFLAKE_H_