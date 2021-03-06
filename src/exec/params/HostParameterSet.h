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
  sim_time_type ResponseTime_Logging_Period; // nanoseconds

  // This parameter is not serialized. This is used to inform the HostSystem
  // class about the input file path.
  const std::string Input_file_path;

  explicit HostParameterSet(const std::string& workload_file_path);
  ~HostParameterSet() override = default;

  std::string stream_log_path(uint16_t flow_id) const;

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

force_inline std::string
HostParameterSet::stream_log_path(uint16_t flow_id) const
{
  return Input_file_path + ".IO_Flow.No_" + std::to_string(flow_id) + ".log";
}

#endif // !HOST_PARAMETER_SEt_H
