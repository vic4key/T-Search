#include "TSearch.h"

#include <vu>

#include <iomanip>
#include <thread>
#include <cctype>
#include <sstream>

template <typename T>
TResult CTSearch<T>::Result = std::make_pair(false, 0);

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
  vu::MsgA(VU_FUNC_NAME);
  m_Base = 0;
  m_DataSize  = 0;
  m_PageSize  = DEFAULT::PAGE_SIZE;
  m_AvaThread = DEFAULT::MAX_NTHREAD;
}

template <typename T>
CTSearch<T>::CTSearch(const T Base, const T DataSize, const T AvaThread, const T PageSize)
{
  vu::MsgA(VU_FUNC_NAME);
  m_Base = Base;
  m_DataSize = DataSize;
  m_PageSize = PageSize;
  m_AvaThread = AvaThread;
}

template <typename T>
CTSearch<T>::~CTSearch()
{
  vu::MsgA(VU_FUNC_NAME);
}

template <typename T>
void CTSearch<T>::SetParameters(const T Base, const T DataSize, const T AvaThread, const T PageSize)
{
  vu::MsgA(VU_FUNC_NAME);

  T nMaxThreads = std::thread::hardware_concurrency();
  if (nMaxThreads == 0) nMaxThreads = 1;

  auto nThread = (AvaThread == DEFAULT::MAX_NTHREAD ? nMaxThreads : AvaThread);
  nThread = (nThread >= nMaxThreads ? nMaxThreads : nThread);

  m_Base = Base;
  m_DataSize = DataSize;
  m_PageSize = PageSize;
  m_AvaThread = nThread;

  CTSearch<T>::Result = std::make_pair(false, 0);
}

vu::uchar ReadByte(void* Address)
{
  return *(vu::uchar*)Address;
}

template <typename T>
unsigned long CTSearch<T>::ExceptionHandler(_EXCEPTION_POINTERS* pExceptionInformation)
{

  return EXCEPTION_EXECUTE_HANDLER;
}

template <typename T>
TResult CTSearch<T>::Searcher(const T Address, const T Size, const TPattern& pattern)
{
  TResult result = std::make_pair(false, 0);

  __try
  {
    T cout = 0;
    auto z = pattern.size();
    for (T i = 0; i < Size - z; i++)
    {
      cout = 0;
      for (vu::uint j = 0; j < z; j++)
      {
        auto e = pattern.at(j);
        if (e.first)
        {
          if (ReadByte((void*)(Address + i + j)) == e.second) cout++;
          else break;
        }
        else cout++;
      }
      if (cout == z)
      {
        result = std::make_pair(true, std::ptrdiff_t(Address + i));
        break;
      }
    }
  }
  __except(CTSearch<T>::ExceptionHandler(GetExceptionInformation()))
  {
    // Exception
  }

  return result;
}

template <typename T>
void CTSearch<T>::ThreadBody(std::shared_ptr<TThreadData> pData)
{
  /* vu::ceMsgA(CE_FUNCTION_NAME); */

  std::stringstream ss;
  ss << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << std::this_thread::get_id();
  vu::MsgA("T[%s] : Running <%p : %08X>", ss.str().c_str(), (void*)pData->Address, pData->Size);

  { /* Update the shared data inside this block */
    std::lock_guard<std::mutex> LG(*(pData->pMutex));
    if (CTSearch<T>::Result.first)
    {
      vu::MsgA("T[%s] : Aborted", ss.str().c_str());
      return;
    }
  }

  auto result = CTSearch<T>::Searcher(pData->Address, pData->Size, pData->Pattern);
  std::string s("");
  if (result.first) s += vu::FormatA(" -> Found at %p", (void*)result.second);

  { /* Update the shared data inside this block */
    std::lock_guard<std::mutex> LG(*(pData->pMutex));
    if (result.first) CTSearch<T>::Result = result;
  }

  vu::MsgA("T[%s] : Completed%s", ss.str().c_str(), s.c_str());
}

template <typename T>
void CTSearch<T>::ExecThreadGroup(
  const std::function<void(std::shared_ptr<TThreadData> pData)> fnThreadBody,
  const TThreadGroupConfig* pTGC
)
{
  vu::MsgA(VU_FUNC_NAME);

  if ((m_AvaThread == 0) || (pTGC == nullptr || pTGC->NumberOfThread == 0)) return;

  std::shared_ptr<TThreadData> threadData(nullptr);

  std::vector<std::thread> threads;
  threads.clear();

  for (T i = 0; i < pTGC->NumberOfThread; i++)
  {
    if (CTSearch<T>::Result.first) break;

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
  vu::MsgA(VU_FUNC_NAME);

  CTSearch<T>::Result = std::make_pair(false, 0);

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
    if ((i == grThread.quotient && (nThread = grThread.remainder) == 0) || CTSearch<T>::Result.first) break;
    args.NumberOfThread = nThread;
    args.Pattern = cPattern;
    args.Address = m_Base + i * m_AvaThread * m_PageSize;
    this->ExecThreadGroup(CTSearch<T>::ThreadBody, &args);
  }

  return true;
}

template <typename T>
const TPattern CTSearch<T>::ToPattern(const std::string& pattern)
{
  TPattern M;

  auto l = vu::SplitStringA(pattern, " ");
  for (auto& e: l)
  {
    if (e.length() == 2 && isxdigit(e[0]) && isxdigit(e[1]))
    {
      auto v = (vu::uchar)strtoul(e.c_str(), nullptr, 16);
      M.push_back(std::make_pair(true, v));
    }
    else
    {
      M.push_back(std::make_pair(false, 0x00));
    }
  }

  return M;
}

template <typename T>
const TResult CTSearch<T>::SearchPattern(const std::string& pattern)
{
  TResult failed = std::make_pair(false, 0);

  auto s = pattern;
  s = vu::TrimStringA(s);
  if (s.empty()) return failed;
  
  if (!this->Search(s)) return failed;

  return CTSearch<T>::Result;
}

template <typename T>
const TResult CTSearch<T>::SearchPattern(const std::wstring& pattern)
{
  TResult failed = std::make_pair(false, 0);

  auto s = vu::ToStringA(pattern);
  s = vu::TrimStringA(s);
  if (s.empty()) return failed;

  if (!this->Search(s)) return failed;

  return CTSearch<T>::Result;
}
