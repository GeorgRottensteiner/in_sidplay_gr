/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 ColumnField Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __COLUMNFIELD_H
#define __COLUMNFIELD_H

#include "LinkedList.h"
#include "Table.h"
#include "Field.h"
#include "Scanner.h"

class ColumnField : public Field
	{
	friend class LinkedList;
	friend class Table;
	friend Field *TranslateObject(unsigned char Type, Table *tbl);
	friend class Scanner;
	friend class Field;
public:	
	

	protected:
		ColumnField(unsigned char FieldID, char *FieldName, unsigned char FieldType, BOOL indexUnique, Table *parentTable);
		ColumnField();
		~ColumnField();
		virtual void ReadTypedData(unsigned char *, size_t len);
		virtual void WriteTypedData(unsigned char *, size_t len);
		virtual size_t GetDataSize(void);
		virtual int Compare(Field *Entry);
		void InitField(void);
		char *Name;
		unsigned char MyType;
		BOOL indexUnique;
		Table *uniqueTable;
		BOOL OpenSubTable(void);
		BOOL RebuildSubtable(void);
		Scanner *dScanner;
		void SetUnique(BOOL tf);
    void RemoveSubtable();
    void SetDataType(unsigned char type);
	
	public:
	
		unsigned char GetDataType(void);
		char *GetFieldName(void);
		Scanner *GetUniqueDataScanner(void);
		Scanner *NewScanner(BOOL notifyUpdates);
		void DeleteScanner(Scanner *s);
		BOOL GetUnique();
	};

#endif