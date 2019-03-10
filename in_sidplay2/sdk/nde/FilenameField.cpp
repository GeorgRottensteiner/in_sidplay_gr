#include "FilenameField.h"
#include "NDE.h"

//---------------------------------------------------------------------------
FilenameField::FilenameField(const char *Str) : StringField(Str)
{
	Type = FIELD_FILENAME;
}

FilenameField::FilenameField(const wchar_t *Str, int strkind) : StringField(Str, strkind)
{
	Type = FIELD_FILENAME;
}

//---------------------------------------------------------------------------
FilenameField::FilenameField()
{
	Type = FIELD_FILENAME;
}

//---------------------------------------------------------------------------
int FilenameField::Compare(Field *Entry)
{
	if (!Entry) return -1;
	if (!CompatibleFields(Entry->GetType(), GetType()))
		return 0;
	return mywcsicmp_fn(GetStringW(), ((FilenameField*)Entry)->GetStringW());
}

//---------------------------------------------------------------------------
int FilenameField::Starts(Field *Entry)
{
	if (!Entry) return -1;
	if (!CompatibleFields(Entry->GetType(), GetType()))
		return 0;
	return (mywcsistr_fn(GetStringW(), ((FilenameField*)Entry)->GetStringW()) == GetStringW());
}

//---------------------------------------------------------------------------
int FilenameField::Contains(Field *Entry)
{
	if (!Entry) return -1;
	if (!CompatibleFields(Entry->GetType(), GetType()))
		return 0;
	return (mywcsistr_fn(GetStringW(), ((FilenameField*)Entry)->GetStringW()) != NULL);
}

// TODO: make file system string comparison functions
//---------------------------------------------------------------------------
bool FilenameField::ApplyFilter(Field *Data, int op)
{
	// TODO: maybe do this?
	if (op == FILTER_ISEMPTY || op == FILTER_ISNOTEMPTY)
	{
		bool r = (op == FILTER_ISEMPTY);
	if (!String && !StringW)
	return r;

	if (String && String[0] == 0)
	return r;

	if (StringW && StringW[0] == 0)
	return r;

	return !r;
	}
	//
	bool r;
	wchar_t *p=0;
	if (Data->GetType() == FIELD_STRING)
		p = ((StringField *)Data)->GetStringW();
	else
		p = ((FilenameField *)Data)->GetStringW();
	wchar_t *d = GetStringW();
	if (!p)
		p = L"";
	if (!d)
		d = L"";

	switch (op)
	{
	case FILTER_EQUALS:
		r = !nde_wcsicmp_fn(d, p);
		break;
	case FILTER_NOTEQUALS:
		r = !!nde_wcsicmp_fn(d, p);
		break;
	case FILTER_CONTAINS:
		r = (NULL != wcsistr_fn(d, p));
		break;
	case FILTER_NOTCONTAINS:
		r = (NULL == wcsistr_fn(d, p));
		break;
	case FILTER_ABOVE:
		r = (bool)(nde_wcsicmp_fn(d, p) > 0);
		break;
	case FILTER_ABOVEOREQUAL:
		r = (bool)(nde_wcsicmp_fn(d, p) >= 0);
		break;
	case FILTER_BELOW:
		r = (bool)(nde_wcsicmp_fn(d, p) < 0);
		break;
	case FILTER_BELOWOREQUAL:
		r = (bool)(nde_wcsicmp_fn(d, p) <= 0);
		break;
	case FILTER_BEGINS:
		r = (bool)(nde_wcsnicmp_fn(d, p, wcslen(p)) == 0);
		break;
	case FILTER_ENDS:
		{
			int lenp = (int)wcslen(p), lend = (int)wcslen(d);
			if (lend < lenp) return 0;  // too short
			r = (bool)(nde_wcsicmp_fn((d + lend) - lenp, p) == 0);
		}
		break;
	case FILTER_LIKE:
		r = (bool)(nde_wcsicmp_fn(d, p) == 0);
		break;
	default:
		r = true;
		break;
	}
	return r;
}
