#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <utility>

#pragma warning(disable: 4661) /* No suitable definition provided for explicit template instantiation request. */

typedef unsigned __int32 TSP32;
typedef unsigned __int64 TSP64;

typedef std::pair<bool, std::ptrdiff_t> TResult;
typedef std::vector<std::pair<bool, unsigned char>> TPattern;

template <typename T>
class CTSearch
{
private:
  T m_Base;
  T m_DataSize;
  T m_PageSize;
  T m_nPage;
  T m_AvaThread;
  T m_nThread;

  std::mutex m_Mutex;

  static TResult Result;

  typedef struct _THREAD_GROUP_CONFIG
  {
    std::mutex* pMutex;
    T NumberOfThread;
    T Address;
    TPattern Pattern;
    _THREAD_GROUP_CONFIG() : pMutex(nullptr), NumberOfThread(0), Address(0) {}
  } TThreadGroupConfig;

  typedef struct _THREAD_DATA
  {
    std::mutex* pMutex;
    T Address;
    T Size;
    TPattern Pattern;
    _THREAD_DATA() : pMutex(nullptr), Address(0), Size(0) {}
  } TThreadData;

  static void ThreadBody(std::shared_ptr<TThreadData> pData);

  void ExecThreadGroup(
    const std::function<void(std::shared_ptr<TThreadData> pData)> fnThreadBody,
    const TThreadGroupConfig* pTGC
  );

  static TResult Searcher(const T Address, const T Size, const TPattern& pattern);

  bool Search(const std::string& pattern);

  const TPattern ToPattern(const std::string& pattern);

public:
  typedef enum _DEFAULT
  {
    AVA_THREAD = 4,      // 4 threads
    PAGE_SIZE  = 4*1024, // 4 KiB
  } DEFAULT;

  CTSearch();

  CTSearch(
    const T Base,
    const T DataSize,
    const T AvaThread = CTSearch::DEFAULT::AVA_THREAD, // 4 threads for searching.
    const T PageSize  = CTSearch::DEFAULT::PAGE_SIZE   // 4 KiB for one thread.
    );

  virtual ~CTSearch();

  void SetParameters(
    const T Base,
    const T DataSize,
    const T AvaThread = CTSearch::DEFAULT::AVA_THREAD, // 4 threads for searching.
    const T PageSize  = CTSearch::DEFAULT::PAGE_SIZE   // 4 KiB for one thread.
    );

  const TResult SearchPattern(const std::string&  pattern = "");
  const TResult SearchPattern(const std::wstring& pattern = L"");
};

template class CTSearch<TSP32>;
template class CTSearch<TSP64>;
