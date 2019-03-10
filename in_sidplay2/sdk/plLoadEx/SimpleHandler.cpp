#include "SimpleHandler.h"
#include "SimpleLoader.h"
const wchar_t *SimpleHandler::EnumerateExtensions(size_t n)
{
	if (n == 0)
		return L"simple";
	else
		return 0;
}

const wchar_t *SimpleHandler::GetName()
{
	return L"Simple Playlist Loader";
}

int SimpleHandler::SupportedFilename(const wchar_t *filename)
{
	size_t filenameLength = wcslen(filename);
	size_t extensionLength = wcslen(L".simple");
	if (filenameLength < extensionLength) return SVC_PLAYLISTHANDLER_FAILED;  // too short
	if (!wcsicmp(filename + filenameLength - extensionLength, L".simple"))
		return SVC_PLAYLISTHANDLER_SUCCESS;
	else
		return SVC_PLAYLISTHANDLER_FAILED;
}

ifc_playlistloader *SimpleHandler::CreateLoader(const wchar_t *filename)
{
	return new SimpleLoader();
}

void SimpleHandler::ReleaseLoader(ifc_playlistloader *loader)
{
	delete (SimpleLoader *)loader;
}

// Define the dispatch table
#define CBCLASS SimpleHandler
START_DISPATCH;
CB(SVC_PLAYLISTHANDLER_ENUMEXTENSIONS, EnumerateExtensions)
CB(SVC_PLAYLISTHANDLER_SUPPORTFILENAME, SupportedFilename)
CB(SVC_PLAYLISTHANDLER_CREATELOADER, CreateLoader)
VCB(SVC_PLAYLISTHANDLER_RELEASELOADER, ReleaseLoader)
CB(SVC_PLAYLISTHANDLER_GETNAME, GetName)
END_DISPATCH;