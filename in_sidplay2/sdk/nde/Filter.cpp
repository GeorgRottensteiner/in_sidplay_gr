/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Filter Class

--------------------------------------------------------------------------- */

// Filters can now be a test on a field or a single operator that will pop
// one operation from the filters stack. upon AddFilter*, return value will

#include "NDE.h"

//---------------------------------------------------------------------------
Filter::Filter(unsigned char _Op) 
{
DataField = NULL;
Op = _Op;
Id = -1;
}

//---------------------------------------------------------------------------
Filter::Filter(Field *Data, unsigned char _Id, unsigned char _Op)
{
DataField = Data;
Op = _Op;
Id = _Id;
}

//---------------------------------------------------------------------------
Filter::~Filter()
{
delete DataField;
}

//---------------------------------------------------------------------------
unsigned char Filter::GetOp(void)
{
return Op;
}

//---------------------------------------------------------------------------
void Filter::SetOp(unsigned char _Op)
{
Op = _Op;
}

//---------------------------------------------------------------------------
Field *Filter::Data(void)
{
return DataField;
}

//---------------------------------------------------------------------------
void Filter::SetData(Field *data)
{
DataField = data;
}

//---------------------------------------------------------------------------
int Filter::GetId(void)
{
return Id;
}

