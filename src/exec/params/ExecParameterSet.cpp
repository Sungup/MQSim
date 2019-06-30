#include "ExecParameterSet.h"

#include <cstring>
#include <fstream>

force_inline void
ExecParameterSet::__dump_config_params(const std::string& file_path) const
{
  std::cout << "Using MQSim's default configuration." << std::endl
            << "Writing the default configuration parameters to the expected "
            << "configuration file." << std::endl;

  Utils::XmlWriter xmlwriter(file_path);
  XML_serialize(xmlwriter);
  xmlwriter.Close();

  std::cout << "[====================] Done!" << std::endl;
}

ExecParameterSet::ExecParameterSet(const std::string& file_path)
  : Host_Configuration(),
    SSD_Device_Configuration()
{
  load_config_params(file_path);
}

void
ExecParameterSet::load_config_params(const std::string& file_path)
{
  std::ifstream ssd_config_file;
  ssd_config_file.open(file_path);

  if (!ssd_config_file) {
    std::cerr << "The specified SSD configuration file does not exist."
              << std::endl;

    __dump_config_params(file_path);
    return;
  }

  /// 1. Read input workload parameters
  std::string line((std::istreambuf_iterator<char>(ssd_config_file)),
                   std::istreambuf_iterator<char>());

  ssd_config_file.close();

  /// 2. Check default flag option in file
  if (line == "USE_INTERNAL_PARAMS") {
    __dump_config_params(file_path);

    return;
  }

  /// TODO Remove RapidXml because of the old c++ standard (<C++11).
  ///
  /// Alloc temp string space for XML parsing. RapidXml support only
  /// the non-const data type. Because of this reason, we should alloc char
  /// array dynamically using new/delete.
  std::shared_ptr<char> temp(new char[line.length() + 1],
                             std::default_delete<char []>());
  strcpy(temp.get(), line.c_str());

  rapidxml::xml_document<> doc;
  doc.parse<0>(temp.get());
  auto* config = doc.first_node("ExecParameterSet");

  if (config) {
    XML_deserialize(config);
  } else {
    std::cerr << "Error in the SSD configuration file!" << std::endl;
    std::cout << "Using MQSim's default configuration." << std::endl;
  }
}

void
ExecParameterSet::XML_serialize(Utils::XmlWriter& xmlwriter) const
{
  xmlwriter.Write_open_tag("ExecParameterSet");

  Host_Configuration.XML_serialize(xmlwriter);
  SSD_Device_Configuration.XML_serialize(xmlwriter);

  xmlwriter.Write_close_tag();
}

void
ExecParameterSet::XML_deserialize(rapidxml::xml_node<> *node)
{
  try {
    for (auto param = node->first_node(); param; param = param->next_sibling()) {
      if (strcmp(param->name(), "HostParameterSet") == 0)
        Host_Configuration.XML_deserialize(param);

      else if (strcmp(param->name(), "DeviceParameterSet") == 0)
        SSD_Device_Configuration.XML_deserialize(param);

    }
  } catch (...) {
    throw mqsim_error("Error in the ExecParameterSet!");
  }
}
