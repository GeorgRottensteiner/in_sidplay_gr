#ifndef _LIBSIDPLAYFP_CONFIG_H_
#define _LIBSIDPLAYFP_CONFIG_H_

#define HAVE_CXX11 1

#define HAVE_BOOL 1

#define HAVE_MMINTRIN_H 1

#define HAVE_STRICMP 1

#define HAVE_STRNICMP 1

#if _MSC_VER < 1900
  #define snprintf sprintf_s
#endif

#define PACKAGE_NAME "reSIDfp"
#define PACKAGE_VERSION "2.0.0alpha"
#define PACKAGE_URL "https://bitbucket.org/kode54/sidplay-residfp/"

#endif

