#ifndef HOST_PARAMETER_SEt_H
#define HOST_PARAMETER_SEt_H

#include <vector>

#include "IOFlowParamSet.h"
#include "ParameterSetBase.h"

class HostParameterSet : public ParameterSetBase
{
public:
  // uint is GB/s
  double PCIe_Lane_Bandwidth;
  uint32_t PCIe_Lane_Count;

  // The overall hardware and software processing delay to send/receive a SATA
  // message in nanoseconds
  sim_time_type SATA_Processing_Delay;

  bool Enable_ResponseTime_Logging;
  sim_time_type ResponseTime_Logging_Period_Length; // nanoseconds
  IOFlowScenario IO_Flow_Definitions;

  // This parameter is not serialized. This is used to inform the Host_System
  // class about the input file path.
  std::string Input_file_path;

  HostParameterSet();
  ~HostParameterSet() override = default;

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

#endif // !HOST_PARAMETER_SEt_H
