/* ---------------------------------------------------------------------------
Nullsoft Database Engine
--------------------
codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

StringField Class

TODO:
* Try to remove assumption that sizeof(wchar_t) == 2
--------------------------------------------------------------------------- */

#include "NDE.h"
#include "StringField.h"

#ifdef _WIN32
#include "../nu/AutoChar.h"
#include "../nu/AutoWide.h"
#endif

static wchar_t CharSwap(wchar_t value)
{
	return (value >> 8) | (value << 8);
}

//---------------------------------------------------------------------------
#ifdef _WIN32
StringField::StringField(const char *Str)
{
	InitField();
	Type = FIELD_STRING;
	if (Str) 
		String = strdup(Str);
	Kind = DELETABLE;
}

StringField::StringField(const wchar_t *Str, int strkind)
{
	InitField();
	Type = FIELD_STRING;
	if (Str) 
	{
		if (strkind == STRING_IS_WCHAR)
			StringW = ndestring_wcsdup(Str);
		else
		{
			StringW = const_cast<wchar_t *>(Str);
			ndestring_retain(StringW);
		}
	}
	Kind = DELETABLE;
}
#endif

#ifdef __APPLE__
StringField::StringField(const wchar_t *Str)
{
	InitField();
	Type = FIELD_STRING;
	if (Str) 
	{
		SetStringW(Str);
	}
	Kind = DELETABLE;
}
#endif
//---------------------------------------------------------------------------
void StringField::InitField(void)
{
	Type = FIELD_STRING;
	String = NULL;
	#ifdef _WIN32
	StringW = NULL;
	optimized_the = 0;
	#endif
	Kind = DELETABLE;
	Physical = FALSE;

}

//---------------------------------------------------------------------------
StringField::StringField()
{
	InitField();
}

//---------------------------------------------------------------------------
StringField::~StringField()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
	#ifdef _WIN32
	free(String);
	ndestring_release(StringW);
	StringW=0;
	#elif defined(__APPLE__)
	if (String)
		CFRelease(String);
	#endif
}

//---------------------------------------------------------------------------
#ifdef _WIN32
void StringField::MakeUnicodeString()
{
	if (String && !StringW)
		StringW = ndestring_wcsdup(AutoWideDup(String));  // TODO: optimize this 
}

void StringField::MakeANSIString()
{
	if (StringW && !String)
		String = AutoCharDup(StringW);
}
#endif

//---------------------------------------------------------------------------
void StringField::ReadTypedData(unsigned char *data, size_t len)
{
	unsigned short c;
	int pos=0;

	CHECK_SHORT(len);
	c = GET_SHORT(); pos+=2;
	if (c)
	{
		bool unicode=false;
		bool reverseEndian=false;
		if (c >= 2 // enough room for BOM
			&& (c % sizeof(wchar_t)) == 0) // can't be unicode if it's not an even multiple of sizeof(wchar_t)
		{
			wchar_t BOM=0;
			GET_BINARY((unsigned char *)&BOM, data, 2, pos);
			if (BOM == 0xFEFF)
			{
			#ifndef __APPLE__ // CFString handles the BOM for us
				pos+=2;
				c-=2;	
				#endif
				unicode=true;
			}
			else if (BOM == 0xFFFE)
			{
			#ifndef __APPLE // CFString handles the BOM for us
				pos+=2;
				c-=2;	
				#endif
				unicode=true;
				reverseEndian=true;
			}
		}

		CHECK_BIN(len, c);
		if (unicode)
		{
		#ifdef __APPLE__
		if (String)
			CFRelease(String);
		String = CFStringCreateWithBytes(kCFAllocatorDefault, data, c, kCFStringEncodingUTF16, true); 
		#elif defined(_WIN32)
			ndestring_release(StringW);
			StringW = ndestring_malloc(c+sizeof(wchar_t));

			GET_BINARY((unsigned char *)StringW, data, c, pos);
			StringW[c/2]=0;
			if (reverseEndian)
			{
				for (unsigned short i=0;i<c;i++)
					StringW[i]=CharSwap(StringW[i]);
			}
			#else
			#error port me
			#endif
		}
		else
		{
		#ifdef __APPLE__
		if (String)
			CFRelease(String);
		String = CFStringCreateWithBytes(kCFAllocatorDefault, data, c, kCFStringEncodingWindowsLatin1, false); 
		#elif defined(_WIN32)
			String = (char *)malloc(c+1);
			GET_BINARY((unsigned char *)String, data, c, pos);
			String[c]=0;
			#else
			#error port me
			#endif
		}
	}
}

//---------------------------------------------------------------------------
void StringField::WriteTypedData(unsigned char *data, size_t len)
{
	int pos=0;

#ifdef _WIN32
	unsigned short c;
	if (StringW)
	{
		c = (unsigned short)wcslen(StringW) * sizeof(wchar_t) + 2 /* for BOM */;
		// write size
		CHECK_SHORT(len);
		PUT_SHORT(c); pos+=2;

		// write byte order mark
		CHECK_BIN(len, 2);
		wchar_t BOM = 0xFEFF;
		PUT_BINARY(data, (unsigned char *)&BOM, 2, pos);
		pos+=2;
		c-=2;

		// write string
		CHECK_BIN(len, c);
		PUT_BINARY(data, (unsigned char *)StringW, c, pos);
	}
	else
	{
		if (String) c = (unsigned short)strlen(String); else c = 0;
		CHECK_SHORT(len);
		PUT_SHORT(c); pos+=2;
		if (String) 
		{
			CHECK_BIN(len, c);
			PUT_BINARY(data, (unsigned char*)String, c, pos);
		}
	}
	#elif defined(__APPLE__)
	CHECK_SHORT(len);
	if (String)
	{
		CFIndex lengthRequired=0;
		CFStringGetBytes(String, CFRangeMake(0, CFStringGetLength(String)), kCFStringEncodingUTF16, 0, true, NULL, 0, &lengthRequired);
		CHECK_BIN(len, lengthRequired+2);
		PUT_SHORT(lengthRequired); pos+=2;

		CFStringGetBytes(String, CFRangeMake(0, CFStringGetLength(String)), kCFStringEncodingUTF16, 0, true, data+pos, lengthRequired, 0);
	}
	else
	{
		PUT_SHORT(0);
	}
	#else
	#error port me
	#endif
}

#ifdef __APPLE__
CFStringRef StringField::GetString()
{
	return String;
}

void StringField::SetStringW(const wchar_t *Str)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	if (!Str) return;
	if (Physical)
		RememberSubtableEntry();
	
	if (String)
		CFRelease(String);
	String=		CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)Str, wcslen(Str)*sizeof(wchar_t), kCFStringEncodingUTF32, false);
}
#endif

#ifdef _WIN32
//---------------------------------------------------------------------------
char *StringField::GetString(void)
{
	MakeANSIString();
	return String;
}

wchar_t *StringField::GetStringW(void)
{
	MakeUnicodeString();
		return StringW;
}

//---------------------------------------------------------------------------
void StringField::SetString(const char *Str)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	if (!Str) return;
	if (Physical)
		RememberSubtableEntry();

	free(String);
	String = NULL;
	ndestring_release(StringW);
	StringW = NULL;
	String = strdup(Str);
	optimized_the=0;
}

//---------------------------------------------------------------------------
void StringField::SetStringW(const wchar_t *Str)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	if (!Str) return;
	if (Physical)
		RememberSubtableEntry();

	free(String);
	String = NULL;
	ndestring_release(StringW);
	StringW = NULL;
	StringW = ndestring_wcsdup(Str);
	optimized_the=0;
}

//---------------------------------------------------------------------------
void StringField::SetNDEString(wchar_t *Str)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	if (!Str) return;
	if (Physical)
		RememberSubtableEntry();

	free(String);
	String = NULL;
	// copy and then release, just in case we're copying into ourselves
	wchar_t *oldStr = StringW;
	StringW = Str;
	ndestring_retain(StringW);
	ndestring_release(oldStr);
	optimized_the=0;
}
#endif
//---------------------------------------------------------------------------
size_t StringField::GetDataSize(void)
{
#ifdef _WIN32
	if (StringW)
	{
		return wcslen(StringW)*sizeof(wchar_t)+2 /*for BOM*/ + 2;
	}
	else
	{
		if (!String) return 2;
		return strlen(String) + 2;
	}
	#elif defined(__APPLE__)
	if (String)
	{
		
		CFIndex lengthRequired=0;
		CFStringGetBytes(String, CFRangeMake(0, CFStringGetLength(String)), kCFStringEncodingUTF16, 0, true, NULL, 0, &lengthRequired);
		return lengthRequired+2;
	}
	else
		return 2;
	#else
	#error port me
	#endif
}

//---------------------------------------------------------------------------
int StringField::Compare(Field *Entry)
{
	if (!Entry) return -1;
	if (Entry->GetType() != GetType()) return 0;
	#ifdef _WIN32
	return mywcsicmp(GetStringW(), ((StringField*)Entry)->GetStringW());
	#elif defined(__APPLE__)
	CFStringRef compareString = ((StringField*)Entry)->GetString();
	if (!String && !compareString) return 0;
	if (!String && compareString) return 1;
	if (!compareString) return -1;
	
	return CFStringCompare(String, compareString, kCFCompareCaseInsensitive|kCFCompareNonliteral);
	#else
	#error port me
	#endif
}

//---------------------------------------------------------------------------
int StringField::Starts(Field *Entry)
{
	if (!Entry) return -1;
	if (Entry->GetType() != GetType()) return 0;
	#ifdef _WIN32
	return (mywcsistr(GetStringW(), ((StringField*)Entry)->GetStringW()) == GetStringW());
	#elif defined(__APPLE__)
	CFStringRef compareString = ((StringField*)Entry)->GetString();
	if (!String || !compareString) return 0;

	CFRange findRange = CFStringFind(String, compareString, 	kCFCompareCaseInsensitive|kCFCompareNonliteral|kCFCompareAnchored);
	if (findRange.location == kCFNotFound)
		return 0;
	return findRange.location == 0;
	
	#else
	#error port me
	#endif
}

//---------------------------------------------------------------------------
int StringField::Contains(Field *Entry)
{
	if (!Entry) return -1;
	if (Entry->GetType() != GetType()) return 0;
	#ifdef _WIN32
	return (mywcsistr(GetStringW(), ((StringField*)Entry)->GetStringW()) != NULL);
	#elif defined(__APPLE__)
	CFStringRef compareString = ((StringField*)Entry)->GetString();
	if (!String || !compareString) return 0;

	CFRange findRange =  CFStringFind(String, compareString, 	kCFCompareCaseInsensitive|kCFCompareNonliteral);
	return findRange.location != kCFNotFound;
	#else
	#error port me
	#endif
}


#ifdef __APPLE__

//---------------------------------------------------------------------------
bool StringField::ApplyFilter(Field *Data, int op)
{
	// TODO: maybe do this?
	// TODO: maybe do this?
	if (op == FILTER_ISEMPTY || op == FILTER_ISNOTEMPTY)
	{
		bool r = (op == FILTER_ISEMPTY);
		if (!String)
			return r;
		
		if (CFStringGetLength(String) == 0)
			return r;		
		
		return !r;
	}
	//
	bool r;
	StringField *compField = (StringField *)Data;
	switch (op)
	{
	case FILTER_EQUALS:
			r = !Compare(Data);
		break;
	case FILTER_NOTEQUALS:
			r = !!Compare(Data);
		break;
	case FILTER_CONTAINS:
			r = !!Contains(Data);
		break;
	case FILTER_NOTCONTAINS:
			r = !Contains(Data);
		break;
	case FILTER_ABOVE:
		r = (bool)(Compare(Data) > 0);
		break;
	case FILTER_ABOVEOREQUAL:
		r = (bool)(Compare(compField) >= 0);
		break;
	case FILTER_BELOW:
		r = (bool)(Compare(compField) < 0);
		break;
	case FILTER_BELOWOREQUAL:
		r = (bool)(Compare(compField) <= 0);
		break;
	case FILTER_BEGINS:
		r = !!Starts(compField);
		break;
	case FILTER_ENDS:
		{
			CFStringRef compareString = ((StringField*)Data)->GetString();
			if (!String || !compareString) return 0;
			
			CFRange findRange = CFStringFind(String, compareString, 	kCFCompareCaseInsensitive|kCFCompareNonliteral|kCFCompareAnchored|kCFCompareBackwards);
			if (findRange.location == kCFNotFound)
				r=0;
			else
				r = findRange.location != 0;
		}
		break;
	case FILTER_LIKE:
/* TODO
		if (compField->optimized_the)
			p = compField->optimized_the;
		else
		{
			SKIP_THE_AND_WHITESPACEW(p);
			compField->optimized_the = p;				
		}

		if (optimized_the)
			d = optimized_the;
		else
		{
			SKIP_THE_AND_WHITESPACEW(d);
			optimized_the=d;
		}
 		r = (bool)(nde_wcsicmp(d, p) == 0);
*/
			r = !!Compare(compField);
		break;
	case FILTER_BEGINSLIKE:
/*
		if (compField->optimized_the)
			p = compField->optimized_the;
		else
		{
			SKIP_THE_AND_WHITESPACEW(p);
			compField->optimized_the = p;				
		}

		if (optimized_the)
			d = optimized_the;
		else
		{
			SKIP_THE_AND_WHITESPACEW(d);
			optimized_the=d;
		}

		r = (bool)(nde_wcsnicmp(d, p, wcslen(p)) == 0);
		*/
			r = !!Starts(compField);
		break;
	default:
		r = true;
		break;
	}
	return r;
}

#elif defined(_WIN32)
// todo: make configurable words to skip, as well as trailing whitespace removal
inline BOOL WINAPI IsCharSpaceW(wchar_t c) { return (c == L' ' || c == L'\t'); }
inline bool IsTheW(const wchar_t *str) { if (str && (str[0] == L't' || str[0] == L'T') && (str[1] == L'h' || str[1] == L'H') && (str[2] == L'e' || str[2] == L'E') && (str[3] == L' ')) return true; else return false; }
#define SKIP_THE_AND_WHITESPACEW(x) { wchar_t *save##x=(wchar_t*)x; while (IsCharSpaceW(*x) && *x) x++; if (IsTheW(x)) x+=4; while (IsCharSpaceW(*x)) x++; if (!*x) x=save##x; }

bool StringField::ApplyFilter(Field *Data, int op)
{
	// TODO: maybe do this?
	
	if (op == FILTER_ISEMPTY || op == FILTER_ISNOTEMPTY)
	{
		bool r = (op == FILTER_ISEMPTY);
		if (!String && !StringW)
			return r;
		
		if (String && String[0] == 0)
			return r;
		
		if (StringW && StringW[0] == 0)
			return r;
		
		return !r;
	}
	//
	bool r;
	StringField *compField = (StringField *)Data;
	
	const wchar_t *p = compField->GetStringW();
	const wchar_t *d = GetStringW();
	if (!p)
		p = L"";
	if (!d)
		d = L"";
	
	switch (op)
	{
		case FILTER_EQUALS:
			r = !nde_wcsicmp(d, p);
			break;
		case FILTER_NOTEQUALS:
			r = !!nde_wcsicmp(d, p);
			break;
		case FILTER_CONTAINS:
			r = (NULL != wcsistr(d, p));
			break;
		case FILTER_NOTCONTAINS:
			r = (NULL == wcsistr(d, p));
			break;
		case FILTER_ABOVE:
			r = (bool)(nde_wcsicmp(d, p) > 0);
			break;
		case FILTER_ABOVEOREQUAL:
			r = (bool)(nde_wcsicmp(d, p) >= 0);
			break;
		case FILTER_BELOW:
			r = (bool)(nde_wcsicmp(d, p) < 0);
			break;
		case FILTER_BELOWOREQUAL:
			r = (bool)(nde_wcsicmp(d, p) <= 0);
			break;
		case FILTER_BEGINS:
			r = (bool)(nde_wcsnicmp(d, p, wcslen(p)) == 0);
			break;
		case FILTER_ENDS:
		{
			int lenp = (int)wcslen(p), lend = (int)wcslen(d);
			if (lend < lenp) return 0;  // too short
			r = (bool)(nde_wcsicmp((d + lend) - lenp, p) == 0);
		}
			break;
		case FILTER_LIKE:
			
			if (compField->optimized_the)
				p = compField->optimized_the;
			else
			{
				SKIP_THE_AND_WHITESPACEW(p);
				compField->optimized_the = p;				
			}
			
			if (optimized_the)
				d = optimized_the;
			else
			{
				SKIP_THE_AND_WHITESPACEW(d);
				optimized_the=d;
			}
			
			r = (bool)(nde_wcsicmp(d, p) == 0);
			break;
			case FILTER_BEGINSLIKE:
			
			if (compField->optimized_the)
				p = compField->optimized_the;
			else
			{
				SKIP_THE_AND_WHITESPACEW(p);
				compField->optimized_the = p;				
			}
			
			if (optimized_the)
				d = optimized_the;
			else
			{
				SKIP_THE_AND_WHITESPACEW(d);
				optimized_the=d;
			}
			
			r = (bool)(nde_wcsnicmp(d, p, wcslen(p)) == 0);
			break;
			default:
			r = true;
			break;
	}
	return r;
}

#endif