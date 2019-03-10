#ifndef NULLSOFT_ML_LOCAL_API_MLDB_H
#define NULLSOFT_ML_LOCAL_API_MLDB_H

#include <bfc/dispatch.h>
#include "../gen_ml/ml.h"

class api_mldb : public Dispatchable
{
protected:
	api_mldb() {}
	~api_mldb() {}
public:
	itemRecordW *GetFile(const wchar_t *filename);
	itemRecordListW *GetAlbum(const wchar_t *albumname, const wchar_t *albumartist);
	itemRecordListW *Query(const wchar_t *query);

	void SetField(const wchar_t *filename, const char *field, const wchar_t *value);
	void Sync();

	void FreeRecord(itemRecordW *record);
	void FreeRecordList(itemRecordListW *recordList);

	int AddFile(const wchar_t *filename);

	DISPATCH_CODES
	{
		API_MLDB_GETFILE = 10,
		API_MLDB_GETALBUM = 20,
		API_MLDB_QUERY = 30,
		API_MLDB_FREERECORD = 40,
		API_MLDB_FREERECORDLIST = 50,
		API_MLDB_SETFIELD = 60,
		API_MLDB_SYNC = 70,
		API_MLDB_ADDFILE = 80,
	};
};

inline itemRecordW *api_mldb::GetFile(const wchar_t *filename)
{
	return _call(API_MLDB_GETFILE, (itemRecordW *)0, filename);
}

inline itemRecordListW *api_mldb::GetAlbum(const wchar_t *albumname, const wchar_t *albumartist)
{
	return _call(API_MLDB_GETALBUM, (itemRecordListW *)0, albumname, albumartist);
}

inline itemRecordListW *api_mldb::Query(const wchar_t *query)
{
	return _call(API_MLDB_QUERY, (itemRecordListW *)0, query);
}

inline void api_mldb::FreeRecord(itemRecordW *record)
{
	_voidcall(API_MLDB_FREERECORD, record);
}

inline void api_mldb::FreeRecordList(itemRecordListW *recordList)
{
	_voidcall(API_MLDB_FREERECORDLIST, recordList);
}


inline void api_mldb::SetField(const wchar_t *filename, const char *field, const wchar_t *value)
{
	_voidcall(API_MLDB_SETFIELD, filename, field, value);
}

inline void api_mldb::Sync()
{
		_voidcall(API_MLDB_SYNC);
}

inline int api_mldb::AddFile(const wchar_t *filename)
{
	return _call(API_MLDB_ADDFILE, (int)0, filename);
}

// {5A94DABC-E19A-4a12-9AA8-852D8BF06532}
static const GUID mldbApiGuid = 
{ 0x5a94dabc, 0xe19a, 0x4a12, { 0x9a, 0xa8, 0x85, 0x2d, 0x8b, 0xf0, 0x65, 0x32 } };


#endif