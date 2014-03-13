#include <examples/wordfreq/partition.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/assign/std/vector.hpp>

#include <math.h>
#include <stdio.h>

using namespace boost::assign;
using std::placeholders::_1;

/*
struct KeyCount
{
  int64_t key;
  int64_t count;
};
std::vector<KeyCount> g_histogram;
*/

// REQUIRE: keys and pivots are sorted
std::vector<int64_t> getHistogramFromKey(const std::vector<int64_t>& keys,
                                         const std::vector<int64_t>& pivots)
{
  std::vector<int64_t> result;
  result.resize(pivots.size());
  size_t j = 0;
  for (size_t i = 0; i < keys.size(); ++i)
  {
    while (keys[i] > pivots[j])
    {
      ++j;
      if (j >= pivots.size())
        goto end;
    }
    ++result[j];
  }
end:
  return result;
}

BOOST_AUTO_TEST_CASE(testGetHistogramFromKey)
{
  std::vector<int64_t> keys;
  std::vector<int64_t> pivots;
  std::vector<int64_t> expected;

  keys += 1, 2, 3, 4, 5;
  pivots += 1, 2, 3, 4, 5;
  expected += 1, 1, 1, 1, 1;
  std::vector<int64_t> histogram = getHistogramFromKey(keys, pivots);
  BOOST_CHECK(histogram == expected);

  pivots.clear(); expected.clear();
  pivots += 3;
  expected += 3;
  histogram = getHistogramFromKey(keys, pivots);
  BOOST_CHECK(histogram == expected);

  pivots.clear(); expected.clear();
  pivots += 6;
  expected += 5;
  histogram = getHistogramFromKey(keys, pivots);
  BOOST_CHECK(histogram == expected);

  pivots.clear(); expected.clear();
  pivots += 1, 3, 5;
  expected += 1, 2, 2;
  histogram = getHistogramFromKey(keys, pivots);
  BOOST_CHECK(histogram == expected);

  keys.clear();
  keys += 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 7;

  pivots.clear(); expected.clear();
  pivots += 1, 3, 5;
  expected += 2, 4, 4;
  histogram = getHistogramFromKey(keys, pivots);
  BOOST_CHECK(histogram == expected);

  pivots.clear(); expected.clear();
  pivots += 1, 3, 5, 6;
  expected += 2, 4, 4, 0;
  histogram = getHistogramFromKey(keys, pivots);
  BOOST_CHECK(histogram == expected);
}

BOOST_AUTO_TEST_CASE(test1)
{
  std::vector<int64_t> keys;
  keys += 1, 2, 3, 4, 5;

  std::vector<int64_t> pivots = wordfreq::partition(
      keys.back(), keys.size(), 1,
      std::bind(getHistogramFromKey, std::ref(keys), _1));

  BOOST_CHECK_EQUAL(pivots.size(), 1);
  BOOST_CHECK_EQUAL(pivots[0], 5);

  pivots = wordfreq::partition(
      keys.back(), keys.size(), 2,
      std::bind(getHistogramFromKey, std::ref(keys), _1));

  BOOST_CHECK_EQUAL(pivots.size(), 2);
  BOOST_CHECK_EQUAL(pivots[0], 2);
  BOOST_CHECK_EQUAL(pivots[1], 5);

  keys += 6;
  pivots = wordfreq::partition(
      keys.back(), keys.size(), 2,
      std::bind(getHistogramFromKey, std::ref(keys), _1));

  BOOST_CHECK_EQUAL(pivots.size(), 2);
  BOOST_CHECK_EQUAL(pivots[0], 3);
  BOOST_CHECK_EQUAL(pivots[1], 6);
}

BOOST_AUTO_TEST_CASE(testUniform)
{
  int maxKey = 1000 * 1000;
  int count = 10000;
  std::vector<int64_t> keys;
  keys.resize(count);
  for (int i = 0; i < count; ++i)
  {
    keys[i] = rand() % maxKey;
  }
  std::sort(keys.begin(), keys.end());

  std::vector<int64_t> pivots = wordfreq::partition(
      keys.back(), keys.size(), 1,
      std::bind(getHistogramFromKey, std::ref(keys), _1));

  BOOST_CHECK_EQUAL(pivots.size(), 1);
  BOOST_CHECK_EQUAL(pivots[0], keys.back());

  {
  pivots = wordfreq::partition(
      keys.back(), keys.size(), 2,
      std::bind(getHistogramFromKey, std::ref(keys), _1));

  BOOST_CHECK_EQUAL(pivots.size(), 2);
  size_t p0 = std::count_if(keys.begin(), keys.end(),
                            std::bind(std::less_equal<int64_t>(), _1, pivots[0]));
  double perWorker = static_cast<double>((keys.size() / pivots.size()));
  double skew = fabs(static_cast<double>(p0) / perWorker - 1.0);
  BOOST_CHECK(skew < 0.05);
  BOOST_CHECK_EQUAL(pivots[1], keys.back());
  }

  for (int nWorkers = 1; nWorkers <= 10; ++nWorkers)
  {
    pivots = wordfreq::partition(
        keys.back(), keys.size(), nWorkers,
        std::bind(getHistogramFromKey, std::ref(keys), _1));
    BOOST_CHECK_EQUAL(pivots.size(), nWorkers);

    double perWorker = static_cast<double>((keys.size() / pivots.size()));

    int64_t last = 0;
    // printf("workers: %d\n", nWorkers);
    for (int i = 0; i < nWorkers; ++i)
    {
      int64_t less_equal = upper_bound(keys.begin(), keys.end(), pivots[i]) - keys.begin();
      int64_t thiscount = less_equal - last;
      last = less_equal;
      double skew = (static_cast<double>(thiscount) / perWorker - 1.0);
      BOOST_CHECK(fabs(skew) < 0.05);
      // printf(" %3d: %f\n", i, skew);
    }
  }
}

BOOST_AUTO_TEST_CASE(testNormal)
{
  // Normal distribution
  // FIXME
}
