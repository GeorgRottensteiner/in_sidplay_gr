#ifndef _XMLEX_MAIN_H_
#define _XMLEX_MAIN_H_
#include "../gen_ml/ml.h" //must fix these to relative paths when project is in the right folder
#include "resource.h"

extern winampMediaLibraryPlugin plugin;
INT_PTR xmlex_pluginMessageProc(int message_type, INT_PTR param1, INT_PTR param2, INT_PTR param3);
extern UINT_PTR xmlex_treeItem;

#include <api/service/api_service.h>
extern api_service *serviceManager;
#define WASABI_API_SVC serviceManager

#endif