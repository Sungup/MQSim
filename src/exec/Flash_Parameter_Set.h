#ifndef FLASH_PARAMETER_SET_H
#define FLASH_PARAMETER_SET_H

#include "../sim/Sim_Defs.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "params/ParameterSetBase.h"

// TODO Remove static features

class Flash_Parameter_Set : ParameterSetBase
{
public:
  static Flash_Technology_Type Flash_Technology;
  static NVM::FlashMemory::Command_Suspension_Mode CMD_Suspension_Support;
  static sim_time_type Page_Read_Latency_LSB;
  static sim_time_type Page_Read_Latency_CSB;
  static sim_time_type Page_Read_Latency_MSB;
  static sim_time_type Page_Program_Latency_LSB;
  static sim_time_type Page_Program_Latency_CSB;
  static sim_time_type Page_Program_Latency_MSB;
  static sim_time_type Block_Erase_Latency;//Block erase latency in nano-seconds
  static uint32_t Block_PE_Cycles_Limit;
  static sim_time_type Suspend_Erase_Time;//in nano-seconds
  static sim_time_type Suspend_Program_Time;//in nano-seconds
  static uint32_t Die_No_Per_Chip;
  static uint32_t Plane_No_Per_Die;
  static uint32_t Block_No_Per_Plane;
  static uint32_t Page_No_Per_Block;//Page no per block
  static uint32_t Page_Capacity;//Flash page capacity in bytes
  static uint32_t Page_Metadat_Capacity;//Flash page metadata capacity in bytes
  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

#endif // !FLASH_PARAMETER_SET_H
