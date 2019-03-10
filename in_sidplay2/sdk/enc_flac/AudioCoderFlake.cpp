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

AudioCoderFlake::AudioCoderFlake(int nch, int srate, int bps, configtype * cfg) : 
  cfg(cfg), error(0), frame(NULL), frame_size(0), written_frame(0), samplecount(0), 
  lastblock(0), blockfilled(0), block(NULL)
{
  if(bps != 16) error=1;
  s.bits_per_sample = bps;
  s.channels = nch;
  s.sample_rate = srate;
  s.samples = 0;

  s.params.compression = cfg->compression;
  s.params.variable_block_size = 1;
  flake_set_defaults(&s.params);

  header_size = flake_encode_init(&s);
  written_header = 0;
  frame = (BYTE*)malloc(s.max_frame_size);
  blocklen = s.params.block_size * s.channels * (s.bits_per_sample/8);
  block = (BYTE*)malloc(blocklen);
}

AudioCoderFlake::~AudioCoderFlake()
{
  if(frame) free(frame);
  if(block) free(block);
}

int AudioCoderFlake::Encode(int framepos, void *in0, int in_avail, int *in_used, void *out0, int out_avail)
{
  BYTE * in = (BYTE*)in0;
  BYTE * out = (BYTE*)out0;
  int out_used=0;
  *in_used=0;

  while(written_header < header_size) // write any header
  { 
    int l = min(out_avail - out_used,header_size - written_header);
    memcpy(out + out_used,s.header + written_header,l);
    written_header += l;
    out_used += l;
    return out_used;
  }

  for(;;)
  {
    if(written_frame < frame_size) // write frame
    { 
      int l = min(out_avail - out_used,frame_size - written_frame);
      memcpy(out + out_used,frame + written_frame,l);
      written_frame += l;
      out_used += l;
      if(out_avail == out_used) break;
      if(lastblock) { lastblock = 2; break; }
    }
    else // encode more
    { 
      int l = min(blocklen - blockfilled,in_avail - *in_used);
      if(l > 0)
      {
        memcpy(block + blockfilled,in + *in_used,l);
        blockfilled += l;
        *in_used += l;
      }
      if(blockfilled == blocklen || (lastblock == 1 && in_avail == *in_used))
      {
        if(lastblock) s.params.block_size = blockfilled / (s.channels * (s.bits_per_sample/8));
        frame_size = flake_encode_frame(&s,frame,(short*)block);
        written_frame = 0;
        blockfilled = 0;
        samplecount += s.params.block_size;
      }
      else break;
    }
  }
  return out_used;
}

void AudioCoderFlake::PrepareToFinish()
{
  lastblock = 1;
}

void AudioCoderFlake::FinishAudio(const wchar_t *filename)
{
	flake_encode_close(&s);

  FILE * f = _wfopen(filename, L"rb+");
  if(f && !fseek(f, 22, SEEK_SET))
  {
    BYTE sc[4] = { 0, 0, 0, 0 };
    sc[0] = (samplecount >> 24) & 0xFF;
    sc[1] = (samplecount >> 16) & 0xFF;
    sc[2] = (samplecount >>  8) & 0xFF;
    sc[3] =  samplecount        & 0xFF;
    fwrite(sc, 1, 4, f);

    fwrite(s.md5digest, 1, 16, f);
    fclose(f);
  }
}

void AudioCoderFlake::FinishAudio(const char *filename)
{
	flake_encode_close(&s);

  FILE * f = fopen(filename, "rb+");
  if(f && !fseek(f, 22, SEEK_SET))
  {
    BYTE sc[4] = { 0, 0, 0, 0 };
    sc[0] = (samplecount >> 24) & 0xFF;
    sc[1] = (samplecount >> 16) & 0xFF;
    sc[2] = (samplecount >>  8) & 0xFF;
    sc[3] =  samplecount        & 0xFF;
    fwrite(sc, 1, 4, f);

    fwrite(s.md5digest, 1, 16, f);
    fclose(f);
  }
}
