#ifndef IO_FLOW_PARAMETER_SET_H
#define IO_FLOW_PARAMETER_SET_H

#include <memory>
#include <string>
#include <vector>

#include "../../host/ASCII_Trace_Definition.h"
#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../../ssd/dcm/DataCacheDefs.h"
#include "../../ssd/interface/Host_Interface_Defs.h"
#include "../../utils/DistributionTypes.h"
#include "../../utils/Workload_Statistics.h"

#include "ParameterSetBase.h"

enum class Flow_Type {
  SYNTHETIC,
  TRACE
};

// -------------------------
// IOFlowParamSet Definition
// -------------------------
class IOFlowParamSet : public ParameterSetBase {
public:
  const Flow_Type Type;

  SSD_Components::Caching_Mode Device_Level_Data_Caching_Mode;

  // The priority class is only considered when the SSD device uses NVMe host
  // interface
  IO_Flow_Priority_Class Priority_Class;

  // Resource partitioning: which channel/chip/die/plane IDs are allocated to
  // this flow
  ChannelIDs Channel_IDs;
  ChipIDs    Chip_IDs;
  DieIDs     Die_IDs;
  PlaneIDs   Plane_IDs;

  // Percentage of the logical space that is written when preconditioning is
  // performed
  uint32_t Initial_Occupancy_Percentage;

protected:
  void _load_default();

public:
  explicit IOFlowParamSet(Flow_Type type);
  ~IOFlowParamSet() override = default;

  void XML_serialize(Utils::XmlWriter& xmlwrite) const override;
  void XML_deserialize(rapidxml::xml_node<> *node) override;
};

typedef std::shared_ptr<IOFlowParamSet>    IOFlowParameterSetPtr;
typedef std::vector<IOFlowParameterSetPtr> IOFlowScenario;
typedef std::vector<IOFlowScenario>        IOFlowScenariosList;

// --------------------------------
// SyntheticFlowParamSet Definition
// --------------------------------
class SyntheticFlowParamSet : public IOFlowParamSet {
public:
  Utils::Request_Generator_Type         Synthetic_Generator_Type;
  Utils::Address_Distribution_Type      Address_Distribution;
  Utils::Request_Size_Distribution_Type Request_Size_Distribution;

  //Percentage of available storage space that is accessed
  uint32_t Working_Set_Percentage;

  char Read_Percentage;

  // This parameters used if the address distribution type is hot/cold (i.e.,
  // (100-H)% of the whole I/O requests are going to a H% hot region of the
  // storage space)
  char Percentage_of_Hot_Region;
  bool Generated_Aligned_Addresses;

  uint32_t Address_Alignment_Unit;

  // Average request size in sectors
  uint32_t Average_Request_Size;

  // Variance of request size in sectors
  uint32_t Variance_Request_Size;

  int Seed;

  // Average number of I/O requests from this flow in the
  uint32_t Average_No_of_Reqs_in_Queue;

  // The bandwidth of I/O flow in bytes per second (it should be
  // a multiplication of sector size)
  uint32_t Bandwidth;

  //Defines when to stop generating I/O requests
  sim_time_type Stop_Time;

  // If Stop_Time is equal to zero, then request generator considers
  // Total_Requests_To_Generate to decide when to stop generating I/O requests
  uint32_t Total_Requests_To_Generate;

public:
  SyntheticFlowParamSet();
  explicit SyntheticFlowParamSet(int seed);
  explicit SyntheticFlowParamSet(rapidxml::xml_node<> *node);

  void load_default(int seed);

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

// --------------------------------
// TraceFlowParameterSet Definition
// --------------------------------
class TraceFlowParameterSet : public IOFlowParamSet {
public:
  std::string File_Path;
  int Percentage_To_Be_Executed;
  int Relay_Count; 
  Trace_Time_Unit Time_Unit;

public:
  TraceFlowParameterSet();
  explicit TraceFlowParameterSet(rapidxml::xml_node<> *node);

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

#endif // !IO_FLOW_PARAMETER_SET_H
