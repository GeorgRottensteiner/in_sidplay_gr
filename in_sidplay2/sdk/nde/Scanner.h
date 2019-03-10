/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Scanner Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __SCANNER_H
#define __SCANNER_H

class Scanner;
/*
class ScannerJoin {
  public:
    
    ScannerJoin(Scanner *scanner, const char *joined_field) : scan(scanner), field(joined_field) {  }
    virtual ~ScannerJoin() {  }

    Scanner *getScanner() { return scan; }
    const char *getField() { return field; }

  private:

    String field;
    Scanner *scan;
};
*/

class Scanner : LinkedListEntry {

	friend class Table;
	friend class Record;
	friend class LinkedList;
	friend class Index;
	
	public: // should be protected 

		Scanner(Table *parentTable, BOOL CanEdit, BOOL orderIntegrity);
		~Scanner();

		Table *pTable;

		void IndexModified(void);
		BOOL iModified;
		BOOL orderIntegrity;

		Record *GetRecord(int Idx);
		void GetCurrentRecord(void);
		bool MatchFilters(void);
		//BOOL MatchJoins(void);
		int CheckFilters(void);
		bool MatchFilter(Filter *filter);
		void CacheLastLocate(int Id, int From, Field *field, Index *i, int j);

    #include "Query.h"

		Index *index;
		Record *CurrentRecord;
		int CurrentRecordIdx;
		LinkedList FilterList;
		LinkedList Stack;
		Index *lastLocateIndex;
		int lastLocateIdx;
		Field *lastLocateFieldClone;
		int lastLocateFrom;
		int lastLocateId;
    //int lastJoinCache;
    //int invalidJoinCache;
		BOOL Edition;
		BOOL canEdit;

		int ResultPtr;
		BOOL FiltersOK;
    ColumnField *subtablecolumn;
    //PtrList<ScannerJoin> joined;
    //int inMatchJoins;
    //String last_query;
  
    Index *GetIndex() { return index; }

	public:

	  ColumnField *GetColumnByName(const char *FieldName);
	  ColumnField *GetColumnById(unsigned char id);

		Field *NewFieldByName(const char *fieldName, unsigned char Perm);
		Field *NewFieldById(unsigned char Id, unsigned char Perm);
		void DeleteField(Field *field);
		void DeleteFieldByName(const char *name); 
		void DeleteFieldById(unsigned char Id);

 		void Cancel(void);
		void Insert(void);
		void Edit(void);
		void Post(void);
		void Delete(void); 
		BOOL Editing(void);

		LinkedList *GetFields(void);
		Field *GetFieldByName(const char *FieldName);
		Field *GetFieldById(unsigned char Id);

		void First(int *killswitch=0);
		void Last(int *killswitch=0);
		int Next(int force_block=1, int *killswitch=0);
		int Previous(int force_block=1, int *killswitch=0);
		BOOL Eof(void);
		BOOL Bof(void);
		void New(void);
		int GetRecordsCount(void);
		void GetRecordById(int Id, BOOL checkFilters=TRUE);
		int GetRecordId(void);
		void Sync(void);
		BOOL LocateByName(char *column, int From, Field *field, int *nskip=NULL);
		BOOL LocateById(int Id, int From, Field *field, int *nskip=NULL);
		BOOL LocateByIdEx(int Id, int From, Field *field, int *nskip, int comp_mode);
		void Push(void);
		void Pop(void);

		// Filters
    int AddFilterByName(wchar_t *name, Field *Data, unsigned char Op);
    int AddFilterById(unsigned char Id, Field *Data, unsigned char Op);
    int AddFilterOp(unsigned char Op);
    void RemoveFilters(void);
    LinkedList *GetFilters(void); // CUT  FG>NO! why ??
    Filter *GetLastFilter(void);

		BOOL SetWorkingIndexByName(char *desc);
		BOOL SetWorkingIndexById(unsigned char Id);

    BOOL HasIndexChanged(void) { return iModified; }
		void ClearDirtyBit(void);
		int GetConsistencyInfo(void);
		float FragmentationLevel(void);
    //void JoinScanner(Scanner *scanner, const char *field);
    //void UnjoinScanner(Scanner *scanner);

    ColumnField *getSubtableColumnField() { return subtablecolumn; }
    void setSubtableColumnField(ColumnField *f) { subtablecolumn = f; }

		Table *GetTable();

    void setBlocking(int b) { allow_blocking = b; }
    int allow_blocking;
    int in_query_parser;

    int disable_date_resolution;
	};


#endif
