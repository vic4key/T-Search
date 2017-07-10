// T-Search.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TSearch.h"
#include <Windows.h>
#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
  #ifdef _M_IX86
    typedef TSP32 TSPDT;
  #else  // _M_AMD64
    typedef TSP64 TSPDT;
  #endif // _M_IX86  /* Note :   * If you are working on another process.   * For performance, you should copy that memory to the current process memory   * and do searching on it.   */

  CTSearch<TSPDT> TSP;
  TSP.SetParameters(TSPDT(GetModuleHandle(_T("kernel32.dll"))), 10*4*1024 + 512);

  std::string EXAMPLE_PATTERN("");
  EXAMPLE_PATTERN = "?? 41 0F B7 4E ?? 41 83 FF ?? 0F 84";
  EXAMPLE_PATTERN = "54 68 69 73 ?? 70 72 ?? 67 72 61 6D";
  auto result = TSP.SearchPattern(EXAMPLE_PATTERN);

  std::cout << "Result : " << result.first << " & " << (void*)result.second << std::endl;

  return 0;
}
