#pragma once

// POSIX compatibility shims for MSVC.
#ifdef _WIN32

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

// strptime is not available in MSVC. Provide an implementation using std::get_time.
inline char *strptime(const char *s, const char *fmt, std::tm *tm)
{
  std::istringstream ss(s);
  ss >> std::get_time(tm, fmt);
  if (ss.fail())
    return nullptr;
  auto pos = ss.tellg();
  if (pos == static_cast<std::streampos>(-1))
    return const_cast<char *>(s + std::strlen(s));
  return const_cast<char *>(s + static_cast<size_t>(pos));
}

// timegm is not available in MSVC. Use _mkgmtime instead.
inline time_t timegm(std::tm *tm)
{
  return _mkgmtime(tm);
}

#endif
