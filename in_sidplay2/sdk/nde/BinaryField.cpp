/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 BinaryField Class

--------------------------------------------------------------------------- */

#include "nde.h"
#include "BinaryField.h"

//---------------------------------------------------------------------------
BinaryField::BinaryField(char *_Data, int len)
{
	InitField();
	Type = FIELD_BINARY;
	if (_Data && len > 0)
	{
		Data = (char *)malloc(len);
		memcpy(Data, _Data, len);
		Size = len;
	}
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void BinaryField::InitField(void)
{
	Type = FIELD_BINARY;
	Data = NULL;
	Size = 0;
	Kind = DELETABLE;
	Physical = FALSE;
}

//---------------------------------------------------------------------------
BinaryField::BinaryField()
{
	InitField();
}

//---------------------------------------------------------------------------
BinaryField::~BinaryField()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
	if (Data) free(Data);
}

//---------------------------------------------------------------------------
void BinaryField::ReadTypedData(unsigned char *data, size_t len)
{
	unsigned short c;
	int pos = 0;

	CHECK_SHORT(len);

	c = GET_SHORT(); pos += 2; 
	if (c && c<=len)
	{
		Size = c;
		if (Data) free(Data);
		Data = (char *)malloc(c);
		GET_BINARY((unsigned char *)Data, data, c, pos);
	}
}

//---------------------------------------------------------------------------
void BinaryField::WriteTypedData(unsigned char *data, size_t len)
{
	unsigned short c;
	size_t pos = 0;

	CHECK_SHORT(len); 
	if (Data && Size<=len)
	{
		c = (unsigned short)Size;
		PUT_SHORT(c); pos += 2;
		if (Data)
			PUT_BINARY(data, (unsigned char*)Data, c, pos);
	}
	else
	{
		PUT_SHORT(0);
	}
}

//---------------------------------------------------------------------------
char *BinaryField::GetData(int *len)
{
	if (len)
		*len = (int)Size;
	return Data;
}

//---------------------------------------------------------------------------
char *BinaryField::GetData(size_t *len)
{
	if (len)
		*len = Size;
	return Data;
}

//---------------------------------------------------------------------------
void BinaryField::SetData(char *_Data, size_t len)
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
size_t BinaryField::GetDataSize(void)
{
	if (!Data) return 2;
	return Size + 2;
}

//---------------------------------------------------------------------------
int BinaryField::Compare(Field *Entry)
{
	if (!Entry) return -1;
	size_t l = 0;
	return memcmp(Data, ((BinaryField*)Entry)->GetData(&l), min(l, Size));
}

//---------------------------------------------------------------------------
bool BinaryField::ApplyFilter(Field *FilterData, int op)
{
	size_t l;
	char *p = ((BinaryField *)FilterData)->GetData(&l);
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

