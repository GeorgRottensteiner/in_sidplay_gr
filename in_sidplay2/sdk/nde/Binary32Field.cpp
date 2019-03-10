/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Binary32Field Class

--------------------------------------------------------------------------- */

#include "nde.h"
#include "Binary32Field.h"

//---------------------------------------------------------------------------
Binary32Field::Binary32Field(char *_Data, size_t len)
{
	InitField();
	Type = FIELD_BINARY32;
	if (_Data && len > 0)
	{
		Data = (char *)malloc(len);
		memcpy(Data, _Data, len);
		Size = len;
	}
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void Binary32FieldReadOnly::InitField(void)
{
	Type = FIELD_BINARY32;
	Data = NULL;
	Size = 0;
	Kind = DELETABLE;
	Physical = FALSE;
}

Binary32Field::Binary32Field()
{
}

//---------------------------------------------------------------------------
Binary32FieldReadOnly::Binary32FieldReadOnly()
{
	InitField();
}

//---------------------------------------------------------------------------
Binary32Field::~Binary32Field()
{
	if (Data) free(Data);
}

Binary32FieldReadOnly::Binary32FieldReadOnly(char *_Data, size_t len)
{
	InitField();
	Type = FIELD_BINARY32;
	Size = len;
	Data = _Data;
	Kind = DELETABLE;
}

Binary32FieldReadOnly::~Binary32FieldReadOnly()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
}


//---------------------------------------------------------------------------
void Binary32Field::ReadTypedData(unsigned char *data, size_t len)
{
	uint32_t c;
	size_t pos = 0;

	CHECK_INT(len); //len-=4;
	c = GET_INT(); pos += 4;
	if (c && c<=len)
	{
		Size = c;
		if (Data) free(Data);
		Data = (char *)malloc(c);
		GET_BINARY((unsigned char *)Data, data, c, pos);
	}
}

//---------------------------------------------------------------------------
void Binary32FieldReadOnly::WriteTypedData(unsigned char *data, size_t len)
{
	uint32_t c;
	size_t pos = 0;

	CHECK_INT(len); //len-=4;
	if (Data && Size<=len)
	{
		c = (uint32_t)Size;
		PUT_INT(c); pos += 4;
		if (Data)
			PUT_BINARY(data, (unsigned char*)Data, c, pos);
	}
	else
	{
		PUT_INT(0);
	}
}

//---------------------------------------------------------------------------
char *Binary32FieldReadOnly::GetData(int *len)
{
	if (len)
		*len = (int)Size;
	return Data;
}

//---------------------------------------------------------------------------
char *Binary32FieldReadOnly::GetData(size_t *len)
{
	if (len)
		*len = Size;
	return Data;
}

//---------------------------------------------------------------------------
void Binary32Field::SetData(char *_Data, size_t len)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	if (!_Data || !len) return;
	if (Physical)
	{
		RememberSubtableEntry();
		free(Data);
		Size = 0;
	}
	Data = (char *)malloc(len);
	memcpy(Data, _Data, len);
	Size = len;
}

//---------------------------------------------------------------------------
size_t Binary32FieldReadOnly::GetDataSize(void)
{
	if (!Data) return 4;
	return Size + 4;
}

//---------------------------------------------------------------------------
int Binary32FieldReadOnly::Compare(Field *Entry)
{
	if (!Entry) return -1;
	size_t l = 0;
	return memcmp(Data, ((Binary32Field*)Entry)->GetData(&l), min(l, Size));
}

//---------------------------------------------------------------------------
bool Binary32FieldReadOnly::ApplyFilter(Field *FilterData, int op)
{
	size_t l;
	char *p = ((Binary32Field *)FilterData)->GetData(&l);
	char *d = Data;
	if (!p)
		p = "";
	if (!d)
		d = "";
	bool r;
	switch (op)
	{
	case FILTER_EQUALS:
		if (l != Size)
			r = false;
		else
			r = !memcmp(d, p, min(Size, l));
		break;
	case FILTER_CONTAINS:
		if (l > Size)
			r = FALSE;
		else
			r = !!memmem(d, p, Size, l);
		break;
	case FILTER_ABOVE:
		r = (memcmp(d, p, min(Size, l)) > 0);
		break;
	case FILTER_BELOW:
		r = (memcmp(d, p, min(Size, l)) < 0);
		break;
	case FILTER_BELOWOREQUAL:
		r = (memcmp(d, p, min(Size, l)) <= 0);
		break;
	case FILTER_ABOVEOREQUAL:
		r = (memcmp(d, p, min(Size, l)) >= 0);
		break;
	case FILTER_ISEMPTY:
		r = (Size == 0);
		break;
	case FILTER_ISNOTEMPTY:
		r = (Size != 0);
		break;
	default:
		r = true;
		break;
	}
	return r;
}

