#include "TSearch.h"

#include <CatEngine.h>
#pragma comment(lib, "CatEngine.lib")

#include <iomanip>
#include <thread>
#include <cctype>

template <typename T>
struct TDivison
{
  T quotient;
  T remainder;
  TDivison() : quotient(0), remainder(0) {};
};

template <typename T>
TDivison<T> DivT(const T Dividend, const T Divisor)
{
  TDivison<T> result;
  if (Divisor == 0) return result;
  result.remainder = Dividend % Divisor;
  result.quotient  = Dividend / Divisor;
  return result;
}

template <typename T>
CTSearch<T>::CTSearch()
{
  ce::ceMsgA(CE_FUNCTION_NAME);
  m_Base = 0;
  m_DataSize  = 0;
  m_PageSize  = DEFAULT::PAGE_SIZE;  // 4 threads for searching.
  m_AvaThread = DEFAULT::AVA_THREAD; // 4 KiB for one thread
}

template <typename T>
CTSearch<T>::CTSearch(const T Base, const T DataSize, const T AvaThread, const T PageSize)
{
  ce::ceMsgA(CE_FUNCTION_NAME);
  m_Base = Base;
  m_DataSize = DataSize;
  m_PageSize = PageSize;
  m_AvaThread = AvaThread;
}

template <typename T>
CTSearch<T>::~CTSearch()
{
  ce::ceMsgA(CE_FUNCTION_NAME);
}

template <typename T>
void CTSearch<T>::SetParameters(const T Base, const T DataSize, const T AvaThread, const T PageSize)
{
  ce::ceMsgA(CE_FUNCTION_NAME);
  m_Base = Base;
  m_DataSize = DataSize;
  m_PageSize = PageSize;
  m_AvaThread = AvaThread;
}

template <typename T>
void CTSearch<T>::ThreadBody(std::shared_ptr<TThreadData> pData)
{
  /* ce::ceMsgA(CE_FUNCTION_NAME); */

  /* Update the shared data inside this block */ {
    std::lock_guard<std::mutex> LG(*(pData->pMutex));
  }

  ce::ceMsgA("T[%04X] : Running <%p : %08X>", std::this_thread::get_id(), (void*)pData->Address, pData->Size);

  std::string pattern("");
  for (auto e : pData->Pattern) pattern += ce::ceFormatA("<%d : %02X>, ", e.first, e.second);
  ce::ceMsgA(pattern);

  Sleep(500);

  ce::ceMsgA("T[%04X] : Completed", std::this_thread::get_id());
}

template <typename T>
void CTSearch<T>::ExecThreadGroup(
  const std::function<void(std::shared_ptr<TThreadData> pData)> fnThreadBody,
  const TThreadGroupConfig* pTGC
)
{
  ce::ceMsgA(CE_FUNCTION_NAME);

  if ((m_AvaThread == 0) || (pTGC == nullptr || pTGC->NumberOfThread == 0)) return;

  std::shared_ptr<TThreadData> threadData(nullptr);

  std::vector<std::thread> threads;
  threads.clear();

  for (T i = 0; i < pTGC->NumberOfThread; i++)
  {
    threadData.reset(new TThreadData);

    threadData->pMutex  = pTGC->pMutex;
    threadData->Pattern = pTGC->Pattern;
    threadData->Address = pTGC->Address + i * m_PageSize;
    threadData->Size    = m_PageSize;
    if (threadData->Address - m_Base + threadData->Size > m_DataSize)
    {
      threadData->Size = m_DataSize % m_PageSize;
    }

    threads.push_back(std::thread(fnThreadBody, threadData));
  }

  for (auto& thread: threads)
  {
    if (thread.joinable()) thread.join();
  }
}

template <typename T>
bool CTSearch<T>::Search(const std::string& pattern)
{
  ce::ceMsgA(CE_FUNCTION_NAME);

  if (m_AvaThread == 0 || (m_Base == 0 || m_DataSize == 0)) return false;

  auto cPattern = this->ToPattern(pattern);
  if (cPattern.empty()) return false;

  const auto grPage = DivT<T>(m_DataSize, m_PageSize);
  auto nGrPage = grPage.quotient + (grPage.remainder == 0 ? 0 : 1);

  const auto grThread = DivT<T>(nGrPage, m_AvaThread);

  TThreadGroupConfig args;
  args.pMutex = &m_Mutex;

  auto nThread = m_AvaThread;
  for (T i = 0; i <= grThread.quotient; i++)
  {
    if (i == grThread.quotient && (nThread = grThread.remainder) == 0) break;

    args.NumberOfThread = nThread;
    args.Pattern = cPattern;
    args.Address = m_Base + i * m_AvaThread * m_PageSize;
    this->ExecThreadGroup(CTSearch<T>::ThreadBody, &args);
  }

  return true;
}

template <typename T>
void CTSearch<T>::SearchPattern(const std::string& pattern)
{
  auto s = pattern;
  s = ce::ceTrimStringA(s);
  if (!s.empty()) this->Search(s);
}

template <typename T>
void CTSearch<T>::SearchPattern(const std::wstring& pattern)
{
  auto s = ce::ceToStringA(pattern);
  s = ce::ceTrimStringA(s);
  if (!s.empty()) this->Search(s);
}

template <typename T>
const TPattern CTSearch<T>::ToPattern(const std::string& pattern)
{
  TPattern M;

  auto l = ce::ceSplitStringA(pattern, " ");
  for (auto& e: l)
  {
    if (e.length() == 2 && isxdigit(e[0]) && isxdigit(e[1]))
    {
      auto v = (ce::uchar)strtoul(e.c_str(), nullptr, 16);
      M.push_back(std::make_pair(true, v));
    }
    else
    {
      M.push_back(std::make_pair(false, 0x00));
    }
  }

  return M;
}
