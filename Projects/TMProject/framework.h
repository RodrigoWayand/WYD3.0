#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN

// ======================================================================
// WINDOWS
// ======================================================================

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <imm.h>

// ======================================================================
// CRT
// ======================================================================

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>

// ======================================================================
// STL
// ======================================================================

#include <vector>
#include <string>
#include <map>
#include <list>

// ======================================================================
// DIRECTX 9
// ======================================================================

#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <dsound.h>

// ======================================================================
// LINK
// ======================================================================

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")

// ======================================================================
// SAFE MACROS
// ======================================================================

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)          \
    {                           \
        if ((p) != NULL)        \
        {                       \
            delete (p);         \
            (p) = NULL;         \
        }                       \
    }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)    \
    {                           \
        if ((p) != NULL)        \
        {                       \
            delete[] (p);       \
            (p) = NULL;         \
        }                       \
    }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)         \
    {                           \
        if ((p) != NULL)        \
        {                       \
            (p)->Release();     \
            (p) = NULL;         \
        }                       \
    }
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(p)            \
    {                           \
        if ((p) != NULL)        \
        {                       \
            free(p);            \
            (p) = NULL;         \
        }                       \
    }
#endif

// ======================================================================
// COMMON
// ======================================================================

#ifndef D3DFVF_BLURVERTEX
#define D3DFVF_BLURVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)
#endif

#ifndef D3DFVF_BLURRENDERVERTEX
#define D3DFVF_BLURRENDERVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#endif

#ifndef D3DFVF_MBVERTEX
#define D3DFVF_MBVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#endif