#pragma once

#include "coding/varint.hpp"

#include "i18n/localisation.hpp"

#include "base/assert.hpp"
#include "base/control_flow.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace rw
{
template <class TSink>
void WriteNonEmpty(TSink & sink, std::string const & s)
{
  CHECK(!s.empty(), ());
  size_t const sz = s.size();
  WriteVarUint(sink, static_cast<uint32_t>(sz - 1));
  sink.Write(s.c_str(), sz);
}

template <class TSource>
void ReadNonEmpty(TSource & src, std::string & s)
{
  uint32_t const sz = ReadVarUint<uint32_t>(src) + 1;
  s.resize(sz);
  src.Read(&s[0], sz);
}
}  // namespace rw

// A class to store strings in multiple languages.
// May be used e.g. to store several translations of a feature's name.
//
// The coding scheme is as follows:
// * Pairs of the form (|lang|, |s|) are stored. |s| is a string in the UTF-8
//   encoding and |lang| is one of the 64 supported languages (see the list in the cpp file).
//
// * Each pair is represented by a byte encoding the lang followed by the
//   UTF-8 bytes of the string. Then, all such representations are concatenated
//   into a single std::string.
//   The language code is encoded with 6 bits that are prepended with "10", i.e.
//   10xx xxxx. In the UTF-8 encoding that would be a continuation byte, so
//   if you start reading the string and such a byte appears out of nowhere in
//   a place where a continuation byte is not expected you may be sure
//   that the string for the current language has ended, and you've reached the
//   string for the next language. Note that this breaks the self-synchronization property.
//
// * The order of the stored strings is not specified. Any language may come first.
class StringUtf8Multilang
{
public:
  // 6 bits language code mask. The language code is encoded with 6 bits that are prepended with
  // "10".
  static int8_t constexpr kLangCodeMask = 0x3F;
  static_assert(localisation::kMaxSupportedLanguages == kLangCodeMask + 1);

  static std::string GetOSMTagByCode(localisation::LanguageIndex const languageIndex);
  static localisation::LanguageIndex GetCodeByOSMTag(std::string const & name);

  inline bool operator==(StringUtf8Multilang const & rhs) const { return m_s == rhs.m_s; }
  inline bool operator!=(StringUtf8Multilang const & rhs) const { return !(*this == rhs); }

  inline void Clear() { m_s.clear(); }
  inline bool IsEmpty() const { return m_s.empty(); }

  // This method complexity is O(||utf8s||) when adding a new name and O(||m_s|| + ||utf8s||) when
  // replacing an existing name.
  void AddString(localisation::LanguageIndex const languageIndex, std::string_view utf8s);
  void AddString(localisation::LanguageCode const languageCode, std::string_view utf8s)
  {
    localisation::LanguageIndex const languageIndex = localisation::ConvertLanguageCodeToLanguageIndex(languageCode);
    if (languageIndex != localisation::kUnsupportedLanguageIndex)
      AddString(languageIndex, utf8s);
  }

  // This method complexity is O(||m_s||).
  void RemoveString(localisation::LanguageIndex const languageIndex);
  void RemoveString(localisation::LanguageCode const languageCode)
  {
    localisation::LanguageIndex const languageIndex = localisation::ConvertLanguageCodeToLanguageIndex(languageCode);
    if (languageIndex != localisation::kUnsupportedLanguageIndex)
      RemoveString(languageIndex);
  }

  // Calls |fn| for each pair of |lang| and |utf8s| stored in this multilang string.
  template <typename Fn>
  void ForEach(Fn && fn) const
  {
    size_t i = 0;
    size_t const sz = m_s.size();
    base::ControlFlowWrapper<Fn> wrapper(std::forward<Fn>(fn));
    while (i < sz)
    {
      size_t const next = GetNextIndex(i);
      localisation::LanguageIndex const languageIndex = m_s[i] & kLangCodeMask;
      if (localisation::ConvertLanguageIndexToLanguageCode(languageIndex) != localisation::kReservedLanguageCode &&
          wrapper(languageIndex, std::string_view(m_s).substr(i + 1, next - i - 1)) == base::ControlFlow::Break)
      {
        break;
      }
      i = next;
    }
  }

  /*
  /// Used for ordered languages, if you want to do something with priority of that order.
  /// \param languages ordered languages names.
  /// \param fn function or functor, using base::ControlFlow as return value.
  /// \return true if ForEachLanguage was stopped by base::ControlFlow::Break, false otherwise.
  template <typename Fn>
  bool ForEachLanguage(std::vector<std::string> const & languages, Fn && fn) const
  {
    auto const & translationPositions = GenerateTranslationPositions();

    base::ControlFlowWrapper<Fn> wrapper(std::forward<Fn>(fn));
    for (std::string const & language : languages)
    {
      int8_t const languageCode = GetLangIndex(language);
      if (GetLangByCode(languageCode) != kReservedLang)
      {
        auto const & translationPositionsIt = translationPositions.find(languageCode);
        if (translationPositionsIt != translationPositions.end() &&
            wrapper(languageCode, GetTranslation(translationPositionsIt->second)) ==
                base::ControlFlow::Break)
        {
          return true;
        }
      }
    }
    return false;
  }
  */

  template <typename String>
  bool GetString(localisation::LanguageIndex const languageIndex, String & utf8s) const
  {
    if (!localisation::IsSupportedLanguageIndex(languageIndex))
      return false;

    size_t i = 0;
    size_t const sz = m_s.size();

    while (i < sz)
    {
      size_t const next = GetNextIndex(i);

      if ((m_s[i] & kLangCodeMask) == languageIndex)
      {
        ++i;
        utf8s = {m_s.c_str() + i, next - i};
        return true;
      }

      i = next;
    }

    return false;
  }
  template <typename String>
  bool GetString(localisation::LanguageCode const languageCode, String & utf8s) const
  {
    localisation::LanguageIndex const languageIndex = localisation::ConvertLanguageCodeToLanguageIndex(languageCode);
    if (languageIndex >= 0)
      return GetString(languageIndex, utf8s);
    else
      return false;
  }

  bool HasString(localisation::LanguageIndex const languageIndex) const;

  int8_t FindString(std::string const & utf8s) const;
  size_t CountLangs() const;

  /// @name Used for serdes.
  /// @{
  template <class TSink>
  void WriteNonEmpty(TSink & sink) const
  {
    rw::WriteNonEmpty(sink, m_s);
  }
  template <class TSource>
  void ReadNonEmpty(TSource & src)
  {
    rw::ReadNonEmpty(src, m_s);
  }

  std::string const & GetBuffer() const { return m_s; }
  static StringUtf8Multilang FromBuffer(std::string && s);
  /// @}

private:
  size_t GetNextIndex(size_t i) const;

  std::string m_s;
};

std::string DebugPrint(StringUtf8Multilang const & s);
