/* ---------------------------------------------------------------------------
Nullsoft Database Engine
--------------------
codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

Field Class

--------------------------------------------------------------------------- */

#include "NDE.h"

#ifdef WIN32
#include <malloc.h>
#elif defined(__APPLE__)
#include <alloca.h>
#endif

//---------------------------------------------------------------------------
void PUT_BINARY(unsigned char *dest, unsigned char *src, size_t size, size_t pos)
{
	if (src && dest)
		memcpy(dest+pos, src, size);
}

//---------------------------------------------------------------------------
void GET_BINARY(unsigned char *dest, unsigned char *src, size_t size, size_t pos) 
{
	if (dest && src || size == 0)
		memcpy(dest, src+pos, size);
}

//---------------------------------------------------------------------------
void PUT_FLOAT(float f, unsigned char *data, size_t pos)
{
	unsigned int y = *(int *)&f;
	data[pos]=(unsigned char)(y&255); data[pos+1]=(unsigned char)((y>>8)&255); data[pos+2]=(unsigned char)((y>>16)&255); data[pos+3]=(unsigned char)((y>>24)&255);
}

//---------------------------------------------------------------------------
float GET_FLOAT(unsigned char *data, size_t pos)
{
	int a = data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24);
	float f = *(float *)&a;
	return f;
}

//---------------------------------------------------------------------------
Field::Field(int FieldPos, Table *parentTable)
{
	InitField();
	pTable = parentTable;
	HTable = pTable->Handle;
	Pos = FieldPos;
	Readonly=FALSE;
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void Field::InitField(void)
{
	Type = FIELD_UNKNOWN;
	HTable = NULL;
	Pos = 0;
	ID=0;
	NextFieldPos = 0;
	PreviousFieldPos = 0;
	MaxSizeOnDisk=0;
	Readonly=FALSE;
	Kind = DELETABLE;
	SubtableRecord=INVALID_RECORD;
	Physical = FALSE;
}

//---------------------------------------------------------------------------
Field::Field()
{
	InitField();
}

//---------------------------------------------------------------------------
Field::~Field()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
}

//---------------------------------------------------------------------------
Field *Field::ReadField(int pos)
{
	Kind = NONDELETABLE;
	return ReadField(pos, FALSE	);
}

//---------------------------------------------------------------------------
Field *Field::ReadField(int pos, BOOL Quick)
{
	int newPos = pos;
	int rType = FIELD_REDIRECTOR;
	unsigned char oType;

	Physical=TRUE;
	Kind = NONDELETABLE;
	while  (rType == FIELD_REDIRECTOR)
	{
		Vfseek(HTable, Pos, SEEK_SET);
		if (Vfread(&ID, sizeof(ID), 1, HTable) != 1) return NULL;
		if (Vfread(&oType, sizeof(oType), 1, HTable) != 1) return NULL;
		if (oType == FIELD_REDIRECTOR)
		{
			if (Vfread(&newPos, sizeof(newPos), 1, HTable) != 1) return NULL;
			Pos = newPos;
		}
		rType = oType;
	}
	if (Quick)
		Vfseek(HTable, sizeof(MaxSizeOnDisk), SEEK_CUR);
	else
	{
		if (Vfread(&MaxSizeOnDisk, sizeof(MaxSizeOnDisk), 1, HTable) != 1) return NULL;
	}
	if (Vfread(&NextFieldPos, sizeof(NextFieldPos), 1, HTable) != 1) return NULL;
	if (Quick)
		return this;
	if (Vfread(&PreviousFieldPos, sizeof(PreviousFieldPos), 1, HTable) != 1) return NULL;

	Field *O=NULL;
	O = TranslateObject(oType, pTable);
	if (O)
	{
		O->ID = ID;
		O->Type = oType;
		O->HTable = HTable;
		O->pTable = pTable;
		O->Pos = Pos;
		O->MaxSizeOnDisk = MaxSizeOnDisk;
		O->NextFieldPos = NextFieldPos;
		O->PreviousFieldPos = PreviousFieldPos;
		O->Perm = Perm;
		O->Kind = Kind;
		O->Physical = TRUE;
		unsigned char *data = NULL;
		if (HTable->cached && MaxSizeOnDisk > VFILE_INC) 
		{
			pTable->IncErrorCount();
			MaxSizeOnDisk = GetDataSize();
			O->MaxSizeOnDisk = MaxSizeOnDisk;
		}
		else if (HTable->cached)
		{
			data = HTable->data+HTable->ptr; // benski> uber-hack
			Vfseek(HTable, MaxSizeOnDisk, SEEK_CUR);
		}
		else
		{
			data = (unsigned char *)alloca(MaxSizeOnDisk);//malloc(MaxSizeOnDisk);
			Vfread(data, MaxSizeOnDisk, 1, HTable);
		}
		
		if (data) 
		{
			O->ReadTypedData(data, MaxSizeOnDisk);
			//free(data);
		}
		return O;
	}
	return NULL;
}

//---------------------------------------------------------------------------
void Field::WriteField(void)
{
	if (Pos == -1) return;
	if (HTable->cached && MaxSizeOnDisk > VFILE_INC) {
		pTable->IncErrorCount();
		MaxSizeOnDisk = GetDataSize();
	}
	if (Pos==0 || GetDataSize() > MaxSizeOnDisk)
	{
		int newPos;
		MaxSizeOnDisk = GetDataSize();
		newPos = AllocNewPos(HTable);
		if (Pos != 0)
		{
			unsigned char v;
			Vfseek(HTable, Pos, SEEK_SET);
			v = 0; Vfwrite(&v, sizeof(v), 1, HTable);
			v = FIELD_REDIRECTOR; Vfwrite(&v, sizeof(v), 1, HTable);
			Vfwrite(&newPos, sizeof(newPos), 1, HTable);
		}
		Pos = newPos;
		if (Previous)
		{
			((Field *)Previous)->NextFieldPos = Pos;
			if (((Field *)Previous)->Pos)
			{
				Vfseek(HTable, ((Field *)Previous)->Pos+sizeof(ID)+sizeof(MaxSizeOnDisk)+sizeof(Type), SEEK_SET);
				Vfwrite(&Pos, sizeof(Pos), 1, HTable);
			}
		}
		if (Next)
		{
			((Field *)Next)->PreviousFieldPos = Pos;
			if (((Field *)Next)->Pos)
			{
				Vfseek(HTable, ((Field *)Next)->Pos+sizeof(ID)+sizeof(NextFieldPos)+sizeof(Type)+sizeof(MaxSizeOnDisk), SEEK_SET);
				Vfwrite(&Pos, sizeof(Pos), 1, HTable);
			}
		}
	}
	if (Previous) PreviousFieldPos = ((Field*)Previous)->GetFieldPos(); else PreviousFieldPos = NULL;
	if (Next) NextFieldPos = ((Field*)Next)->GetFieldPos(); else NextFieldPos = NULL;
	Vfseek(HTable, Pos, SEEK_SET);
	Vfwrite(&ID, sizeof(ID), 1, HTable);
	Vfwrite(&Type, sizeof(Type), 1, HTable);
	Vfwrite(&MaxSizeOnDisk, sizeof(MaxSizeOnDisk), 1, HTable);
	Vfwrite(&NextFieldPos, sizeof(NextFieldPos), 1, HTable);
	Vfwrite(&PreviousFieldPos, sizeof(PreviousFieldPos), 1, HTable);
	unsigned char *data = (unsigned char*)alloca/*malloc*/(MaxSizeOnDisk);
	WriteTypedData(data, MaxSizeOnDisk);
	Vfwrite(data, MaxSizeOnDisk, 1, HTable);
	//if (data) free(data);
	UpdateSubtableEntry();	
}

//---------------------------------------------------------------------------
int Field::GetNextFieldPos(void)
{
	return NextFieldPos;
}

//---------------------------------------------------------------------------
void Field::SetTable(Table *parentTable)
{
	pTable = parentTable;
	HTable = pTable->Handle;
}

//---------------------------------------------------------------------------
int Field::GetFieldPos(void)
{
	return Pos;
}

//---------------------------------------------------------------------------
Field *Field::Clone(void)
{
	Field *clone = TranslateObject(Type, pTable);
	size_t size = GetDataSize();
	unsigned char *data = (unsigned char*)alloca/*malloc*/(size);
	WriteTypedData(data, size);
	clone->ReadTypedData(data, size);
	//if (data) free(data);
	clone->Type = Type;
	clone->HTable = HTable;
	clone->Pos = FIELD_CLONE;
	clone->ID = ID;
	clone->NextFieldPos = 0;
	clone->PreviousFieldPos = 0;
	clone->MaxSizeOnDisk=size;
	clone->Readonly = TRUE;
	clone->Kind = DELETABLE;
	return clone;

}

//---------------------------------------------------------------------------
BOOL Field::IsReadonly(void)
{
	return Readonly;
}

//---------------------------------------------------------------------------
void Field::SetDeletable(void)
{
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void Field::RememberSubtableEntry(void)
{
	if (SubtableRecord != INVALID_RECORD) return;
	ColumnField *c = pTable->GetColumnById(ID);
	if (!c->indexUnique)
		return;
	Field *lf = this;
	BOOL found = c->uniqueTable->LocateById(1, FIRST_RECORD, lf);
	if (found)
		SubtableRecord = c->uniqueTable->GetRecordId();
}

//---------------------------------------------------------------------------
void Field::UpdateSubtableEntry(void)
{
	UpdateSubtableEntry(FALSE);
}

//---------------------------------------------------------------------------
void Field::UpdateSubtableEntry(BOOL Remove)
{
	int n;
	IntegerField *f;
	Field *gf;

	if (Type == FIELD_COLUMN) return;
	if (Type == FIELD_INDEX) return;

	ColumnField *c = pTable->GetColumnById(ID);
	if (!c->indexUnique)
		return;
	if (SubtableRecord != INVALID_RECORD)
	{
		c->uniqueTable->GetRecordById(SubtableRecord);
		f = (IntegerField *)c->uniqueTable->GetFieldById(2);
		n = f->GetValue()-1;
		if (n == 0)
			c->uniqueTable->Delete();
		else
		{
			c->uniqueTable->Edit();
			f->SetValue(n);
			c->uniqueTable->Post();
		}
	}
	if (Remove)
		return;
	Field *lf = this;
	BOOL found = c->uniqueTable->LocateById(1, FIRST_RECORD, lf);
	if (found)
	{
		f = (IntegerField *)c->uniqueTable->GetFieldById(2);
		n = f->GetValue()+1;
		c->uniqueTable->Edit();
		f->SetValue(n);
		c->uniqueTable->Post();
		SubtableRecord = c->uniqueTable->GetRecordId();
	}
	else
	{
		c->uniqueTable->New();
		c->uniqueTable->NewFieldById(1, PERM_READ);
		gf = c->uniqueTable->GetFieldById(1);
		size_t size = lf->GetDataSize();
		unsigned char *data = (unsigned char*)alloca/*malloc*/(size);
		lf->WriteTypedData(data, size);
		gf->ReadTypedData(data, size);
		//free(data);
		c->uniqueTable->NewFieldById(2, PERM_READWRITE);
		f = (IntegerField *)c->uniqueTable->GetFieldById(2);
		f->SetValue(1);
		c->uniqueTable->Post();
		SubtableRecord = c->uniqueTable->GetRecordId();
	}
}

//---------------------------------------------------------------------------
void Field::OnRemove(void)
{
	RememberSubtableEntry();
	UpdateSubtableEntry(TRUE);
}

//---------------------------------------------------------------------------
int Field::GetType() {
	return Type;    
}

unsigned char Field::GetPerm() {
	return Perm;
}