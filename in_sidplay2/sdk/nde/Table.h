/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Table Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __TABLE_H
#define __TABLE_H

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define CONSISTENCY_OK                      0 // Table is valid
#define CONSISTENCY_DONTKNOW                1 // No info whatsoever
#define CONSISTENCY_CORRUPTED               2 // Table corrupted, restore failed, can't rebuild
#define CONSISTENCY_RESTORED                3 // Table was corrupted but successfully restored
#define CONSISTENCY_BACKUPCORRUPTED         4 // Table and backup are corrupted, can't rebuild
#define CONSISTENCY_REBUILT                 5 // Unique values table corrupted, restore failed, table rebuilt successfully

class Table
{
	friend class Record;
	friend class Scanner;
	friend class Index;
	friend class Field;
	friend class ColumnField;
	friend class Database;

private:
	void Init();
	void Reset();

protected:

	char *Name;
	char *IdxName;
	VFILE *Handle;
	VFILE *IdxHandle;
	BOOL AutoCreate;
	Record *FieldsRecord;
	BOOL HasNewHdr;
	int NIndexes;
	Record *IndexList;
	Database *db;
	Scanner *dScanner;
	LinkedList *Scanners;
	Scanner *NewScanner(BOOL, BOOL);
	int consistencyInfo;
	void IndexModified(void);
	int ConsistencyCheck(int in);
	void WriteConsistency(void);
	void SetConsistencyInfo(int s);
	void SetGlobalLocateUpToDate(BOOL is);
	BOOL Cached;
	BOOL doCRC;
	BOOL GLocateUpToDate;
	int numErrors;

	// Tables
	Table(char *TableName, char *IdxName, BOOL Create, Database *db, BOOL Cached);
	~Table();
	BOOL Open(void);
	void Close(void);

	static bool Compact_ColumnWalk(LinkedListEntry *Entry, int id, void *data1, void *data2);
	static bool Compact_IndexWalk(LinkedListEntry *Entry, int id, void *data1, void *data2);
public:

	// Columns
	ColumnField *NewColumn(unsigned char Id, char *name, unsigned char type, BOOL indexUniques);
	void DeleteColumn(ColumnField *field); // todo
	void DeleteColumnByName(const char *name); // todo
	void DeleteColumnById(unsigned char Id); // todo
	void PostColumns(void);
	Record *GetColumns(void);
	ColumnField *GetColumnByName(const char *FieldName);
	ColumnField *GetColumnById(unsigned char Id);
	BOOL HasNewColumns(void);
	void SetUniqueByName(char *name, BOOL tf);
	void SetUniqueById(int ID, BOOL tf);

	// Fields
	Field *NewFieldByName(const char *fieldName, unsigned char Perm);
	Field *NewFieldById(unsigned char Id, unsigned char Perm);
	LinkedList *GetFields(void);
	Field *GetFieldByName(const char *FieldName);
	Field *GetFieldById(unsigned char Id);
	void DeleteField(Field *field);
	void DeleteFieldByName(const char *name);
	void DeleteFieldById(unsigned char Id);

	// Records
	void First(void);
	void Last(void);
	void Next(void);
	void Previous(void);
	BOOL Eof(void);
	BOOL Bof(void);
	void New(void);
	void Insert(void);
	void Edit(void);
	void Cancel(void);
	void Post(void);
	void Delete(void);
	BOOL Editing(void);
	int GetRecordsCount(void);
	void GetRecordById(int Id);
	int GetRecordId(void);
	void Sync(void);
	BOOL LocateByName(char *column, int From, Field *field);
	BOOL LocateById(int Id, int From, Field *field);
	BOOL LocateByIdEx(int Id, int From, Field *field, int comp_mode);
	void Push(void);
	void Pop(void);

	// Indexes
	void AddIndexByName(char *FieldName, char *KeyName);
	void AddIndexById(unsigned char Id, char *KeyName);
	LinkedList *GetIndexes(void);
	IndexField *GetIndexByName(char *name);
	IndexField *GetIndexById(unsigned char Id);
	BOOL SetWorkingIndexByName(char *desc);
	BOOL SetWorkingIndexById(unsigned char Id);
	BOOL CheckIndexing(void);
	void DropIndexByName(char *desc);
	void DropIndexById(unsigned char Id);
	void DropIndex(IndexField *Ptr);

	// Filters
	int AddFilterByName(wchar_t *name, Field *Data, unsigned char Op);
	int AddFilterById(unsigned char Id, Field *Data, unsigned char Op);
	int AddFilterOp(unsigned char Op);
	void RemoveFilters(void);

	// Scanners
	Scanner *NewScanner(BOOL notifyUpdates);
	Scanner *GetDefaultScanner(void);
	void DeleteScanner(Scanner *scan);

	// Misc
	float FragmentationLevel(void);
	int GetConsistencyInfo(void);
	void MakeBackup();
	int RestoreBackup();
	void SetCRCChecks(BOOL tf);
	void Compact(int *progress = NULL);
	void doRecovery();
	void SetDlgInfo(int nrecords, int currecord, int nlost);

	int HasErrors()
	{
		return numErrors > 0;
	}
	int NumErrors()
	{
		return numErrors;
	}
	void IncErrorCount()
	{
		numErrors++;
	}
};

#endif