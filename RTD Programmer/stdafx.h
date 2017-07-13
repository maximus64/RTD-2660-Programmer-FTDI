// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <stdio.h>

#ifdef _WIN32
#include "targetver.h"

#include <tchar.h>
#elif __linux__
#include <stdint.h>
typedef uint8_t       BYTE;

inline int64_t _ftelli64(FILE *file)
{
    return ftello(file);
}

inline int _fseeki64(FILE *file, int64_t offset, int origin)
{
    return fseeko(file, offset, origin);
}

#endif
