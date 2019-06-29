#ifndef DEVICE_PARAMETER_SET_H
#define DEVICE_PARAMETER_SET_H

#include "../../nvm_chip/NVM_Types.h"
#include "../../ssd/dcm/DataCacheDefs.h"
#include "../../ssd/gc_and_wl/GCandWLUnitDefs.h"
#include "../../ssd/interface/Host_Interface_Defs.h"
#include "../../ssd/mapping/AddressMappingUnitDefs.h"
#include "../../ssd/tsu/TSU_Defs.h"
#include "../../ssd/ONFIChannelDefs.h"
#include "../../ssd/SSD_Defs.h"

#include "FlashParameterSet.h"
#include "ParameterSetBase.h"

class DeviceParameterSet : public ParameterSetBase {
public:
  // Seed for random number generation
  // (used in device's random number generators)
  mutable int Seed;
  bool Enabled_Preconditioning;
  NVM::NVM_Type Memory_Type;
  HostInterface_Types HostInterface_Type;

  // For NVMe, it determines the size of the submission/completion queues;
  // for SATA, it determines the size of NCQ_Control_Structure
  uint16_t IO_Queue_Depth;

  // Used in NVMe host interface
  uint16_t Queue_Fetch_Size;
  SSD_Components::Caching_Mechanism Caching_Mechanism;

  // Data cache sharing among concurrently running I/O flows,
  // if NVMe host interface is used
  SSD_Components::Cache_Sharing_Mode Data_Cache_Sharing_Mode;

  uint32_t Data_Cache_Capacity;       // bytes
  uint32_t Data_Cache_DRAM_Row_Size;  // bytes
  uint32_t Data_Cache_DRAM_Data_Rate; // MT/s

  // The number of bytes that are transferred in one burst
  // (it depends on the number of DRAM chips)
  uint32_t Data_Cache_DRAM_Data_Busrt_Size;

  // Latency parameters to access DRAM in the data cache,
  // the unit is nano-seconds
  sim_time_type Data_Cache_DRAM_tRCD;
  sim_time_type Data_Cache_DRAM_tCL;
  sim_time_type Data_Cache_DRAM_tRP;

  SSD_Components::Flash_Address_Mapping_Type Address_Mapping;

  // If mapping is ideal, then all the mapping entries are found in the DRAM
  // and there is no need to read mapping entries from flash
  bool Ideal_Mapping_Table;

  // Size of SRAM/DRAM space that is used to cache address mapping table,
  // the unit is bytes
  uint32_t CMT_Capacity;

  // How the entire CMT space is shared among concurrently running flows
  SSD_Components::CMT_Sharing_Mode CMT_Sharing_Mode;
  SSD_Components::Flash_Plane_Allocation_Scheme_Type Plane_Allocation_Scheme;
  SSD_Components::Flash_Scheduling_Type Transaction_Scheduling_Policy;

  // The ratio of spare space with respect to the whole available storage space
  double Overprovisioning_Ratio;

  // The threshold for the ratio of free pages that used to trigger GC
  double GC_Exec_Threshold;
  SSD_Components::GC_Block_Selection_Policy_Type GC_Block_Selection_Policy;
  bool Use_Copyback_for_GC;
  bool Preemptible_GC_Enabled;

  // The hard gc execution threshold, used to stop preemptible gc execution
  double GC_Hard_Threshold;
  bool Dynamic_Wearleveling_Enabled;
  bool Static_Wearleveling_Enabled;
  uint32_t Static_Wearleveling_Threshold;

  // in nano-seconds, if the remaining time of the ongoing erase is smaller
  // than Prefered_suspend_erase_time_for_read, then the ongoing erase operation
  // will be suspended
  sim_time_type Preferred_suspend_erase_time_for_read;

  // in nano-seconds, if the remaining time of the ongoing erase is smaller
  // than Prefered_suspend_erase_time_for_write, then the ongoing erase
  // operation will be suspended
  sim_time_type Preferred_suspend_erase_time_for_write;

  // in nano-seconds, if the remaining time of the ongoing write is smaller
  // than Prefered_suspend_write_time_for_read, then the ongoing erase operation
  // will be suspended
  sim_time_type Preferred_suspend_write_time_for_read;

  uint32_t Flash_Channel_Count;
  uint32_t Flash_Channel_Width; // bytes
  uint32_t Channel_Transfer_Rate; // MT/s
  uint32_t Chip_No_Per_Channel;

  SSD_Components::ONFI_Protocol Flash_Comm_Protocol;
  FlashParameterSet Flash_Parameters;

  DeviceParameterSet();
  ~DeviceParameterSet() override = default;

  int gen_seed() const;

  void XML_serialize(Utils::XmlWriter& xmlwriter) const final;
  void XML_deserialize(rapidxml::xml_node<> *node) final;
};

force_inline int
DeviceParameterSet::gen_seed() const
{
  return Seed++;
}

#endif // !DEVICE_PARAMETER_SET_H
