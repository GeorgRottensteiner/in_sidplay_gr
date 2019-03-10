/* ---------------------------------------------------------------------------
Nullsoft Database Engine
--------------------
codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

Double-Linked List Class

Caution: The entire library relies on this base class. Modify with care!

--------------------------------------------------------------------------- */

#include "NDE.h"

//---------------------------------------------------------------------------
LinkedListEntry *LinkedListEntry::GetNext(void)
{
	return Next;
}

//---------------------------------------------------------------------------
LinkedListEntry *LinkedListEntry::GetPrevious(void)
{
	return Previous;
}

//---------------------------------------------------------------------------
LinkedListEntry::LinkedListEntry()
{
	Int=0;
	Next=NULL;
	Previous=NULL;
}

//---------------------------------------------------------------------------
LinkedListEntry::~LinkedListEntry()
{
}

//---------------------------------------------------------------------------
void LinkedListEntry::SetDeletable(void)
{
	// do nothing by default
}

//---------------------------------------------------------------------------
LinkedList::LinkedList()
{
	Head = NULL;
	Foot = NULL;
	NElements=0;
}

//---------------------------------------------------------------------------
LinkedList::~LinkedList()
{
	while (Head)
		RemoveEntry(Head);
}

//---------------------------------------------------------------------------
void LinkedList::AddEntry(LinkedListEntry *Entry, BOOL Cat)
{
#ifdef WIN32
	//EnterCriticalSection(&Critical);
#endif
	if (!Cat)
	{
		if (Head)
			Head->Previous = Entry;
		Entry->Next = Head;
		Entry->Previous = NULL;
		Head = Entry;
		if (!Foot)
			Foot = Entry;
	}
	else
	{
		if (Foot)
			Foot->Next = Entry;
		Entry->Previous = Foot;
		Entry->Next = NULL;
		Foot = Entry;
		if (!Head)
			Head = Entry;
	}
	NElements++;
#ifdef WIN32
	//LeaveCriticalSection(&Critical);
#endif
}

//---------------------------------------------------------------------------
void LinkedList::RemoveEntry(LinkedListEntry *Entry)
{
	if (!Entry) return;
#ifdef WIN32
	//EnterCriticalSection(&Critical);
#endif
	if (Entry->Next)
		Entry->Next->Previous = Entry->Previous;
	else
		Foot = Entry->Previous;
	if (Entry->Previous)
		Entry->Previous->Next = Entry->Next;
	else
		Head = Entry->Next;

	Entry->SetDeletable(); // so we can override some descendants destructors to throw an exception when people are not authorized to delete them
	// we, on the other hand, are friends, so calling this protected will give us the right to delete them
	delete Entry;

	NElements--;
#ifdef WIN32
	//LeaveCriticalSection(&Critical);
#endif
}

//---------------------------------------------------------------------------
void LinkedList::WalkList(WalkListProc WalkProc, int ID, void *Data1, void *Data2)
{
	if (!WalkProc) return;
	LinkedListEntry *Entry = Head;
	while (Entry)
	{
		if (!WalkProc(Entry, ID, Data1, Data2)) break;
		Entry = Entry->Next;
	}
}

