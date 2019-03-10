/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Record Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __RECORD_H
#define __RECORD_H

#include <stdio.h>

class Record
	{
	friend class Table;
	friend class Index;
	friend class Scanner;

  public:
    LinkedList *GetFields();

	protected:
	
		VFILE *HTable;
		VFILE *Idx;
		void WriteIndex(void);
		LinkedList Fields;
		Record(int RecordPos, int RecordIdx, int insertionPoint, VFILE *FileHandle, VFILE *IdxHandle, Table *p, Scanner *s);
		~Record();
		Field *GetField(unsigned char ID);
		void AddField(Field *field);
		void RemoveField(Field *field);
		void WriteFields(void);
		void Delete(void);
		void Undelete(void);
		int RecordIndex;
		Table *ParentTable;
		Scanner *ParentScanner;
		int InsertionPoint;
		BOOL Invalid;
	};

#endif
