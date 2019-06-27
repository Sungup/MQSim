#include "XMLWriter.h"
#include "../sim/Engine.h"
#include "Exception.h"

using namespace Utils;

XmlWriter::XmlWriter(const std::string& file_name)
  : indent(0),
    openTags(0),
    openElements(0)
{
  if (!Open(file_name))
    throw mqsim_error("Cannot open file to write");
}

bool
XmlWriter::exists(const std::string& file_name)
{
  std::fstream checkFile(file_name);

  return checkFile.is_open();
}

bool
XmlWriter::Open(const std::string& file_name)
{
  outFile.open(file_name);

  if (!outFile.is_open())
    return false;

  //outFile << "<!--XML Document-->\n";
  outFile << "<?xml version=\"1.0\" encoding=\"us-ascii\"?>\n";
  indent = 0;
  openTags = 0;
  openElements = 0;

  return true;
}

void
XmlWriter::Close()
{
  if (outFile.is_open())
    outFile.close();
}

void
XmlWriter::Write_open_tag(const std::string& open_tag)
{
  if (!outFile.is_open())
    throw mqsim_error("The XML output file is closed. Unable to write to file");

  for (int i = 0; i < indent; i++)
    outFile << "\t";

  tempOpenTag.resize(openTags + 1);
  outFile << "<" << open_tag << ">\n";
  tempOpenTag[openTags] = open_tag;
  indent += 1;
  openTags += 1;
}

void
XmlWriter::Write_close_tag()
{
  if (!outFile.is_open())
    throw mqsim_error("The XML output file is closed. Unable to write to file");

  indent -= 1;
  for (int i = 0; i < indent; i++)
    outFile << "\t";

  outFile << "</" << tempOpenTag[openTags - 1] << ">\n";
  tempOpenTag.resize(openTags - 1);
  openTags -= 1;
}

void XmlWriter::Write_start_element_tag(const std::string& element_tag)
{
  if (!outFile.is_open())
    throw mqsim_error("The XML output file is closed. Unable to write to file");

  for (int i = 0; i < indent; i++)
    outFile << "\t";

  tempElementTag.resize(openElements + 1);
  tempElementTag[openElements] = element_tag;
  openElements += 1;
  outFile << "<" << element_tag;
}

void
XmlWriter::Write_end_element_tag()
{
  if (!outFile.is_open())
    throw mqsim_error("The XML output file is closed. Unable to write to file");

  outFile << "/>\n";
  //outFile << "</" << tempElementTag[openElements - 1] << ">\n";
  tempElementTag.resize(openElements - 1);
  openElements -= 1;
}

void
XmlWriter::Write_attribute(const std::string& out_attribute)
{
  if (!outFile.is_open())
    throw mqsim_error("The XML output file is closed. Unable to write to file");

  outFile << " " << out_attribute;
}

void
XmlWriter::Write_string(const std::string& out_string)
{
  if (!outFile.is_open())
    throw mqsim_error("The XML output file is closed. Unable to write to file");

  outFile << ">" << out_string;
}

void
XmlWriter::write_attr_str(const std::string& name,
                          const std::string& value)
{
  if (!outFile.is_open())
    throw mqsim_error("The XML output file is closed. Unable to write to file");

  for (int i = 0; i < indent + 1; i++)
    outFile << "\t";

  outFile << " <" << name + ">" + value + "</" << name + ">\n";
}

void
XmlWriter::write_attr_str_inline(const std::string& name,
                                 const std::string& value)
{
  Write_attribute(name + "=\"" + value + "\"");
}
