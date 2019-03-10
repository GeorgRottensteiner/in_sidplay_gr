/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 All Purposes Functions

--------------------------------------------------------------------------- */

#include "NDE.h"
#include "BinaryField.h"
#include "Binary32Field.h"
#include <stdio.h>
#include <string.h>

//---------------------------------------------------------------------------
bool CompatibleFields(unsigned char oldType, unsigned char newType)
{

	if (oldType == newType) // duh :)
		return true;
	// going from an int field to another int equivalent field is OK
	if ((oldType == FIELD_INTEGER || oldType == FIELD_BOOLEAN || oldType == FIELD_DATETIME || oldType == FIELD_LENGTH) &&
		(newType == FIELD_INTEGER || newType == FIELD_BOOLEAN || newType == FIELD_DATETIME || newType == FIELD_LENGTH)) {
			return true;
		}

		// going from string to filename or filename to string is OK
		if ((oldType == FIELD_FILENAME && newType == FIELD_STRING)
			|| (oldType == FIELD_STRING && newType == FIELD_FILENAME))
		{
			return true;
		}
		return false;
}
//---------------------------------------------------------------------------
int AllocNewPos(VFILE *Handle)
{
	Vfseek(Handle, 0, SEEK_END);
	return Vftell(Handle);
}

//---------------------------------------------------------------------------
Field *TranslateObject(unsigned char Type, Table *tbl)
{
	switch (Type)
	{
	case FIELD_COLUMN: //0
		return new ColumnField();
	case FIELD_INDEX: //1
		return new IndexField();
	case FIELD_STRING: // 3
		return new StringField();
	case FIELD_INTEGER: // 4
		return new IntegerField();
	case FIELD_BINARY: // 6
		return new BinaryField();
	case FIELD_DATETIME: // 10
		return new DateTimeField();
	case FIELD_LENGTH: // 11
		return new LengthField();
	case FIELD_FILENAME: // 12
		return new FilenameField();
	case FIELD_INT64: // 13
		return new Int64Field();
	case FIELD_BINARY32: // 14
		return new Binary32Field();
	case FIELD_INT128: // 15
		return new Int128Field();
	default:
#ifdef WIN32
		if (!tbl->HasErrors())
		{
			//MessageBox(plugin.hwndParent, "Your database has been corrupted!\n\nWinamp will try to continue, but some of the library metadata may be lost :(", "Database Error", 0);
		}
#else
		printf("NDE Error: unknown field type encountered\n");
#endif
		tbl->IncErrorCount();
		return new Field();
	}
}

//---------------------------------------------------------------------------
char *stristr(char *s1, char *s2)
{
	size_t s2len = strlen(s2);
	char *p;
	for (p = s1;*p;p++)
		if (!strnicmp(p, s2, s2len))
			return p;
	return NULL;
}

// a faster way of doing min(wcslen(str), _len)
size_t nde_wcsnlen(const wchar_t *str, size_t _len)
{
	size_t len = 0;
	while (*str++)
	{
		if (_len == len)
			return len;
		len++;
	}
	return len;
}

// len must be <= wcslen(b)
int nde_wcsnicmp(const wchar_t *a, const wchar_t *b, size_t len)
{
#ifdef WIN32
	return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE| NORM_IGNORENONSPACE, a, (int)nde_wcsnlen(a, len), b, (int)len) - 2;
#else
	return wcsnicmp(a, b, len);
#endif
}

wchar_t *wcsistr(const wchar_t *s1, const wchar_t *s2)
{
	size_t s2len = wcslen(s2);
	const wchar_t *p;
	for (p = s1;*p;p++)
		if (!nde_wcsnicmp(p, s2, s2len))
			return (wchar_t *)p;
	return NULL;
}

//---------------------------------------------------------------------------
char *memmem(char *a, char *b, size_t s, size_t l)
{
	size_t n = s - l;
	while (n--)
	{
		if (!memcmp(a, b, l))
			return a;
		a++;
	}
	return NULL;
}
//---------------------------------------------------------------------------

int nde_wcsicmp(const wchar_t *a, const wchar_t *b)
{
#ifdef WIN32
	return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE| NORM_IGNORENONSPACE, a, -1, b, -1) - 2;
#else
	return wcsicmp(a, b);
#endif
}

int mywcsicmp(const wchar_t *a, const wchar_t *b)
{
	int r;
	if (!a && !b) return 0;
	if (!a && b) return 1;
	if (!b) return -1;
	r = nde_wcsicmp(a, b);
	return min(max(r, -1), 1);
}

wchar_t* mywcsistr(const wchar_t *a, const wchar_t *b)
{
	return (!a || !b) ? NULL : wcsistr(a, b);
}

#ifdef __APPLE__
wchar_t *wcsdup(const wchar_t *val)
{
	wchar_t *newStr = 0;
	if (val)
	{
		size_t len = wcslen(val);
		newStr = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
		wcsncpy(newStr, val, len + 1);
	}
	return newStr;
}

#endif

wchar_t* mywcsistr_fn(const wchar_t *a, const wchar_t *b)
{
	return (!a || !b) ? NULL : wcsistr_fn(a, b);
}

int mywcsicmp_fn(const wchar_t *a, const wchar_t *b)
{
		int r;
	if (!a && !b) return 0;
	if (!a && b) return 1;
	if (!b) return -1;
	r = nde_wcsicmp_fn(a, b);
	return min(max(r, -1), 1);
}

wchar_t *wcsistr_fn(const wchar_t *s1, const wchar_t *s2)
{
	size_t s2len = wcslen(s2);
	const wchar_t *p;
	for (p = s1;*p;p++)
		if (!nde_wcsnicmp_fn(p, s2, s2len))
			return (wchar_t *)p;
	return NULL;
}

int nde_wcsicmp_fn(const wchar_t *a, const wchar_t *b)
{
	return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, a, -1, b, -1) - 2;
}

int nde_wcsnicmp_fn(const wchar_t *a, const wchar_t *b, size_t len)
{
	return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, a, (int)nde_wcsnlen(a, len), b, (int)len) - 2;
}