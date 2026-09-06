#include "testing/testing.hpp"

#include "base/stable_id_table.hpp"

using namespace base;
using namespace std;

struct S
{
  std::string m_name;
  size_t m_hash;

  bool operator==(S const & other) const { return m_name == other.m_name; }
};

std::ostream & operator<<(std::ostream & os, S const & s)
{
  os << "{" << s.m_name << "," << s.m_hash << "}";
  return os;
}

namespace std
{
template <>
struct hash<S>
{
  size_t operator()(S const & s) const noexcept { return s.m_hash; }
};
}  // namespace std

template <typename BigID, BigID const & Tombstone, typename LittleID, LittleID ReservedCount>
void test_tombstone(StableIDTable<BigID, Tombstone, LittleID, ReservedCount> const & table, LittleID id, bool expected)
{
  bool got_exception = false;
  bool got_tombstone = false;
  try
  {
    table.At(id);
  }
  catch (std::out_of_range const & err)
  {
    got_exception = true;
    got_tombstone = strstr(err.what(), "tombstone") != nullptr;
  }
  TEST(got_exception && (got_tombstone == expected), (got_exception, got_tombstone, expected));
}

UNIT_TEST(StableIDTable_Smoke)
{
  static S const tombstone{};
  StableIDTable<S, tombstone, uint8_t, 200> table;
  auto const a = S{"a", 1};
  auto const b = S{"b", 10};
  auto const a1 = table.EnsureStableID(a);
  auto const b1 = table.EnsureStableID(b);
  TEST_EQUAL(a1, 201, ());
  TEST_EQUAL(b1, 210, ());
  auto const a2 = table.EnsureStableID(a);
  auto const b2 = table.GetStableID(b);
  TEST_EQUAL(a1, a2, ());
  TEST_EQUAL(b1, b2, ());
  auto const a3 = table.At(a2);
  auto const b3 = table.At(b2);
  TEST_EQUAL(a3, a, ());
  TEST_EQUAL(b3, b, ());
  auto const a4 = table.Remove(a3);
  auto const b4 = table.Remove(b3);
  TEST_EQUAL(a4, a1, ());
  TEST_EQUAL(b4, b1, ());
  TEST_THROW(table.At(a4), out_of_range, ());
  TEST_THROW(table.At(b4), out_of_range, ());
  TEST_THROW(table.At(10), out_of_range, ());
}

UNIT_TEST(StableIDTable_WrapAndTombstone)
{
  static S const tombstone{};
  StableIDTable<S, tombstone, uint8_t, 5> table;
  auto const a = S{"a", 0};
  auto const b = S{"b", 0};
  auto const c = S{"c", 2};
  auto const d = S{"d", 2};
  auto const x = S{"x", 250};
  auto const y = S{"y", 250};
  auto const z = S{"z", 250};
  auto const a1 = table.EnsureStableID(a);
  auto const b1 = table.EnsureStableID(b);
  auto const c1 = table.EnsureStableID(c);
  auto const d1 = table.EnsureStableID(d);
  TEST_EQUAL(a1, 5, ());
  TEST_EQUAL(b1, 6, ());
  TEST_EQUAL(c1, 7, ());
  TEST_EQUAL(d1, 8, ());
  auto const c2 = table.Remove(c);
  TEST_EQUAL(c2, c1, ());
  auto const x1 = table.EnsureStableID(x);
  auto const y1 = table.EnsureStableID(y);
  auto const z1 = table.EnsureStableID(z);
  TEST_EQUAL(x1, 255, ());
  TEST_EQUAL(y1, 7, ());
  TEST_EQUAL(z1, 9, ());
  auto const x2 = table.Remove(x);
  TEST_EQUAL(x2, x1, ());
  test_tombstone(table, x2, true);
  auto const y2 = table.GetStableID(y);
  auto const z2 = table.GetStableID(z);
  TEST_EQUAL(y2, y1, ());
  TEST_EQUAL(z2, z1, ());
  auto const a2 = table.Remove(a);
  TEST_EQUAL(a2, a1, ());
  TEST_THROW(table.At(a2), out_of_range, ());
  auto const a3 = table.EnsureStableID(a);
  TEST_EQUAL(a3, a1, ());
  auto const y3 = table.GetStableID(y);
  auto const z3 = table.GetStableID(z);
  TEST_EQUAL(y3, y1, ());
  TEST_EQUAL(z3, z1, ());
  auto const x3 = table.EnsureStableID(x);
  TEST_EQUAL(x3, x1, ());
  auto const d2 = table.Remove(d);
  TEST_EQUAL(d2, d1, ());
  test_tombstone(table, d2, true);
  auto const z4 = table.Remove(z);
  TEST_EQUAL(z4, z1, ());
  test_tombstone(table, d2, false);
  test_tombstone(table, z4, false);
  auto const z5 = table.EnsureStableID(z);
  TEST_EQUAL(z5, 8, ());
}
