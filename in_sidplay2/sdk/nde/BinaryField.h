/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 BinaryField Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __BINARYFIELD_H
#define __BINARYFIELD_H

#include "Field.h"

class BinaryField : public Field
{
	friend Field *TranslateObject(unsigned char Type);
	friend class LinkedList;
	friend class Table;

protected:
	virtual void ReadTypedData(unsigned char *, size_t len);
	virtual void WriteTypedData(unsigned char *, size_t len);
	virtual size_t GetDataSize(void);
	virtual int Compare(Field *Entry);
	virtual bool ApplyFilter(Field *Data, int flags);
	void InitField(void);
	char *Data;
	size_t Size;

public:
	~BinaryField();
	BinaryField(char *Data, int len);
	BinaryField();
	char *GetData(int *len);
	char *GetData(size_t *len);
	void SetData(char *Data, size_t len);
};

#endif
