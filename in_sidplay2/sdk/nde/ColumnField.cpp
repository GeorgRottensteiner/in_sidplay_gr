/* ---------------------------------------------------------------------------
Nullsoft Database Engine
--------------------
codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

ColumnField Class

--------------------------------------------------------------------------- */

#include "nde.h"

//---------------------------------------------------------------------------
ColumnField::ColumnField(unsigned char FieldID, char *FieldName, unsigned char FieldType, BOOL _indexUnique, Table *parentTable)
{
	InitField();
	Type = FIELD_COLUMN;
	MyType = FieldType;
	Name = strdup(FieldName);
	ID = FieldID;
	Kind = DELETABLE;
	pTable = parentTable;
	indexUnique = _indexUnique;
	dScanner = NULL;
	uniqueTable = NULL;
	if (indexUnique && MyType)
		if (!OpenSubTable())
		{
			if (!RebuildSubtable())
				indexUnique = FALSE;
		}
}

//---------------------------------------------------------------------------
void ColumnField::InitField(void)
{
	MyType = FIELD_UNKNOWN;
	Type = FIELD_COLUMN;
	Name = NULL;
	indexUnique = FALSE;
	uniqueTable = NULL;
	ID = 0;
	Kind = DELETABLE;
	pTable = NULL;
}

//---------------------------------------------------------------------------
ColumnField::ColumnField()
{
	InitField();
}


//---------------------------------------------------------------------------
ColumnField::~ColumnField()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
	if (Name) free(Name);
	if (uniqueTable)
		pTable->db->CloseTable(uniqueTable);
}

//---------------------------------------------------------------------------
void ColumnField::ReadTypedData(unsigned char *data, size_t len)
{
	unsigned char c;
	int pos=0;

	CHECK_CHAR(len);
	MyType = GET_CHAR(); pos++;

	CHECK_CHAR(len);
	indexUnique = (BOOL)GET_CHAR(); pos++;

	CHECK_CHAR(len);
	c = GET_CHAR(); pos++;
	if (c)
	{
		CHECK_BIN(len, c);
		Name = (char*)malloc(c+1);
		GET_BINARY((unsigned char *)Name, data, c, pos);
		Name[c]=0;
	}
	if (indexUnique)
		if (!OpenSubTable())
		{
			if (!RebuildSubtable())
				indexUnique = FALSE;
		}
}

//---------------------------------------------------------------------------
void ColumnField::WriteTypedData(unsigned char *data, size_t len)
{
	unsigned char c;
	int pos = 0;

	CHECK_CHAR(len);
	PUT_CHAR(MyType); pos++;

	CHECK_CHAR(len);
	PUT_CHAR((char)indexUnique); pos++;

	if (Name) c = (char)strlen(Name); else c = 0;

	CHECK_CHAR(len);
	PUT_CHAR(c); pos++;

	if (Name) 
	{
		CHECK_BIN(len, c);
		PUT_BINARY(data, (unsigned char*)Name, c, pos);
	}
}

//---------------------------------------------------------------------------
unsigned char ColumnField::GetDataType(void)
{
	return MyType;
}

//---------------------------------------------------------------------------
void ColumnField::SetDataType(unsigned char type) {
	if ((MyType == FIELD_INTEGER || MyType == FIELD_BOOLEAN || MyType == FIELD_DATETIME || MyType == FIELD_LENGTH) && 
		(type == FIELD_INTEGER || type == FIELD_BOOLEAN || type == FIELD_DATETIME || type == FIELD_LENGTH)) {
			MyType = type;
		}
		// going from string to filename or filename to string is OK
		if ((MyType == FIELD_FILENAME && type == FIELD_STRING)
			|| (MyType == FIELD_STRING && type == FIELD_FILENAME))
		{
			MyType = type;
		}
}

//---------------------------------------------------------------------------
char *ColumnField::GetFieldName(void)
{
	return Name;
}

//---------------------------------------------------------------------------
size_t ColumnField::GetDataSize(void)
{
	size_t s=3;
	if (Name)
		s+=strlen(Name);
	return s;
}

//---------------------------------------------------------------------------
int ColumnField::Compare(Field * /*Entry*/)
{
	return 0;
}

//---------------------------------------------------------------------------
BOOL ColumnField::OpenSubTable(void)
{
	char path_table[_MAX_PATH];
	char path_idx[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char t[64];

	_splitpath( pTable->Name, drive, dir, fname, ext );
	sprintf(t, "_u%d", ID);
	strcat(fname, t);
	_makepath( path_table, drive, dir, fname, ext);
	_splitpath( pTable->IdxName, drive, dir, fname, ext );
	sprintf(t, "_u%d", ID);
	strcat(fname, t);
	_makepath( path_idx, drive, dir, fname, ext);
	uniqueTable = pTable->db->OpenTable(path_table, path_idx, FALSE, pTable->Cached);
	if (!uniqueTable)
		return FALSE;

	if (!uniqueTable->GetColumnById(1))
		uniqueTable->NewColumn(1, "Value", MyType, FALSE);
	if (!uniqueTable->GetColumnById(2))
		uniqueTable->NewColumn(2, "Count", FIELD_INTEGER, FALSE);
	if (uniqueTable->HasNewColumns())
		uniqueTable->PostColumns();
	dScanner = uniqueTable->NewScanner(FALSE);
	dScanner->setSubtableColumnField(this);
	if (!dScanner->SetWorkingIndexById(1))
	{
		uniqueTable->AddIndexById(1, "Primary Index");
		if (dScanner) dScanner->SetWorkingIndexById(1);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
Scanner *ColumnField::NewScanner(BOOL notifyUpdates) {
	if (!uniqueTable) return NULL;
	return uniqueTable->NewScanner(notifyUpdates); // no need to keep a list, uniqueTable already does it
}

//---------------------------------------------------------------------------
void ColumnField::DeleteScanner(Scanner *s) {
	if (!s || !uniqueTable) return;
	uniqueTable->DeleteScanner(s);
}

//---------------------------------------------------------------------------
Scanner *ColumnField::GetUniqueDataScanner(void)
{
	return dScanner;
}

//---------------------------------------------------------------------------
BOOL ColumnField::RebuildSubtable(void)
{
	char path_table[_MAX_PATH];
	char path_idx[_MAX_PATH];
	char path_lock[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char t[64];

	_splitpath( pTable->Name, drive, dir, fname, ext );
	sprintf(t, "_u%d", ID);
	strcat(fname, t);
	_makepath( path_table, drive, dir, fname, ext);
	_makepath( path_lock, drive, dir, fname, ".lck");
	_splitpath( pTable->IdxName, drive, dir, fname, ext );
	sprintf(t, "_u%d", ID);
	strcat(fname, t);
	_makepath( path_idx, drive, dir, fname, ext);

	if (!access(path_table, 0))
		unlink(path_table);
	if (!access(path_idx, 0))
		unlink(path_idx);
	if (!access(path_lock, 0))
		unlink(path_lock);

	uniqueTable = pTable->db->OpenTable(path_table, path_idx, TRUE, pTable->Cached);
	if (!uniqueTable)
	{
		MessageBox(NULL, "Could not open nor rebuild subtable", "Oops", 16);
		return FALSE;
	}

	if (!uniqueTable->GetColumnById(1))
		uniqueTable->NewColumn(1, "Value", MyType, FALSE);
	if (!uniqueTable->GetColumnById(2))
		uniqueTable->NewColumn(2, "Count", FIELD_INTEGER, FALSE);
	if (uniqueTable->HasNewColumns())
		uniqueTable->PostColumns();
	dScanner = uniqueTable->NewScanner(FALSE);
	if (!dScanner->SetWorkingIndexById(1))
	{
		uniqueTable->AddIndexById(1, "Primary Index");
		if (dScanner) dScanner->SetWorkingIndexById(1);
	}

	//EnterCriticalSection(&Critical);

	uniqueTable->First();
	pTable->dScanner->Push();
	pTable->dScanner->First();

	IntegerField *i;
	Field *lf, *gf;
	BOOL found;
	int a=0;

	size_t data_size = 65536;
	unsigned char *data = (unsigned char *)malloc(data_size);
	

	while (!pTable->dScanner->Eof())
	{
		lf = (StringField *)pTable->GetFieldById(ID);
		if (a++ % 32) Sleep(1);
		if (lf)
		{
			found = uniqueTable->LocateById(1, FIRST_RECORD, lf);
			if (found)
			{
				uniqueTable->Edit();
				i = (IntegerField *)uniqueTable->GetFieldById(2);
				i->SetValue(i->GetValue()+1);
				uniqueTable->Post();
			}
			else
			{
				uniqueTable->New();
				uniqueTable->NewFieldById(1, PERM_READ);
				gf = uniqueTable->GetFieldById(1);
				size_t size = lf->GetDataSize();
				if (size > data_size)
				{
					data = (unsigned char *)realloc(data, size);
					data_size = size;
				}
				lf->WriteTypedData(data, size);
				gf->ReadTypedData(data, size);
				uniqueTable->NewFieldById(2, PERM_READWRITE);
				i = (IntegerField *)uniqueTable->GetFieldById(2);
				i->SetValue(1);
				uniqueTable->Post();
			}
		}
		pTable->dScanner->Next();
	}
	free(data);

	pTable->dScanner->Pop();
	//LeaveCriticalSection(&Critical);
	uniqueTable->SetConsistencyInfo(CONSISTENCY_REBUILT);
	return TRUE;
}

void ColumnField::SetUnique(BOOL tf) {
	if (tf) {
		indexUnique = tf;
		if (indexUnique && MyType)
			if (!OpenSubTable())
			{
				if (!RebuildSubtable())
					indexUnique = FALSE;
			}
	} else 
		RemoveSubtable();
}

BOOL ColumnField::GetUnique(void) {
	return indexUnique;  
}

void ColumnField::RemoveSubtable() {
	char path_table[_MAX_PATH];
	char path_idx[_MAX_PATH];
	char path_lock[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char t[64];

	_splitpath( pTable->Name, drive, dir, fname, ext );
	sprintf(t, "_u%d", ID);
	strcat(fname, t);
	_makepath( path_table, drive, dir, fname, ext);
	_makepath( path_lock, drive, dir, fname, ".lck");
	_splitpath( pTable->IdxName, drive, dir, fname, ext );
	sprintf(t, "_u%d", ID);
	strcat(fname, t);
	_makepath( path_idx, drive, dir, fname, ext);

	if (!access(path_table, 0))
		unlink(path_table);
	if (!access(path_idx, 0))
		unlink(path_idx);
	if (!access(path_lock, 0))
		unlink(path_lock);
}

