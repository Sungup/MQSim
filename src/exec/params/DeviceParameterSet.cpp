#include "DeviceParameterSet.h"

#include <algorithm>

using namespace SSD_Components;

DeviceParameterSet::DeviceParameterSet()
  : Seed(123),
    Enabled_Preconditioning(true),
    Memory_Type(NVM::NVM_Type::FLASH),
    HostInterface_Type(HostInterface_Types::NVME),
    IO_Queue_Depth(1024),
    Queue_Fetch_Size(512),
    Caching_Mechanism(Caching_Mechanism::ADVANCED),
    Data_Cache_Sharing_Mode(Cache_Sharing_Mode::SHARED),
    Data_Cache_Capacity(1024 * 1024 * 512),
    Data_Cache_DRAM_Row_Size(8192),
    Data_Cache_DRAM_Data_Rate(800),
    Data_Cache_DRAM_Data_Busrt_Size(4),
    Data_Cache_DRAM_tRCD(13),
    Data_Cache_DRAM_tCL(13),
    Data_Cache_DRAM_tRP(13),
    Address_Mapping(Flash_Address_Mapping_Type::PAGE_LEVEL),
    Ideal_Mapping_Table(false),
    CMT_Capacity(2 * 1024 * 1024),
    CMT_Sharing_Mode(CMT_Sharing_Mode::SHARED),
    Plane_Allocation_Scheme(Flash_Plane_Allocation_Scheme_Type::CWDP),
    Transaction_Scheduling_Policy(Flash_Scheduling_Type::OUT_OF_ORDER),
    Overprovisioning_Ratio(0.07),
    GC_Exec_Threshold(0.05),
    GC_Block_Selection_Policy(GC_Block_Selection_Policy_Type::RGA),
    Use_Copyback_for_GC(false),
    Preemptible_GC_Enabled(true),
    GC_Hard_Threshold(0.005),
    Dynamic_Wearleveling_Enabled(true),
    Static_Wearleveling_Enabled(true),
    Static_Wearleveling_Threshold(100),
    Preferred_suspend_erase_time_for_read(700000),
    Preferred_suspend_erase_time_for_write(700000),
    Preferred_suspend_write_time_for_read(100000),
    Flash_Channel_Count(8),
    Flash_Channel_Width(1),
    Channel_Transfer_Rate(300),
    Chip_No_Per_Channel(4),
    Flash_Comm_Protocol(ONFI_Protocol::NVDDR2),
    Flash_Parameters()
{ }

void
DeviceParameterSet::XML_serialize(Utils::XmlWriter& xmlwriter) const
{
  xmlwriter.Write_open_tag("DeviceParameterSet");

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Seed);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Enabled_Preconditioning);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Memory_Type);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, HostInterface_Type);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, IO_Queue_Depth);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Queue_Fetch_Size);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Caching_Mechanism);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_Sharing_Mode);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_Capacity);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_DRAM_Row_Size);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_DRAM_Data_Rate);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_DRAM_Data_Busrt_Size);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_DRAM_tRCD);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_DRAM_tCL);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Data_Cache_DRAM_tRP);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Address_Mapping);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Use_Copyback_for_GC);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, CMT_Capacity);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, CMT_Sharing_Mode);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Plane_Allocation_Scheme);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Transaction_Scheduling_Policy);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Overprovisioning_Ratio);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, GC_Exec_Threshold);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, GC_Block_Selection_Policy);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Use_Copyback_for_GC);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Preemptible_GC_Enabled);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, GC_Hard_Threshold);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Dynamic_Wearleveling_Enabled);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Static_Wearleveling_Enabled);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Static_Wearleveling_Threshold);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Preferred_suspend_erase_time_for_read);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Preferred_suspend_erase_time_for_write);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Preferred_suspend_write_time_for_read);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Flash_Channel_Count);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Flash_Channel_Width);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Channel_Transfer_Rate);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Chip_No_Per_Channel);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Flash_Comm_Protocol);

  Flash_Parameters.XML_serialize(xmlwriter);

  xmlwriter.Write_close_tag();
}

void
DeviceParameterSet::XML_deserialize(rapidxml::xml_node<> *node)
{
  try {
    for (auto param = node->first_node(); param; param = param->next_sibling()) {
      if (strcmp(param->name(), "Seed") == 0)
        Seed = std::stoi(param->value());

      else if (strcmp(param->name(), "Enabled_Preconditioning") == 0)
        Enabled_Preconditioning = to_bool(param->value());

      else if (strcmp(param->name(), "Memory_Type") == 0)
        Memory_Type = to_nvm_type(param->value());

      else if (strcmp(param->name(), "HostInterface_Type") == 0)
        HostInterface_Type = to_host_interface_type(param->value());

      else if (strcmp(param->name(), "IO_Queue_Depth") == 0)
        IO_Queue_Depth = (uint16_t) std::stoull(param->value());

      else if (strcmp(param->name(), "Queue_Fetch_Size") == 0)
        Queue_Fetch_Size = (uint16_t) std::stoull(param->value());

      else if (strcmp(param->name(), "Caching_Mechanism") == 0)
        Caching_Mechanism = to_cacheing_mechanism(param->value());

      else if (strcmp(param->name(), "Data_Cache_Sharing_Mode") == 0)
        Data_Cache_Sharing_Mode = to_cache_sharing_mode(param->value());

      else if (strcmp(param->name(), "Data_Cache_Capacity") == 0)
        Data_Cache_Capacity = std::stoul(param->value());

      else if (strcmp(param->name(), "Data_Cache_DRAM_Row_Size") == 0)
        Data_Cache_DRAM_Row_Size = std::stoul(param->value());

      else if (strcmp(param->name(), "Data_Cache_DRAM_Data_Rate") == 0)
        Data_Cache_DRAM_Data_Rate = std::stoul(param->value());

      else if (strcmp(param->name(), "Data_Cache_DRAM_Data_Busrt_Size") == 0)
        Data_Cache_DRAM_Data_Busrt_Size = std::stoul(param->value());

      else if (strcmp(param->name(), "Data_Cache_DRAM_tRCD") == 0)
        Data_Cache_DRAM_tRCD = std::stoul(param->value());

      else if (strcmp(param->name(), "Data_Cache_DRAM_tCL") == 0)
        Data_Cache_DRAM_tCL = std::stoul(param->value());

      else if (strcmp(param->name(), "Data_Cache_DRAM_tRP") == 0)
        Data_Cache_DRAM_tRP = std::stoul(param->value());

      else if (strcmp(param->name(), "Address_Mapping") == 0)
        Address_Mapping = to_flash_addr_mapping(param->value());

      else if (strcmp(param->name(), "Ideal_Mapping_Table") == 0)
        Ideal_Mapping_Table = to_bool(param->value());

      else if (strcmp(param->name(), "CMT_Capacity") == 0)
        CMT_Capacity = std::stoul(param->value());

      else if (strcmp(param->name(), "CMT_Sharing_Mode") == 0)
        CMT_Sharing_Mode = to_cmt_sharing_mode(param->value());

      else if (strcmp(param->name(), "Plane_Allocation_Scheme") == 0)
        Plane_Allocation_Scheme = to_flash_plane_alloc_scheme(param->value());

      else if (strcmp(param->name(), "Transaction_Scheduling_Policy") == 0)
        Transaction_Scheduling_Policy = to_scheduling_policy(param->value());

      else if (strcmp(param->name(), "Overprovisioning_Ratio") == 0)
        Overprovisioning_Ratio = std::stod(param->value());

      else if (strcmp(param->name(), "GC_Exec_Threshold") == 0)
        GC_Exec_Threshold = std::stod(param->value());

      else if (strcmp(param->name(), "GC_Block_Selection_Policy") == 0)
        GC_Block_Selection_Policy = to_gc_block_selection_policy(param->value());

      else if (strcmp(param->name(), "Use_Copyback_for_GC") == 0)
        Use_Copyback_for_GC = to_bool(param->value());

      else if (strcmp(param->name(), "Preemptible_GC_Enabled") == 0)
        Preemptible_GC_Enabled = to_bool(param->value());

      else if (strcmp(param->name(), "GC_Hard_Threshold") == 0)
        GC_Hard_Threshold = std::stod(param->value());

      else if (strcmp(param->name(), "Dynamic_Wearleveling_Enabled") == 0)
        Dynamic_Wearleveling_Enabled = to_bool(param->value());

      else if (strcmp(param->name(), "Static_Wearleveling_Enabled") == 0)
        Static_Wearleveling_Enabled = to_bool(param->value());

      else if (strcmp(param->name(), "Static_Wearleveling_Threshold") == 0)
        Static_Wearleveling_Threshold = std::stoul(param->value());

      else if (strcmp(param->name(), "Prefered_suspend_erase_time_for_read") == 0)
        Preferred_suspend_erase_time_for_read = std::stoull(param->value());

      else if (strcmp(param->name(), "Preferred_suspend_erase_time_for_write") == 0)
        Preferred_suspend_erase_time_for_write = std::stoull(param->value());

      else if (strcmp(param->name(), "Preferred_suspend_write_time_for_read") == 0)
        Preferred_suspend_write_time_for_read = std::stoull(param->value());

      else if (strcmp(param->name(), "Flash_Channel_Count") == 0)
        Flash_Channel_Count = std::stoul(param->value());

      else if (strcmp(param->name(), "Flash_Channel_Width") == 0)
        Flash_Channel_Width = std::stoul(param->value());

      else if (strcmp(param->name(), "Channel_Transfer_Rate") == 0)
        Channel_Transfer_Rate = std::stoul(param->value());

      else if (strcmp(param->name(), "Chip_No_Per_Channel") == 0)
        Chip_No_Per_Channel = std::stoul(param->value());

      else if (strcmp(param->name(), "Flash_Comm_Protocol") == 0)
        Flash_Comm_Protocol = to_onfi_protocol(param->value());

      else if (strcmp(param->name(), "FlashParameterSet") == 0)
        Flash_Parameters.XML_deserialize(param);
    }

  } catch (...) {
    PRINT_ERROR("Error in DeviceParameterSet!")
  }

  if(Overprovisioning_Ratio < 0.05)
    PRINT_MESSAGE("The specified overprovisioning ratio is too small. The simluation may not run correctly.")
}
