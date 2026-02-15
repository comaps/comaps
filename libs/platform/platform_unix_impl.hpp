#pragma once

#include <functional>
#include <string>
#include <vector>

#include <boost/regex.hpp>

namespace pl
{
void EnumerateFiles(std::string const & directory, std::function<void(char const *)> const & fn);

void EnumerateFilesByRegExp(std::string const & directory, boost::regex const & regexp, std::vector<std::string> & res);

inline void EnumerateFiles(std::string const & directory, std::vector<std::string> & res)
{ 
  EnumerateFiles(directory, [&](char const * entry)
  {
    res.push_back(std::string(entry));
  });
}
}  // namespace pl
