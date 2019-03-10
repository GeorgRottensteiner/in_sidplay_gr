/* 
---------------------------------------------------------------------------
Nullsoft Database Engine
--------------------
codename: Near Death Experience
---------------------------------------------------------------------------
*/

/* 
---------------------------------------------------------------------------

StringField Class Prototypes

---------------------------------------------------------------------------
*/

#ifndef __STRINGFIELD_H
#define __STRINGFIELD_H

#ifdef _WIN32
#include "NDEString.h"
#elif defined(__APPLE__)
#include <CoreFoundation/CFString.h>
#endif

class StringField : public Field
{
protected:
	virtual void ReadTypedData(unsigned char *, size_t len);
	virtual void WriteTypedData(unsigned char *, size_t len);
	virtual size_t GetDataSize(void);
	virtual int Compare(Field *Entry);
	virtual int Starts(Field *Entry);
	virtual int Contains(Field *Entry);

	virtual bool ApplyFilter(Field *Data, int op);
	void InitField(void);
	
#ifdef _WIN32
	void MakeUnicodeString();
	void MakeANSIString();
	char *String;
	wchar_t *StringW;
	const wchar_t *optimized_the;
	#elif defined(__APPLE__)
	CFStringRef String;
#endif
public:
	StringField();
	~StringField();
	
#ifdef _WIN32
	StringField(const char *Str);
	StringField(const wchar_t *Str, int strkind=STRING_IS_WCHAR);
	char *GetString(void);
	wchar_t *GetStringW(void);
	void SetString(const char *Str);
	void SetStringW(const wchar_t *Str);
	void SetNDEString(wchar_t *Str);
	#elif defined(__APPLE__)
	StringField(const wchar_t *Str);
	CFStringRef GetString(); // CFRetain this if you need to keep it for a while
	void SetStringW(const wchar_t *Str);
#endif
};

#endif

