// T-Search.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <CatEngine.h>
#pragma comment(lib, "CatEngine.lib")

int _tmain(int argc, _TCHAR* argv[])
{
  ce::ceBox(ce::ceGetConsoleWindow(), _T("T-Search"));
  return 0;
}
