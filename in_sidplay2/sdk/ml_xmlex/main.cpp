#include "main.h"
#include "../Winamp/wa_ipc.h"

int Init();
void Quit();
UINT_PTR xmlex_treeItem = 0;
api_service *serviceManager = 0;

winampMediaLibraryPlugin plugin =
{
	MLHDR_VER,
		"XML Reader Example",
		Init,
		Quit,
		xmlex_pluginMessageProc,
		0,
		0,
		0,
};

int Init() 
{
	//starting point for wasabi, where services are shared
	WASABI_API_SVC = (api_service *)SendMessage(plugin.hwndWinampParent, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
	
	//set up tree item, gen_ml will call xmlex_pluginMessageProc if/when the treeview item gets selected
	MLTREEITEMW newTree;
	newTree.size = sizeof(MLTREEITEMW);
	newTree.parentId = 0;
	newTree.title = L"XML Example"; 
	newTree.hasChildren = 0;
	newTree.id = 0;
	SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM) &newTree, ML_IPC_TREEITEM_ADDW);
	xmlex_treeItem = newTree.id;	
	return 0; // 0 for success.  returning non-zero will cause gen_ml to abort loading your plugin
}

void Quit() 
{
}

extern "C" __declspec(dllexport) winampMediaLibraryPlugin *winampGetMediaLibraryPlugin()
{
	return &plugin;
}