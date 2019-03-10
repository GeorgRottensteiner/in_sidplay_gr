/* ---------------------------------------------------------------------------
Nullsoft Database Engine
--------------------
codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

IndexField Class

--------------------------------------------------------------------------- */

#include "NDE.h"

//---------------------------------------------------------------------------
IndexField::IndexField(unsigned char id, int Pos, int type, char *FieldName)
{
	InitField();
	Type = FIELD_INDEX;
	Name = strdup(FieldName);
	ID = id;
	Position = Pos;
	DataType = type;
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void IndexField::InitField(void)
{
	index = 0;
	Type = FIELD_INDEX;
	Name = NULL;
	ID = 0;
	Position = -1;
	DataType = FIELD_UNKNOWN;
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
IndexField::IndexField()
{
	InitField();
}

//---------------------------------------------------------------------------
IndexField::~IndexField()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
	if (Name) free(Name);
	if (index) delete index;
}

//---------------------------------------------------------------------------
void IndexField::ReadTypedData(unsigned char *data, size_t len)
{
	unsigned char c;
	int pos=0;
	CHECK_INT(len);
	Position = GET_INT(); pos += 4;
	CHECK_INT(len);
	DataType = GET_INT(); pos += 4;
	CHECK_CHAR(len);
	c = GET_CHAR(); pos++;
	if (c)
	{
		CHECK_BIN(len, c);
		Name = (char *)malloc(c+1);
		GET_BINARY((unsigned char *)Name, data, c, pos);
		Name[c]=0;
	}
}

//---------------------------------------------------------------------------
void IndexField::WriteTypedData(unsigned char *data, size_t len)
{
	unsigned char c;
	int pos=0;
	CHECK_INT(len);
	PUT_INT(Position); pos += 4;

	CHECK_INT(len);
	PUT_INT(DataType); pos += 4;

	if (Name) c = (char)strlen(Name); else c = 0;
	CHECK_CHAR(len);
	PUT_CHAR(c); pos++;

	CHECK_BIN(len, c);
	if (Name) 
	{
		PUT_BINARY(data, (unsigned char *)Name, c, pos);
	}
}

//---------------------------------------------------------------------------
char *IndexField::GetIndexName(void)
{
	return Name;
}

//---------------------------------------------------------------------------
size_t IndexField::GetDataSize(void)
{
	int s=8;
	if (Name)
		s+=(int)strlen(Name);
	s++;
	return s;
}

//---------------------------------------------------------------------------
int IndexField::Compare(Field * /*Entry*/)
{
	return 0;
}

//---------------------------------------------------------------------------
int IndexField::TranslateToIndex(int Id, IndexField *toindex)
{
	if (index && toindex && toindex->index)
		return index->TranslateIndex(Id, toindex->index);
	return -1;
}


