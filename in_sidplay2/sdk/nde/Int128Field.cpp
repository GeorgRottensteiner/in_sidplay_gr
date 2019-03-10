/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Int128Field Class

--------------------------------------------------------------------------- */

#include "nde.h"
#include "Int128Field.h"
#include <time.h>

//---------------------------------------------------------------------------
Int128Field::Int128Field(void *data)
{
	InitField();
	Type = FIELD_INT128;
	memcpy(Value, data, 16);
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void Int128Field::InitField(void)
{
	Type = FIELD_INT128;
	memset(Value, 0, 16);
	Kind = DELETABLE;
	Physical = false;
}

//---------------------------------------------------------------------------
Int128Field::Int128Field()
{
	InitField();
}

//---------------------------------------------------------------------------
Int128Field::~Int128Field()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
}

//---------------------------------------------------------------------------
void Int128Field::ReadTypedData(unsigned char *data, size_t len)
{
	if (len < 16) return;
	memcpy(Value, data, 16);
}

//---------------------------------------------------------------------------
void Int128Field::WriteTypedData(unsigned char *data, size_t len)
{
	if (Physical)
		RememberSubtableEntry();
	if (len < 16) return;
	memcpy(data, Value, 16);
}

//---------------------------------------------------------------------------
void *Int128Field::GetValue(void)
{
	return Value;
}

//---------------------------------------------------------------------------
void Int128Field::SetValue(void *Val)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	memcpy(Value, Val, 16);
}

//---------------------------------------------------------------------------
size_t Int128Field::GetDataSize(void)
{
	return 16;
}

//---------------------------------------------------------------------------
int Int128Field::Compare(Field *Entry)
{
	if (!Entry) return -1;
	return memcmp(Value, ((Int128Field*)Entry)->GetValue(), 16);
}

static char zerobuf[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//---------------------------------------------------------------------------
bool Int128Field::ApplyFilter(Field *Data, int op)
{
	void *p = ((Int128Field *)Data)->GetValue();
	void *d = Value;
	if (!p)
		p = zerobuf;
	if (!d)
		d = zerobuf;
	bool r;
	switch (op)
	{
	case FILTER_EQUALS:
		r = !memcmp(d, p, 16);
		break;
	case FILTER_CONTAINS:
		r = !memcmp(d, p, 16);
		break;
	case FILTER_ABOVE:
		r = (memcmp(d, p, 16) > 0);
		break;
	case FILTER_BELOW:
		r = (memcmp(d, p, 16) < 0);
		break;
	case FILTER_BELOWOREQUAL:
		r = (memcmp(d, p, 16) <= 0);
		break;
	case FILTER_ABOVEOREQUAL:
		r = (memcmp(d, p, 16) >= 0);
		break;
	case FILTER_ISEMPTY:
		r = !d || (!memcmp(d, zerobuf, 16));
		break;
	case FILTER_ISNOTEMPTY:
		r = d && (memcmp(d, zerobuf, 16));
		break;
	default:
		r = true;
		break;
	}
	return r;
}

