/* ---------------------------------------------------------------------------
                           Nullsoft Database Engine
                             --------------------
                        codename: Near Death Experience
--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------

 Raw Index Class

--------------------------------------------------------------------------- */

#include "NDE.h"
#include <stdio.h>


char *iSign="NDEINDEX";

//---------------------------------------------------------------------------
Index::Index(VFILE *H, unsigned char id, int pos, int type, BOOL newindex, int nentries, Table *parentTable)
{
	Handle = H;
	IndexTable = NULL;
	MaxSize=0;
	Modified = FALSE;
	locateUpToDate = FALSE;
	Id = id;
	Position = pos;
	DataType = type;
	SecIndex = NULL;
	InChain=FALSE;
	InInsert=FALSE;
	InChainIdx = -1;
	pTable = parentTable;
	TableHandle = pTable->Handle;
	SetGlobalLocateUpToDate(FALSE);

	if (!newindex)
	{
		Vfseek(Handle, (int)strlen(__INDEX_SIGNATURE__), SEEK_SET);
		Vfread(&NEntries, sizeof(NEntries), 1, Handle);
	}
	else
		NEntries = nentries;
	LoadIndex(newindex);
}

//---------------------------------------------------------------------------
Index::~Index()
{
	if (IndexTable)
		free(IndexTable);
}

//---------------------------------------------------------------------------
int Index::Insert(int N)
{
	return Insert(NULL, N, FALSE);
}

//---------------------------------------------------------------------------
int Index::Insert(Index *parindex, int N, BOOL localonly)
{
	if (InChain) return -1;
	Index *p = parindex;
	IndexField *f;
	int pidx = -1;
	Modified=TRUE;
	locateUpToDate = FALSE;
	SetGlobalLocateUpToDate(FALSE);
	InInsert=TRUE;
	if (N > NEntries) N = NEntries;
	if ((int)(NEntries+1) > MaxSize)
	{
		int *newBlock;
		MaxSize *=2;//+= BLOCK_SIZE;
		newBlock = (int *)calloc(MaxSize, sizeof(int)*2);
		memcpy(newBlock, IndexTable, NEntries*sizeof(int)*2);
		free(IndexTable);
		IndexTable = newBlock;
	}

	if (N < NEntries && Id == PRIMARY_INDEX)
	{
		memmove(IndexTable+(N+1)*2, IndexTable+(N*2), (NEntries-N)*sizeof(int)*2);
		NEntries++;
	}
	else
	{
		N=NEntries;
		NEntries++;
	}

	Update(N, 0, NULL, localonly);

	// Should be always safe to cat the new record since if we are primary index,
	// then secondary is sorted, so value will be moved at update
	// if we are a secondary index, then an insertion will insert at the end of the primary index anyway
	if (!localonly && SecIndex)
	{
		int pp;
		InChain=TRUE;
		pp = SecIndex->index->Insert(this, N, FALSE);
		InChain=FALSE;
		IndexTable[N*2+1] = pp == -1 ? N : pp;
		if (N < NEntries-1 && Id == PRIMARY_INDEX)
		{
			if (!parindex)
			{
				int v = pp;
				f = SecIndex;
				if (f)
				{
					while (f->index->SecIndex->index != this)
					{
						v = f->index->GetCooperative(v);
						f = f->index->SecIndex;
					}
					p = f->index;
					pidx = v;
				}
			}
			if (p && pidx != -1)
			{
				p->SetCooperative(pidx, N);
				UpdateMe(p, N, NEntries);
			}
		}
	}

	InInsert=FALSE;
	return N;
}

//---------------------------------------------------------------------------
void Index::LoadIndex(BOOL newindex)
{
	if (IndexTable)
		free(IndexTable);
	MaxSize = ((NEntries) / BLOCK_SIZE + 1) * BLOCK_SIZE;
	IndexTable = (int *)calloc(MaxSize, sizeof(int)*2);
	if (!newindex)
	{
		Vfseek(Handle, (int)strlen(__INDEX_SIGNATURE__)+4+((NEntries*4*2)+4)*(Position+1), SEEK_SET);
		int v;
		Vfread(&v, sizeof(v), 1, Handle);
		Id = (unsigned char)v;
		Vfread(IndexTable, NEntries*2, sizeof(int), Handle);
	}
}

//---------------------------------------------------------------------------
int Index::Update(int Idx, int Pos, Record *record, BOOL localonly)
{
return Update(NULL, -1, Idx, Pos, record, FALSE, localonly);
}

//---------------------------------------------------------------------------
int Index::Update(Index *parindex, int paridx, int Idx, int Pos, Record *record, BOOL forceLast, BOOL localonly)
{
	int NewIdx=Idx;
	int oldSecPtr;
	int state;
	if (InChain) return InChainIdx;
	oldSecPtr = IndexTable[Idx*2+1];
	if (!forceLast && Id == PRIMARY_INDEX || record == NULL || Idx < 2)
	{
		if (Idx < NEntries && Idx >= 0)
		{
			IndexTable[Idx*2] = Pos;
			if (!localonly && SecIndex && SecIndex->index != this && !InInsert)
			{
				InChain=TRUE;
				InChainIdx = Idx;
				SecIndex->index->Update(this, Idx, IndexTable[Idx*2+1], Pos, record, forceLast, FALSE);
				InChainIdx = -1;
				InChain=FALSE;
			}
		}
		else
		{
#ifdef WIN32
			MessageBox(NULL, "Updating outside range", "Oops", 16);
#else
			printf("NDE Error: updating outside range!\n");
#endif
		}
	}
	else
	{
		if (forceLast)
			NewIdx = NEntries;
		else
			if (Pos != Get(Idx) || Id != PRIMARY_INDEX)
				NewIdx = FindSortedPlace(record->GetField(Id), Idx, &state, QFIND_ENTIRE_SCOPE);
		if (NewIdx <= NEntries && NewIdx >= 2)
		{
			if (NewIdx != Idx)
			{
				Index *p = parindex;
				IndexField *f;
				int pidx = paridx;
				NewIdx = MoveIndex(Idx, NewIdx);
				if (SecIndex->index != this)
				{
					if (!parindex)
					{
						int v = GetCooperative(NewIdx);
						f = SecIndex;
						if (f)
						{
							while (f->index->SecIndex->index != this)
							{
								v = f->index->GetCooperative(v);
								f = f->index->SecIndex;
							}
							p = f->index;
							pidx = v;
						}
					}
					if (p)
					{
						p->SetCooperative(pidx, NewIdx);
						UpdateMe(p, NewIdx, Idx);
					}
				}
			}
			IndexTable[NewIdx*2] = Pos;
			if (!localonly && SecIndex && SecIndex->index != this && !InInsert) // Actually, we should never be InInsert and here, but lets cover our ass
			{
				InChain=TRUE;
				InChainIdx = oldSecPtr;
				SecIndex->index->Update(this, NewIdx, oldSecPtr, Pos, record, forceLast, FALSE);
				InChainIdx = -1;
				InChain=FALSE;
			}
		}
		else
		{
#ifdef WIN32
			MessageBox(NULL, "QSort failed and tried to update index outside range", "Oops", 16);
#else
			printf("NDE Error: qsort failed and tried to update index outside range!\n");
#endif
		}
	}
	Modified=TRUE;
	locateUpToDate = FALSE;
	SetGlobalLocateUpToDate(FALSE);
	pTable->IndexModified();
	return NewIdx;
}

//---------------------------------------------------------------------------
int Index::Get(int Idx)
{
	if (Idx < NEntries)
		return IndexTable[Idx*2];
	else
	{
#ifdef WIN32
		MessageBox(NULL, "Requested index outside range", "Oops", 16);
#else
		printf("NDE Error: requested index outside range!\n");
#endif
		return -1;
	}
}

//---------------------------------------------------------------------------
void Index::Set(int Idx, int P)
{
	if (Idx < NEntries)
		IndexTable[Idx*2]=P;
	else
	{
#ifdef WIN32
		MessageBox(NULL, "Updating index outside range", "Oops", 16);
#else
		printf("NDE Error: updating index outside range!\n");
#endif
	}
}

//---------------------------------------------------------------------------
void Index::WriteIndex(void)
{
	if (Id == PRIMARY_INDEX)
	{
		Vfseek(Handle, (int)strlen(__INDEX_SIGNATURE__), SEEK_SET);
		Vfwrite(&NEntries, sizeof(NEntries), 1, Handle);
	}
	Vfseek(Handle, (int)strlen(__INDEX_SIGNATURE__)+4+((NEntries*4*2)+4)*(Position+1), SEEK_SET);
	int v=(int)Id;
	Vfwrite(&v, sizeof(v), 1, Handle);
	Vfwrite(IndexTable, NEntries*2, sizeof(int), Handle);
	Modified=FALSE;
}

//---------------------------------------------------------------------------
int Index::MoveIndex(int idx, int newidx)
{
	if (idx == newidx)
		return newidx;
	int oldPos=IndexTable[idx*2], oldPtr=IndexTable[idx*2+1];

	if (NEntries > idx+1)
		memmove(IndexTable+idx*2, IndexTable+(idx+1)*2, (NEntries-idx)*sizeof(int)*2);
	if (newidx > idx)
		newidx--;
	if (NEntries > newidx)
		memmove(IndexTable+(newidx+1)*2, IndexTable+newidx*2, (NEntries-newidx-1)*sizeof(int)*2);
	IndexTable[newidx*2] = oldPos;
	IndexTable[newidx*2+1] = oldPtr;
	return newidx;
}

//---------------------------------------------------------------------------
void Index::Colaborate(IndexField *secindex)
{
int i;

SecIndex = secindex;

for (i=0;i<2;i++)
  /*secindex->index->*/SetCooperative(i, i);

}

//---------------------------------------------------------------------------
void Index::Propagate(void)
{
	int i,p;
	Record *rec;

	if (!SecIndex || SecIndex->ID == PRIMARY_INDEX) return;
	SecIndex->index->NEntries=2;
	for (i=0;i<2;i++)
	{
		SecIndex->index->Set(i, Get(i));
		SecIndex->index->SetCooperative(i, GetCooperative(i));
	}

	Scanner *s = pTable->NewScanner(FALSE, FALSE);
	if (!s)
	{
#ifdef WIN32
		MessageBox(NULL, "Failed to create a scanner in reindex", "Oops", 16);
#else
		printf("NDE Error: failed to create a scanner in reindex!\n");
#endif
		return;
	}

	int *coopSave = (int *)malloc(sizeof(int)*NEntries);
	if (!coopSave)
	{
#ifdef WIN32
		MessageBox(NULL, "Alloc failed in reindex", "Oops", 16);
#else
		printf("NDE Error: alloc failed in reindex!\n");
#endif
		return;
	}

	for (i=0;i<NEntries;i++)
	{
		coopSave[i] = GetCooperative(i);
		SetCooperative(i, i);
	}

	if (SecIndex->index->SecIndex->index->Id != PRIMARY_INDEX)
	{
#ifdef WIN32
		MessageBox(NULL, "Propagating existing index", "Oops", 16);
#else
		printf("NDE Error: propagating existing index!\n");
#endif
		return;
	}

	s->SetWorkingIndexById(-1);

	for (i=2;i<NEntries;i++)
	{
		rec = s->GetRecord(coopSave[i]);
		if (rec)
		{
			SecIndex->index->NEntries++;
			//    SecIndex->index->Insert(NULL, i, TRUE);
			SecIndex->index->SetCooperative(i, coopSave[i]);
			SecIndex->index->Set(i, Get(i));
			p = SecIndex->index->Update(i, SecIndex->index->Get(i), rec, TRUE);
			/*    SecIndex->index->SetCooperative(i, q);*/
			if (i % 32) Sleep(1);
			delete rec;
		}
		else
		{
#ifdef WIN32
			MessageBox(NULL, "Unable to read record in reindex", "Oops", 16);
#else
			printf("NDE Error: unable to read record in reindex!\n");
#endif
		}
	}

	free(coopSave);

	if (NEntries != SecIndex->index->NEntries) {
#ifdef WIN32
		MessageBox(NULL, "Secindex->NEntries desynchronized in reindex", "Oops", 16);
#else
		printf("NDE Error: Secindex->NEntries desynchronized in reindex!\n");
#endif
	}

	pTable->DeleteScanner(s);
}

//---------------------------------------------------------------------------
void Index::SetCooperative(int Idx, int secpos)
{
if (Idx < NEntries && Idx >= 0)
  IndexTable[Idx*2+1] = secpos;
else
  {
  #ifdef WIN32
  MessageBox(NULL, "Cooperative update outside range", "Oops", 16);
  #else
  printf("NDE Error: cooperative update outside range!\n");
  #endif
  }
}

//---------------------------------------------------------------------------
int Index::GetCooperative(int Idx)
{
if (Idx < NEntries && Idx >= 0)
  return IndexTable[Idx*2+1];
else
  {
  #ifdef WIN32
  MessageBox(NULL, "Cooperative request outside range", "Oops", 16);
  #else
  printf("NDE Error: cooperative request outside range!\n");
  #endif
  }
return -1;
}

//---------------------------------------------------------------------------
int Index::NeedFix() {
  for (int i=2;i<NEntries;i++) {
    if (IndexTable[i*2+1] <= 0) return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
void Index::UpdateMe(Index *Me, int NewIdx, int OldIdx)
{
	int i;
	int j=NewIdx > OldIdx ? -1 : 1;
	int v;
	IndexField *f;

	for (i=min(NewIdx, OldIdx);i<=max(NewIdx, OldIdx)&&i<NEntries;i++)
	{
		if (i == NewIdx) continue;
		v = GetCooperative(i);
		f = SecIndex;
		while (f->index->SecIndex->index != this)
		{
			v = f->index->GetCooperative(v);
			f = f->index->SecIndex;
		}
		Me->SetCooperative(v, Me->GetCooperative(v)+j);
	}
}

//---------------------------------------------------------------------------
Field *Index::QuickFindField(unsigned char Idx, int Pos)
{
	int ThisPos = Pos;
	Field *Entry=NULL;

	while (ThisPos)
	{
		Vfseek(TableHandle, ThisPos, SEEK_SET);
		Entry = new Field(ThisPos, pTable);
		Entry->ReadField(ThisPos, TRUE);
		if (Entry->GetFieldId() == Idx)
			break;
		ThisPos = Entry->GetNextFieldPos();
		Entry->SetDeletable();
		delete Entry;
		Entry = NULL;
	}
	if (!ThisPos || !Entry)
	{
		if (Entry)
		{
			Entry->SetDeletable();
			delete Entry;
		}
		return NULL;
	}
	Entry->SetTable(pTable);
	Field *TypedEntry = Entry->ReadField(ThisPos);
	Entry->SetDeletable();
	delete Entry;
	return TypedEntry;
}

//---------------------------------------------------------------------------
// Dynamic qsort (i definitly rule)
int Index::FindSortedPlace(Field *thisField, int curIdx, int *laststate, int start)
{
	return FindSortedPlaceEx(thisField, curIdx, laststate, start, COMPARE_MODE_EXACT);
}

//---------------------------------------------------------------------------
// and here again ugly switch 
int Index::FindSortedPlaceEx(Field *thisField, int curIdx, int *laststate, int start, int comp_mode)
{
	int top=start != QFIND_ENTIRE_SCOPE ? start : 2;
	int bottom=NEntries-1;
	int compEntry;
	int cePos;
	Field *compField=NULL;
	int i;
	Field *cfV;

	if (NEntries < 3) return 2;
	switch(comp_mode)
	{
		case COMPARE_MODE_EXACT:
			while (bottom-top >= 1)
			{
				compEntry=(bottom-top)/2+top;
				if (compEntry == curIdx) compEntry++;
				if (compEntry == bottom) break;
				cePos = Get(compEntry);
				if (!cePos)  bottom = compEntry;
				else
				{
					compField = QuickFindField(Id, Get(compEntry));
					cfV = compField;
					if (!thisField)
					{
						if (!compField)	i = 0;
						else i = 1;
					}
					else i = thisField->Compare(cfV);
					switch (i)
					{
						case 1:
							top = compEntry+1;
							break;
						case -1:
							bottom = compEntry-1;
							break;
						case 0:
							*laststate=0;
							if (compField)
							{
								compField->SetDeletable();
								delete compField;
							}
							return compEntry+1;
					}
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
				}
			}
			compEntry=(bottom-top)/2+top;
			if (compEntry == curIdx) return curIdx;
			compField = QuickFindField(Id, Get(compEntry));
			cfV = compField;
			if (thisField)  *laststate = thisField->Compare(cfV);
			else
			{
				if (!compField) *laststate = 0;
				else *laststate = 1;
			}
			break;

		case COMPARE_MODE_CONTAINS:
			while (bottom-top >= 1)
			{
				compEntry=(bottom-top)/2+top;
				if (compEntry == curIdx) compEntry++;
				if (compEntry == bottom) break;
				cePos = Get(compEntry);
				if (!cePos)  bottom = compEntry;
				else
				{
					compField = QuickFindField(Id, Get(compEntry));
					cfV = compField;
					if (!thisField)
					{
						if (!compField)	i = 0;
						else i = 1;
					}
					else i = thisField->Contains(cfV);
					switch (i)
					{
						case 1:
							top = compEntry+1;
							break;
						case -1:
							bottom = compEntry-1;
							break;
						case 0:
							*laststate=0;
							if (compField)
							{
								compField->SetDeletable();
								delete compField;
							}
							return compEntry+1;
					}
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
				}
			}
			compEntry=(bottom-top)/2+top;
			if (compEntry == curIdx) return curIdx;
			compField = QuickFindField(Id, Get(compEntry));
			cfV = compField;
			if (thisField)  *laststate = thisField->Contains(cfV);
			else
			{
				if (!compField) *laststate = 0;
				else *laststate = 1;
			}
			break;

		case COMPARE_MODE_STARTS:
			while (bottom-top >= 1)
			{
				compEntry=(bottom-top)/2+top;
				if (compEntry == curIdx) compEntry++;
				if (compEntry == bottom) break;
				cePos = Get(compEntry);
				if (!cePos)  bottom = compEntry;
				else
				{
					compField = QuickFindField(Id, Get(compEntry));
					cfV = compField;
					if (!thisField)
					{
						if (!compField)	i = 0;
						else i = 1;
					}
					else i = thisField->Starts(cfV);
					switch (i)
					{
						case 1:
							top = compEntry+1;
							break;
						case -1:
							bottom = compEntry-1;
							break;
						case 0:
							*laststate=0;
							if (compField)
							{
								compField->SetDeletable();
								delete compField;
							}
							return compEntry+1;
					}
					if (compField)
					{
						compField->SetDeletable();
						delete compField;
					}
				}
			}
			compEntry=(bottom-top)/2+top;
			if (compEntry == curIdx) return curIdx;
			compField = QuickFindField(Id, Get(compEntry));
			cfV = compField;
			if (thisField)  *laststate = thisField->Starts(cfV);
			else
			{
				if (!compField) *laststate = 0;
				else *laststate = 1;
			}
			break;

	}
	switch (*laststate)
	{
		case -1:
			if (compField)
			{
				compField->SetDeletable();
				delete compField;
			}
			return compEntry;
		case 1:
		case 0:
			if (compField)
			{
				compField->SetDeletable();
				delete compField;
			}
			return /*compEntry==NEntries-1 ? compEntry : */compEntry+1;
	}
	return 2; // we're not supposed to be here :/
}

int Index::QuickFind(int Id, Field *field, int start)
{
	return QuickFindEx(Id, field, start, COMPARE_MODE_EXACT);
}

//---------------------------------------------------------------------------
int Index::QuickFindEx(int Id, Field *field, int start, int comp_mode)
{
	int laststate;
	Field *compField=NULL, *cfV;
	int i = FindSortedPlaceEx(field, Id, &laststate, start, comp_mode)-1; // -1 because we don't insert but just search
	if (laststate != 0)
		return FIELD_NOT_FOUND;
	if (i < start)
		return FIELD_NOT_FOUND;

	switch(comp_mode)
	{
	case COMPARE_MODE_CONTAINS:
		while (--i>=2)
		{
			compField = QuickFindField(Id, Get(i));
			cfV = compField;
			if (field->Contains(cfV) != 0)
			{
				if (compField)
				{
					compField->SetDeletable();
					delete compField;
				}
				return i+1;
			}
			if (compField)
			{
				compField->SetDeletable();
				delete compField;
			}
		}
		break;
	case COMPARE_MODE_EXACT:
		while (--i>=2)
		{
			compField = QuickFindField(Id, Get(i));
			cfV = compField;
			if (field->Compare(cfV) != 0)
			{
				if (compField)
				{
					compField->SetDeletable();
					delete compField;
				}
				return i+1;
			}
			if (compField)
			{
				compField->SetDeletable();
				delete compField;
			}
		}
		break;
	case COMPARE_MODE_STARTS:
		while (--i>=2)
		{
			compField = QuickFindField(Id, Get(i));
			cfV = compField;
			if (field->Starts(cfV) != 0)
			{
				if (compField)
				{
					compField->SetDeletable();
					delete compField;
				}
				return i+1;
			}
			if (compField)
			{
				compField->SetDeletable();
				delete compField;
			}
		}
		break;
	}

	return i+1;
}

//---------------------------------------------------------------------------
int Index::TranslateIndex(int Pos, Index *index)
{
int v;
IndexField *f;

v = GetCooperative(Pos);
f = SecIndex;
while (f->index != this && f->index != index)
    {
    v = f->index->GetCooperative(v);
    f = f->index->SecIndex;
    }
return v;
}

//---------------------------------------------------------------------------
void Index::Delete(int Idx, int Pos, Record *record)
{
	Update(NULL, -1, Idx, Pos, record, TRUE, FALSE);
	Shrink();
}

//---------------------------------------------------------------------------
void Index::Shrink(void)
{
	if (InChain) return;
	if (SecIndex && SecIndex->index != this) // Actually, we should never be InInsert and here, but lets cover our ass
	{
		InChain=TRUE;
		SecIndex->index->Shrink();
		InChain=FALSE;
	}
	NEntries--;
}

//---------------------------------------------------------------------------
void Index::SetGlobalLocateUpToDate(BOOL isUptodate) {
  if (!pTable) return;
  pTable->SetGlobalLocateUpToDate(isUptodate);
}

unsigned char Index::GetId() {
  return Id;
}
