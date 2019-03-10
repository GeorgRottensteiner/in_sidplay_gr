/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 IndexField Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __INDEXFIELD_H
#define __INDEXFIELD_H

class IndexField : public Field
	{
	friend Field *TranslateObject(unsigned char Type, Table *tbl);
	friend class LinkedList;
	friend class Table;
	friend class Index;
	friend class Scanner;

	protected:

		IndexField(unsigned char id, int Pos, int type, char *FieldName);
		IndexField();
		~IndexField();
		virtual void ReadTypedData(unsigned char *, size_t len);
		virtual void WriteTypedData(unsigned char *, size_t len);
		virtual size_t GetDataSize(void);
		virtual int Compare(Field *Entry);
		void InitField(void);
		Index *index;
		int Position;
		int DataType;
		char *Name;
		int CurrentRecordPos;
	
	public:
		
		char *GetIndexName(void);
		int TranslateToIndex(int Id, IndexField *index);
	};

#endif