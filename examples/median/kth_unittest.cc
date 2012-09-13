#include <examples/median/kth.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <limits>

std::vector<int64_t> g_data;

template<int N>
void setVec(int64_t (&arr)[N])
{
  g_data.assign(arr, arr+N);
  std::sort(g_data.begin(), g_data.end());
}

void search(int64_t guess, int64_t* smaller, int64_t* same)
{
  std::vector<int64_t>::iterator it = std::lower_bound(g_data.begin(), g_data.end(), guess);
  *smaller = it - g_data.begin();
  *same = std::upper_bound(g_data.begin(), g_data.end(), guess) - it;
}

bool check()
{
  for (size_t i = 0; i < g_data.size(); ++i)
  {
    std::pair<int64_t, bool> result = getKth(search, i+1, g_data.size(), g_data.front(), g_data.back());
    if (!(result.second && result.first == g_data[i]))
      return false;
    LOG_INFO << "***** Result is " << result.first;
  }
  return true;
}

const int64_t values[] =
{
    std::numeric_limits<int64_t>::min(),
    std::numeric_limits<int64_t>::min() + 1,
    std::numeric_limits<int64_t>::min() + 2,
    -2, -1, 0, 1, 2,
    std::numeric_limits<int64_t>::max() - 2,
    std::numeric_limits<int64_t>::max() - 1,
    std::numeric_limits<int64_t>::max()
};

const int n_values = sizeof(values)/sizeof(values[0]);

BOOST_AUTO_TEST_CASE(testSetLogLevel)
{
  muduo::Logger::setLogLevel(muduo::Logger::WARN);
}

BOOST_AUTO_TEST_CASE(testOneElement)
{
  for (int i = 0; i < n_values; ++i)
  {
    int64_t data[] = { values[i] };
    LOG_INFO << "***** Input is " << data[0];
    setVec(data);
    BOOST_CHECK_EQUAL(check(), true);
  }
}

BOOST_AUTO_TEST_CASE(testTwoElements)
{
  for (int i = 0; i < n_values; ++i)
    for (int j = 0; j < n_values; ++j)
    {
      int64_t data[] = { values[i], values[j] };
      setVec(data);
      BOOST_CHECK_EQUAL(check(), true);
    }
}

BOOST_AUTO_TEST_CASE(testThreeElements)
{
  // TODO
}

