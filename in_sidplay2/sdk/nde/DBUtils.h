/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 All Purposes Functions Prototypes

--------------------------------------------------------------------------- */

#ifndef __DBUTILS_H
#define __DBUTILS_H

#include <stdio.h>
bool CompatibleFields(unsigned char oldType, unsigned char newType);
int AllocNewPos(VFILE *Handle);
Field *TranslateObject(int Type, Table *tbl);
char *stristr(char *s1, char *s2);
wchar_t *wcsistr(const wchar_t *s1, const wchar_t *s2);
int mywcsicmp(const wchar_t *a, const wchar_t *b);
wchar_t* mywcsistr(const wchar_t *a, const wchar_t *b);
char *memmem(char *a, char *b, size_t s, size_t l);
int nde_wcsicmp(const wchar_t *a, const wchar_t *b);  
int nde_wcsnicmp(const wchar_t *a, const wchar_t *b, size_t len); // len must be <= wcslen(b)


// filesystem safe versions
wchar_t* mywcsistr_fn(const wchar_t *a, const wchar_t *b);
int mywcsicmp_fn(const wchar_t *a, const wchar_t *b);
wchar_t *wcsistr_fn(const wchar_t *s1, const wchar_t *s2);
int nde_wcsicmp_fn(const wchar_t *a, const wchar_t *b);  
int nde_wcsnicmp_fn(const wchar_t *a, const wchar_t *b, size_t len); // len must be <= wcslen(b)
#ifdef __APPLE__
wchar_t *wcsdup(const wchar_t *val);
#endif

#endif