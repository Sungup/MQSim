#include "IOFlowParamSet.h"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <set>
#include <string>

// -------------------------
// Internal static functions
// -------------------------
template <typename T>
force_inline void
__copy_ids(char* value, std::vector<T>& id_list)
{
  /// All id list in XML file seperated by comma(,)
#define SKIP_EMPTY_STRING() do {                                        \
    while (point < lend && (*point == '\t' || *point == ' ')) ++point; \
  } while (false)

  std::set<uint32_t> ids;

  char* point = value;
  const char* lend = point + strlen(point);

  SKIP_EMPTY_STRING();
  while (point < lend) {
    ids.insert(strtoul(point, &point, 10));
    ++point;

    SKIP_EMPTY_STRING();
  }

  id_list.assign(ids.begin(), ids.end());
}

// ------------------------------
// IOFlowParamSet Implementations
// ------------------------------
IOFlowParamSet::IOFlowParamSet(Flow_Type type)
  : ParameterSetBase(),
    Type(type),
    Device_Level_Data_Caching_Mode(SSD_Components::Caching_Mode::WRITE_CACHE),
    Priority_Class(IO_Flow_Priority_Class::HIGH),
    Channel_IDs(),
    Chip_IDs(),
    Die_IDs(),
    Plane_IDs(),
    Initial_Occupancy_Percentage(50),
    __init_occupancy_rate(0.0)
{
  __update_hidden_data();
}

force_inline void
IOFlowParamSet::__update_hidden_data()
{
  __init_occupancy_rate = double(Initial_Occupancy_Percentage) / 100.;
}

force_inline void
IOFlowParamSet::_load_default()
{
  Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::WRITE_CACHE;
  Priority_Class = IO_Flow_Priority_Class::HIGH;

  Channel_IDs.resize(8);
  Chip_IDs.resize(4);
  Die_IDs.resize(2);
  Plane_IDs.resize(2);

  std::iota(Channel_IDs.begin(), Channel_IDs.end(), 0);
  std::iota(Chip_IDs.begin(),    Chip_IDs.end(),    0);
  std::iota(Die_IDs.begin(),     Die_IDs.end(),     0);
  std::iota(Plane_IDs.begin(),   Plane_IDs.end(),   0);

  __update_hidden_data();
}

// All serialization and deserialization functions should be replaced by a C++
// reflection implementation
void
IOFlowParamSet::XML_serialize(Utils::XmlWriter& xmlwriter) const
{
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Priority_Class);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Device_Level_Data_Caching_Mode);

  xmlwriter.Write_attribute_string("Channel_IDs", join_vector(Channel_IDs));
  xmlwriter.Write_attribute_string("Chip_IDs",    join_vector(Chip_IDs));
  xmlwriter.Write_attribute_string("Die_IDs",     join_vector(Die_IDs));
  xmlwriter.Write_attribute_string("Plane_IDs",   join_vector(Plane_IDs));

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Initial_Occupancy_Percentage);
}

void
IOFlowParamSet::XML_deserialize(rapidxml::xml_node<> *node)
{
  try {
    for (auto param = node->first_node(); param; param = param->next_sibling()) {
      if (strcmp(param->name(), "Device_Level_Data_Caching_Mode") == 0)
        Device_Level_Data_Caching_Mode = to_caching_mode(param->value());

      else if (strcmp(param->name(), "Priority_Class") == 0)
        Priority_Class = to_io_flow_priority(param->value());

      else if (strcmp(param->name(), "Channel_IDs") == 0)
        __copy_ids(param->value(), Channel_IDs);

      else if (strcmp(param->name(), "Chip_IDs") == 0)
        __copy_ids(param->value(), Chip_IDs);

      else if (strcmp(param->name(), "Die_IDs") == 0)
        __copy_ids(param->value(), Die_IDs);

      else if (strcmp(param->name(), "Plane_IDs") == 0)
        __copy_ids(param->value(), Plane_IDs);

      else if (strcmp(param->name(), "Initial_Occupancy_Percentage") == 0)
        Initial_Occupancy_Percentage = std::stoul(param->value());

    }

  } catch (...) {
    throw mqsim_error("Error in IOFlowParamSet!");
  }

  __update_hidden_data();
}

// ------------------------------------
// SyntheticFlowParamSet Implementation
// ------------------------------------
force_inline void
SyntheticFlowParamSet::__update_hidden_data()
{
  if (Working_Set_Percentage > 100 || Working_Set_Percentage < 1)
    Working_Set_Percentage = 100;

  __read_rate = double(Read_Percentage) / 100.;
  __hot_region_rate = double(Percentage_of_Hot_Region) / 100.;
  __working_set_rate = double(Working_Set_Percentage) / 100.;

  __avg_arrival_time = (Bandwidth == 0)
                         ? 0
                         : NSEC_TO_SEC_COEFF
                             / ((Bandwidth / SECTOR_SIZE_IN_BYTE) / Average_Request_Size);
}

SyntheticFlowParamSet::SyntheticFlowParamSet()
  : IOFlowParamSet(Flow_Type::SYNTHETIC),
    Synthetic_Generator_Type(Utils::RequestFlowControlType::QUEUE_DEPTH),
    Address_Distribution(Utils::Address_Distribution_Type::RANDOM_UNIFORM),
    Request_Size_Distribution(Utils::Request_Size_Distribution_Type::FIXED),
    Working_Set_Percentage(85),
    Read_Percentage(100),
    Percentage_of_Hot_Region(0),
    Generated_Aligned_Addresses(true),
    Address_Alignment_Unit(16),
    Average_Request_Size(8),
    Variance_Request_Size(0),
    Seed(0),
    Average_No_of_Reqs_in_Queue(2),
    Bandwidth(0),
    Stop_Time(1000000000),
    Total_Requests_To_Generate(0),
    __read_rate(0.0),
    __hot_region_rate(0.0),
    __working_set_rate(0.0),
    __avg_arrival_time(0)
{
  __update_hidden_data();
}

SyntheticFlowParamSet::SyntheticFlowParamSet(int seed)
  : IOFlowParamSet(Flow_Type::SYNTHETIC),
    Synthetic_Generator_Type(Utils::RequestFlowControlType::QUEUE_DEPTH),
    Address_Distribution(Utils::Address_Distribution_Type::RANDOM_UNIFORM),
    Request_Size_Distribution(Utils::Request_Size_Distribution_Type::FIXED),
    Working_Set_Percentage(85),
    Read_Percentage(100),
    Percentage_of_Hot_Region(0),
    Generated_Aligned_Addresses(true),
    Address_Alignment_Unit(16),
    Average_Request_Size(8),
    Variance_Request_Size(0),
    Seed(0),
    Average_No_of_Reqs_in_Queue(2),
    Bandwidth(0),
    Stop_Time(1000000000),
    Total_Requests_To_Generate(0),
    __read_rate(0.0),
    __hot_region_rate(0.0),
    __working_set_rate(0.0),
    __avg_arrival_time(0)
{
  load_default(seed);
}

SyntheticFlowParamSet::SyntheticFlowParamSet(rapidxml::xml_node<>* node)
  : IOFlowParamSet(Flow_Type::SYNTHETIC),
    Synthetic_Generator_Type(Utils::RequestFlowControlType::QUEUE_DEPTH),
    Address_Distribution(Utils::Address_Distribution_Type::RANDOM_UNIFORM),
    Request_Size_Distribution(Utils::Request_Size_Distribution_Type::FIXED),
    Working_Set_Percentage(85),
    Read_Percentage(100),
    Percentage_of_Hot_Region(0),
    Generated_Aligned_Addresses(true),
    Address_Alignment_Unit(16),
    Average_Request_Size(8),
    Variance_Request_Size(0),
    Seed(0),
    Average_No_of_Reqs_in_Queue(2),
    Bandwidth(0),
    Stop_Time(1000000000),
    Total_Requests_To_Generate(0),
    __read_rate(0.0),
    __hot_region_rate(0.0),
    __working_set_rate(0.0),
    __avg_arrival_time(0)
{
  XML_deserialize(node);
}

void
SyntheticFlowParamSet::load_default(int seed)
{
  IOFlowParamSet::_load_default();

  Working_Set_Percentage = 85;
  Synthetic_Generator_Type = Utils::RequestFlowControlType::QUEUE_DEPTH;
  Read_Percentage = 100;

  Address_Distribution = Utils::Address_Distribution_Type::RANDOM_UNIFORM;
  Percentage_of_Hot_Region = 0;
  Generated_Aligned_Addresses = true;
  Address_Alignment_Unit = 16;

  Request_Size_Distribution = Utils::Request_Size_Distribution_Type::FIXED;
  Average_Request_Size = 8;
  Variance_Request_Size = 0;

  Seed = seed;
  Average_No_of_Reqs_in_Queue = 2;
  Bandwidth = 262144;

  Stop_Time = 1000000000;
  Total_Requests_To_Generate = 0;

  __update_hidden_data();
}

void
SyntheticFlowParamSet::XML_serialize(Utils::XmlWriter& xmlwriter) const
{
  xmlwriter.Write_open_tag("SyntheticFlowParamSet");
  IOFlowParamSet::XML_serialize(xmlwriter);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Working_Set_Percentage);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Synthetic_Generator_Type);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Read_Percentage);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Address_Distribution);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Percentage_of_Hot_Region);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Generated_Aligned_Addresses);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Address_Alignment_Unit);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Request_Size_Distribution);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Average_Request_Size);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Variance_Request_Size);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Seed);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Average_No_of_Reqs_in_Queue);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Bandwidth);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Stop_Time);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Total_Requests_To_Generate);

  xmlwriter.Write_close_tag();
}

void
SyntheticFlowParamSet::XML_deserialize(rapidxml::xml_node<> *node)
{
  IOFlowParamSet::XML_deserialize(node);

  try {
    for (auto param = node->first_node(); param; param = param->next_sibling()) {
      if (strcmp(param->name(), "Working_Set_Percentage") == 0)
        Working_Set_Percentage = std::stoi(param->value());

      else if (strcmp(param->name(), "Synthetic_Generator_Type") == 0)
        Synthetic_Generator_Type = to_request_generator_type(param->value());

      else if (strcmp(param->name(), "Read_Percentage") == 0)
        Read_Percentage = std::stoi(param->value());

      else if (strcmp(param->name(), "Address_Distribution") == 0)
        Address_Distribution = to_address_distribution_type(param->value());

      else if (strcmp(param->name(), "Percentage_of_Hot_Region") == 0)
        Percentage_of_Hot_Region = std::stoi(param->value());

      else if (strcmp(param->name(), "Generated_Aligned_Addresses") == 0)
        Generated_Aligned_Addresses = to_bool(param->value());

      else if (strcmp(param->name(), "Address_Alignment_Unit") == 0)
        Address_Alignment_Unit = std::stoi(param->value());

      else if (strcmp(param->name(), "Request_Size_Distribution") == 0)
        Request_Size_Distribution = to_request_size_distribution_type(param->value());

      else if (strcmp(param->name(), "Average_Request_Size") == 0)
        Average_Request_Size = std::stoi(param->value());

      else if (strcmp(param->name(), "Variance_Request_Size") == 0)
        Variance_Request_Size = std::stoi(param->value());

      else if (strcmp(param->name(), "Seed") == 0)
        Seed = std::stoi(param->value());

      else if (strcmp(param->name(), "Average_No_of_Reqs_in_Queue") == 0)
        Average_No_of_Reqs_in_Queue = std::stoi(param->value());

      else if (strcmp(param->name(), "Bandwidth") == 0)
        Bandwidth = std::stoi(param->value());

      else if (strcmp(param->name(), "Stop_Time") == 0)
        Stop_Time = std::stoll(param->value());

      else if (strcmp(param->name(), "Total_Requests_To_Generate") == 0)
        Total_Requests_To_Generate = std::stoi(param->value());

    }

  } catch (...) {
    throw mqsim_error("Error in SyntheticFlowParamSet!");
  }

  __update_hidden_data();
}

// -------------------------------------
// Constructor for TraceFlowParameterSet
// -------------------------------------
TraceFlowParameterSet::TraceFlowParameterSet()
  : IOFlowParamSet(Flow_Type::TRACE),
    File_Path(),
    Percentage_To_Be_Executed(100),
    Replay_Count(1)
{ }

TraceFlowParameterSet::TraceFlowParameterSet(rapidxml::xml_node<>* node)
  : IOFlowParamSet(Flow_Type::TRACE),
    File_Path(),
    Percentage_To_Be_Executed(100),
    Replay_Count(1)
{
  XML_deserialize(node);
}

void
TraceFlowParameterSet::XML_serialize(Utils::XmlWriter& xmlwriter) const
{
  xmlwriter.Write_open_tag("TraceFlowParameterSet");
  IOFlowParamSet::XML_serialize(xmlwriter);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, File_Path);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Percentage_To_Be_Executed);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Replay_Count);

  xmlwriter.Write_close_tag();
}

void
TraceFlowParameterSet::XML_deserialize(rapidxml::xml_node<> *node)
{
  IOFlowParamSet::XML_deserialize(node);

  try {
    for (auto param = node->first_node(); param; param = param->next_sibling()) {
      if (strcmp(param->name(), "Replay_Count") == 0)
        Replay_Count = std::stoi(param->value());

      else if (strcmp(param->name(), "Percentage_To_Be_Executed") == 0)
        Percentage_To_Be_Executed = std::stoi(param->value());

      else if (strcmp(param->name(), "File_Path") == 0)
        File_Path = param->value();

    }
  } catch (...) {
    throw mqsim_error("Error in TraceFlowParameterSet!");

  }
}

// ---------------------
// StreamIdInfo Definition
// ---------------------

template <typename T>
force_inline T
__build_id_list(uint32_t count)
{
  T id_list(count);
  std::iota(id_list.begin(), id_list.end(), 0);

  return id_list;
}

force_inline void
StreamIdInfo::__init_sata_stream(const DeviceParameterSet& params,
                                 uint32_t flow_count)
{
  for (uint32_t i = 0; i < flow_count; ++i) {
    __stream_channel_ids.emplace_back(
      __build_id_list<ChannelIDs>(params.Flash_Channel_Count)
    );

    __stream_chip_ids.emplace_back(
      __build_id_list<ChipIDs>(params.Chip_No_Per_Channel)
    );

    __stream_die_ids.emplace_back(
      __build_id_list<DieIDs>(params.Flash_Parameters.Die_No_Per_Chip)
    );

    __stream_plane_ids.emplace_back(
      __build_id_list<PlaneIDs>(params.Flash_Parameters.Plane_No_Per_Die)
    );
  }
}

force_inline void
StreamIdInfo::__init_nvme_stream(const IOFlowScenario& io_flows)
{
  for (auto& flow : io_flows) {
    __stream_channel_ids.emplace_back(
      ChannelIDs(flow->Channel_IDs.begin(), flow->Channel_IDs.end())
    );

    __stream_chip_ids.emplace_back(
      ChipIDs(flow->Chip_IDs.begin(), flow->Chip_IDs.end())
    );

    __stream_die_ids.emplace_back(
      DieIDs(flow->Die_IDs.begin(), flow->Die_IDs.end())
    );

    __stream_plane_ids.emplace_back(
      PlaneIDs(flow->Plane_IDs.begin(), flow->Plane_IDs.end())
    );
  }
}

StreamIdInfo::StreamIdInfo(const DeviceParameterSet& params,
                           const IOFlowScenario& io_flows)
  : stream_count(params.HostInterface_Type == HostInterface_Types::SATA
                 ? 1U
                 : io_flows.size())
{
  if (params.HostInterface_Type == HostInterface_Types::SATA)
    __init_sata_stream(params, io_flows.size());
  else
    __init_nvme_stream(io_flows);
}
