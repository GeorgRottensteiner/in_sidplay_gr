#include "XSPFHandler.h"
#include "XSPFLoader.h"

const wchar_t *XSPFHandler::EnumerateExtensions(size_t n)
{
	switch(n)
	{
	case 0:
		return L"xspf";
	default:
		return 0;
	}
}

const char *XSPFHandler::EnumerateMIMETypes(size_t n)
{
	switch(n)
	{
	case 0:
		return "application/xspf+xml";
	default:
		return 0;
	}
}

const wchar_t *XSPFHandler::GetName()
{
	return L"XML Shareable Playlist Format";
}

// returns SUCCESS and FAILED, so be careful ...
int XSPFHandler::SupportedFilename(const wchar_t *filename)
{
	size_t filenameLength = wcslen(filename);
	size_t extensionLength = wcslen(L".xspf");
	if (filenameLength < extensionLength) return SVC_PLAYLISTHANDLER_FAILED;  // too short
	if (!wcsicmp(filename + filenameLength - extensionLength, L".xspf"))
		return SVC_PLAYLISTHANDLER_SUCCESS;
	else
		return SVC_PLAYLISTHANDLER_FAILED;
}

int XSPFHandler::SupportedMIMEType(const char *type)
{
	if (!strcmp(type, "application/xspf+xml"))
		return SVC_PLAYLISTHANDLER_SUCCESS;
	else
		return SVC_PLAYLISTHANDLER_FAILED;
}

ifc_playlistloader *XSPFHandler::CreateLoader(const wchar_t *filename)
{
	return new XSPFLoader;
}

void XSPFHandler::ReleaseLoader(ifc_playlistloader *loader)
{
	delete static_cast<XSPFLoader *>(loader);
}

// Define the dispatch table
#define CBCLASS XSPFHandler
START_DISPATCH;
CB(SVC_PLAYLISTHANDLER_ENUMEXTENSIONS, EnumerateExtensions)
CB(SVC_PLAYLISTHANDLER_ENUMMIMETYPES, EnumerateMIMETypes)
CB(SVC_PLAYLISTHANDLER_SUPPORTFILENAME, SupportedFilename)
CB(SVC_PLAYLISTHANDLER_CREATELOADER, CreateLoader)
VCB(SVC_PLAYLISTHANDLER_RELEASELOADER, ReleaseLoader)
CB(SVC_PLAYLISTHANDLER_GETNAME, GetName)
END_DISPATCH;