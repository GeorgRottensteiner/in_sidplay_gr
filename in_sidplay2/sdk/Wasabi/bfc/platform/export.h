#ifndef NULLSOFT_BFC_EXPORT_H
#define NULLSOFT_BFC_EXPORT_H

#ifdef _MSC_VER
#define DLLEXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define DLLEXPORT __attribute__ ((visibility("default")))
#else
#error port me!
#endif

#endif