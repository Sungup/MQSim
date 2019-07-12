#ifndef EXECUTION_PARAMETER_SET_H
#define EXECUTION_PARAMETER_SET_H

#include <vector>

#include "DeviceParameterSet.h"
#include "HostParameterSet.h"
#include "IOFlowParamSet.h"
#include "ParameterSetBase.h"

class ExecParameterSet : public ParameterSetBase {
public:
  HostParameterSet   Host_Configuration;
  DeviceParameterSet SSD_Device_Configuration;

private:
  void __dump_config_params(const std::string& file_path) const;

public:
  ExecParameterSet(const std::string& ssd_config_path,
                   const std::string& workload_path);

  void load_config_params(const std::string& file_path);

  std::string result_file_path(int scenario_no) const;

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

force_inline std::string
ExecParameterSet::result_file_path(int scenario_no) const
{
  return Host_Configuration.Input_file_path
           + "_scenario_" + std::to_string(scenario_no) + ".xml";
}

#endif