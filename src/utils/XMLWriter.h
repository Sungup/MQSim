#ifndef XMLWRITE_H
#define XMLWRITE_H

#include <fstream>
#include <string>
#include <vector>

#include "InlineTools.h"

#define GLOBAL_TO_STRING(T) \
force_inline std::string to_string(T v) { return std::to_string(v); }

GLOBAL_TO_STRING(int)
GLOBAL_TO_STRING(long)
GLOBAL_TO_STRING(long long)
GLOBAL_TO_STRING(unsigned int)
GLOBAL_TO_STRING(unsigned long)
GLOBAL_TO_STRING(unsigned long long)
GLOBAL_TO_STRING(float)
GLOBAL_TO_STRING(double)
GLOBAL_TO_STRING(long double)

namespace Utils
{
  class XmlWriter {
  public:
    XmlWriter() = default;
    explicit XmlWriter(const std::string& file_name);

    bool Open(const std::string& file_name);
    void Close();
    bool exists(const std::string& file_name);

    void Write_open_tag(const std::string& open_tag);
    void Write_close_tag();

    void Write_start_element_tag(const std::string& element_tag);
    void Write_end_element_tag();
    void Write_attribute(const std::string& out_attribute);
    void Write_string(const std::string& out_string);

    template<typename T>
    void Write_attribute_string(const std::string& name, const T& value);

    template <typename T>
    void Write_attribute_string_inline(const std::string& name, const T& value);

  private:
    std::ofstream outFile;
    int indent;
    int openTags;
    int openElements;
    std::vector<std::string> tempOpenTag;
    std::vector<std::string> tempElementTag;

    void write_attr_str(const std::string& name,
                        const std::string& value);

    void write_attr_str_inline(const std::string& name,
                               const std::string& value);
  };

  // =========================
  // Write_attribute_string<T>
  // =========================
  template<>
  force_inline void
  XmlWriter::Write_attribute_string<std::string>(const std::string& name,
                                                 const std::string& value)
  {
    XmlWriter::write_attr_str(name, value);
  }

  template<>
  force_inline void
  XmlWriter::Write_attribute_string<bool>(const std::string& name,
                                          const bool& value)
  {
    XmlWriter::write_attr_str(name, value ? "true" : "false");
  }

  template<typename T>
  force_inline void
  XmlWriter::Write_attribute_string(const std::string& name,
                                    const T& value)
  {
    XmlWriter::write_attr_str(name, to_string(value));
  }

  // ================================
  // Write_attribute_string inline<T>
  // ================================
  template<>
  force_inline void
  XmlWriter::Write_attribute_string_inline<std::string>(const std::string& name,
                                                        const std::string& value)
  {
    XmlWriter::write_attr_str_inline(name, value);
  }

  template<>
  force_inline void
  XmlWriter::Write_attribute_string_inline<bool>(const std::string& name,
                                                 const bool& value)
  {
    XmlWriter::write_attr_str_inline(name, value ? "true" : "false");
  }

  template<typename T>
  force_inline void
  XmlWriter::Write_attribute_string_inline(const std::string& name,
                                           const T& value)
  {
    XmlWriter::write_attr_str_inline(name, to_string(value));
  }

#define XML_WRITER_MACRO_WRITE_ATTR_STR(WRITER, NAME) \
  (WRITER).Write_attribute_string(#NAME, (NAME))
}
#endif //!XMLWRITE_H