/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Field Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __FIELD_H
#define __FIELD_H

#include "LinkedList.h"
#include <stdio.h>

#define PUT_INT(y) data[pos]=(unsigned char)(y&255); data[pos+1]=(unsigned char)((y>>8)&255); data[pos+2]=(unsigned char)((y>>16)&255); data[pos+3]=(unsigned char)((y>>24)&255)
#define GET_INT() (int)(data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
#define PUT_SHORT(y) data[pos]=(unsigned char)(y&255); data[pos+1]=(unsigned char)((y>>8)&255);
#define GET_SHORT() (unsigned short)(data[pos]|(data[pos+1]<<8))
#define PUT_PINT(y) data[*pos]=(unsigned char)(y&255); data[*pos+1]=(unsigned char)((y>>8)&255); data[*pos+2]=(unsigned char)((y>>16)&255); data[*pos+3]=(unsigned char)((y>>24)&255)
extern float GET_FLOAT(unsigned char *data, size_t pos);
extern void PUT_FLOAT(float f, unsigned char *data, size_t pos);
extern void PUT_BINARY(unsigned char *dest, unsigned char *src, size_t size, size_t pos);
extern void GET_BINARY(unsigned char *dest, unsigned char *src, size_t size, size_t pos);
#define PUT_CHAR(y) data[pos]=y
#define GET_CHAR() data[pos]
#define CHECK_CHAR(l) { if (l < 1) return; l--; }
#define CHECK_INT(l) { if (l < 4) return; l-=4; }
#define CHECK_INT64(l) { if (l < 8) return; l-=8; }
#define CHECK_SHORT(l) { if (l < 2) return; l-=2; }
#define CHECK_BIN(l, size) { if (l < size) return; l-=size; }

#define ASSERT(x) { \
if (!(x)) \
OutputDebugString("Assertion failed "); \
}

#define _CHECK_CHAR(l) { ASSERT(l >= 1); l-=1; }
#define _CHECK_INT(l) { ASSERT(l >= 4); l-=4; }
#define _CHECK_SHORT(l) { ASSERT(l >= 2); l-=2; }
#define _CHECK_BIN(l, size) { ASSERT(l >= size); l-=size; }

class Field: public LinkedListEntry
	{
	friend class LinkedList;
	friend class Record;
	friend class Table;
	friend class Index;
	friend class PrivateField;
	friend Field *TranslateObject(unsigned char Type, Table *tbl);
	friend class Scanner;
	friend class Filter;
	friend class ColumnField;
	
	protected:
	
		Field(int FieldPos, Table *parentTable);
		Field *ReadField(int pos);
		Field *ReadField(int pos, BOOL readTyped);
		virtual void SetDeletable(void);
		void WriteField(void);
		virtual void ReadTypedData(unsigned char *, size_t /*len*/) { };
		virtual void WriteTypedData(unsigned char *, size_t) { };
    virtual size_t GetDataSize(void) { return 0; };
		virtual int Compare(Field * /*Entry*/) { return 0; };
		virtual int Starts(Field * /*Entry*/) { return 0; };
		virtual int Contains(Field * /*Entry*/) { return 0; };
    virtual bool ApplyFilter(Field * /*Data*/, int /*flags*/) { return false; };
		int GetFieldPos(void);
		int GetNextFieldPos(void);
		void SetTable(Table *parentTable);
		VFILE *HTable;
		Table *pTable;
		int Pos;
		int NextFieldPos;
		int PreviousFieldPos;
		int Kind;
		unsigned char ID;
		unsigned char Type;
    size_t MaxSizeOnDisk;
		unsigned char Perm;
		BOOL Readonly;
		BOOL IsReadonly(void);
		Field();
		void InitField(void);
		void RememberSubtableEntry(void);
		void UpdateSubtableEntry(void);
		void UpdateSubtableEntry(BOOL Remove);
		int SubtableRecord;
		BOOL Physical;
		void OnRemove(void);

	public:

		virtual ~Field();
		Field *Clone(void);
		unsigned char GetFieldId(void) { return ID; }
    virtual int GetType();
    virtual unsigned char GetPerm();
	};

#endif
