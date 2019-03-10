/* ---------------------------------------------------------------------------

                 Nullsoft Database Engine - Codename: Neutrino

--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Global Include File

--------------------------------------------------------------------------- */

#ifndef __NDE_H
#define __NDE_H

// TODO: find better Mac preproc symbol

extern char *tSign;
extern char *iSign;

// Magic headers
#define __TABLE_SIGNATURE__ tSign
#define __INDEX_SIGNATURE__ iSign

// Linked list entry types
#define UNKNOWN           0
#define FIELD             1
#define FILTER           2
#define SCANNER           3

// Field types
#define FIELD_CLONE     255

#define FIELD_UNKNOWN   255
enum
{
	FIELD_COLUMN   =  0,
	FIELD_INDEX     =  1,
	FIELD_REDIRECTOR =2,
	FIELD_STRING     =3,
	FIELD_INTEGER    = 4,
	FIELD_BOOLEAN    = 5,
	FIELD_BINARY     =6, // max size 65536
	FIELD_GUID       =7,
	FIELD_PRIVATE    = 8,
	FIELD_BITMAP     =6,
	FIELD_FLOAT      = 9,
	FIELD_DATETIME   =10,
	FIELD_LENGTH     =11,
	FIELD_FILENAME	=12,
	FIELD_INT64    = 13,
	FIELD_BINARY32  =14, // binary field, but 32bit sizes instead of 16bit
	FIELD_INT128   = 15,  // mainly for storing MD5 hashes
};


#define DELETABLE         1
#define NONDELETABLE     2

// Records constants
#define NEW_RECORD    -128
#define FIELD_NOT_FOUND -1
#define INVALID_RECORD  -1

// Index constants
#define PRIMARY_INDEX   255
#define QFIND_ENTIRE_SCOPE -1
#define FIRST_RECORD    -1

// compare modes 
#define COMPARE_MODE_CONTAINS		1
#define COMPARE_MODE_EXACT			2
#define COMPARE_MODE_STARTS			3

#define throwException(x) {}

// Filter types
enum {
  FILTER_NONE = 100,
  FILTER_EQUALS,
  FILTER_NOTEQUALS,
  FILTER_CONTAINS,
  FILTER_NOTCONTAINS,
  FILTER_ABOVE,
  FILTER_BELOW,
  FILTER_ABOVEOREQUAL,
  FILTER_BELOWOREQUAL,
  FILTER_BEGINS,
  FILTER_ENDS,
  FILTER_LIKE,
  FILTER_ISEMPTY,
  FILTER_ISNOTEMPTY,
	FILTER_BEGINSLIKE,
};

#define FILTER_NOT       0
#define FILTER_AND       1
#define FILTER_OR         2

#define FILTERS_INVALID    -1
#define FILTERS_INCOMPLETE  1
#define FILTERS_COMPLETE    1
#define ADDFILTER_FAILED    2

// Permissions
#define PERM_DENYALL     0
#define PERM_READ         1
#define PERM_READWRITE   3


#if defined(WIN32_NOLIB) || !defined(WIN32)
#define NDE_API
#else
 #ifdef NDE_EXPORTS
    #define NDE_API __declspec(dllexport)
 #else
    #define NDE_API __declspec(dllimport)
 #endif
#endif

// All our classes so we can foreward reference
class NDE_API LinkedListEntry;
class NDE_API LinkedList;
class NDE_API Field;
class NDE_API ColumnField;
class NDE_API IndexField;
class NDE_API StringField;
class NDE_API IntegerField;
class NDE_API Int64Field;
class NDE_API Int128Field;
class NDE_API GUIDField;
class NDE_API FloatField;
class NDE_API BooleanField;
class NDE_API BinaryField;
class NDE_API Binary32Field;
class NDE_API Binary32FieldReadOnly;
class NDE_API BitmapField;
class NDE_API PrivateField;
class NDE_API FilenameField;
class NDE_API Record;
class NDE_API Scanner;
class NDE_API Table;
class NDE_API Index;
class NDE_API Filter;
class NDE_API Database;
class NDE_API NdeApi;

#ifndef WIN32
#define NDE_NOWIN32FILEIO
#define NO_TABLE_WIN32_LOCKING
typedef int BOOL;
typedef void* HANDLE;
#define TRUE 1
#define FALSE 0
#include <bfc/platform/types.h>
#define HINSTANCE int
//#define HWND int
#define DWORD int
#define wsprintf sprintf
#define OutputDebugString(x) ;
#define GetCurrentProcessId() getpid()

#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <wchar.h>
#include <sys/stat.h>
void clear_stdin(void);

void CopyFile(const char *filename, const char *destfilename, BOOL b);
void DeleteFile(const char *filename);
BOOL MoveFile(const char *filename, const char *destfilename);
void Sleep(int ms);

#define stricmp strcasecmp
#define strcmpi strcasecmp
#define strnicmp strncasecmp
#define strncmpi strncasecmp

#define _wtoi(x) wcstol(x,0,10)
// TODO: find case insensitive compare on Mac OS X
#define wcsicmp wcscmp 
#define wcsnicmp wcsncmp

#define _MAX_PATH 8192
#define _MAX_DRIVE 256
#define _MAX_DIR 7424
#define _MAX_FNAME 256
#define _MAX_EXT 256

#define min(x,y) ((x > y) ? y : x)
#define max(x,y) ((x < y) ? y : x)

#endif

// All our includes
#include "Vfs.h"
#include "LinkedList.h"
#include "Field.h"
#include "ColumnField.h"
#include "IndexField.h"
#include "StringField.h"
#include "IntegerField.h"
#include "Int64Field.h"
#include "Int128Field.h"
#include "BinaryField.h"
#include "Binary32Field.h"
#include "FilenameField.h"
#include "Record.h"
#include "Scanner.h"
#include "Table.h"
#include "Database.h"
#include "Index.h"
#include "Filter.h"
#include "DBUtils.h"


#endif
