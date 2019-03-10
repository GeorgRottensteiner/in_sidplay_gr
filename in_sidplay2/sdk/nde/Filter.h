/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
	
	Filter Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __FILTER_H
#define __FILTER_H

class Filter : public LinkedListEntry
	{
	friend class Table;
	friend class LinkedList;
	friend class Scanner;
	public:
		Field* DataField;
		unsigned char Op;
		unsigned char Id;
    Filter(unsigned char _Op);
		Filter(Field *Data, unsigned char Id, unsigned char Op);
    void SetData(Field *data);
		~Filter() ;

	public:
		unsigned char GetOp(void);
		void SetOp(unsigned char Op);
		Field *Data(void);
		int GetId(void);
	};

#endif
