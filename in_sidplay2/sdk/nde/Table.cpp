/* ---------------------------------------------------------------------------
Nullsoft Database Engine
--------------------
codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

Table Class

--------------------------------------------------------------------------- */
#include "NDE.h"
#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include "crc.h"
#ifdef __APPLE__
#include <bfc/file/splitpath.h>
#endif

char *tSign="NDETABLE";

//---------------------------------------------------------------------------
Table::Table(char *TableName, char *Idx, BOOL Create, Database *_db, BOOL _Cached)
{
	Cached = _Cached;
	db = _db;
	AutoCreate = Create;
	Name = strdup(TableName);
	IdxName = strdup(Idx);
	Init();
}

//---------------------------------------------------------------------------
void Table::Init()
{
	numErrors = 0;
	SetConsistencyInfo(CONSISTENCY_DONTKNOW);
	Scanners = new LinkedList();
	dScanner = NULL;
	dScanner = NewScanner(TRUE, FALSE);
	Handle=NULL;
	IdxHandle=NULL;
	FieldsRecord=NULL;
	HasNewHdr=FALSE;
	IndexList=NULL;
	doCRC = FALSE;
	GLocateUpToDate = FALSE;
}

//---------------------------------------------------------------------------
Table::~Table()
{
	Reset();
	if (Name)
		free(Name);
	if (IdxName)
		free(IdxName);
}

//---------------------------------------------------------------------------
void Table::Reset() 
{
	delete IndexList;
	IndexList=0;
	delete FieldsRecord;
	FieldsRecord=0;
	delete Scanners;
	Scanners=0;
	if (Handle)
		Vfclose(Handle);
	if (IdxHandle)
		Vfclose(IdxHandle);
}

//---------------------------------------------------------------------------
void Table::SetCRCChecks(BOOL tf) {
	doCRC = tf;
}

//---------------------------------------------------------------------------
BOOL Table::Open(void)
{
	BOOL Valid;
	int justcreated = 0;

	if (doCRC && !ConsistencyCheck(0))
		return FALSE;


#ifdef WIN32
	// lock
	char lockbuf[4096];
	wsprintf(lockbuf,"%s.lock",Name);
	int retry_cnt=0;

	HANDLE hFile;

	do
	{
		hFile = CreateFile(lockbuf,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			if (GetLastError() == ERROR_ACCESS_DENIED)
				return false;
			Sleep(100);
		}
	}
	while (hFile == INVALID_HANDLE_VALUE && retry_cnt++ < 100); // try for 10 seconds

	if (hFile == INVALID_HANDLE_VALUE) return FALSE; // db already locked, fail somewhat gracefully
#endif

	Handle = Vfopen(Name, "r+b", Cached);
	IdxHandle = Vfopen(IdxName, "r+b", FALSE);
	Valid = (Handle && IdxHandle);

#ifndef NO_TABLE_WIN32_LOCKING
	// unlock
	if (Valid || !AutoCreate)
	{
		CloseHandle(hFile);
		DeleteFile(lockbuf);
	}
	else
#else
	if (!Valid && AutoCreate)
#endif
	{

		Handle = Vfopen(Name, "w+b", Cached);
		IdxHandle = Vfopen(IdxName, "w+b", FALSE);
		Valid = (Handle && IdxHandle);

#ifndef NO_TABLE_WIN32_LOCKING
		CloseHandle(hFile);
		DeleteFile(lockbuf);
#endif

		if (Valid)
		{
			Vfwrite(__TABLE_SIGNATURE__, strlen(__TABLE_SIGNATURE__), 1, Handle);
			Vfwrite(__INDEX_SIGNATURE__, strlen(__TABLE_SIGNATURE__), 1, IdxHandle);
			int v=2;//strlen(__TABLE_SIGNATURE__);
			Vfwrite(&v, sizeof(v), 1, IdxHandle);
			//    v = 0; fwrite(&v, sizeof(v), 1, IdxHandle);
			v = -1; Vfwrite(&v, sizeof(v), 1, IdxHandle);
			v = 0; Vfwrite(&v, sizeof(v), 1, IdxHandle);
			v = 0; Vfwrite(&v, sizeof(v), 1, IdxHandle);
			Vfwrite(&v, sizeof(v), 1, IdxHandle);
			Sync();
			justcreated = 1;
		}
	}


	if (!Valid)
	{
		if (Handle) Vfclose(Handle);
		if (IdxHandle) Vfclose(IdxHandle);
		Handle = NULL;
		IdxHandle = NULL;
	}
	else
	{
		int Ptr;
		int N=-1;

		char test1[9]={0,};
		char test2[9]={0,};

		Vfseek(Handle, 0, SEEK_SET);
		Vfread(test1, strlen(__TABLE_SIGNATURE__), 1, Handle);
		Vfseek(IdxHandle, 0, SEEK_SET);
		Vfread(test2, strlen(__INDEX_SIGNATURE__), 1, IdxHandle);
		test1[8]=0;
		test2[8]=0;
		if (strcmp(test1, __TABLE_SIGNATURE__) || strcmp(test2, __INDEX_SIGNATURE__))
		{
			if (Handle) Vfclose(Handle);
			if (IdxHandle) Vfclose(IdxHandle);
			return FALSE;
		}

		// Load default index
		IndexField *field;
		field = new IndexField(-1, -1, -1, "None");
		field->index = new Index(IdxHandle, -1, -1, -1, FALSE, 0, this);

		// Get indexes
		Ptr = field->index->Get(1);
		IndexList = new Record(Ptr, 1, 1, Handle, IdxHandle, this, NULL);
		if (!IndexList)
		{
			field->SetDeletable();
			delete field;
			if (Handle) Vfclose(Handle);
			if (IdxHandle) Vfclose(IdxHandle);
			return FALSE;
		}

		// Init them
		IndexField *p = (IndexField *)IndexList->Fields.GetHead();
		while (p)
		{
			p->index = new Index(IdxHandle, p->ID, N++, p->Type, FALSE, 0, this);
			p = (IndexField *)p->GetNext();
		}

		// Add default in case its not there (if it is it won't be added by addfield)
		IndexList->AddField(field);

		// Get the default index (whether loaded or preloaded)
		dScanner->index = ((IndexField*)IndexList->GetField(-1))->index;

		// If it's different from preloaded, delete preloaded
		if (field->index != dScanner->index)
		{
			field->SetDeletable();
			delete field;
			field=0;
		}

		// Set up colaboration
		p = (IndexField *)IndexList->Fields.GetHead();
		while (p)
		{
			if (p->GetNext())
				p->index->Colaborate((IndexField *)p->GetNext());
			else
				p->index->Colaborate((IndexField *)IndexList->Fields.GetHead());
			p = (IndexField *)p->GetNext();
		}

		// Get columns
		Ptr = dScanner->index->Get(0);
		FieldsRecord = new Record(Ptr, 0, 0, Handle, IdxHandle, this, NULL);
		if (!FieldsRecord)
		{
			delete IndexList;
			IndexList=0;
			if (Handle) Vfclose(Handle);
			if (IdxHandle) Vfclose(IdxHandle);
			return FALSE;
		}
	}

	if (Valid && !justcreated)
	{
		IndexField *p = (IndexField *)IndexList->Fields.GetHead();
		int needfix = 0;
		while (p)
		{
			needfix = p->index->NeedFix();
			if (needfix) break;
			p = (IndexField *)p->GetNext();
		}
		if (needfix) doRecovery();
	}
	if (Valid) First();
	return Valid;
}

//---------------------------------------------------------------------------
void Table::Close(void)
{
	int v=0;

	if (IndexList)
	{
		IndexField *f = (IndexField *)IndexList->Fields.GetHead();
		while (f)
		{
			f->index->WriteIndex();
			f = (IndexField *)f->GetNext();
		}
	}

	delete Scanners;
	Scanners = NULL;


	if (Handle)
	{
		Vfclose(Handle);
		Handle = NULL;
		v |= 1;
	}
	if (IdxHandle)
	{
		Vfclose(IdxHandle);
		IdxHandle = NULL;
		v |= 2;
	}
	if (v != 3)
		return;
	if (doCRC)
		WriteConsistency();
}

//---------------------------------------------------------------------------
int Table::GetRecordsCount(void)
{
	return dScanner->GetRecordsCount();
}

//---------------------------------------------------------------------------
void Table::First(void)
{
	dScanner->First();
}

//---------------------------------------------------------------------------
void Table::Next(void)
{
	dScanner->Next();
}

//---------------------------------------------------------------------------
void Table::Previous(void)
{
	dScanner->Previous();
}

//---------------------------------------------------------------------------
void Table::Last(void)
{
	dScanner->Last();
}

//---------------------------------------------------------------------------
void Table::GetRecordById(int Id)
{
	dScanner->GetRecordById(Id);
}

//---------------------------------------------------------------------------
BOOL Table::Eof(void)
{
	return dScanner->Eof();
}

//---------------------------------------------------------------------------
BOOL Table::Bof(void)
{
	return dScanner->Eof();
}

//---------------------------------------------------------------------------
void Table::Sync(void)
{
#ifndef NO_TABLE_WIN32_LOCKING
	if (!Handle) return;

	char lockbuf[4096];
	wsprintf(lockbuf,"%s.lock",Handle->filename);
	int retry_cnt=0;

	HANDLE hFile;

	do
	{
		hFile = CreateFile(lockbuf,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
		if (hFile == INVALID_HANDLE_VALUE) Sleep(100);
	}
	while (hFile == INVALID_HANDLE_VALUE && retry_cnt++ < 100); // try for 10 seconds

	if (hFile == INVALID_HANDLE_VALUE) return; // db already locked, fail gracefully
#endif

	if (IndexList)
	{
		IndexField *field = (IndexField *)IndexList->Fields.GetHead();
		while (field)
		{
			field->index->WriteIndex();
			field = (IndexField *)field->GetNext();
		}
	}
	int err=0;
	if (!err && Handle) err|=Vsync(Handle);
	if (!err && IdxHandle) err|=Vsync(IdxHandle);
	if (!err && FieldsRecord)
	{
		ColumnField *field = (ColumnField *)FieldsRecord->Fields.GetHead();
		while (field)
		{
			if (field->indexUnique && field->uniqueTable)
				field->uniqueTable->Sync();
			field = (ColumnField *)field->GetNext();
		}
	}

#ifndef NO_TABLE_WIN32_LOCKING
	CloseHandle(hFile);
	DeleteFile(lockbuf);
#endif
}

//---------------------------------------------------------------------------
ColumnField *Table::NewColumn(unsigned char FieldID, char *FieldName, unsigned char FieldType, BOOL indexUnique)
{
	ColumnField *f = GetColumnById(FieldID);
	if (f) {
		int t = f->GetDataType();
		if (t != FieldType) {
			OutputDebugString("column ");
			OutputDebugString(FieldName);
			OutputDebugString(" already exists but is of the wrong type\n");
			if (CompatibleFields(t, FieldType))
			{
				OutputDebugString("going from one equivalent type to another, converting column\n");
				f->SetDataType(FieldType);
				goto aok;
			}
		}
		return NULL;
	}
aok:
	if (GetColumnByName(FieldName))
		return NULL;
	ColumnField *field = new ColumnField(FieldID, FieldName, FieldType, indexUnique, this);
	FieldsRecord->AddField(field);
	HasNewHdr=TRUE;
	return field;
}

//---------------------------------------------------------------------------
void Table::SetUniqueByName(char *name, BOOL tf) {
	ColumnField *c = GetColumnByName(name);
	SetUniqueById(c->ID, tf);
}

//---------------------------------------------------------------------------
void Table::SetUniqueById(int ID, BOOL tf) {
	ColumnField *c = GetColumnById(ID);
	if (!tf) {
		if (c->GetUnique()) {
			c->SetUnique(FALSE);
			FieldsRecord->WriteFields();
		}
	} else {
		if (!c->GetUnique()) {
			c->SetUnique(TRUE);
			FieldsRecord->WriteFields();
		}
	}

}

//---------------------------------------------------------------------------
void Table::PostColumns(void)
{
	FieldsRecord->WriteFields();
	HasNewHdr=FALSE;
}

//---------------------------------------------------------------------------
ColumnField *Table::GetColumnByName(const char *FieldName)
{
	return dScanner->GetColumnByName(FieldName);
}

//---------------------------------------------------------------------------
ColumnField *Table::GetColumnById(unsigned char Idx)
{
	return dScanner->GetColumnById(Idx);
}

//---------------------------------------------------------------------------
Field *Table::GetFieldByName(const char *FieldName)
{
	return dScanner->GetFieldByName(FieldName);
}

//---------------------------------------------------------------------------
Field *Table::GetFieldById(unsigned char Id)
{
	return dScanner->GetFieldById(Id);
}

//---------------------------------------------------------------------------
Field *Table::NewFieldByName(const char *fieldName, unsigned char Perm)
{
	return dScanner->NewFieldByName(fieldName, Perm);
}

//---------------------------------------------------------------------------
Field *Table::NewFieldById(unsigned char Id, unsigned char Perm)
{
	return dScanner->NewFieldById(Id, Perm);
}

//---------------------------------------------------------------------------
void Table::Post(void)
{
	dScanner->Post();
}

//---------------------------------------------------------------------------
void Table::New(void)
{
	dScanner->New();
}

//---------------------------------------------------------------------------
void Table::Insert(void)
{
	dScanner->Insert();
}

//---------------------------------------------------------------------------
void Table::Delete(void)
{
	dScanner->Delete();
}

//---------------------------------------------------------------------------
BOOL Table::HasNewColumns(void)
{
	return HasNewHdr;
}

//---------------------------------------------------------------------------
IndexField *Table::GetIndexByName(char *name)
{
	if (!IndexList)
		return NULL;
	IndexField *p = (IndexField *)IndexList->Fields.GetHead();
	while (p)
	{
		if (!stricmp(p->GetIndexName(), name))
			return p;
		p = (IndexField *)p->GetNext();
	}
	return NULL;
}

//---------------------------------------------------------------------------
IndexField *Table::GetIndexById(unsigned char Id)
{
	if (!IndexList)
		return NULL;
	return (IndexField *)IndexList->GetField(Id);
}

//---------------------------------------------------------------------------
int Table::GetRecordId(void)
{
	return dScanner->GetRecordId();
}

//---------------------------------------------------------------------------
void Table::AddIndexByName(char *name, char *desc)
{
	unsigned char Idx;
	ColumnField *header = GetColumnByName(name);
	if (header)
	{
		Idx = header->ID;
		AddIndexById(Idx, desc);
	}
}

//---------------------------------------------------------------------------
void Table::AddIndexById(unsigned char Id, char *desc)
{
	if (GetIndexById(Id)) return;
	ColumnField *col = GetColumnById(Id);
	if (!col)
		return;
	IndexField *newindex = new IndexField(Id, IndexList->Fields.GetNElements()-1, col->GetDataType(), desc);
	newindex->index = new Index(IdxHandle, Id, IndexList->Fields.GetNElements()-1, col->GetDataType(), TRUE, dScanner->index->NEntries, this);
	IndexList->AddField(newindex);

	IndexField *previous = (IndexField *)newindex->Previous;
	previous->index->Colaborate(newindex);
	newindex->index->Colaborate(((IndexField *)IndexList->Fields.GetHead()));

	previous->index->Propagate();

	IndexList->WriteFields();
}

//---------------------------------------------------------------------------
BOOL Table::SetWorkingIndexById(unsigned char Id)
{
	return dScanner->SetWorkingIndexById(Id);
}

//---------------------------------------------------------------------------
BOOL Table::SetWorkingIndexByName(char *desc)
{
	return dScanner->SetWorkingIndexByName(desc);
}

//---------------------------------------------------------------------------
BOOL Table::CheckIndexing(void)
{
	IndexField *field;
	int i,v;
	if (IndexList->Fields.GetNElements() < 2) return TRUE;

	for (i=0;i<dScanner->index->NEntries;i++)
	{
		field = (IndexField*)IndexList->Fields.Head;
		v = i;
		do {
			v = field->index->GetCooperative(v);
			field = (IndexField *)field->Next;
		} while (field);
		if (v != i)
		{
			return FALSE;
		}
	}
	return TRUE;
}

//---------------------------------------------------------------------------
LinkedList *Table::GetIndexes(void)
{
	if (!IndexList) return NULL;
	return &IndexList->Fields;
}

//---------------------------------------------------------------------------
void Table::DropIndex(IndexField *Ptr)
{
	if (!Ptr || Ptr->Type != FIELD_INDEX) return;
	if (dScanner->index == Ptr->index)
	{
		dScanner->index = ((IndexField*)IndexList->GetField(-1))->index;

		IndexField *p = (IndexField *)IndexList->Fields.GetHead();
		while (p)
		{
			if (p->GetNext())
				p->index->Colaborate((IndexField *)p->GetNext());
			else
				p->index->Colaborate((IndexField *)IndexList->Fields.GetHead());
			p = (IndexField *)p->GetNext();
		}
	}
	IndexList->Fields.RemoveEntry(Ptr);
	if (dScanner->index->SecIndex == Ptr)
		dScanner->index->SecIndex = 0;
	IndexField *p = (IndexField *)IndexList->Fields.GetHead();
	while (p)
	{
		p->index->Modified = TRUE;
		p = (IndexField *)p->GetNext();
	}
	IndexList->WriteFields();
}

//---------------------------------------------------------------------------
void Table::DropIndexByName(char *desc)
{
	IndexField *indx = GetIndexByName(desc);
	if (!stricmp(desc, "None")) return;

	if (indx)
		DropIndex(indx);
}

//---------------------------------------------------------------------------
void Table::DropIndexById(unsigned char Id)
{
	if (!IndexList)
		return;
	if (Id == (unsigned char)-1) return;
	IndexField *indx=(IndexField *)IndexList->GetField(Id);
	if (indx)
		DropIndex(indx);
}

//---------------------------------------------------------------------------
int Table::AddFilterByName(wchar_t *name, Field *Data, unsigned char Op)
{
	return dScanner->AddFilterByName(name, Data, Op);
}

//---------------------------------------------------------------------------
int Table::AddFilterById(unsigned char Id, Field *Data, unsigned char Op)
{
	return dScanner->AddFilterById(Id, Data, Op);
}

//---------------------------------------------------------------------------
int Table::AddFilterOp(unsigned char Op)
{
	return dScanner->AddFilterOp(Op);
}

//---------------------------------------------------------------------------
void Table::RemoveFilters(void)
{
	dScanner->RemoveFilters();
}

//---------------------------------------------------------------------------
void Table::Edit(void)
{
	dScanner->Edit();
}

//---------------------------------------------------------------------------
BOOL Table::Editing(void)
{
	return dScanner->Editing();
}

//---------------------------------------------------------------------------
void Table::Cancel(void)
{
	dScanner->Cancel();
}

//---------------------------------------------------------------------------
BOOL Table::LocateByName(char *col, int From, Field *field)
{
	return dScanner->LocateByName(col, From, field);
}

//---------------------------------------------------------------------------
BOOL Table::LocateById(int Id, int From, Field *field)
{
	return dScanner->LocateById(Id, From, field);
}

//---------------------------------------------------------------------------
BOOL Table::LocateByIdEx(int Id, int From, Field *field, int comp_mode)
{
	return dScanner->LocateByIdEx(Id, From, field, NULL, comp_mode);
}
//---------------------------------------------------------------------------
LinkedList *Table::GetFields(void)
{
	return dScanner->GetFields();
}

//---------------------------------------------------------------------------
Record *Table::GetColumns(void)
{
	if (!FieldsRecord)
		return NULL;
	return FieldsRecord;
}


//---------------------------------------------------------------------------
float Table::FragmentationLevel(void)
{
	return dScanner->FragmentationLevel();
}

//---------------------------------------------------------------------------
void Table::DeleteFieldByName(const char *name)
{
	dScanner->DeleteFieldByName(name);
}

//---------------------------------------------------------------------------
void Table::DeleteFieldById(unsigned char Id)
{
	dScanner->DeleteFieldById(Id);
}

//---------------------------------------------------------------------------
void Table::DeleteField(Field *field)
{
	dScanner->DeleteField(field);
}

//---------------------------------------------------------------------------
void Table::Push(void)
{
	dScanner->Push();
}

//---------------------------------------------------------------------------
void Table::Pop(void)
{
	dScanner->Pop();
}

//---------------------------------------------------------------------------
Scanner *Table::NewScanner(BOOL orderIntegrity)
{
	return NewScanner(FALSE, orderIntegrity);
}

//---------------------------------------------------------------------------
Scanner *Table::NewScanner(BOOL canEdit, BOOL orderIntegrity)
{
	Scanner *s = new Scanner(this, canEdit, orderIntegrity);
	/*if (Scanners->GetNElements() > 0)*/
	if (dScanner) s->index = dScanner->index;
	Scanners->AddEntry(s, TRUE);
	return s;
}

//---------------------------------------------------------------------------
Scanner *Table::GetDefaultScanner(void)
{
	return dScanner;
}

//---------------------------------------------------------------------------
void Table::DeleteScanner(Scanner *scan)
{
	if (!scan) return;
	ColumnField *f = scan->getSubtableColumnField();
	if (f)
		f->DeleteScanner(scan);
	else
		Scanners->RemoveEntry(scan);
}

//---------------------------------------------------------------------------
void Table::IndexModified(void)
{
	Scanner *s = (Scanner *)Scanners->GetHead();
	while (s)
	{
		if (s != dScanner)
			s->IndexModified();
		s = (Scanner *)s->GetNext();
	}
}

//---------------------------------------------------------------------------
int Table::ConsistencyCheck(int in)
{
	char path_lock[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath( Name, drive, dir, fname, ext );
	_makepath( path_lock, drive, dir, fname, ".lck");

	if (access(path_lock, 0)) // file does not exist
	{
		SetConsistencyInfo(CONSISTENCY_DONTKNOW);
		return 1; // no consistency check available, assume ok
	}

	FILE *lock = fopen(path_lock, "rb");
	if (!lock)
	{
		SetConsistencyInfo(CONSISTENCY_DONTKNOW);
		return 1; // failed to open lock, assume ok
	}


	unsigned int lockcrctable, lockcrcidx;
	fread(&lockcrctable, 4, 1, lock);
	fread(&lockcrcidx, 4, 1, lock);
	fclose(lock);
	unsigned int thiscrctable = crc32file(Name);
	unsigned int thiscrcidx = crc32file(IdxName);

	if (lockcrctable != thiscrctable || lockcrcidx != thiscrcidx)
	{
		if (in)
		{
			SetConsistencyInfo(CONSISTENCY_BACKUPCORRUPTED);
			return 0; // corrupted
		}
		if (!RestoreBackup())
		{
			SetConsistencyInfo(CONSISTENCY_CORRUPTED);
			return 0; // corrupted
		}
		if (!ConsistencyCheck(1))
			return 0;
		else
			SetConsistencyInfo(CONSISTENCY_RESTORED);
	}
	else
	{
		SetConsistencyInfo(CONSISTENCY_OK); // Data is valid, let's backup it if we need
		//  #ifdef DBBACKUP
		MakeBackup();
		//  #endif
	}

	return 1; // data is valid
}

//---------------------------------------------------------------------------
void Table::MakeBackup()
{
	char backup_path_test[_MAX_PATH];
	char backup_path[_MAX_PATH];
	char path_lock[_MAX_PATH];
	char dest[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char bdir[_MAX_DIR];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath( Name, drive, bdir, fname, ext );
	if (bdir[strlen(bdir)-1] != '\\')
		strcat(bdir, "\\");
	strcat(bdir, "Backup\\");

	_makepath( backup_path, drive, bdir, NULL, NULL);
	strcpy(backup_path_test, backup_path);
	strcat(backup_path_test, "NUL");
	if (access(backup_path_test, 0))
#ifdef WIN32
		mkdir(backup_path);
#else
		mkdir(backup_path, 0755);
#endif

	_makepath( dest, drive, bdir, fname, ext);
	CopyFile(Name, dest, FALSE);

	_splitpath( IdxName, NULL, NULL, fname, ext );
	_makepath( dest, drive, bdir, fname, ext);
	CopyFile(IdxName, dest, FALSE);

	_splitpath( Name, drive, dir, fname, ext );
	_makepath( path_lock, drive, dir, fname, ".lck");
	_makepath( dest, drive, bdir, fname, ".lck");

	CopyFile(path_lock, dest, FALSE);
}

//---------------------------------------------------------------------------
int Table::RestoreBackup()
{
	char path_lock[_MAX_PATH];
	char source[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char bdir[_MAX_DIR];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	if (!Name || !*Name)
		return 0;

	_splitpath( Name, drive, bdir, fname, ext );
	if (bdir[strlen(bdir)-1] != '\\')
		strcat(bdir, "\\");
	strcat(bdir, "Backup\\");
	_makepath( source, drive, bdir, fname, ext);
	CopyFile(source, Name, FALSE);

	_splitpath( IdxName, NULL, NULL, fname, ext );
	_makepath( source, drive, bdir, fname, ext);
	CopyFile(source, IdxName, FALSE);

	_splitpath( Name, drive, dir, fname, ext );
	_makepath( path_lock, drive, dir, fname, ".lck");
	_makepath( source, drive, bdir, fname, ".lck");

	CopyFile(source, path_lock, FALSE);

	return 1;
}

//---------------------------------------------------------------------------
void Table::WriteConsistency(void)
{
	char path_lock[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	//FUCKO
	//return;

	_splitpath( Name, drive, dir, fname, ext );
	_makepath( path_lock, drive, dir, fname, ".lck");

	FILE *lock = fopen(path_lock, "rb");
	if (!lock)
	{ // Failed to open
		lock = fopen(path_lock, "w+b"); // try to create it
		if (!lock)
			return; // failed to open or create lock
	}

	unsigned int thiscrctable = crc32file(Name);
	unsigned int thiscrcidx = crc32file(IdxName);
	fwrite(&thiscrctable, 4, 1, lock);
	fwrite(&thiscrcidx, 4, 1, lock);
	fclose(lock);

	return;
}

//---------------------------------------------------------------------------
void Table::SetConsistencyInfo(int s)
{
	consistencyInfo = s;
}

//---------------------------------------------------------------------------
int Table::GetConsistencyInfo(void)
{
	return consistencyInfo;
}

//---------------------------------------------------------------------------
void Table::SetGlobalLocateUpToDate(BOOL is) {
	GLocateUpToDate = is;
}

#ifdef _WIN32
bool Table::Compact_ColumnWalk(LinkedListEntry *Entry, int id, void *data1, void *data2)
{
	ColumnField *field = static_cast<ColumnField *>(Entry);
	Table *ctable = (Table *)data1;
	LinkedList *sublist = (LinkedList *)data2;

	ColumnField *ccol = ctable->NewColumn(field->GetFieldId(), field->GetFieldName(), field->GetDataType(), field->GetUnique());
	if (field->GetUnique()) {
		Scanner *subscan = ccol->GetUniqueDataScanner();
		if (subscan != NULL) {
			Table *subtable = subscan->GetTable();
			if (subtable != NULL) {
				StringField *f = new StringField();
				f->SetString(subtable->Name);
				sublist->AddEntry(f, 1);
				f = new StringField();
				f->SetString(subtable->IdxName);
				sublist->AddEntry(f, 1);
			}
		}
	}
	return true;
}

bool Table::Compact_IndexWalk(LinkedListEntry *Entry, int id, void *data1, void *data2)
{
	IndexField *field = static_cast<IndexField *>(Entry);
	Table *ctable = (Table *)data1;

	if (stricmp(field->GetIndexName(), "None"))
		ctable->AddIndexById(field->GetFieldId(), field->GetIndexName());
	return true;
}
//---------------------------------------------------------------------------
void Table::Compact(int *progress) {
	// ok so we're gonna be cheating a bit, instead of figuring out how to safely modify all those
	// nifty indexes that cross reference themselves and blablabla, we're just gonna duplicate the
	// whole table from scratch, overwrite ourselves, and reopen the table. duh.

	// crate a temporary table in windows temp dir
	char temp_table[MAX_PATH+12];
	char temp_index[MAX_PATH+12];
	char old_table[MAX_PATH+12];
	char old_index[MAX_PATH+12];
	DWORD pid=GetCurrentProcessId();

	sprintf(temp_table, "%s.new%08X", Name,pid);
	sprintf(temp_index, "%s.new%08X", IdxName,pid);
	sprintf(old_table, "%s.old%08X", Name,pid);
	sprintf(old_index, "%s.old%08X", IdxName,pid);

	// delete them, in case we crashed while packing

	DeleteFile(temp_table);
	DeleteFile(temp_index);
	DeleteFile(old_table);
	DeleteFile(old_index);

	// create a brand new db and a brand new table
	Table *ctable = db->OpenTable(temp_table, temp_index, NDE_OPEN_ALWAYS, Cached);

	// make a list to keep track of subtables
	LinkedList sublist;

	// duplicate the columns
	Record *record = GetColumns();
	LinkedList *collist = NULL;
	if (record != NULL) 
	{
		collist = record->GetFields();
		if (collist != NULL) 
			collist->WalkList(Compact_ColumnWalk, 0, (void *)ctable, (void *)&sublist);
	}
	ctable->PostColumns();

	// duplicate the indexes
	LinkedList *indlist = GetIndexes();
	if (indlist != NULL) 
		indlist->WalkList(Compact_IndexWalk, 0, (void *)ctable, 0);

	// duplicate the data
	int reccount = GetRecordsCount();
	int ndrop = 0;

	int count = 0;
	First();
	size_t data_size = 65536;
	unsigned char *data = (unsigned char *)malloc(65536);

	while (1) {
		int lasterr = NumErrors();
		GetDefaultScanner()->GetRecordById(count, FALSE);
		count++;

		if (Eof() || count > reccount) break;

		if (NumErrors() > lasterr) 
			continue;

		Index *idx = GetDefaultScanner()->GetIndex();
		int pos = idx->Get(GetDefaultScanner()->GetRecordId());

		if (pos == 0) ndrop++;
		SetDlgInfo(reccount, GetRecordId(), ndrop);
		if (pos == 0) continue;

		int pr = (int)((float)GetRecordId()/(float)reccount*100.0f);
		if (progress != NULL) *progress = pr;
		int gotstuff = 0;

		if (collist != NULL) {
			ColumnField *colfield = static_cast<ColumnField *>(collist->GetHead());
			while (colfield != NULL) {
				unsigned char fieldid = colfield->GetFieldId();
				//char *fieldname = colfield->GetFieldName();
				Field *mfield = this->GetFieldById(fieldid);
				//Field *mfield = GetFieldByName(fieldname);
				if (mfield != NULL) {
					if (!gotstuff) {
						ctable->New();
						gotstuff = 1;
					}
					Field *cfield = ctable->NewFieldById(fieldid, mfield->GetPerm());
					//Field *cfield = ctable->NewFieldByName(fieldname, mfield->GetPerm());
					size_t len = mfield->GetDataSize();
					if (len > data_size)
					{
						data_size = len;
						data = (unsigned char *)realloc(data, data_size);
					}
					mfield->WriteTypedData(data, len);
					cfield->ReadTypedData(data, len);
				}
				colfield = static_cast<ColumnField *>(colfield->GetNext());
			}
		}
		if (gotstuff) ctable->Post(); else ndrop++;
	}
	free(data);

	SetDlgInfo(reccount, GetRecordId(), ndrop);

	// done creating temp table
	db->CloseTable(ctable);

	// close this table
	Close();
	Reset();

	if (MoveFile(Name,old_table))
	{
		if (MoveFile(IdxName,old_index))
		{
			if (!MoveFile(temp_table,Name) || !MoveFile(temp_index,IdxName))
			{
				// failed, try to copy back
				DeleteFile(Name);
				DeleteFile(IdxName);
				MoveFile(old_table,Name); // restore old file
				MoveFile(old_index,IdxName); // restore old file
			}
		}
		else
		{
			MoveFile(old_table,Name); // restore old file
		}
	}

	// clean up our temp files
	DeleteFile(temp_table);
	DeleteFile(temp_index);
	DeleteFile(old_table);
	DeleteFile(old_index);

	while (sublist.GetNElements() > 0) {
		StringField *subfile = static_cast<StringField *>(sublist.GetHead());
		DeleteFile(subfile->GetString());
		sublist.RemoveEntry(subfile);
	}

	if (progress != NULL) *progress = 100;

	// reopen our table
	Init();
	Open();
}
#endif

#ifdef WIN32_NOLIB
HWND recoveryDlg = (HWND)0;

BOOL CALLBACK RecoveryDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG: {
			EnableWindow(GetDlgItem(hwndDlg, IDOK), 0);
			SendMessage(GetDlgItem(recoveryDlg, IDC_PROGRESS_PERCENT), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendMessage(GetDlgItem(recoveryDlg, IDC_PROGRESS_PERCENT), PBM_SETPOS, 0, 0);
			return 1;
												}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
		case IDOK:
			recoveryDlg = (HWND)0;
			EndDialog(hwndDlg, IDOK);
			return 0;
			}
			break;
	}
	return NULL;
}
#endif

void Table::SetDlgInfo(int nrecords, int currecord, int lost) {
#ifdef WIN32_NOLIB
	if (recoveryDlg == NULL) return;
	char str[64];
	wsprintf(str, "%d", nrecords);
	SetDlgItemText(recoveryDlg, IDC_STATIC_TOTAL, str);
	wsprintf(str, "%d", currecord);
	SetDlgItemText(recoveryDlg, IDC_STATIC_RECOVERED, str);
	wsprintf(str, "%d", lost);
	SetDlgItemText(recoveryDlg, IDC_STATIC_LOST, str);
	SendMessage(GetDlgItem(recoveryDlg, IDC_PROGRESS_PERCENT), PBM_SETPOS, (int)((double)currecord/nrecords*100.0), 0);
	wsprintf(str, "%d%%", (int)((double)currecord/nrecords*100.0));
	SetDlgItemText(recoveryDlg, IDC_STATIC_PERCENT, str);
	if (currecord % 100 == 0) UpdateWindow(recoveryDlg);
#endif
}

void Table::doRecovery() {
#ifdef WIN32_NOLIB
	recoveryDlg = CreateDialog(db->GetInstance(), MAKEINTRESOURCE(IDD_NDE_RECOVERY), NULL, RecoveryDlgProc);
	ShowWindow(recoveryDlg, SW_NORMAL);
	SetWindowPos(recoveryDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow(recoveryDlg);
	UpdateWindow(recoveryDlg);
#endif
	Compact();
#ifdef WIN32_NOLIB
	EnableWindow(GetDlgItem(recoveryDlg, IDOK), 1);
#endif
}
