#pragma once

#include <iostream>   /* standart input|output stream*/
#include <Windows.h>  /* Provides access to the Windows API */
#include <Tlhelp32.h> /* Information about currently executing applications */
#include <vector>     /* Arrays that can change in size */
#include <fstream>    /* Working with files */
#include <psapi.h>    /* Working with process modules++ etc... */
#include <thread>     /* Creates threads */
#include <chrono>     /* Working with time milliseconds, nanoseconds etc... */
#include <memory>     /* General utilities to manage dynamic memory */
#include <mutex>      /* Protect data from multiple threads. */
#include <math.h>     /* Mathematical operations*/
#include <random>     /* Random =D */
#include <filesystem> /* fstream++ */
#include <time.h>     /* Time =D */
#include <string>     /* Useless */
#include <cstdint>    /* Letters =D */
#include <direct.h>   /* sniggers oyy! */
#include <conio.h>
#include <WbemIdl.h>
#include <comdef.h>
#include <ntddscsi.h>
#include <winternl.h>
#include <sstream>
#include <type_traits>

using namespace std;
#define StrToWStr(s) (wstring(s, &s[strlen(s)]).c_str())
#define _CRT_SECURE_NO_WARNINGS