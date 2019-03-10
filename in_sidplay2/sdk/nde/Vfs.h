#ifndef __NDE_VFS_H
#define __NDE_VFS_H

#include <bfc/platform/types.h>

#ifdef WIN32
#include <windows.h>
#endif

//#define NDE_ALLOW_NONCACHED

#if defined(NDE_NOWIN32FILEIO) && !defined(_WIN32)
#include <stdio.h>
#endif

/*
#ifdef NDE_ALLOW_NONCACHED
  #ifndef NDE_NOWIN32FILEIO
    #error NDE_ALLOW_NONCACHED at least for now requires NDE_NOWIN32FILEIO
  #endif
#endif
*/

#define VFILE_INC 65536

#define VFS_READ        1
#define VFS_WRITE        2
#define VFS_SEEKEOF      4
#define VFS_CREATE      8
#define VFS_NEWCONTENT 16
#define VFS_MUSTEXIST  32

typedef struct {
    unsigned char *data;
    unsigned long ptr;
    unsigned long filesize;
    unsigned long maxsize;
    char *filename;
    char mode;
    BOOL cached;
    int dirty;
    int flushtable;

#ifdef NDE_ALLOW_NONCACHED
  #ifdef NDE_NOWIN32FILEIO
    FILE *rfile;
  #else
    HANDLE hfile;
		bool endoffile;
  #endif
#endif

} VFILE;

#include "NDE.h"

#ifdef __cplusplus
extern "C" {
#endif

VFILE *Vfopen(char *fl, char *mode, BOOL Cached);
size_t Vfread(void *ptr, size_t size, size_t n, VFILE *buf);
void Vfseek(VFILE *fl, long i, int whence);
unsigned long Vftell(VFILE *fl);
void Vfclose(VFILE *fl);
void Vfwrite(void *ptr, size_t size, size_t n, VFILE *f);
void Vfputs(char *str, VFILE *fl);
// benski> unused: void Vgrow(VFILE *f);
void Vfputc(char c, VFILE *fl);
int Vfeof(VFILE *fl);
// benski> unused: char *Vfgets(char *dest, int n, VFILE *fl);
// benski> unused: char Vfgetc(VFILE *fl);
int Vsync(VFILE *fl); // 1 on error updating

#ifdef __cplusplus
}
#endif

#endif

