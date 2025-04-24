#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace base64 {

inline bool decode(const std::string &in, std::vector<uint8_t> &out) {
  static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                          "abcdefghijklmnopqrstuvwxyz"
                                          "0123456789+/";

  int val = 0, valb = -8;
  for (unsigned char c : in) {
    if (isspace(c))
      continue;
    if (c == '=')
      break;
    auto pos = base64_chars.find(c);
    if (pos == std::string::npos)
      return false;
    val = (val << 6) + pos;
    valb += 6;
    if (valb >= 0) {
      out.push_back(uint8_t((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return true;
}

}  // namespace base64
