#ifndef FLASH_PARAMETER_SET_H
#define FLASH_PARAMETER_SET_H

#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../../sim/Sim_Defs.h"

#include "ParameterSetBase.h"

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

public:
  FlashParameterSet();

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

#endif // !FLASH_PARAMETER_SET_H
