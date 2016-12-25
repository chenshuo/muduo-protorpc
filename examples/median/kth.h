#ifndef MUDUO_PROTORPC2_EXAMPLES_MEDIAN_KTH_H
#define MUDUO_PROTORPC2_EXAMPLES_MEDIAN_KTH_H

#include <muduo/base/Logging.h>

#include <functional>

// https://gist.github.com/3703911
// array: [-1, 0, 1, 2, 3, 8 ]
// K-th : [ 1, 2, 3, 4, 5, 6 ]
inline
std::pair<int64_t, bool> getKth(std::function<void(int64_t, int64_t*, int64_t*)> search,
                                const int64_t k,
                                const int64_t count, int64_t min, int64_t max)
{
  int steps = 0;
  int64_t guess = max;
  bool succeed = false;
  while (min <= max)
  {
    int64_t smaller = 0;
    int64_t same = 0;
    search(guess, &smaller, &same);
    LOG_INFO << "guess = " << guess
             << ", smaller = " << smaller
             << ", same = " << same
             << ", min = " << min
             << ", max = " << max;

    if (++steps > 100) {
      LOG_ERROR << "Algorithm failed, too many steps";
      break;
    }

    if (smaller < k && smaller + same >= k) {
      succeed = true;
      break;
    }

    if (smaller + same < k) {
      min = guess + 1;
    } else if (smaller >= k) {
      max = guess;
    } else {
      LOG_ERROR << "Algorithm failed, no improvement";
      break;
    }
    guess = min + static_cast<uint64_t>(max - min)/2;
    assert(min <= guess && guess <= max);
  }

  std::pair<int64_t, bool> result(guess, succeed);
  return result;
}

#endif // MUDUO_PROTORPC2_EXAMPLES_MEDIAN_KTH_H
