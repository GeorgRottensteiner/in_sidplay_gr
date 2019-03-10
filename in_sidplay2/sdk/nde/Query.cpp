#include "NDE.h"
#include "../nu/AutoWide.h"


//---------------------------------------------------------------------------
BOOL Scanner::Query(const char *query)
{
	return Query(AutoWide(query));
}

BOOL Scanner::Query(const wchar_t *query)
{
	if (!query) return FALSE;
	if (last_query) free(last_query);
	last_query = wcsdup(query);
	RemoveFilters();
	in_query_parser = 1;
	BOOL r = Query_Parse(query);
	
	if (r == FALSE)
	{
		if (!disable_date_resolution) RemoveFilters();
		last_query_failed = TRUE;
	}
	in_query_parser = 0;
	Query_CleanUp();
	return r & CheckFilters();
}

const wchar_t *Scanner::GetLastQuery()
{
	return last_query;
}

typedef struct
{
	wchar_t *token;
	int tid;
} tokenstruct;

tokenstruct Tokens[] =   // Feel free to add more...
{
	{L"AND", TOKEN_AND },
	{L"OR", TOKEN_OR },
	{L"HAS", TOKEN_CONTAINS },
	{L"NOTHAS",TOKEN_NOTCONTAINS},
	{L"BEGINS", TOKEN_BEGINS },
	{L"ENDS", TOKEN_ENDS },
	{L"ISEMPTY", TOKEN_ISEMPTY},
	{L"ISNOTEMPTY",TOKEN_ISNOTEMPTY},
	{L"LIKE", TOKEN_LIKE},
	{L"BEGINSLIKE", TOKEN_BEGINSLIKE},
};

typedef struct
{
	int Op;
	int Level;
} OpLevel;

/*
    our state machine

                               &, |
            ----------<-------------------------<----------------------<-----------
           |                                                                       |
           v    ID (Col)        =, >, <...       ID / Data / [f]          )        |
     ---->(0) ----->-----> (1) ------>-----> (2) ------>------> (3) ------>-----> (4) <--
    |     |^                \isempty------------->------------/  |^                |     |
    | !(  ||                                                     ||----            |  )  |
     --<--  ---------<---------------------------<-------------<-|     |            -->--
                               &, |                              v [f] |
                                                                  -->--

*/

//---------------------------------------------------------------------------
BOOL Scanner::Query_Parse(const wchar_t *query)
{
	const wchar_t *p = query; // pointer on next token to read
	int size;
	int state = 0;
	int pcount = 0;
	VListEntry<OpLevel> *entry;

	if (pstack.GetNElements() > 0)
		Query_CleanUp();

	while (1)
	{
		p = Query_EatSpace(p);
		int t = Query_GetNextToken(p, &size, &token);
		if (t == TOKEN_UNKNOWN)
		{
			Query_SyntaxError((int)(p-query));
			return FALSE;
		}
		if (t == TOKEN_EOQ)
			break;
		switch (state)
		{
		case 0:
			switch (t)
			{
			case TOKEN_PAROPEN:
				state = 0;
				// check too many parenthesis open
				if (pcount == 255)
				{
					Query_SyntaxError((int)(p-query)); // should not be _syntax_ error
					return FALSE;
				}
				// up one level
				pcount++;
				break;
			case TOKEN_NOT:
				// push not in this level
				OpLevel o;
				o.Op = FILTER_NOT;
				o.Level = pcount;
				entry = new VListEntry<OpLevel>;
				entry->SetVal(o);
				pstack.AddEntry(entry, TRUE);
				state = 0;
				break;
			case TOKEN_IDENTIFIER:
				state = 1;
				// create filter column
				if (AddFilterByName(token, NULL, FILTER_NONE) == ADDFILTER_FAILED)
				{
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}
				break;
			default:
				Query_SyntaxError((int)(p-query));
				return FALSE;
			}
			break;
		case 1:
			switch (t)
			{
			case TOKEN_EQUAL:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_EQUALS);
				break;
			}
			case TOKEN_ABOVE:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_ABOVE);
				break;
			}
			case TOKEN_BELOW:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_BELOW);
				break;
			}
			case TOKEN_CONTAINS:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_CONTAINS);
				break;
			}
			case TOKEN_NOTCONTAINS:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_NOTCONTAINS);
				break;
			}
			case TOKEN_AOREQUAL:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_ABOVEOREQUAL);
				break;
			}
			case TOKEN_BOREQUAL:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_BELOWOREQUAL);
				break;
			}
			case TOKEN_NOTEQUAL:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_NOTEQUALS);
				break;
			}
			case TOKEN_BEGINS:
			{
				state = 2;
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_BEGINS);
			}
			break;
			case TOKEN_ENDS:
			{
				state = 2;
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_ENDS);
			}
			break;
			case TOKEN_LIKE:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_LIKE);
				break;
			}
			case TOKEN_BEGINSLIKE:
			{
				state = 2;
				// set filter op
				Filter *f = GetLastFilter();
				f->SetOp(FILTER_BEGINSLIKE);
				break;
			}
			case TOKEN_ISNOTEMPTY:
			case TOKEN_ISEMPTY:
			{
				state = 3;
				Filter *f = GetLastFilter();
				f->SetOp(t==TOKEN_ISEMPTY ? FILTER_ISEMPTY : FILTER_ISNOTEMPTY);
				// pop all operators in this level beginning by the last inserted
				entry = (VListEntry<OpLevel> *)pstack.GetFoot();
				while (entry)
				{
					if (entry->GetVal().Level == pcount)
					{
						AddFilterOp(entry->GetVal().Op);
						VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
						pstack.RemoveEntry(entry);
						entry = _entry;
					}
					else
						break;
				}
			}
			break;
			default:
				Query_SyntaxError((int)(p-query));
				return FALSE;
			}
			break;
		case 2:
			if (t == TOKEN_SQBRACKETOPEN)
			{
				state = 3;
				const wchar_t *s = wcschr(p, L']');
				if (!s)
				{
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}
				p = Query_EatSpace(p);
				if (*p == L'[') p++;
				wchar_t *format = (wchar_t *)malloc((s-p+1)*sizeof(wchar_t));
				wcsncpy(format, p, s-p);
				format[s-p] = 0;

				Filter *f = GetLastFilter();
				int id = f->GetId();
				ColumnField *c = GetColumnById(id);
				int tt = c ? c->GetDataType() : -1;
				if (disable_date_resolution || !c || (tt != FIELD_INTEGER && tt != FIELD_DATETIME && tt != FIELD_LENGTH && tt != FIELD_BOOLEAN))
				{

					if (disable_date_resolution)
					{
						StringField *field = (StringField *)f->Data();

						if (!field)
						{
							// format was used without a value, assume value is 0
							f->SetData(new StringField(L""));
							entry = (VListEntry<OpLevel> *)pstack.GetFoot();
							while (entry)
							{
								if (entry->GetVal().Level == pcount)
								{
									AddFilterOp(entry->GetVal().Op);
									VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
									pstack.RemoveEntry(entry);
									entry = _entry;
								}
								else
									break;
							}
							field = (StringField*)f->Data();
						}

						field->SetStringW(format);
						free(format);
						p = s+1;
						continue;
					}


					free(format);
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}

				IntegerField *field = (IntegerField *)f->Data();

				if (!field)
				{
					// format was used without a value, assume value is 0
					f->SetData(new IntegerField(0));
					entry = (VListEntry<OpLevel> *)pstack.GetFoot();
					while (entry)
					{
						if (entry->GetVal().Level == pcount)
						{
							AddFilterOp(entry->GetVal().Op);
							VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
							pstack.RemoveEntry(entry);
							entry = _entry;
						}
						else
							break;
					}
					field = (IntegerField *)f->Data();
				}

				int r = field->ApplyConversion(format);
				free(format);
				if (!r)
				{
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}
				p = s+1;
				continue;
			}
//        switch (t) {
			//      case TOKEN_IDENTIFIER:
			else   // JF> we make this relaxed, so anything is valid as a value
			{
				state = 3;
				// set filter data
				Filter *f = GetLastFilter();
				int id = f->GetId();
				ColumnField *c = GetColumnById(id);
				switch (c ? c->GetDataType() : -1)
				{
				case FIELD_DATETIME:
					if (disable_date_resolution)
						goto field_string_override;
				case FIELD_LENGTH:
				{
					int i;
					IntegerField *i_f = new IntegerField();
					i = _wtoi(token); // todo: Replace with own conversion and error checking
					wchar_t *p;
					if ((p=wcsstr(token,L":")))
					{
						i*=60;
						i+=_wtoi(++p);
						if ((p=wcsstr(p,L":")))
						{
							i*=60;
							i+=_wtoi(++p);
						}
					}
					i_f->SetValue(i);
					f->SetData(i_f);
				}
				break;

				case FIELD_BOOLEAN:
				case FIELD_INTEGER:
				{
					int i;
					IntegerField *i_f = new IntegerField();
					i = _wtoi(token); // todo: Replace with own conversion and error checking
					i_f->SetValue(i);
					f->SetData(i_f);
				}
				break;
				case FIELD_INT64:
				{
					int64_t i;
					Int64Field *i_f = new Int64Field();
					#ifdef _WIN32
					i = _wtoi64(token); // todo: Replace with own conversion and error checking
					#elif defined(__APPLE__)
					i = wcstoll(token, 0, 10);
					#else
					#error port me
					#endif
					i_f->SetValue(i);
					f->SetData(i_f);
				}
				break;
				case FIELD_FILENAME:
					{
						FilenameField *s_f = new FilenameField();
						s_f->SetStringW(token);
						f->SetData(s_f);
					}
					break;
				case FIELD_STRING:
field_string_override:
					{
						StringField *s_f = new StringField();
						s_f->SetStringW(token);
						f->SetData(s_f);
					}
					break;
				default:
					Query_SyntaxError((int)(p-query));
					return FALSE;
					break;

				}
				// pop all operators in this level beginning by the last inserted
				entry = (VListEntry<OpLevel> *)pstack.GetFoot();
				while (entry)
				{
					if (entry->GetVal().Level == pcount)
					{
						AddFilterOp(entry->GetVal().Op);
						VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
						pstack.RemoveEntry(entry);
						entry = _entry;
					}
					else
						break;
				}
				break;
			}
//          default:
			//          Query_SyntaxError(p-query);
			//        return FALSE;
//        }
			break;
		case 3:
			switch (t)
			{
			case TOKEN_SQBRACKETOPEN:
			{
				const wchar_t *s = wcschr(p, L']');
				if (!s)
				{
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}
				p = Query_EatSpace(p);
				if (*p == L'[') p++;
				wchar_t *format = (wchar_t *)malloc((s-p+1)*sizeof(wchar_t));
				wcsncpy(format, p, s-p);
				format[s-p] = 0;

				Filter *f = GetLastFilter();
				int id = f->GetId();
				ColumnField *c = GetColumnById(id);
				int tt = c ? c->GetDataType() : -1;
				if (disable_date_resolution || !c || (tt != FIELD_INTEGER && tt != FIELD_DATETIME && tt != FIELD_LENGTH && tt != FIELD_BOOLEAN))
				{
					if (disable_date_resolution)
					{
						StringField *field = (StringField *)f->Data();

						if (!field)
						{
							// format was used without a value, assume value is 0
							f->SetData(new StringField(L""));
							entry = (VListEntry<OpLevel> *)pstack.GetFoot();
							while (entry)
							{
								if (entry->GetVal().Level == pcount)
								{
									AddFilterOp(entry->GetVal().Op);
									VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
									pstack.RemoveEntry(entry);
									entry = _entry;
								}
								else
									break;
							}
							field = (StringField *)f->Data();
						}

						field->SetStringW(format);
						free(format);
						p = s+1;
						continue;
					}
					free(format);
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}

				IntegerField *field = (IntegerField *)f->Data();

				if (!field)
				{
					// format was used without a value, assume value is 0
					f->SetData(new IntegerField(0));
					entry = (VListEntry<OpLevel> *)pstack.GetFoot();
					while (entry)
					{
						if (entry->GetVal().Level == pcount)
						{
							AddFilterOp(entry->GetVal().Op);
							VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
							pstack.RemoveEntry(entry);
							entry = _entry;
						}
						else
							break;
					}
					field = (IntegerField *)f->Data();
				}

				int r = field->ApplyConversion(format);
				free(format);
				if (!r)
				{
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}
				p = s+1;
				continue;
			}
			break;

			case TOKEN_PARCLOSE:
				state = 4;
				// check parenthesis count
				if (pcount == 0)
				{
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}
				// down one level
				pcount--;
				// pop all operators in this level, beginning by the last inserted
				while (entry)
				{
					if (entry->GetVal().Level == pcount)
					{
						AddFilterOp(entry->GetVal().Op);
						VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
						pstack.RemoveEntry(entry);
						entry = _entry;
					}
					else
						break;
				}
				break;
			case TOKEN_AND:
			{
				state = 0;
				// push and
				OpLevel o;
				o.Op = FILTER_AND;
				o.Level = pcount;
				entry = new VListEntry<OpLevel>;
				entry->SetVal(o);
				pstack.AddEntry(entry, TRUE);
				break;
			}
			case TOKEN_OR:
			{
				state = 0;
				// push or
				OpLevel o;
				o.Op = FILTER_OR;
				o.Level = pcount;
				entry = new VListEntry<OpLevel>;
				entry->SetVal(o);
				pstack.AddEntry(entry, TRUE);
				break;
			}
			default:
				Query_SyntaxError((int)(p-query));
				return FALSE;
			}
			break;
		case 4:
			switch (t)
			{
			case TOKEN_AND:
			{
				state = 0;
				// push and
				OpLevel o;
				o.Op = FILTER_AND;
				o.Level = pcount;
				entry = new VListEntry<OpLevel>;
				entry->SetVal(o);
				pstack.AddEntry(entry, TRUE);
				break;
			}
			case TOKEN_OR:
			{
				state = 0;
				// push or
				OpLevel o;
				o.Op = FILTER_OR;
				o.Level = pcount;
				entry = new VListEntry<OpLevel>;
				entry->SetVal(o);
				pstack.AddEntry(entry, TRUE);
				break;
			}
			case TOKEN_PARCLOSE:
				state = 4;
				// check parenthesis count
				if (pcount == 0)
				{
					Query_SyntaxError((int)(p-query));
					return FALSE;
				}
				// down one level
				pcount--;
				// pop all operators in this level, beginning by the last inserted
				while (entry)
				{
					if (entry->GetVal().Level == pcount)
					{
						AddFilterOp(entry->GetVal().Op);
						VListEntry<OpLevel> *_entry = (VListEntry<OpLevel> *)entry->GetPrevious();
						pstack.RemoveEntry(entry);
						entry = _entry;
					}
					else
						break;
				}
				break;
			default:
				Query_SyntaxError((int)(p-query));
				return FALSE;
			}
			break;
		default:
			// Ahem... :/
			break;
		}
		p += size;
	}
	if (pcount > 0)
	{
		Query_SyntaxError((int)(p-query));
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------

void Scanner::Query_SyntaxError(int c)
{

}

//---------------------------------------------------------------------------
void Scanner::Query_CleanUp()
{
	while (pstack.GetNElements() > 0)
	{
		VListEntry<int> *e;
		e = (VListEntry<int> *)pstack.GetHead();
		pstack.RemoveEntry(e);
	}
}


//---------------------------------------------------------------------------
const wchar_t *Scanner::Query_EatSpace(const wchar_t *p)
{
	while (*p && *p == L' ') p++;
	return p;
}

//---------------------------------------------------------------------------
const wchar_t *Scanner::Query_ProbeNonAlphaNum(const wchar_t *p)
{
	int inquote=0;
	while (*p && (!Query_isControlChar(*p) || (inquote)))
	{
		if (*p == L'\"')
		{
			if (!inquote)
				inquote = 1;
			else
				return p+1;
		}
		p++;
	}
	return p;
}

//---------------------------------------------------------------------------
int Scanner::Query_isControlChar(wchar_t p)
{
	switch (p)
	{
	case L'&':
	case L'|':
	case L'!':
	case L'(':
	case L'[':
	case L')':
	case L']':
	case L'>':
	case L'<':
	case L'=':
	case L',':
	case L' ':
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
wchar_t *Scanner::Query_ProbeAlphaNum(wchar_t *p)
{
	while (*p && Query_isControlChar(*p)) p++;
	return p;
}

//---------------------------------------------------------------------------
wchar_t *Scanner::Query_ProbeSpace(wchar_t *p)
{
	while (*p && *p != ' ') p++;
	return p;
}

//---------------------------------------------------------------------------
int Scanner::Query_LookupToken(wchar_t *t)
{
	for (int i=0;i<sizeof(Tokens)/sizeof(tokenstruct);i++)
	{
		if (!wcsicmp(Tokens[i].token, t))
			return Tokens[i].tid;
	}
	return TOKEN_IDENTIFIER;
}

//---------------------------------------------------------------------------
int Scanner::Query_GetNextToken(const wchar_t *p, int *size, wchar_t **_token, int tokentable)
{

	int t = TOKEN_EOQ;
	const wchar_t *startptr = p;

	if (!*p) return TOKEN_EOQ;

	p = Query_EatSpace(p);

	const wchar_t *e = Query_ProbeNonAlphaNum(p);

	if (e != p)   // We have a word
	{
		if (*_token) free(*_token);
		*_token = wcsdup(p);
		(*_token)[e-p]=0; //token.rSplit(e-p);
		if (*(*_token) == L'\"' && (*_token)[wcslen(*_token)-1] == L'\"')
		{
			int l=(int)wcslen(*_token)-2;
			if (l>0)
			{
				memcpy(*_token,(*_token)+1,l*sizeof(wchar_t));
				(*_token)[l]=0;
				wchar_t *escaped = Query_Unescape(*_token);
				free(*_token);
				*_token = escaped;
			}
			else
				(*_token)[0]=0;// we have an empty string
		}

		switch (tokentable)
		{
		case -1:
			t = TOKEN_IDENTIFIER;
			break;
		case 0:
			t = Query_LookupToken(*_token);
			break;
		case 1:
			t = IntegerField::LookupToken(*_token);
		}
		p = e;
	}
	else   // We have a symbol
	{
		switch (*p)
		{
		case L'&':
			if (*(p+1) == L'&') p++;
			t = TOKEN_AND;
			break;
		case L'|':
			if (*(p+1) == L'|') p++;
			t = TOKEN_OR;
			break;
		case L'!':
			if (*(p+1) == L'=')
			{
				p++;
				t = TOKEN_NOTEQUAL;
				break;
			}
			t = TOKEN_NOT;
			break;
		case L'(':
			t = TOKEN_PAROPEN;
			break;
		case L')':
			t = TOKEN_PARCLOSE;
			break;
		case L'[':
			t = TOKEN_SQBRACKETOPEN;
			break;
		case L']':
			t = TOKEN_SQBRACKETCLOSE;
			break;
		case L',':
			t = TOKEN_COMMA;
			break;
		case L'>':
			if (*(p+1) == L'=')
			{
				p++;
				t = TOKEN_AOREQUAL;
				break;
			}
			if (*(p+1) == L'<')
			{
				p++;
				t = TOKEN_NOTEQUAL;
				break;
			}
			t = TOKEN_ABOVE;
			break;
		case L'<':
			if (*(p+1) == L'=')
			{
				p++;
				t = TOKEN_BOREQUAL;
				break;
			}
			if (*(p+1) == L'>')
			{
				p++;
				t = TOKEN_NOTEQUAL;
				break;
			}
			t = TOKEN_BELOW;
			break;
		case L'=':
			if (*(p+1) == L'>')
			{
				p++;
				t = TOKEN_AOREQUAL;
				break;
			}
			if (*(p+1) == L'<')
			{
				p++;
				t = TOKEN_BOREQUAL;
				break;
			}
			if (*(p+1) == L'!')
			{
				p++;
				t = TOKEN_NOTEQUAL;
				break;
			}
			if (*(p+1) == L'=') p++;
			t = TOKEN_EQUAL;
			break;
		default:
			t = TOKEN_UNKNOWN;
			break;
		}
		p++;
	}

	*size = (int)(p - startptr);
	return t;
}

// you must free the mem!
wchar_t *Scanner::Query_Unescape(wchar_t *p)
{
	if (!p) return p;
	if (!*p) return wcsdup(p);
	int l = (int)wcslen(p);
	wchar_t *unescaped = (wchar_t *)malloc((l+1)*sizeof(wchar_t));
	wchar_t *d = unescaped;
	while (*p)
	{
		if (*p == L'%')
		{
			int abort = 0;
			if (!*(p+1))
			{
				*d++ = *p++; break;
			}
			if (*(p+1) == L'%')
			{
				*d = *p;
				p = p+2;
				d++;
			}
			int a = *(p+1);
			int b = 0;
			if (a != 0) b = *(p+2);
			a = towupper(a);
			b = towupper(b);
			if (a >=L'0' && a <= L'9') a -= L'0';
			else if (a >= L'A' && a <= L'F')
			{
				a -= L'A'; a += 10;
			}
			else abort = 1;
			if (b >=L'0' && b <= L'9') b -= L'0';
			else if (b >= L'A' && b <= L'F')
			{
				b -= L'A'; b += 10;
			}
			else abort = 1;
			if (abort)
			{
				*d++ = *p++;
				continue;
			}
			*d++ = a*16+b;
			p = p+3;
			continue;
		}
		*d = *p;
		p++; d++;
	}
	*d = 0;
	return unescaped;
}

