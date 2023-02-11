#pragma once
#include "globals.h"
#pragma comment(lib, "wbemuuid.lib")
using namespace std;

class WMICI
{
public:
	void createtext(string path);
	string GetName(string win32, string name);
	int GetNumber(string win32, string name);
};