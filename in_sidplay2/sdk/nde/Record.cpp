/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Record Class

--------------------------------------------------------------------------- */

#include "NDE.h"
#include <stdio.h>


//---------------------------------------------------------------------------
Record::Record(int RecordPos, int RecordIdx, int insertionPoint, VFILE *TableHandle, VFILE *IdxHandle, Table *p, Scanner *s)
{
Invalid = FALSE;
HTable = TableHandle;
Idx = IdxHandle;
RecordIndex=RecordIdx;
ParentTable = p;
ParentScanner = s;
InsertionPoint = insertionPoint;
int max=ParentTable->FieldsRecord ? ParentTable->FieldsRecord->Fields.GetNElements() : 128;
int n=0;
if (RecordPos != NULL)
	{
	int ThisPos = RecordPos;
	while (ThisPos)
		{
		if (n >= max)
			break;
		Vfseek(HTable, ThisPos, SEEK_SET);
		Field Entry (ThisPos, ParentTable);
		Field *TypedEntry = Entry.ReadField(ThisPos);
		Entry.SetDeletable();

    if (!TypedEntry) break; // some db error?

		AddField(TypedEntry);
		ThisPos = TypedEntry->GetNextFieldPos();
		n++;
		}
	}
}

//---------------------------------------------------------------------------
Record::~Record()
{

}

//---------------------------------------------------------------------------
Field *Record::GetField(unsigned char ID)
{
if (Invalid) return NULL;
Field *p = (Field *)Fields.GetHead();
while (p)
	{
	if (p->GetFieldId() == ID)
		return p;
	p = (Field *)p->GetNext();
	}
return NULL;
}

//---------------------------------------------------------------------------
void Record::AddField(Field *field)
{
if (!field || Invalid)	return;
if (GetField(field->ID))
 return;

if (ParentScanner && ParentScanner->GetFieldById(field->ID))
	return;
field->SetTable(ParentTable);
Fields.AddEntry(field, TRUE);
}

//---------------------------------------------------------------------------
void Record::WriteFields(void)
{
if (Invalid) return;
Field *p = (Field *)Fields.GetHead();
while (p)
	{
	p->WriteField();
	p = (Field *)p->GetNext();
	}
WriteIndex();
}

//---------------------------------------------------------------------------
void Record::WriteIndex(void)
{
if (Invalid) return;
int P=0;
if (RecordIndex == NEW_RECORD)
	RecordIndex = ParentTable->dScanner->index->Insert(InsertionPoint);
if (Fields.GetNElements())
	P=((Field *)Fields.GetHead())->GetFieldPos();
RecordIndex = ParentTable->dScanner->index->Update(RecordIndex, P, this, FALSE);
}

//---------------------------------------------------------------------------
void Record::Delete(void)
{
ParentTable->dScanner->index->Delete(RecordIndex, ParentTable->dScanner->index->Get(RecordIndex), this);
Field *f = (Field *)Fields.GetHead();
while (f)
	{
	f->OnRemove();
	f = (Field *)f->GetNext();
	}
Invalid = TRUE;
}

//---------------------------------------------------------------------------
void Record::RemoveField(Field *field)
{
if (Invalid) return;
if (!ParentTable->dScanner->Edition) throwException(EXCEPTION_NOT_IN_EDIT_MODE);
if (!field)
	return;
field->OnRemove();
Fields.RemoveEntry(field);
}

LinkedList *Record::GetFields() {
  return &Fields;
}