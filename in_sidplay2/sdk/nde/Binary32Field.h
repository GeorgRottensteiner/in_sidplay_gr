/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Binary32Field Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __NDE_BINARY32FIELD_H
#define __NDE_BINARY32FIELD_H

#include "Field.h"

/* You can use this base class if you don't want to copy data, e.g. for writing to the database */
class Binary32FieldReadOnly : public Field
{
public:
	Binary32FieldReadOnly();
	Binary32FieldReadOnly(char *Data, size_t len);
	virtual ~Binary32FieldReadOnly();
	char *GetData(int *len);
	char *GetData(size_t *len);

protected:
	virtual void WriteTypedData(unsigned char *, size_t len);
	virtual size_t GetDataSize(void);
	virtual int Compare(Field *Entry);
	virtual bool ApplyFilter(Field *Data, int flags);
	void InitField(void);
	char *Data;
	size_t Size;
};

class Binary32Field : public Binary32FieldReadOnly
{
	friend Field *TranslateObject(unsigned char Type);
	friend class LinkedList;
	friend class Table;

protected:
	virtual void ReadTypedData(unsigned char *, size_t len);

public:
	virtual ~Binary32Field();
	Binary32Field(char *Data, size_t len);
	Binary32Field();
	
	void SetData(char *Data, size_t len);
};

#endif
