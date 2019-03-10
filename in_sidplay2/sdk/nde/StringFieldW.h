/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 StringFieldW Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __STRINGFIELDW_H
#define __STRINGFIELDW_H

class StringFieldW : public Field
	{
	friend Field *TranslateObject(unsigned char Type, Table *tbl);
	friend LinkedList;
	friend Table;

	protected:

		virtual void ReadTypedData(unsigned char *, int len);
		virtual void WriteTypedData(unsigned char *, int len);
		virtual int GetDataSize(void);
		virtual int Compare(Field *Entry);
    virtual bool ApplyFilter(Field *Data, int op);
		void InitField(void);
		wchar_t *String;

	public:

		~StringFieldW();
		StringFieldW(const wchar_t *Str);
		StringFieldW();
		wchar_t *GetString(void);
		void SetString(const wchar_t *Str);
		void SetString(const char *Str); 
	};

#endif
