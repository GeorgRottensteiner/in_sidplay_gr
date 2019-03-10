/*
Ben Allison benski@winamp.com Nov 14 2007
Simple reference counted string, to avoid a whole bunch of wcsdup's in NDE and ml_local
*/

#pragma once
#include <bfc/platform/types.h>

enum
{
	STRING_IS_WCHAR=0,
	STRING_IS_NDESTRING=1,
};

#include "nde.h"

NDE_API wchar_t *ndestring_wcsdup(const wchar_t *str);
NDE_API wchar_t *ndestring_malloc(size_t str_size);
NDE_API void ndestring_release(wchar_t *str);
NDE_API void ndestring_retain(wchar_t *str);
