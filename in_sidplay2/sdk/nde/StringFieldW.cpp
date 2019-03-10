/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 StringField Class

--------------------------------------------------------------------------- */

#include "NDE.h"

//---------------------------------------------------------------------------
StringFieldW::StringFieldW(const wchar_t *Str)
{
InitField();
Type = FIELD_STRINGW;
if (Str) String = _wcsdup(Str);
Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void StringFieldW::InitField(void)
{
Type = FIELD_STRINGW;
String = NULL;
Kind = DELETABLE;
Physical = FALSE;
}

//---------------------------------------------------------------------------
StringFieldW::StringFieldW()
{
InitField();
}

//---------------------------------------------------------------------------
StringFieldW::~StringFieldW()
{
if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
if (String) free(String);
}

//---------------------------------------------------------------------------
void StringFieldW::ReadTypedData(unsigned char *data, int len)
{
	unsigned short c;
	int pos=0;

	CHECK_SHORT(len);
	c = GET_SHORT(); pos+=2;
	if (c)
	{
		wchar_t BOM=0;
		CHECK_BIN(len, 2);
		GET_BINARY((unsigned char *)&BOM, data, sizeof(wchar_t), pos);
		if (BOM == 0xFEFF) // UTF-16 LE
		{
			pos+=2;
			CHECK_BIN(len, c);
			String = (wchar_t*)malloc(c+2);
			GET_BINARY((unsigned char *)String, data, c, pos);
			String[c/2]=0x00;
		}
		else // ANSI text
		{
			CHECK_BIN(len, c);
			char *temp = (char *)malloc(c+2);
			GET_BINARY((unsigned char *)temp , data, c, pos);
			temp[c]=0x00;
			int len=MultiByteToWideChar(CP_ACP, 0, temp, -1, 0, 0);
			String = (wchar_t*)malloc(len * sizeof(wchar_t));
			MultiByteToWideChar(CP_ACP, 0, temp, -1, String, len);
		}
	}
}

//---------------------------------------------------------------------------
void StringFieldW::WriteTypedData(unsigned char *data, int len)
{
	unsigned short c;
	int pos=0;

	if (String) 
		c = (unsigned short)((wcslen(String)+1)*sizeof(wchar_t)) + 2 /* +2 for BOM */; 
	else 
	c = 0;
	CHECK_SHORT(len);
	PUT_SHORT(c); pos+=2;
	if (String) 
  {
		const wchar_t BOM= 0xFEFF; // mark text as UTF-16 LE
		CHECK_BIN(len, 2);
	  PUT_BINARY(data, (unsigned char*)&BOM, sizeof(wchar_t), pos);
		pos+=2;

		CHECK_BIN(len, c);
	  PUT_BINARY(data, (unsigned char*)String, c, pos);
  }
}

//---------------------------------------------------------------------------
wchar_t *StringFieldW::GetString(void)
{
return String;
}

//---------------------------------------------------------------------------
void StringFieldW::SetString(const wchar_t *Str)
{
if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
if (!Str) return;
if (Physical)
  {
  RememberSubtableEntry();
  free(String);
  String = NULL;
  }
String = _wcsdup(Str);
}

void StringFieldW::SetString(const char *Str)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	if (!Str) return;
	if (Physical)
  {
		RememberSubtableEntry();
		free(String);
		String = NULL;
  }
	int len=MultiByteToWideChar(CP_ACP, 0, Str, -1, 0, 0);
	String = (wchar_t*)malloc(len * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, Str, -1, String, len);
}

//---------------------------------------------------------------------------
int StringFieldW::GetDataSize(void)
{
if (!String) return 2;
return (int)wcslen(String)*2+4;
}

//---------------------------------------------------------------------------
int StringFieldW::Compare(Field *Entry)
{
if (!Entry) return -1;
if (Entry->GetType() != GetType()) return 0;
return mywcsicmp(GetString(), ((StringFieldW*)Entry)->GetString());
}

BOOL WINAPI IsCharSpaceW(WCHAR wc)
{
   WORD CharType;
   return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_SPACE);
}

//---------------------------------------------------------------------------
BOOL StringFieldW::ApplyFilter(Field *Data, int op)
{
BOOL r;
wchar_t *p = ((StringFieldW *)Data)->GetString();
wchar_t *d = String;
if (!p)
  p = L"";
if (!d)
  d = L"";

switch (op)
  {
  case FILTER_EQUALS:
    r = (BOOL)!_wcsicmp(d, p);
    break;
  case FILTER_NOTEQUALS:
    r = (BOOL)_wcsicmp(d, p);
    break;
  case FILTER_CONTAINS:
    r = (NULL != wcsistr(d, p));
    break;
  case FILTER_NOTCONTAINS:
    r = (NULL == wcsistr(d, p));
    break;
  case FILTER_ABOVE:
    r = (BOOL)(_wcsicmp(d, p) > 0);
    break;
  case FILTER_ABOVEOREQUAL:
    r = (BOOL)(_wcsicmp(d, p) >= 0);
    break;
  case FILTER_BELOW:
    r = (BOOL)(_wcsicmp(d, p) < 0);
    break;
  case FILTER_BELOWOREQUAL:
    r = (BOOL)(_wcsicmp(d, p) <= 0);
    break;
  case FILTER_BEGINS:
    r = (BOOL)(_wcsnicmp(d, p, wcslen(p)) == 0);
    break;
  case FILTER_ENDS: {
          int lenp = (int)wcslen(p), lend = (int)wcslen(d);
    if (lend < lenp) return 0;  // too short
    r = (BOOL)(_wcsicmp((d + lend) - lenp, p) == 0);
        }
    break;
  case FILTER_LIKE:

// todo: make configurable words to skip, as well as trailing whitespace removal
#define SKIP_THE_AND_WHITESPACE(x) { wchar_t *save##x=(wchar_t*)x; while (IsCharSpaceW(*x) && *x) x=CharNextW(x); if (!_wcsnicmp(x,L"the ",4)) x+=4; while (IsCharSpaceW(*x)) x=CharNextW(x); if (!*x) x=save##x; }

    SKIP_THE_AND_WHITESPACE(d)
    SKIP_THE_AND_WHITESPACE(p)
    r = (BOOL)(_wcsicmp(d, p) == 0);
    break;
  default:
    r = TRUE;
    break;
  }
return r;
}
