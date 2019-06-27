#ifndef EXECUTION_PARAMETER_SET_H
#define EXECUTION_PARAMETER_SET_H

#include <vector>
#include "ParameterSetBase.h"
#include "../Device_Parameter_Set.h"
#include "IOFlowParamSet.h"
#include "../Host_Parameter_Set.h"

// TODO Remove static features

class ExecParameterSet : public ParameterSetBase {
public:
  Host_Parameter_Set Host_Configuration;
  Device_Parameter_Set SSD_Device_Configuration;

private:
  void __dump_config_params(const std::string& file_path) const;

public:
  ExecParameterSet() = default;
  explicit ExecParameterSet(const std::string& file_path);

  void load_config_params(const std::string& file_path);

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

#endif