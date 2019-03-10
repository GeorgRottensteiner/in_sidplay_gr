/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Virtual File System

--------------------------------------------------------------------------- */
#include "NDE.h"

#ifdef NDE_NOWIN32FILEIO
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "vfs.h"

#ifndef EOF
#define EOF -1
#endif

#if defined(NDE_ALLOW_NONCACHED) && !defined(NDE_NOWIN32FILEIO)
size_t ReadFileN(void *buffer, size_t size, size_t count, VFILE *f)
{
	uint8_t *b = (uint8_t *) buffer;
	size_t times;
	for (times=0;times!=count;times++)
	{
		size_t size_itr=size;
		while (size_itr)
		{
			DWORD bytesRead;
			DWORD toRead = min(0xffffffffUL, size_itr);
			ReadFile(f->hfile, b, toRead, &bytesRead, NULL);
			if (bytesRead != toRead)
			{
				f->endoffile=true;
				// TODO: rewind
				return times;
			}
			size_itr-=toRead;
			b+=toRead;
		}
	}
	return times;
}

size_t WriteFileN(void *buffer, size_t size, size_t count, VFILE *f)
{
	uint8_t *b = (uint8_t *) buffer;
	size_t times;
	for (times=0;times!=count;times++)
	{
		size_t size_itr=size;
		while (size_itr)
		{
			DWORD bytesRead;
			DWORD toRead = min(0xffffffffUL, size_itr);
			WriteFile(f->hfile, b, toRead, &bytesRead, NULL);
			if (bytesRead != toRead)
			{
				f->endoffile=true;
				// TODO: rewind
				return times;
			}
			size_itr-=toRead;
			b+=toRead;
		}
	}
	return times;
}
#endif
//----------------------------------------------------------------------------
VFILE *Vfopen(char *fl, char *mode, BOOL Cached)
{
	if (!fl) return NULL;
	VFILE *f = (VFILE *)malloc(sizeof(VFILE));
	if (!f)
		return NULL;
	f->mode = 0;
	f->data = NULL;
	f->filesize = 0;
	f->maxsize = 0;
	f->ptr = 0;
	f->dirty=0;
	f->flushtable=0;
#ifdef NDE_ALLOW_NONCACHED
	f->cached = Cached;
#else
	f->cached = TRUE;
#endif
	if (!strchr(mode, '+'))
	{
		if (strchr(mode, 'r'))
			f->mode = VFS_READ | VFS_MUSTEXIST;
		if (strchr(mode, 'w'))
			f->mode = VFS_WRITE | VFS_CREATE | VFS_NEWCONTENT;
		if (strchr(mode, 'a'))
			f->mode = VFS_WRITE | VFS_CREATE | VFS_SEEKEOF;
	}
	else
	{
		if (strstr(mode, "r+"))
			f->mode = VFS_WRITE | VFS_MUSTEXIST;
		if (strstr(mode, "w+"))
			f->mode = VFS_WRITE | VFS_CREATE | VFS_NEWCONTENT;
		if (strstr(mode, "a+"))
			f->mode = VFS_WRITE | VFS_CREATE | VFS_SEEKEOF;
	}
	if (f->mode == 0 || ((f->mode & VFS_READ) && (f->mode & VFS_WRITE)))
	{
		free(f);
		return NULL;
	}

#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		f->rfile = fopen(fl, mode);
		if (f->rfile)
#else
		f->endoffile=false;
		int readFlags=GENERIC_READ, openFlags=0;
		if (f->mode & VFS_WRITE)			readFlags|=GENERIC_WRITE;
		if (f->mode & VFS_MUSTEXIST)			openFlags=OPEN_EXISTING;
		if (f->mode & VFS_CREATE) openFlags = OPEN_ALWAYS;
		if (f->mode & VFS_NEWCONTENT) openFlags = CREATE_ALWAYS;
		f->hfile=CreateFile(fl,readFlags,FILE_SHARE_READ,0,openFlags,FILE_FLAG_SEQUENTIAL_SCAN,0);
		if (f->hfile!=INVALID_HANDLE_VALUE)
#endif
			f->filename = strdup(fl);
		else
		{
			free(f);
			return NULL;
		}
		return f;
	}
#endif

	if (f->mode & VFS_MUSTEXIST)
	{
#ifdef NDE_NOWIN32FILEIO
		if (access(fl, 0))
		{
			free(f);
			return NULL;
		}
#else
		if (GetFileAttributes(fl) == INVALID_FILE_ATTRIBUTES)
		{
			free(f);
			return NULL;
		}
#endif
	}

	if (!(f->mode & VFS_NEWCONTENT))
	{
#ifdef NDE_NOWIN32FILEIO
		FILE *pf;
		pf = fopen(fl, "rb");
		if (!pf)
		{
			f->data = (unsigned char *)calloc(VFILE_INC, 1);
			if (f->data == NULL)
			{
				free(f);
				return NULL;
			}
			f->filesize = 0;
			f->maxsize = VFILE_INC;
		}
		else
		{
			fseek(pf, 0, SEEK_END);
			f->filesize = ftell(pf);
			fseek(pf, 0, SEEK_SET);
			f->data = (unsigned char *)calloc(f->filesize, 1);
			if (f->data == NULL)
			{
				free(f);
				return NULL;
			}
			f->maxsize = f->filesize;
			fread(f->data, f->filesize, 1, pf);
			fclose(pf);
		}
#else
		int attempts=0;
		HANDLE hFile=INVALID_HANDLE_VALUE;
again:
		if (attempts<100) // we'll try for 10 seconds
		{
			hFile=CreateFile(fl,GENERIC_READ,FILE_SHARE_READ/*|FILE_SHARE_WRITE*/,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0);
			if (hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION)
			{
				Sleep(100); // let's try again
				goto again;
			}
		}
		else if (hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION)
		{
			// screwed up STILL? eeergh I bet it's another program locking it, let's try with more sharing flags
			hFile=CreateFile(fl,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0);
		}

		if (hFile==INVALID_HANDLE_VALUE)
		{
			f->data = (unsigned char *)calloc(VFILE_INC, 1);
			if (f->data == NULL)
			{
				free(f);
				return NULL;
			}
			f->filesize = 0;
			f->maxsize = VFILE_INC;
		}
		else
		{
			int fsize_ret_value=GetFileSize(hFile,NULL);
			if (fsize_ret_value==INVALID_FILE_SIZE)
				return NULL;
			f->filesize = static_cast<size_t>(fsize_ret_value);
			f->data = (unsigned char *)calloc(f->filesize, 1);
			if (f->data == NULL)
			{
				CloseHandle(hFile);
				free(f);
				return NULL;
			}
			f->maxsize = f->filesize;
			DWORD r;
			// TODO: benski> I think we should switch this to overlapped I/O (to allow I/O to happen as we're parsing)
			//               or switch to a memory mapped file... but we'll need to check with the profiler
			if (!ReadFile(hFile,f->data,f->filesize,&r,NULL) || r != f->filesize)
			{
				CloseHandle(hFile);
				free(f->data);
				free(f);
				return NULL;
			}
			CloseHandle(hFile);
		}
#endif
	}

	if (f->mode & VFS_SEEKEOF)
		f->ptr = f->filesize;

	f->filename = strdup(fl);
	return f;
}

//----------------------------------------------------------------------------
void Vfclose(VFILE *f)
{
	if (!f) return;

#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
		free(f->filename);
#ifdef NDE_NOWIN32FILEIO
		if (f->rfile)
			fclose(f->rfile);
		f->rfile=0;
#else
		if (f->hfile!=INVALID_HANDLE_VALUE)
			CloseHandle(f->hfile);
		f->hfile=INVALID_HANDLE_VALUE;
#endif
		free(f);
		return;
	}
#endif

	if (!(f->mode & VFS_WRITE))
	{
		free(f->filename);
		free(f->data);
		free(f);
		return;
	}

	Vsync(f);

	free(f->filename);
	free(f->data);
	free(f);
}

//----------------------------------------------------------------------------
size_t Vfread(void *ptr, size_t size, size_t n, VFILE *f)
{
	if (!ptr || !f) return 0;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		return fread(ptr, size, n, f->rfile);
#else
		return ReadFileN(ptr, size, n, f);
#endif
	}
#endif
	size_t s = size*n;
	if (!s) return 0;
	if (s + f->ptr > f->filesize)
	{
		//FUCKO: remove this
		if (!(f->ptr < f->filesize))
		{
#ifdef WIN32
			char buf[1024];
			wsprintf(buf,"NDE/VFS: VFS read at %d/%d (%d bytes) is bad\n",f->ptr,f->filesize,s);
			OutputDebugString(buf);
			extern HWND g_hwnd;
#endif
//      if (!f->flushtable) // this would be ideal, if we could figure out f->flushtable
			//      f->flushtable=MessageBox(g_hwnd,"DB read failed, DB may be corrupted.\r\n\r\n"
			//    "Hit Retry to continue, or Cancel to clear the DB and start over","Winamp Library Error",MB_RETRYCANCEL) == IDCANCEL; //fucko
			//MessageBox(g_hwnd,"DB read failed, DB may be corrupted. If this error persists, remove all files from the library.",
//        "Winamp Library Error",MB_OK);
			return 0;
		}
		s = f->filesize - f->ptr;
	}
	memcpy(ptr, f->data+f->ptr, s);
	f->ptr += s;
	return (s/size);
}

//----------------------------------------------------------------------------
void Vfwrite(void *ptr, size_t size, size_t n, VFILE *f)
{
	if (!ptr || !f) return;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		fwrite(ptr, size, n, f->rfile);
#else
		WriteFileN(ptr, size, n, f);
#endif
		return;
	}
#endif
	f->dirty=1;
	size_t s = (size*n);
	if (s + f->ptr > f->maxsize)
	{
		// grow f->data,f->maxsize to be (s + f->ptr + VFILE_INC-1)&~(VFILE_INC-1)
		// instead of calling Vgrow again which gets kinda slow
		size_t newsize=(s + f->ptr + VFILE_INC-1)&~(VFILE_INC-1);
		f->data=(unsigned char *)realloc(f->data,newsize);
		if (f->data == NULL) return;
		memset(f->data+f->maxsize,0,newsize-f->maxsize);
		f->maxsize=newsize;
	}
	memcpy(f->data + f->ptr, ptr, s);
	f->ptr += s;
	if (f->ptr > f->filesize)
		f->filesize = f->ptr;
}

//----------------------------------------------------------------------------
void Vgrow(VFILE *f)
{
	if (!f) return;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached) return;
#endif
	unsigned char *newBlock = (unsigned char *)calloc(f->maxsize + VFILE_INC, 1);
	if (f->data)
		memcpy(newBlock, f->data, f->maxsize);
	f->maxsize += VFILE_INC;
	if (f->data)
		free(f->data);
	f->data = newBlock;
}

//----------------------------------------------------------------------------
void Vfputs(char *str, VFILE *f)
{
	if (!f) return;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		fputs(str, f->rfile);
#else
		WriteFileN(str, strlen(str), 1, f);
#endif
		return;
	}
#endif
	Vfwrite(str, strlen(str), 1, f);
}

//----------------------------------------------------------------------------
void Vfputc(char c, VFILE *f)
{
	if (!f) return;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		fputc(c, f->rfile);
#else
		DWORD written;
		WriteFile(f->hfile, &c, sizeof(c), &written, NULL);
#endif
		return;
	}
#endif
	Vfwrite(&c, 1, 1, f);
}

/* benski> unused:
// not mac compliant
//----------------------------------------------------------------------------
char *Vfgets(char *dest, int n, VFILE *f) {
  if (!f) return NULL;
#ifdef NDE_ALLOW_NONCACHED
  if (!f->cached)
	{
	#ifdef NDE_NOWIN32FILEIO
    return fgets(dest, n, f->rfile);
		#else
		#error port me!
		#endif
	}
#endif

  unsigned char c=0;
  char *p;
  int l=0;

  p = dest;
  while (l < n && !Vfeof(f)) {
    c = f->data[f->ptr];
    f->ptr++;
    *p = c;
    p++;
    l++;
    if (c == '\n') {
      if (!Vfeof(f) && f->data[f->ptr] == '\r') {
        f->ptr++;
      }
    break;
    }
  }
  *p=0;
  return dest;
}
*/

/* benski> unused:
//----------------------------------------------------------------------------
char Vfgetc(VFILE *f) {
  if (!f) return EOF;
#ifdef NDE_ALLOW_NONCACHED
  if (!f->cached)
	#ifdef NDE_NOWIN32FILEIO
    return fgetc(f->rfile);
		#else
		#error port me#
		#endif
#endif
  if (!Vfeof(f))
    return f->data[f->ptr++];
  return EOF;
}
*/

//----------------------------------------------------------------------------
unsigned long Vftell(VFILE *f)
{
	if (!f) return (unsigned)-1;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		return ftell(f->rfile);
#else
		return SetFilePointer(f->hfile, 0, NULL, FILE_CURRENT);
#endif
	}
#endif
	return f->ptr;
}

//----------------------------------------------------------------------------
void Vfseek(VFILE *f, long i, int whence)
{
	if (!f) return;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		fseek(f->rfile, i, whence);
#else
		SetFilePointer(f->hfile, i, NULL, whence);
		f->endoffile = false;
#endif
		return;
	}
#endif
	switch (whence)
	{
	case SEEK_SET:
		f->ptr = i;
		break;
	case SEEK_CUR:
		f->ptr += i;
		break;
	case SEEK_END:
		f->ptr = f->filesize+i;
		break;
	}
}

//----------------------------------------------------------------------------
int Vfeof(VFILE *f)
{
	if (!f) return -1;
#ifdef NDE_ALLOW_NONCACHED
	if (!f->cached)
	{
#ifdef NDE_NOWIN32FILEIO
		return feof(f->rfile);
#else
		return !!f->endoffile;
#endif
	}
#endif
	return (f->ptr >= f->filesize);
}

//----------------------------------------------------------------------------
int Vsync(VFILE *f)
{
	if (!f) return 0;
	if (!f->dirty) return 0;

	if (f->mode & VFS_WRITE)
	{
#ifdef NDE_ALLOW_NONCACHED
		if (!f->cached)
		{
#ifdef NDE_NOWIN32FILEIO
			int p=ftell(f->rfile);
			fclose(f->rfile);
			f->rfile = fopen(f->filename, "r+b");
			if (!f->rfile)
				return 1;
			fseek(f->rfile, p, SEEK_SET);
#else
			LONG p = SetFilePointer(f->hfile, 0, NULL, FILE_CURRENT);
			CloseHandle(f->hfile);
			f->hfile = CreateFile(f->filename,GENERIC_READ|GENERIC_WRITE,0,0,OPEN_EXISTING,0,NULL);
			if (f->hfile == INVALID_HANDLE_VALUE)
				return 1;
			SetFilePointer(f->hfile, p, NULL, SEEK_SET);
			f->endoffile=false;
#endif
			return 0;
		}
#endif

		char *newfn=(char*)malloc(strlen(f->filename)+32);
		char *oldfn=(char*)malloc(strlen(f->filename)+32);
		DWORD mypid=GetCurrentProcessId();

		wsprintf(newfn,"%s.n3w%08X",f->filename,mypid);
		wsprintf(oldfn,"%s.o1d%08X",f->filename,mypid);
#ifdef NDE_NOWIN32FILEIO
		unlink(newfn);
		unlink(oldfn);
#else
		DeleteFile(newfn);
		DeleteFile(oldfn);
#endif

//#ifdef WIN32_NOLIB

//#endif
#ifdef NDE_NOWIN32FILEIO
tryagain:
		FILE *pf = fopen(newfn, "wb");
		if (!pf || fwrite(f->data, f->filesize, 1, pf) != 1)
		{
			if (pf) fclose(pf);

#ifdef WIN32_NOLIB
			extern HWND g_hwnd;
			int a=MessageBox(g_hwnd,"Error flushing DB to disk (is your drive full?).\r\n\r\n"
			                 "Hit Retry to try again (recommended), or Cancel to abort (NOT recommended, and may leave the DB in an unuseable state).\r\n",
			                 "Winamp Library Error",MB_RETRYCANCEL);
			if (a == IDRETRY) goto tryagain;
#else
			printf("Error flushing DB to disk (is your drive full?)\n\nHit (R)etry to try again (recommended), or (C)ancel to abort (NOT recommended, and may leave the DB in an unuseable state)");
			fflush(stdout);
			char c;
			while (1)
			{
				scanf("%c", &c);
				clear_stdin();
				c = toupper(c);
				if (c == 'R') goto tryagain;
				else if (c == 'C') break;
			}
#endif

			unlink(newfn);
			free(newfn);
			free(oldfn);
			return 1;
		}
		fclose(pf);
		rename(f->filename,oldfn); // save old file
#else
		HANDLE hFile = CreateFile(newfn,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,NULL);
		int success=0;
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD o;
			if (WriteFile(hFile,f->data,f->filesize,&o,NULL) && o == f->filesize) success++;
			CloseHandle(hFile);
		}
		if (!success)
		{

#ifdef WIN32_NOLIB
#ifdef WIN32
			extern HWND g_hwnd;
			int a=MessageBox(g_hwnd,"Error flushing DB to disk (is your drive full?).\r\n\r\n"
			                 "Hit Retry to try again (recommended), or Cancel to abort (NOT recommended, and may leave the DB in an unuseable state).\r\n",
			                 "Winamp Library Error",MB_RETRYCANCEL);
			if (a == IDRETRY) goto tryagain;

#else // WIN32
		printf("Error flushing DB to disk (is your drive full?)\n\nHit (R)etry to try again (recommended), or (C)ancel to abort (NOT recommended, and may leave the DB in an unuseable state)");
		fflush(stdout);
		char c;
		while (1)
		{
			scanf("%c", &c);
			clear_stdin();
			c = toupper(c);
			if (c == 'R') goto tryagain;
			else if (c == 'C') break;
		}
#endif // WIN32
#endif // WIN32_NOLIB

			DeleteFile(newfn);
			free(newfn);
			free(oldfn);
			return 1;
		}

		if (MoveFile(f->filename,oldfn) == 0) // if the function fails
		{
			CopyFile(f->filename,oldfn, FALSE);
			DeleteFile(f->filename);
		}
#endif


		int rv=0;

#ifdef NDE_NOWIN32FILEIO
tryagain2:
		if (rename(newfn,f->filename))
		{
#ifdef WIN32_NOLIB
			extern HWND g_hwnd;
			int a=MessageBox(g_hwnd,"Error updating DB file on disk. This should never really happen.\r\n\r\n"
			                 "Hit Retry to try again (recommended), or Cancel to abort (NOT recommended, and may leave the DB in an unuseable state).\r\n",
			                 "Winamp Library Error",MB_RETRYCANCEL);
			if (a == IDRETRY) goto tryagain2;

#else
			printf("Error updating DB file on disk. This should never really happen\n\nHit (R)etry to try again (recommended), or (C)ancel to abort (NOT recommended, and may leave the DB in an unuseable state)");
			fflush(stdout);
			char c;
			while (1)
			{
				scanf("%c", &c);
				clear_stdin();
				c = toupper(c);
				if (c == 'R') goto tryagain2;
				else if (c == 'C') break;
			}
#endif

			rename(oldfn,f->filename); // restore old file
			rv=1;
		}

		// clean up our temp files
		unlink(oldfn);
		unlink(newfn);

#else

		if (MoveFile(newfn,f->filename) == 0 && CopyFile(newfn,f->filename, FALSE) == 0)
		{

#ifdef WIN32_NOLIB
#ifdef WIN32
			extern HWND g_hwnd;
			int a=MessageBox(g_hwnd,"Error updating DB file on disk. This should never really happen.\r\n\r\n"
			                 "Hit Retry to try again (recommended), or Cancel to abort (NOT recommended, and may leave the DB in an unuseable state).\r\n",
			                 "Winamp Library Error",MB_RETRYCANCEL);
			if (a == IDRETRY) goto tryagain2;

#else // WIN32
		printf("Error updating DB file on disk. This should never really happen\n\nHit (R)etry to try again (recommended), or (C)ancel to abort (NOT recommended, and may leave the DB in an unuseable state)");
		fflush(stdout);
		char c;
		while (1)
		{
			scanf("%c", &c);
			clear_stdin();
			c = toupper(c);
			if (c == 'R') goto tryagain2;
			else if (c == 'C') break;
		}
#endif //WIN32
#endif // WIN32_NOLIB

			MoveFile(oldfn,f->filename); // restore old file
			rv=1;
		}

		// clean up our temp files
		DeleteFile(oldfn);
		DeleteFile(newfn);
#endif

		free(newfn);
		free(oldfn);
		return rv;
	}
	f->dirty=0;
	return 0;
}


