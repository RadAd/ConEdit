// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

//#include <stdio.h>
#include <tchar.h>

#include <Windows.h>
#include <vector>


void AssertMsg(const char* test, const char* msg, const char* file, int line);
#define ASSERT_MSG(t, m) if (!(t)) AssertMsg(#t, (m), __FILE__, __LINE__)
#define ASSERT(t) ASSERT_MSG(t, nullptr)


// TODO: reference additional headers your program requires here
