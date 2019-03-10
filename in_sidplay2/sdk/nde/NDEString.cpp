#include "NDEString.h"

wchar_t *ndestring_wcsdup(const wchar_t *str)
{
	if (!str)
		return 0;
	size_t len = wcslen(str);
	size_t *self = (size_t*)malloc(sizeof(size_t) + (len+1)*sizeof(wchar_t));
	*self=1;
	wchar_t *new_str = (wchar_t *)( ((int8_t *)self)+sizeof(size_t));
	memcpy(new_str, str, (len+1)*sizeof(wchar_t));
	return new_str;
}

wchar_t *ndestring_malloc(size_t str_size)
{
	size_t *self = (size_t*)malloc(sizeof(size_t) + (str_size));
	*self=1;
	wchar_t *new_str = (wchar_t *)( ((int8_t *)self)+sizeof(size_t));
	return new_str;
}

void ndestring_release(wchar_t *str)
{
	if (str)
	{
	size_t *self = (size_t *)(((int8_t *)str)-sizeof(size_t));
	(*self)--;
	if (*self == 0)
		free(self);
	}
}

void ndestring_retain(wchar_t *str)
{
	if (str)
	{
	size_t *self = (size_t *)(((int8_t *)str)-sizeof(size_t));
	(*self)++;
	}
}
