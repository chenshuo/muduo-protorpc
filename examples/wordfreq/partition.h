#ifndef MUDUO_PROTORPC2_EXAMPLES_WORDFREQ_PARTITION_H
#define MUDUO_PROTORPC2_EXAMPLES_WORDFREQ_PARTITION_H

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>
#include <muduo/base/Types.h>
#include <muduo/base/Logging.h>

namespace wordfreq
{

typedef std::vector<int64_t> GetHistogram(const std::vector<int64_t>& pivots);

// return pivots
// http://pcl.intel-research.net/publications/CloudRAMsort-2012.pdf
// FIXME: test for edge cases
inline std::vector<int64_t> partition(int64_t maxKey,
                                      int64_t keyCount,
                                      int nWorkers,
                                      const std::function<GetHistogram>& getHist)
{
  assert(maxKey > 0);
  assert(keyCount > 0);
  assert(nWorkers > 0);

  int slotsPerWorker = 32;
  int nPivots = 0;
  std::vector<int64_t> pivots, histogram;

  bool done = false;
  while (!done)
  {
    nPivots = slotsPerWorker * nWorkers;
    int64_t increment = maxKey / nPivots;
    if (increment == 0)
    {
      increment = 1;
      nPivots = static_cast<int>(maxKey);
    }

    for (int i = 1; i < nPivots; ++i)
    {
      pivots.push_back(i * increment);
    }
    pivots.push_back(maxKey);

    assert(pivots.size() == static_cast<size_t>(nPivots));

    histogram = getHist(pivots);

    bool retry = false;
    for (int64_t count : histogram)
    {
      if (count > 2 * keyCount / nPivots)
      {
        LOG_INFO << "retry";
        retry = true;
        break;
      }
    }
    if (retry && slotsPerWorker <= 1024)
    {
      // FIXME: use adaptive method, only insert pivots in dense slot
      slotsPerWorker *= 2;
    }
    else
    {
      done = true;
    }
  }

  std::vector<int64_t> result;

  std::vector<int64_t> cum(nPivots);
  std::partial_sum(histogram.begin(), histogram.end(), cum.begin());
  assert(cum.back() == keyCount);
  int64_t perWorker = keyCount / nWorkers;
  for (int i = 1; i < nWorkers; ++i)
  {
    std::vector<int64_t>::iterator it =
      std::lower_bound(cum.begin(), cum.end(), i * perWorker);
    assert(it != cum.end());
    // FIXME: try it-1
    result.push_back(pivots[it - cum.begin()]);
  }
  result.push_back(maxKey);
  assert(result.size() == static_cast<size_t>(nWorkers));
  assert(result.back() == maxKey);
  return result;
}

}

#endif // MUDUO_PROTORPC2_EXAMPLES_WORDFREQ_PARTITION_H
