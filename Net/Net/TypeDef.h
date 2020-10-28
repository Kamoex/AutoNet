#pragma once

namespace AutoNet
{
#define PLATFORM_WIN32 1
#define PLATFORM_LINUX 2

#ifndef _WIN32
#define PLATEFORM_TYPE PLATFORM_LINUX
#endif

#ifndef __linux
#define PLATEFORM_TYPE PLATFORM_WIN32
#endif

#define SAFE_DELETE(p) if(p) {delete p; p = nullptr;}
#define SAFE_DELETE_ARRY(p) if(p) {delete[] p; p = nullptr;}

#define FALSE 0
#define TRUE  1

    typedef char                CHAR;
    typedef short               SHORT;
    typedef int                 INT;
    typedef long long           INT64;
    typedef float               FLOAT;
    typedef double              DOBULE;

    typedef unsigned char       UCHAR;
    typedef unsigned short      WORD;
    typedef unsigned int        UINT;
    typedef unsigned long       DWORD;
    typedef unsigned long long  UINT64;

    typedef wchar_t             WCHAR;
    typedef bool                BOOL;

    typedef unsigned int        SESSION_ID;



}