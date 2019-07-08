#ifndef FLASH_PARAMETER_SET_H
#define FLASH_PARAMETER_SET_H

#include <numeric>

#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../../sim/Sim_Defs.h"

#include "ParameterSetBase.h"

typedef std::vector<sim_time_type> LatencyList;

class FlashParameterSet : public ParameterSetBase {
public:
  Flash_Technology_Type Flash_Technology;
  NVM::FlashMemory::Command_Suspension_Mode CMD_Suspension_Support;

  sim_time_type Page_Read_Latency_LSB;
  sim_time_type Page_Read_Latency_CSB;
  sim_time_type Page_Read_Latency_MSB;

  sim_time_type Page_Program_Latency_LSB;
  sim_time_type Page_Program_Latency_CSB;
  sim_time_type Page_Program_Latency_MSB;

  //Block erase latency in nano-seconds
  sim_time_type Block_Erase_Latency;

  uint32_t Block_PE_Cycles_Limit;

  //in nano-seconds
  sim_time_type Suspend_Erase_Time;
  sim_time_type Suspend_Program_Time;

  uint32_t Die_No_Per_Chip;
  uint32_t Plane_No_Per_Die;
  uint32_t Block_No_Per_Plane;
  uint32_t Page_No_Per_Block;

  //Flash page capacity in bytes
  uint32_t Page_Capacity;

  //Flash page metadata capacity in bytes
  uint32_t Page_Metadata_Capacity;

  const LatencyList read_latencies;
  const LatencyList write_latencies;

private:
  uint32_t __page_size_in_sector;
  uint64_t __plane_size_in_sector;

  void __update_rw_lat();
public:
  FlashParameterSet();

  uint32_t page_size_in_sector() const;
  uint64_t plane_size_in_sector() const;

  sim_time_type avg_read_latency() const;
  sim_time_type avg_write_latency() const;

  bool program_suspension_support() const;
  bool erase_suspension_support() const;

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

force_inline uint32_t
FlashParameterSet::page_size_in_sector() const
{
  return __page_size_in_sector;
}

force_inline uint64_t
FlashParameterSet::plane_size_in_sector() const
{
  return __plane_size_in_sector;
}

force_inline sim_time_type
FlashParameterSet::avg_read_latency() const
{
  return std::accumulate(read_latencies.begin(), read_latencies.end(), 0ULL)
           / read_latencies.size();
}

force_inline sim_time_type
FlashParameterSet::avg_write_latency() const
{
  return std::accumulate(write_latencies.begin(), write_latencies.end(), 0ULL)
           / write_latencies.size();
}

force_inline bool
FlashParameterSet::program_suspension_support() const
{
  using namespace NVM::FlashMemory;

  return CMD_Suspension_Support & Command_Suspension_Mode::PROGRAM;
}

force_inline bool
FlashParameterSet::erase_suspension_support() const
{
  using namespace NVM::FlashMemory;

  return CMD_Suspension_Support & Command_Suspension_Mode::ERASE;
}


#endif // !FLASH_PARAMETER_SET_H
