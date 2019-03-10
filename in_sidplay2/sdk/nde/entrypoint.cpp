#include "NDE.h"
#include "database.h"

#ifdef WIN32
#if 0
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
			#ifdef WIN32
				InitializeCriticalSection(&Critical);
			#endif
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
			#ifdef WIN32
				DeleteCriticalSection(&Critical);
			#endif
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
#endif
#endif