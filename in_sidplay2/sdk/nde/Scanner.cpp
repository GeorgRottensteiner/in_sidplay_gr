/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Scanner Class

--------------------------------------------------------------------------- */

#include "NDE.h"
#include "../nu/AutoChar.h"
#include "../nu/AutoWide.h"
#include "BinaryField.h"
#include "Binary32Field.h"
#include <assert.h>
//---------------------------------------------------------------------------
Scanner::Scanner(Table *parentTable, BOOL CanEdit, BOOL _orderIntegrity)
{
	disable_date_resolution=0;
	orderIntegrity = _orderIntegrity;
	index = NULL;
	canEdit = CanEdit;
	pTable = parentTable;
	Edition=FALSE;

	lastLocateIndex = NULL;
	lastLocateId = -1;
	lastLocateFrom = -128;
	lastLocateFieldClone = NULL;
	CurrentRecord=NULL;
	CurrentRecordIdx=0;
	iModified = FALSE;
	FiltersOK = FALSE;
	subtablecolumn = NULL;

	/*inMatchJoins = 0;
	lastJoinCache = 0;
	invalidJoinCache = 1;*/
	last_query = NULL;
	last_query_failed = FALSE;
	allow_blocking = 1;
	token = NULL;
	in_query_parser = 0;
}

//---------------------------------------------------------------------------
Scanner::~Scanner()
{

	delete CurrentRecord;

	if (lastLocateFieldClone)
	{
		lastLocateFieldClone->SetDeletable();
		delete lastLocateFieldClone;
	}

	Query_CleanUp();

	if (token) free(token);
	if (last_query) free(last_query);
}

//---------------------------------------------------------------------------
Record *Scanner::GetRecord(int Idx)
{
	int Ptr;
	Ptr = index->Get(Idx);
	return new Record(Ptr, Idx, Idx, pTable->Handle, pTable->IdxHandle, pTable, this);
}

//---------------------------------------------------------------------------
void Scanner::GetCurrentRecord(void)
{
	if (orderIntegrity && iModified)
		throwException(EXCEPTION_COMPROMIZED_INDEX_ORDER);

	delete CurrentRecord;
	CurrentRecord = NULL;

//invalidJoinCache = 1;
	if (Eof() || Bof()) return;
	CurrentRecord = GetRecord(CurrentRecordIdx);
}

//---------------------------------------------------------------------------
void Scanner::GetRecordById(int Id, BOOL checkFilters)
{
	CurrentRecordIdx=max(min(index->NEntries, Id+2), 0);
	GetCurrentRecord();
	if (!checkFilters || MatchFilters())
		return;
	Next();
}

//---------------------------------------------------------------------------
void Scanner::First(int *killswitch)
{
	if (last_query_failed) return;
	GetRecordById(0);
	if (!MatchFilters() && !Eof())
		Next(1,killswitch);
}

//---------------------------------------------------------------------------
int Scanner::Next(int force_block, int *killswitch)
{
	if (last_query_failed) return 0;

	while (!Eof() && !Bof())
	{
		CurrentRecordIdx++;
		GetCurrentRecord();
		if (MatchFilters())
			break;
		else
		{
			if ((killswitch && *killswitch) || (!allow_blocking && !force_block))
				return 0;
		}
	}
	return 1;
}

//---------------------------------------------------------------------------
int Scanner::Previous(int force_block, int *killswitch)
{
	if (last_query_failed) return 0;

	while (CurrentRecordIdx >= 2)
	{
		CurrentRecordIdx--;
		GetCurrentRecord();
		if (MatchFilters())
			break;
		else
		{
			if ((killswitch && *killswitch) || (!allow_blocking && !force_block))
				return 0;
		}
	}
	return 1;
}

//---------------------------------------------------------------------------
void Scanner::Last(int *killswitch)
{
	if (last_query_failed) return;
	GetRecordById(index->NEntries-3); // -3 here because 1)GetRecordById is public, so -2, and 2)last entry is nentries-1, so -1
	if (!MatchFilters() && !Bof())
		Previous(1,killswitch);
	if (CurrentRecordIdx < 2)
	{
		CurrentRecordIdx = index->NEntries;
		GetCurrentRecord(); // will only delete current record if it exists
	}
}

//---------------------------------------------------------------------------
BOOL Scanner::Eof(void)
{
	if (last_query_failed) return TRUE;
	return CurrentRecordIdx >= index->NEntries;
}

//---------------------------------------------------------------------------
BOOL Scanner::Bof(void)
{
	if (last_query_failed) return TRUE;
	return CurrentRecordIdx < 2;
}

//---------------------------------------------------------------------------
Field *Scanner::GetFieldByName(const char *FieldName)
{
	unsigned char Idx;
	ColumnField *header = pTable->GetColumnByName(FieldName);
	if (header)
	{
		Idx = header->ID;
		return GetFieldById(Idx);
	}
	return NULL;
}

//---------------------------------------------------------------------------
Field *Scanner::GetFieldById(unsigned char Id)
{
	if (!CurrentRecord)
		return NULL;
	Field *field = CurrentRecord->GetField(Id);
	int column_type = pTable->GetColumnById(Id)->GetDataType();
	if (field && !CompatibleFields(field->GetType(), column_type))
	{
		return NULL;
	}
	return field;
}

//---------------------------------------------------------------------------
Field *Scanner::NewFieldByName(const char *fieldName, unsigned char Perm)
{
	unsigned char Id;
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	ColumnField *header = pTable->GetColumnByName(fieldName);
	if (header)
	{
		Id = header->ID;
		Field *field = NewFieldById(Id, Perm);
		return field;
	}
	return NULL;
}

//---------------------------------------------------------------------------
Field *Scanner::NewFieldById(unsigned char Id, unsigned char Perm)
{
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	ColumnField *field = (ColumnField *)pTable->FieldsRecord->GetField(Id);
	if (!field)
		return NULL;
	Field *O=GetFieldById(Id);
	if (O) return O;
	switch (field->MyType)
	{
	case FIELD_STRING:
		O = new StringField();
		break;
	case FIELD_INTEGER:
		O = new IntegerField();
		break;
	case FIELD_INT64:
		O = new Int64Field();
		break;
	case FIELD_INT128:
		O = new Int128Field();
		break;
	case FIELD_DATETIME:
		if (disable_date_resolution)
			O= new StringField();
		else
			O = new DateTimeField();
		break;
	case FIELD_LENGTH:
		O = new LengthField();
		break;
	case FIELD_FILENAME:
		O = new FilenameField();
		break;
	case FIELD_BINARY:
		O = new BinaryField();
		break;
	case FIELD_BINARY32:
		O = new Binary32Field();
		break;
	default:
#ifdef WIN32
		MessageBox(NULL, "unknown field type for id", "debug", 0);
#else
		printf("NDE Error: unknown field type for id\n");
#endif
		O = new Field();
		break;
	}
	O->Type = field->MyType;
	O->ID = Id;
	CurrentRecord->AddField(O);
	return O;
}

//---------------------------------------------------------------------------
void Scanner::Post(void)
{
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	if (!Edition) throwException(EXCEPTION_NOT_IN_EDIT_MODE);
	if (!CurrentRecord) return;
	/*if (CurrentRecord->RecordIndex == NEW_RECORD)
	  NEntries++;*/
	CurrentRecord->WriteFields();
	CurrentRecordIdx = CurrentRecord->RecordIndex;
	Edition=FALSE;
}

//---------------------------------------------------------------------------
void Scanner::New(void)
{
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);

	delete CurrentRecord;
	CurrentRecord = NULL;

	CurrentRecord = new Record(0, NEW_RECORD, index->NEntries, pTable->Handle, pTable->IdxHandle, pTable, this);
	CurrentRecordIdx = NEW_RECORD;
	Edition = TRUE;
}

//---------------------------------------------------------------------------
void Scanner::Insert(void)
{
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);

	delete CurrentRecord;
	CurrentRecord = NULL;

	CurrentRecord = new Record(0, NEW_RECORD, CurrentRecordIdx, pTable->Handle, pTable->IdxHandle, pTable, this);
	CurrentRecordIdx = NEW_RECORD;
	Edition=TRUE;
}

//---------------------------------------------------------------------------
void Scanner::Delete(void)
{
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	if (CurrentRecord)
	{
		CurrentRecord->Delete();
	}
	if (Eof())
		Previous();
	GetRecordById(CurrentRecordIdx-2);
}

//---------------------------------------------------------------------------
int Scanner::GetRecordId(void)
{
	return CurrentRecordIdx != NEW_RECORD ? CurrentRecordIdx-2 : CurrentRecordIdx;
}

//---------------------------------------------------------------------------
void Scanner::Edit(void)
{
	if (Edition) return;
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	if (!CurrentRecord)
		return;
	/*Field *f = (Field *)CurrentRecord->Fields->GetHead();
	while (f)
	  {
	  f->SubtableRecord = INVALID_RECORD;
	  f = (Field *)f->GetNext();
	  }*/
	Edition = TRUE;
}

//---------------------------------------------------------------------------
BOOL Scanner::Editing(void)
{
	return Edition;
}

//---------------------------------------------------------------------------
void Scanner::Cancel(void)
{
	Edition = FALSE;
	GetCurrentRecord();
}

//---------------------------------------------------------------------------
BOOL Scanner::LocateByName(char *col, int From, Field *field, int *nskip)
{
	ColumnField *f = pTable->GetColumnByName(col);
	if (!f)
		return NULL;
	return LocateById(f->GetFieldId(), From, field, nskip);
}

//---------------------------------------------------------------------------
void Scanner::CacheLastLocate(int Id, int From, Field *field, Index *i, int j)
{
	lastLocateId = Id;
	lastLocateFrom = From;
	if (lastLocateFieldClone)
	{
		lastLocateFieldClone->SetDeletable();
		delete lastLocateFieldClone;
		lastLocateFieldClone = NULL;
	}
	lastLocateFieldClone = field->Clone();
	lastLocateIndex = i;
	i->locateUpToDate = TRUE;
	pTable->SetGlobalLocateUpToDate(TRUE);
	lastLocateIdx = j;
}


//---------------------------------------------------------------------------
BOOL Scanner::LocateById(int Id, int From, Field *field, int *nskip)
{
	return LocateByIdEx(Id, From, field, nskip, COMPARE_MODE_EXACT);
}
//---------------------------------------------------------------------------
BOOL Scanner::LocateByIdEx(int Id, int From, Field *field, int *nskip, int comp_mode)
{
	IndexField *i = pTable->GetIndexById(Id);
	Field *compField;
	int j;
	int n;
	Field *cfV;

	if (index->NEntries == 2)
		return FALSE;

	int success;

	if (nskip) *nskip=0;
// I know this is stupid but.... May be do something later
	switch (comp_mode)
	{
	case COMPARE_MODE_CONTAINS:
		while (1)
		{
			success = FALSE;
			if (!i)
			{
				// No index for this column. Using slow locate, enumerates the database, still faster than what the user
				// can do since we have access to QuickFindField which only read field headers
				// in order to locate the field we have to compare. user could only read the entire record.
				if (From == FIRST_RECORD)
					From = 2;
				else
					From+=3;
				if (From == lastLocateFrom && Id == lastLocateId && field->Contains(lastLocateFieldClone)==0 && index == lastLocateIndex && (index->locateUpToDate || pTable->GLocateUpToDate))
				{
					if (CurrentRecordIdx != lastLocateIdx) GetRecordById(lastLocateIdx-2, FALSE);
					success = TRUE;
					goto nextiter_1;
				}
				for (j=From;j<index->NEntries;j++)
				{
					compField = index->QuickFindField(Id, index->Get(j));
					cfV = /*(compField && compField->Type == FIELD_PRIVATE) ? ((PrivateField *)compField)->myField :*/ compField;
					if (!field->Contains(cfV))
					{
						if (compField)
						{
							compField->SetDeletable();
							delete compField;
						}
						if (CurrentRecordIdx != j) GetRecordById(j-2, FALSE);
						CacheLastLocate(Id, From, field, index, j);
						success = TRUE;
						goto nextiter_1;
					}
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
				}
				success = FALSE;
				goto nextiter_1;
			}
			else
			{
				// Index available. Using fast locate. nfetched=log2(nrecords) for first locate, 1 more fetch per locate on same criteria
				int p;
				if (From == FIRST_RECORD) From = 2;
				else From = index->TranslateIndex(From+2, i->index)+1;
				if (From == lastLocateFrom && Id == lastLocateId && field->Contains(lastLocateFieldClone)==0 && index == lastLocateIndex && (i->index->locateUpToDate || pTable->GLocateUpToDate))
				{
					if (CurrentRecordIdx != lastLocateIdx) GetRecordById(lastLocateIdx-2, FALSE);
					success = TRUE;
					goto nextiter_1;
				}
				if (From >= index->NEntries)
				{
					return FALSE;
				}
				compField = i->index->QuickFindField(Id, i->index->Get(From));
				cfV = /*(compField && compField->Type == FIELD_PRIVATE) ? ((PrivateField *)compField)->myField :*/ compField;
				if (field->Contains(cfV) == 0)
				{
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
					n = i->index->TranslateIndex(From, index);
					if (CurrentRecordIdx != n) GetRecordById(n-2, FALSE);
					CacheLastLocate(Id, From, field, i->index, n);
					success = TRUE;
					goto nextiter_1;
				}
				if (compField)
				{
					compField->SetDeletable();
					delete compField;
				}
				p = i->index->QuickFindEx(Id, field, From, comp_mode);
				if (p != FIELD_NOT_FOUND)
				{
					n = (index->Id == Id) ? p : i->index->TranslateIndex(p, index);
					if (CurrentRecordIdx != n) GetRecordById(n-2, FALSE);
					CacheLastLocate(Id, From, field, i->index, n);
					success = TRUE;
					goto nextiter_1;
				}
			}
nextiter_1: // eek
			if (success)
			{
				if (!MatchFilters() && !Eof())
				{
					From = GetRecordId();
					if (nskip) *nskip++;
				}
				else break;
			}
			else break;
		}

		break;
	case COMPARE_MODE_EXACT:
		while (1)
		{
			success = FALSE;
			if (!i)
			{
				// No index for this column. Using slow locate, enumerates the database, still faster than what the user
				// can do since we have access to QuickFindField which only read field headers
				// in order to locate the field we have to compare. user could only read the entire record.
				if (From == FIRST_RECORD)
					From = 2;
				else
					From+=3;
				if (From == lastLocateFrom && Id == lastLocateId && field->Compare(lastLocateFieldClone)==0 && index == lastLocateIndex && (index->locateUpToDate || pTable->GLocateUpToDate))
				{
					if (CurrentRecordIdx != lastLocateIdx) GetRecordById(lastLocateIdx-2, FALSE);
					success = TRUE;
					goto nextiter_2;
				}
				for (j=From;j<index->NEntries;j++)
				{
					compField = index->QuickFindField(Id, index->Get(j));
					cfV = /*(compField && compField->Type == FIELD_PRIVATE) ? ((PrivateField *)compField)->myField :*/ compField;
					if (!field->Compare(cfV))
					{
						if (compField)
						{
							compField->SetDeletable();
							delete compField;
						}
						if (CurrentRecordIdx != j) GetRecordById(j-2, FALSE);
						CacheLastLocate(Id, From, field, index, j);
						success = TRUE;
						goto nextiter_2;
					}
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
				}
				success = FALSE;
				goto nextiter_2;
			}
			else
			{
				// Index available. Using fast locate. nfetched=log2(nrecords) for first locate, 1 more fetch per locate on same criteria
				int p;
				if (From == FIRST_RECORD) From = 2;
				else From = index->TranslateIndex(From+2, i->index)+1;
				if (From == lastLocateFrom && Id == lastLocateId && field->Compare(lastLocateFieldClone)==0 && index == lastLocateIndex && (i->index->locateUpToDate || pTable->GLocateUpToDate))
				{
					if (CurrentRecordIdx != lastLocateIdx) GetRecordById(lastLocateIdx-2, FALSE);
					success = TRUE;
					goto nextiter_2;
				}
				if (From >= index->NEntries)
				{
					return FALSE;
				}
				compField = i->index->QuickFindField(Id, i->index->Get(From));
				cfV = /*(compField && compField->Type == FIELD_PRIVATE) ? ((PrivateField *)compField)->myField :*/ compField;
				if (field->Compare(cfV) == 0)
				{
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
					n = i->index->TranslateIndex(From, index);
					if (CurrentRecordIdx != n) GetRecordById(n-2, FALSE);
					CacheLastLocate(Id, From, field, i->index, n);
					success = TRUE;
					goto nextiter_2;
				}
				if (compField)
				{
					compField->SetDeletable();
					delete compField;
				}
				p = i->index->QuickFindEx(Id, field, From, comp_mode);
				if (p != FIELD_NOT_FOUND)
				{
					n = (index->Id == Id) ? p : i->index->TranslateIndex(p, index);
					if (CurrentRecordIdx != n) GetRecordById(n-2, FALSE);
					CacheLastLocate(Id, From, field, i->index, n);
					success = TRUE;
					goto nextiter_2;
				}
			}
nextiter_2: // eek
			if (success)
			{
				if (!MatchFilters() && !Eof())
				{
					From = GetRecordId();
					if (nskip) *nskip++;
				}
				else break;
			}
			else break;
		}

		break;
	case COMPARE_MODE_STARTS:
		while (1)
		{
			success = FALSE;
			if (!i)
			{
				// No index for this column. Using slow locate, enumerates the database, still faster than what the user
				// can do since we have access to QuickFindField which only read field headers
				// in order to locate the field we have to compare. user could only read the entire record.
				if (From == FIRST_RECORD)
					From = 2;
				else
					From+=3;
				if (From == lastLocateFrom && Id == lastLocateId && field->Starts(lastLocateFieldClone)==0 && index == lastLocateIndex && (index->locateUpToDate || pTable->GLocateUpToDate))
				{
					if (CurrentRecordIdx != lastLocateIdx) GetRecordById(lastLocateIdx-2, FALSE);
					success = TRUE;
					goto nextiter_3;
				}
				for (j=From;j<index->NEntries;j++)
				{
					compField = index->QuickFindField(Id, index->Get(j));
					cfV = /*(compField && compField->Type == FIELD_PRIVATE) ? ((PrivateField *)compField)->myField :*/ compField;
					if (!field->Starts(cfV))
					{
						if (compField)
						{
							compField->SetDeletable();
							delete compField;
						}
						if (CurrentRecordIdx != j) GetRecordById(j-2, FALSE);
						CacheLastLocate(Id, From, field, index, j);
						success = TRUE;
						goto nextiter_3;
					}
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
				}
				success = FALSE;
				goto nextiter_3;
			}
			else
			{
				// Index available. Using fast locate. nfetched=log2(nrecords) for first locate, 1 more fetch per locate on same criteria
				int p;
				if (From == FIRST_RECORD) From = 2;
				else From = index->TranslateIndex(From+2, i->index)+1;
				if (From == lastLocateFrom && Id == lastLocateId && field->Starts(lastLocateFieldClone)==0 && index == lastLocateIndex && (i->index->locateUpToDate || pTable->GLocateUpToDate))
				{
					if (CurrentRecordIdx != lastLocateIdx) GetRecordById(lastLocateIdx-2, FALSE);
					success = TRUE;
					goto nextiter_3;
				}
				if (From >= index->NEntries)
				{
					return FALSE;
				}
				compField = i->index->QuickFindField(Id, i->index->Get(From));
				cfV = /*(compField && compField->Type == FIELD_PRIVATE) ? ((PrivateField *)compField)->myField :*/ compField;
				if (field->Starts(cfV) == 0)
				{
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
					n = i->index->TranslateIndex(From, index);
					if (CurrentRecordIdx != n) GetRecordById(n-2, FALSE);
					CacheLastLocate(Id, From, field, i->index, n);
					success = TRUE;
					goto nextiter_3;
				}
				if (compField)
				{
					compField->SetDeletable();
					delete compField;
				}
				p = i->index->QuickFindEx(Id, field, From, comp_mode);
				if (p != FIELD_NOT_FOUND)
				{
					n = (index->Id == Id) ? p : i->index->TranslateIndex(p, index);
					if (CurrentRecordIdx != n) GetRecordById(n-2, FALSE);
					CacheLastLocate(Id, From, field, i->index, n);
					success = TRUE;
					goto nextiter_3;
				}
			}
nextiter_3: // eek
			if (success)
			{
				if (!MatchFilters() && !Eof())
				{
					From = GetRecordId();
					if (nskip) *nskip++;
				}
				else break;
			}
			else break;
		}

		break;
	}


	return success;
}

//---------------------------------------------------------------------------
LinkedList *Scanner::GetFields(void)
{
	if (!CurrentRecord)
		return NULL;
	return &CurrentRecord->Fields;
}

//---------------------------------------------------------------------------
void Scanner::DeleteFieldByName(const char *name)
{
	unsigned char Idx;
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	ColumnField *header = pTable->GetColumnByName(name);
	if (header)
	{
		Idx = header->ID;
		DeleteFieldById(Idx);
	}
	return;
}

//---------------------------------------------------------------------------
void Scanner::DeleteFieldById(unsigned char Id)
{
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	Field *field = CurrentRecord->GetField(Id);
	if (!field) return;
	CurrentRecord->RemoveField(field);
}

//---------------------------------------------------------------------------
void Scanner::DeleteField(Field *field)
{
	if (!canEdit) throwException(EXCEPTION_READ_ONLY_RECORD_SET);
	if (!field) return;
	CurrentRecord->RemoveField(field);
}

//---------------------------------------------------------------------------
void Scanner::Push(void)
{
	LinkedListEntry *Element = new LinkedListEntry();
	Element->Int = CurrentRecordIdx;
	Stack.AddEntry(Element, TRUE);
}

//---------------------------------------------------------------------------
void Scanner::Pop(void)
{
	LinkedListEntry *Element = Stack.GetFoot();
	if (!Element)
		return;
	CurrentRecordIdx = Element->Int;
	Stack.RemoveEntry(Element);
	GetCurrentRecord();
}

//---------------------------------------------------------------------------
float Scanner::FragmentationLevel(void)
{
	int oldP = GetRecordId();
	int i;
	size_t totalSize=0;

	if (CurrentRecord)
	{
		delete CurrentRecord;
		CurrentRecord = NULL;
		CurrentRecordIdx = 0;
	}

	for (i=0;i<index->NEntries;i++)
	{
		Record *r = GetRecord(i);
		if (r)
		{
			Field *f = (Field *)r->Fields.GetHead();
			while (f)
			{
				totalSize += f->GetDataSize() + sizeof(f->ID)+sizeof(f->Type)+sizeof(f->MaxSizeOnDisk)+sizeof(f->NextFieldPos)+sizeof(f->PreviousFieldPos);
				f = (Field *)f->GetNext();
			}
			delete r;
		}
	}
	GetRecordById(oldP);
	Vfseek(pTable->Handle, 0, SEEK_END);
	return (((float)(Vftell(pTable->Handle)-strlen(__TABLE_SIGNATURE__)) / (float)totalSize) - 1) * 100;
}

//---------------------------------------------------------------------------
int Scanner::GetRecordsCount(void)
{
	if (index)
		return index->NEntries-2;
	else
		return 0;
}

//---------------------------------------------------------------------------
BOOL Scanner::SetWorkingIndexById(unsigned char Id)
{
	IndexField *indx = pTable->GetIndexById(Id);
	int v = CurrentRecordIdx;
	if (indx)
	{
		if (!Eof() && !Bof())
		{
			IndexField *f = index->SecIndex;
			v = index->GetCooperative(CurrentRecordIdx);
			while (f != indx)
			{
				v = f->index->GetCooperative(v);
				f = f->index->SecIndex;
			}
		}
		index = indx->index;
		CurrentRecordIdx = v;
		GetCurrentRecord();
	}
	return (indx != NULL);
}

//---------------------------------------------------------------------------
BOOL Scanner::SetWorkingIndexByName(char *desc)
{
	IndexField *indx = pTable->GetIndexByName(desc);
	if (indx)
		return SetWorkingIndexById(indx->ID);
	else
		return SetWorkingIndexById(-1);
//return FALSE;
}

//---------------------------------------------------------------------------
void Scanner::IndexModified(void)
{
	iModified = TRUE;
}

//---------------------------------------------------------------------------
void Scanner::ClearDirtyBit(void)
{
	iModified = FALSE;
}

//---------------------------------------------------------------------------
int Scanner::GetConsistencyInfo(void)
{
	return pTable->GetConsistencyInfo();
}

//---------------------------------------------------------------------------
Table *Scanner::GetTable(void)
{
	return pTable;
}

//---------------------------------------------------------------------------
ColumnField *Scanner::GetColumnByName(const char *FieldName)
{
	ColumnField *p = (ColumnField *)(pTable->FieldsRecord)->Fields.GetHead();
	while (p)
	{
		if (!stricmp(p->GetFieldName(), FieldName))
			return p;
		p = (ColumnField *)p->GetNext();
	}
	return NULL;
}

//---------------------------------------------------------------------------
ColumnField *Scanner::GetColumnById(unsigned char Idx)
{
	if (!(pTable->FieldsRecord))
		return NULL;
	return (ColumnField *)(pTable->FieldsRecord)->GetField(Idx);
}

//---------------------------------------------------------------------------
int Scanner::AddFilterByName(wchar_t *name, Field *Data, unsigned char Op)
{
	ColumnField *f = pTable->GetColumnByName(AutoChar(name));
	if (f)
		return AddFilterById(f->GetFieldId(), Data, Op);
	return ADDFILTER_FAILED;
}

//---------------------------------------------------------------------------
int Scanner::AddFilterById(unsigned char Id, Field *Data, unsigned char Op)
{
	ColumnField *f = pTable->GetColumnById(Id);
	if (f)
	{
		Filter *filter = new Filter(Data, f->GetFieldId(), Op);
		FilterList.AddEntry(filter, TRUE);
	}
	else
		return ADDFILTER_FAILED;

	if (in_query_parser) return 1;
	return CheckFilters();
}

//---------------------------------------------------------------------------
int Scanner::AddFilterOp(unsigned char Op)
{
	Filter *filter = new Filter(Op);
	FilterList.AddEntry(filter, TRUE);
	if (in_query_parser) return 1;
	return CheckFilters();
}

//---------------------------------------------------------------------------
Filter *Scanner::GetLastFilter(void)
{
	if (FilterList.GetNElements() == 0) return NULL;
	return (Filter *)FilterList.GetFoot();
}

//---------------------------------------------------------------------------
void Scanner::RemoveFilters(void)
{
	last_query_failed = FALSE;
	while (FilterList.GetNElements() > 0)
		FilterList.RemoveEntry(FilterList.GetHead());
	FiltersOK = FALSE;
}

//---------------------------------------------------------------------------
BOOL Scanner::CheckFilters(void)
{
	int f=0;
	FiltersOK = FALSE;
	if (FilterList.GetNElements() == 0) // Should never happen
		return FILTERS_INVALID;
	Filter *filter = (Filter *)FilterList.GetHead();
	while (filter)
	{
		if (f == 256) return FILTERS_INVALID;
		int op = filter->GetOp();
		if (filter->Data() || op == FILTER_ISEMPTY || op == FILTER_ISNOTEMPTY)
			f++;
		else
		{
			if (op != FILTER_NOT)
				f--;
		}
		if (f == 0) return FILTERS_INVALID;
		filter = (Filter *)filter->GetNext();
	}

	if (f == 1)
	{
		FiltersOK = TRUE;
		return FILTERS_COMPLETE;
	}
	return FILTERS_INCOMPLETE;
}

//---------------------------------------------------------------------------
LinkedList *Scanner::GetFilters(void)
{
	return &FilterList;
}

static bool EmptyMeansTrue(int op)
{
	return op == FILTER_ISEMPTY
	       || op == FILTER_NOTEQUALS
	       || op == FILTER_NOTCONTAINS;
}

//---------------------------------------------------------------------------
bool Scanner::MatchFilter(Filter *filter)
{
	Field *field = GetFieldById(filter->Id);
	int op = filter->GetOp();
	/* old behaviour
	if (!field)
		return EmptyMeansTrue(op);
	else if (op == FILTER_ISEMPTY)
		return FALSE;
	else if (op == FILTER_ISNOTEMPTY)
		return TRUE;
	*/
	// new behaviour
	if (!field)
	{
		Field * f = filter->Data();
		// if field is empty and we're doing an equals op, match if f is also empty
		if (op == FILTER_EQUALS && f) return f->ApplyFilter(f,FILTER_ISEMPTY);
		if (op == FILTER_NOTEQUALS && f) return f->ApplyFilter(f,FILTER_ISNOTEMPTY);
		return EmptyMeansTrue(op);
	}
	// no need to check for op == FILTER_ISEMPTY, the fields now handle that

	return field->ApplyFilter(filter->Data(), filter->GetOp());
}

struct Results
{
	void operator=(bool _val)
	{
		calculated=true;
		value=_val;
	}

	bool Calc(Scanner *scanner)
	{
		if (!calculated)
		{
			value = scanner->MatchFilter(filter);
			calculated=true;
		}
		return value;
	}

	void SetFilter(Filter *_filter)
	{
		calculated=false;
		filter=_filter;
	}
private:
	bool value;
	bool calculated;
	Filter *filter;
};


//---------------------------------------------------------------------------
bool Scanner::MatchFilters(void)
{
	if (!FiltersOK || FilterList.GetNElements() == 0)
	{
//  return MatchJoins();
		return TRUE;
	}

	ResultPtr = 0;

	Results resultTable[256];

	Filter *filter = (Filter *)FilterList.GetHead();
	while (filter)
	{
		if (ResultPtr == 256)
		{
			FiltersOK = FALSE; // Should never happen, case already discarded by CheckFilters
			return TRUE;
		}
		int op = filter->GetOp();
		if (filter->Data() || op == FILTER_ISEMPTY || op == FILTER_ISNOTEMPTY)
			resultTable[ResultPtr++].SetFilter(filter);
		else
			switch (op)
			{
			case FILTER_AND:
				if (ResultPtr > 1)
					resultTable[ResultPtr-2] = resultTable[ResultPtr-2].Calc(this) && resultTable[ResultPtr-1].Calc(this);
				ResultPtr--;
				break;
			case FILTER_OR:
				if (ResultPtr > 1)
					resultTable[ResultPtr-2] = resultTable[ResultPtr-2].Calc(this) || resultTable[ResultPtr-1].Calc(this);
				ResultPtr--;
				break;
			case FILTER_NOT:
				if (ResultPtr > 0)
					resultTable[ResultPtr-1] = !resultTable[ResultPtr-1].Calc(this);
				break;
			}
		filter = (Filter *)filter->GetNext();
	}

	if (ResultPtr != 1) // Should never happen, case already discarded by CheckFilters
	{
		FiltersOK = FALSE;
		return TRUE;
	}

	if (!resultTable[0].Calc(this)) return 0;
//  return MatchJoins();
	//return FALSE;
	return TRUE;
}
/*
//---------------------------------------------------------------------------
void Scanner::JoinScanner(Scanner *scanner, const char *field) {
  joined.addItem(new ScannerJoin(scanner, field));
  invalidJoinCache = 1;
}

//---------------------------------------------------------------------------
void Scanner::UnjoinScanner(Scanner *scanner) {
  for (int i=0;i<joined.getNumItems();i++)
    if (joined.enumItem(i)->getScanner() == scanner) {
      ScannerJoin *sj = joined.enumItem(i);
      joined.removeByPos(i);
      i--;
      delete sj;
      invalidJoinCache = 1;
    }
}

//---------------------------------------------------------------------------
BOOL Scanner::MatchJoins() {
  if (!invalidJoinCache) {
    return lastJoinCache;
  }
  invalidJoinCache = 0;
  if (inMatchJoins) return 1;
  inMatchJoins = 1;
  for (int i=0;i<joined.getNumItems();i++) {
    int r = 0;
    ScannerJoin *scan = joined.enumItem(i);
    const char *q = scan->getScanner()->GetLastQuery();
    if (!q || !*q) continue;
    Field *f = GetFieldByName((char *)scan->getField());
    if (f) {
      f = f->GetType() == FIELD_PRIVATE ? ((PrivateField *)f)->myField : f;
      Field *ff = NULL;
      switch (f->GetType()) {
        case FIELD_INTEGER:
        case FIELD_BOOLEAN:
          ff = new IntegerField;
          (static_cast<IntegerField*>(ff))->SetValue((static_cast<IntegerField*>(f))->GetValue());
          break;
        case FIELD_STRING:
          ff = new StringField;
          (static_cast<StringField*>(ff))->SetString((static_cast<StringField*>(f))->GetString());
          break;
        case FIELD_BINARY:
          ff = new BinaryField;
          char *data;
          int datalen;
          data = (static_cast<BinaryField*>(f))->GetData(&datalen);
          (static_cast<BinaryField*>(ff))->SetData(data, datalen);
          break;
        case FIELD_GUID:
          ff = new GUIDField;
          (static_cast<GUIDField*>(f))->SetValue((static_cast<GUIDField*>(f))->GetValue());
          break;
        case FIELD_FLOAT:
          ff = new FloatField;
          (static_cast<FloatField*>(f))->SetValue((static_cast<FloatField*>(f))->GetValue());
          break;
      }
      Scanner *s = scan->getScanner();
      int nskip;
      s->Push();
      r = s->LocateByName((char *)scan->getField(), -1, ff, &nskip);
      s->Pop();
      delete ff;
      if (!r) {
        inMatchJoins = 0;
        lastJoinCache = 0;
        return 0;
      }
    }
  }
  inMatchJoins = 0;
  lastJoinCache = 1;
  return 1;
}
*/