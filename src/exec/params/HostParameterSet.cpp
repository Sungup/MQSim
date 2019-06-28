#include "HostParameterSet.h"

HostParameterSet::HostParameterSet()
  : PCIe_Lane_Bandwidth(0.4),
    PCIe_Lane_Count(4),
    SATA_Processing_Delay(400000),
    Enable_ResponseTime_Logging(false),
    ResponseTime_Logging_Period_Length(400000),
    IO_Flow_Definitions(),
    Input_file_path()
{ }

void
HostParameterSet::XML_serialize(Utils::XmlWriter& xmlwriter) const
{
  xmlwriter.Write_open_tag("HostParameterSet");

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, PCIe_Lane_Bandwidth);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, PCIe_Lane_Count);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, SATA_Processing_Delay);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Enable_ResponseTime_Logging);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, ResponseTime_Logging_Period_Length);

  xmlwriter.Write_close_tag();
}

void
HostParameterSet::XML_deserialize(rapidxml::xml_node<> *node)
{
  try {
    for (auto param = node->first_node(); param; param = param->next_sibling()) {
      if (strcmp(param->name(), "PCIe_Lane_Bandwidth") == 0)
        PCIe_Lane_Bandwidth = std::stod(param->value());

      else if (strcmp(param->name(), "PCIe_Lane_Count") == 0)
        PCIe_Lane_Count = std::stoul(param->value());

      else if (strcmp(param->name(), "SATA_Processing_Delay") == 0)
        SATA_Processing_Delay = std::stoul(param->value());

      else if (strcmp(param->name(), "Enable_ResponseTime_Logging") == 0)
        Enable_ResponseTime_Logging = to_bool(param->value());

      else if (strcmp(param->name(), "ResponseTime_Logging_Period_Length") == 0)
        ResponseTime_Logging_Period_Length = std::stoul(param->value());
    }

  } catch (...) {
    throw mqsim_error("Error in the HostParameterSet!");

  }
}