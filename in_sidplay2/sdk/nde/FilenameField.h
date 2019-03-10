#ifndef NDE_FILENAMEFIELD_H
#define NDE_FILENAMEFIELD_H

/*
  Mostly the same as StringField
  but this implements OS-dependent string comparisons that make sense for the file system
*/

#include "NDE.h"
#include "NDEString.h"

class FilenameField : public StringField
{
protected:
	virtual int Compare(Field *Entry);
	virtual int Starts(Field *Entry);
	virtual int Contains(Field *Entry);

	virtual bool ApplyFilter(Field *Data, int op);

public:
	FilenameField(const char *Str);
	FilenameField(const wchar_t *Str, int strkind=STRING_IS_WCHAR);
	FilenameField();
	
};

#endif