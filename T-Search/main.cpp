// T-Search.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include "TSearch.h"

int _tmain(int argc, _TCHAR* argv[])
{
  #ifdef _M_IX86
    typedef TSP32 TSPDT;
  #else  // _M_AMD64
    typedef TSP64 TSPDT;
  #endif // _M_IX86
  CTSearch<TSPDT> TSP;
  TSP.SetParameters((TSPDT)GetModuleHandle(NULL), 10*4*1024 + 512);
  TSP.SearchPattern("8B 65 E8 8B 45 E4 A3 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? 00 75 07 50");

  return 0;
}
