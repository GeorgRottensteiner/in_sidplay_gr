/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Int64Field Class

--------------------------------------------------------------------------- */

#include "nde.h"
#include "Int64Field.h"
#include <time.h>

//---------------------------------------------------------------------------
Int64Field::Int64Field(int64_t Val)
{
	InitField();
	Type = FIELD_INT64;
	Value = Val;
	Kind = DELETABLE;
}

//---------------------------------------------------------------------------
void Int64Field::InitField(void)
{
	Type = FIELD_INT64;
	Value = 0;
	Kind = DELETABLE;
	Physical = false;
}

//---------------------------------------------------------------------------
Int64Field::Int64Field()
{
	InitField();
}

//---------------------------------------------------------------------------
Int64Field::~Int64Field()
{
	if (Kind != DELETABLE) throwException(EXCEPTION_NON_DELETABLE_OBJECT);
}

//---------------------------------------------------------------------------
void Int64Field::ReadTypedData(unsigned char *data, size_t len)
{
	CHECK_INT64(len);
	Value = *((int64_t *)data);
}

//---------------------------------------------------------------------------
void Int64Field::WriteTypedData(unsigned char *data, size_t len)
{
	if (Physical)
		RememberSubtableEntry();
	CHECK_INT64(len);
	*((int64_t *)data) = Value;
}

//---------------------------------------------------------------------------
int64_t Int64Field::GetValue(void)
{
	return Value;
}

//---------------------------------------------------------------------------
void Int64Field::SetValue(int64_t Val)
{
	if (Readonly) throwException(EXCEPTION_READ_ONLY_FIELD);
	Value = Val;
}

//---------------------------------------------------------------------------
size_t Int64Field::GetDataSize(void)
{
	return sizeof(int64_t);
}

//---------------------------------------------------------------------------
int Int64Field::Compare(Field *Entry)
{
	if (!Entry) return -1;
	return GetValue() < ((Int64Field*)Entry)->GetValue() ? -1 : (GetValue() > ((Int64Field*)Entry)->GetValue() ? 1 : 0);
}

//---------------------------------------------------------------------------
bool Int64Field::ApplyFilter(Field *Data, int op)
{
	bool r;
	switch (op)
	{
	case FILTER_EQUALS:
		r = Value == ((Int64Field *)Data)->GetValue();
		break;
	case FILTER_NOTEQUALS:
		r = Value != ((Int64Field *)Data)->GetValue();
		break;
	case FILTER_NOTCONTAINS:
		r = (bool)!(Value & ((Int64Field *)Data)->GetValue());
		break;
	case FILTER_CONTAINS:
		r = !!(Value & ((Int64Field *)Data)->GetValue());
		break;
	case FILTER_ABOVE:
		r = (bool)(Value > ((Int64Field *)Data)->GetValue());
		break;
	case FILTER_BELOW:
		r = (bool)(Value < ((Int64Field *)Data)->GetValue());
		break;
	case FILTER_BELOWOREQUAL:
		r = (bool)(Value <= ((Int64Field *)Data)->GetValue());
		break;
	case FILTER_ABOVEOREQUAL:
		r = (bool)(Value >= ((Int64Field *)Data)->GetValue());
		break;
	case FILTER_ISEMPTY:
		r = (Value == 0 || Value == -1);
		break;
	case FILTER_ISNOTEMPTY:
		r = !(Value == 0 || Value == -1);
		break;
	default:
		r = true;
		break;
	}
	return r;
}

