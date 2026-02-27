#include "coding/string_utf8_multilang.hpp"

#include <algorithm>
#include <array>

std::string StringUtf8Multilang::GetOSMTagByCode(localisation::LanguageIndex const languageIndex)
{
  localisation::LanguageCode const languageCode = localisation::ConvertLanguageIndexToLanguageCode(languageIndex);
  if (languageIndex == localisation::kInternationalNameIndex || languageIndex == localisation::kAlternativeNameIndex || languageIndex == localisation::kOldNameIndex)
    return languageCode;
  else if (languageIndex == localisation::kDefaultNameIndex)
    return "name";
  else if (!languageCode.empty())
    return std::string{"name:"}.append(languageCode);
  else
  {
    ASSERT_FAIL(("Language can not be an empty string"));
    return "";
  }
}

localisation::LanguageIndex StringUtf8Multilang::GetCodeByOSMTag(std::string const & name)
{
  localisation::LanguageCode languageCode;
  if (name.starts_with("name:"))
    languageCode = name.substr(5);
  else if (name == "name")
    languageCode = "default";
  else
    languageCode = name;

  return localisation::ConvertLanguageCodeToLanguageIndex(languageCode);
}

size_t StringUtf8Multilang::GetNextIndex(size_t i) const
{
  ++i;
  size_t const sz = m_s.size();

  while (i < sz && (m_s[i] & 0xC0) != 0x80)
    if ((m_s[i] & 0x80) == 0)
      i += 1;
    else if ((m_s[i] & 0xFE) == 0xFE)
      i += 7;
    else if ((m_s[i] & 0xFC) == 0xFC)
      i += 6;
    else if ((m_s[i] & 0xF8) == 0xF8)
      i += 5;
    else if ((m_s[i] & 0xF0) == 0xF0)
      i += 4;
    else if ((m_s[i] & 0xE0) == 0xE0)
      i += 3;
    else if ((m_s[i] & 0xC0) == 0xC0)
      i += 2;

  return i;
}

void StringUtf8Multilang::AddString(localisation::LanguageIndex const languageIndex, std::string_view utf8s)
{
  size_t i = 0;
  size_t const sz = m_s.size();

  while (i < sz)
  {
    size_t const next = GetNextIndex(i);

    if ((m_s[i] & kLangCodeMask) == languageIndex)
    {
      ++i;
      m_s.replace(i, next - i, utf8s);
      return;
    }

    i = next;
  }

  m_s.push_back(languageIndex | 0x80);
  m_s.insert(m_s.end(), utf8s.begin(), utf8s.end());
}

void StringUtf8Multilang::RemoveString(localisation::LanguageIndex const languageIndex)
{
  size_t i = 0;
  size_t const sz = m_s.size();

  while (i < sz)
  {
    size_t const next = GetNextIndex(i);

    if ((m_s[i] & kLangCodeMask) == languageIndex)
    {
      m_s.erase(i, next - i);
      return;
    }

    i = next;
  }
}

bool StringUtf8Multilang::HasString(localisation::LanguageIndex const languageIndex) const
{
  if (!localisation::IsSupportedLanguageIndex(languageIndex))
    return false;

  for (size_t i = 0; i < m_s.size(); i = GetNextIndex(i))
    if ((m_s[i] & kLangCodeMask) == languageIndex)
      return true;

  return false;
}

localisation::LanguageIndex StringUtf8Multilang::FindString(std::string const & utf8s) const
{
  localisation::LanguageIndex result = localisation::kUnsupportedLanguageIndex;

  ForEach([&utf8s, &result](localisation::LanguageIndex const languageIndex, std::string_view name)
  {
    if (name == utf8s)
    {
      result = languageIndex;
      return base::ControlFlow::Break;
    }
    return base::ControlFlow::Continue;
  });

  return result;
}

size_t StringUtf8Multilang::CountLangs() const
{
  size_t count = 0;
  for (size_t i = 0; i < m_s.size(); i = GetNextIndex(i))
    ++count;

  return count;
}

std::string DebugPrint(StringUtf8Multilang const & s)
{
  std::string result;

  bool isFirst = true;
  s.ForEach([&result, &isFirst](int8_t code, std::string_view name)
  {
    if (isFirst)
      isFirst = false;
    else
      result += ' ';

    result.append(localisation::ConvertLanguageIndexToLanguageCode(code)).append(":").append(name);
  });

  return result;
}

StringUtf8Multilang StringUtf8Multilang::FromBuffer(std::string && s)
{
  ASSERT(!s.empty(), ());
  StringUtf8Multilang res;
  res.m_s = std::move(s);
  ASSERT_GREATER(res.CountLangs(), 0, ());
  return res;
}
