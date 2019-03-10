/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Database Class

--------------------------------------------------------------------------- */

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "NDE.h"

#ifdef WIN32
//CRITICAL_SECTION Critical;
#endif

//---------------------------------------------------------------------------
Database::Database()
{
#ifdef WIN32
hInstance = (HINSTANCE)0;
#endif
}

#ifdef WIN32
//---------------------------------------------------------------------------
Database::Database(HINSTANCE hinst)
{
hInstance = hinst;
}
#endif

//---------------------------------------------------------------------------
Database::~Database()
{
}

#ifdef WIN32
//---------------------------------------------------------------------------
void Database::SetInstance(HINSTANCE inst) {
  hInstance = inst;
}

HINSTANCE Database::GetInstance() {
  return hInstance;
}
#endif

//---------------------------------------------------------------------------
Table *Database::OpenTable(char *TableName, char *IdxName, BOOL Create, BOOL Cached)
//char *tablefn, char*indexfn, BOOL create)
{
	Table *table = new Table(TableName, IdxName, Create, this, Cached);
	if (table)
	{
		if (table->Open())
			return table;
		table->Close();
		delete table;
	}
	return NULL;
}

//---------------------------------------------------------------------------
void Database::CloseTable(Table *table)
{
	if (table)
	{
		table->Close();
		delete table;
	}
}
