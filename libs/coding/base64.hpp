#pragma once

#include <string>

namespace base64
{
std::string Encode(std::string_view bytesToEncode);
std::string Decode(std::string const & base64CharsToDecode);
// base64url variant without padding (RFC 4648)
std::string EncodeUrl(std::string_view bytesToEncode);
}  // namespace base64
