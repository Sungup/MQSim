#ifndef STRING_TOOLS_H
#define STRING_TOOLS_H

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Utils
{
  class Helper_Functions
  {
  public:
    static std::string Path_separator()
    {
#ifdef _WIN32
      return "\\";
#else
      return "/";
#endif
    }
    static void Tokenize(const std::string& str,
                         char delimiter,
                         std::vector<std::string>& output_tokens_list)
    {
      auto size = uint32_t(str.size());
      uint32_t start = 0UL, end = 0UL;

      while (end < size) {
        if (str[end] == delimiter && start <= end) {
          output_tokens_list.emplace_back(str.substr(start, end - start + 1U));
          start = end + 1;
        }

        end++;
      }

      if (str[end - 1] != delimiter)
        output_tokens_list.emplace_back(str.substr(start, end - start));
    }

    static void Remove_cr(std::string& str)//remove carriage return in linux
    {
      if (str[str.size() - 1] == '\r')
        str.erase(str.size() - 1, 1);
    }
  };

  force_inline void
  to_upper(std::string& v)
  {
    std::transform(v.begin(), v.end(), v.begin(), ::toupper);
  }
}

force_inline bool
to_bool(std::string v)
{
  Utils::to_upper(v);

  return v != "FALSE";
}

template <typename T>
force_inline std::string
join_vector(const std::vector<T>& v, const char* seperator = ",")
{
  std::stringstream ss;

  const char* sp = "";

  for (auto& item : v) {
    ss << sp << item;
    sp = seperator;
  }

  return ss.str();
}

#endif