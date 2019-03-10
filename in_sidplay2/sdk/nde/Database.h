/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Database Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __DATABASE_H
#define __DATABASE_H

#include "NDE.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>

#ifdef WIN32
//extern CRITICAL_SECTION Critical;
#endif

#define NDE_CACHE TRUE
#define NDE_NOCACHE FALSE

#define NDE_OPEN_ALWAYS TRUE
#define NDE_OPEN_EXISTING FALSE
class Database
  {
  public:
    Database();
#ifdef WIN32
    Database(HINSTANCE hinst);
    HINSTANCE GetInstance();
    void SetInstance(HINSTANCE hinst);
#endif
		~Database();
    Table *OpenTable(char *table, char*index, BOOL create, BOOL cached);
    void CloseTable(Table *table);
   private:
#ifdef WIN32
      HINSTANCE hInstance;
#endif
  };


#endif
