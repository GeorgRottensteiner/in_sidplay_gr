/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Double-Linked List Class Prototypes

--------------------------------------------------------------------------- */

#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H

class LinkedListEntry
	{
	friend class LinkedList;
	friend class Table;
	friend class Scanner;

	public:
		int Int;
		LinkedListEntry *Next;
		LinkedListEntry *Previous;
	public:
		LinkedListEntry *GetNext();
		LinkedListEntry *GetPrevious();
		LinkedListEntry();
		virtual ~LinkedListEntry();
		virtual void SetDeletable(void);

	};

template <class T> class VListEntry : public LinkedListEntry 
  {
  public:
    void SetVal(T val) 
      {
      Val = val;
      }
    T GetVal(void) 
      {
      return Val;
      }

  private:
    T Val;
  };

template <class T> class PListEntry : public LinkedListEntry 
  {
  public:
    void SetVal(T *val) 
      {
      Val = val;
      }
    T *GetVal(void) 
      {
      return Val;
      }

  private:
    T *Val;
  };


typedef bool (*WalkListProc)(LinkedListEntry *Entry, int, void*, void*);

class LinkedList
	{
	friend class Record;
	friend class Table;
	friend class Scanner;

	protected:
		int NElements;
		LinkedListEntry *Head;
		LinkedListEntry *Foot;
		LinkedList();
		~LinkedList();
		void AddEntry(LinkedListEntry *Entry, BOOL Cat);
		void RemoveEntry(LinkedListEntry *Entry);

	public:
		void WalkList(WalkListProc WalkProc, int ID, void *Data1, void *Data2);
		int GetNElements(void) { return NElements; }
		LinkedListEntry *GetHead(void) { return Head; }
		LinkedListEntry *GetFoot(void) { return Foot; }
  };

#endif