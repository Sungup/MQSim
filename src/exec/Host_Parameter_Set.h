#ifndef HOST_PARAMETER_SEt_H
#define HOST_PARAMETER_SEt_H

#include <vector>
#include "params/ParameterSetBase.h"
#include "params/IOFlowParameterSet.h"

// TODO Remove static features

class Host_Parameter_Set : public ParameterSetBase
{
public:
  static double PCIe_Lane_Bandwidth;//uint is GB/s
  static uint32_t PCIe_Lane_Count;
  static sim_time_type SATA_Processing_Delay;//The overall hardware and software processing delay to send/receive a SATA message in nanoseconds
  static bool Enable_ResponseTime_Logging;
  static sim_time_type ResponseTime_Logging_Period_Length;
  static IOFlowScenario IO_Flow_Definitions;
  static std::string Input_file_path;//This parameter is not serialized. This is used to inform the Host_System class about the input file path.

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

#endif // !HOST_PARAMETER_SEt_H
