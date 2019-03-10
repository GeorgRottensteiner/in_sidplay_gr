/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Raw Index Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __RAWINDEX_H
#define __RAWINDEX_H

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define BLOCK_SIZE 2048 // 8192 entries blocks

class Index : public LinkedListEntry
	{
	friend class Record;
	friend class Table;
	friend class IndexField;
	friend class Scanner;

	protected:

		Index(VFILE *Handle, unsigned char id, int pos, int type, BOOL newindex, int nentries, Table *parentTable);
		~Index();
		int Get(int Idx);
		void Set(int Idx, int P);
		VFILE *Handle;
		VFILE *TableHandle;
		Table *pTable;
		int NEntries;
		void LoadIndex(BOOL newindex);
		void WriteIndex(void);
		int Insert(Index *parindex, int N, BOOL localonly);
		int Insert(int N);
		int Update(int Idx, int Pos, Record *record, BOOL localonly);
		int Update(Index *parindex, int paridx, int Idx, int Pos, Record *record, BOOL forceLast, BOOL localonly);
    unsigned char GetId();
		int *IndexTable;
		int MaxSize;
		unsigned char Id;
		BOOL Modified;
		BOOL InChain;
		int Position;
		int DataType;
		BOOL InInsert;
		int InChainIdx;
		IndexField *SecIndex;
		int FindSortedPlace(Field *field, int idx, int *laststate, int start);
		int FindSortedPlaceEx(Field *field, int idx, int *laststate, int start, int comp_mode);
		int MoveIndex(int idx, int newidx);
		void Colaborate(IndexField *secindex);
		void SetCooperative(int Idx, int secpos);
		int GetCooperative(int Idx);
		void UpdateMe(Index *Me, int newidx, int oldidx);
		Field *QuickFindField(unsigned char Id, int Pos);
		int QuickFind(int Id, Field *field, int start);
		int QuickFindEx(int Id, Field *field, int start, int comp_mode);
		int TranslateIndex(int Pos, Index *index);
		void Delete(int Idx, int Pos, Record *record);
		void Shrink(void);
		BOOL locateUpToDate;
		void Propagate(void);
    void SetGlobalLocateUpToDate(BOOL isUptodate);
    int NeedFix();
};


#endif