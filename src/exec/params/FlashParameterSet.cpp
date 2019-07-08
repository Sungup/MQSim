#include "FlashParameterSet.h"

#include <algorithm>
#include <string.h>

#include "../../sim/Engine.h"
#include "../../ssd/SSD_Defs.h"

force_inline void
FlashParameterSet::__update_rw_lat()
{
  int bits_per_cell = static_cast<int>(Flash_Technology);

  auto& rd_lat = const_cast<LatencyList&>(read_latencies);
  auto& wr_lat = const_cast<LatencyList&>(write_latencies);

  rd_lat.clear(); wr_lat.clear();
  rd_lat.reserve(bits_per_cell); wr_lat.reserve(bits_per_cell);

  rd_lat.emplace_back(Page_Read_Latency_LSB);
  wr_lat.emplace_back(Page_Program_Latency_LSB);

  if (2 < bits_per_cell) {
    rd_lat.emplace_back(Page_Read_Latency_CSB);
    wr_lat.emplace_back(Page_Program_Latency_CSB);
  }

  if (1 < bits_per_cell) {
    rd_lat.emplace_back(Page_Read_Latency_MSB);
    wr_lat.emplace_back(Page_Program_Latency_MSB);
  }
}

FlashParameterSet::FlashParameterSet()
  : Flash_Technology(Flash_Technology_Type::MLC),
    CMD_Suspension_Support(NVM::FlashMemory::Command_Suspension_Mode::ERASE),
    Page_Read_Latency_LSB(75000),
    Page_Read_Latency_CSB(75000),
    Page_Read_Latency_MSB(75000),
    Page_Program_Latency_LSB(750000),
    Page_Program_Latency_CSB(750000),
    Page_Program_Latency_MSB(750000),
    Block_Erase_Latency(3800000),
    Block_PE_Cycles_Limit(10000),
    Suspend_Erase_Time(700000),
    Suspend_Program_Time(100000),
    Die_No_Per_Chip(2),
    Plane_No_Per_Die(2),
    Block_No_Per_Plane(2048),
    Page_No_Per_Block(256),
    Page_Capacity(8192),
    Page_Metadata_Capacity(1872),
    read_latencies(),
    write_latencies(),
    __page_size_in_sector(Page_Capacity / SECTOR_SIZE_IN_BYTE),
    __plane_size_in_sector(Block_No_Per_Plane
                             * Page_No_Per_Block
                             * __page_size_in_sector)
{
  __update_rw_lat();
}

void
FlashParameterSet::XML_serialize(Utils::XmlWriter& xmlwriter) const
{
  xmlwriter.Write_open_tag("FlashParameterSet");

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Flash_Technology);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, CMD_Suspension_Support);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Read_Latency_LSB);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Read_Latency_CSB);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Read_Latency_MSB);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Program_Latency_LSB);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Program_Latency_CSB);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Program_Latency_MSB);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Block_Erase_Latency);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Block_PE_Cycles_Limit);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Suspend_Erase_Time);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Suspend_Program_Time);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Die_No_Per_Chip);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Plane_No_Per_Die);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Block_No_Per_Plane);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_No_Per_Block);

  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Capacity);
  XML_WRITER_MACRO_WRITE_ATTR_STR(xmlwriter, Page_Metadata_Capacity);

  xmlwriter.Write_close_tag();
}

void
FlashParameterSet::XML_deserialize(rapidxml::xml_node<> *node)
{
  try {
    for (auto param = node->first_node();
         param;
         param = param->next_sibling()) {
      if (strcmp(param->name(), "Flash_Technology") == 0)
        Flash_Technology = to_flash_type(param->value());

      else if (strcmp(param->name(), "CMD_Suspension_Support") == 0)
        CMD_Suspension_Support = to_command_suspension_mode(param->value());

      else if (strcmp(param->name(), "Page_Read_Latency_LSB") == 0)
        Page_Read_Latency_LSB = std::stoull(param->value());

      else if (strcmp(param->name(), "Page_Read_Latency_CSB") == 0)
        Page_Read_Latency_CSB = std::stoull(param->value());

      else if (strcmp(param->name(), "Page_Read_Latency_MSB") == 0)
        Page_Read_Latency_MSB = std::stoull(param->value());

      else if (strcmp(param->name(), "Page_Program_Latency_LSB") == 0)
        Page_Program_Latency_LSB = std::stoull(param->value());

      else if (strcmp(param->name(), "Page_Program_Latency_CSB") == 0)
        Page_Program_Latency_CSB = std::stoull(param->value());

      else if (strcmp(param->name(), "Page_Program_Latency_MSB") == 0)
        Page_Program_Latency_MSB = std::stoull(param->value());

      else if (strcmp(param->name(), "Block_Erase_Latency") == 0)
        Block_Erase_Latency = std::stoull(param->value());

      else if (strcmp(param->name(), "Block_PE_Cycles_Limit") == 0)
        Block_PE_Cycles_Limit = std::stoul(param->value());

      else if (strcmp(param->name(), "Suspend_Erase_Time") == 0)
        Suspend_Erase_Time = std::stoull(param->value());

      else if (strcmp(param->name(), "Suspend_Program_Time") == 0)
        Suspend_Program_Time = std::stoull(param->value());

      else if (strcmp(param->name(), "Die_No_Per_Chip") == 0)
        Die_No_Per_Chip = std::stoul(param->value());

      else if (strcmp(param->name(), "Plane_No_Per_Die") == 0)
        Plane_No_Per_Die = std::stoul(param->value());

      else if (strcmp(param->name(), "Block_No_Per_Plane") == 0)
        Block_No_Per_Plane = std::stoul(param->value());

      else if (strcmp(param->name(), "Page_No_Per_Block") == 0)
        Page_No_Per_Block = std::stoul(param->value());

      else if (strcmp(param->name(), "Page_Capacity") == 0)
        Page_Capacity = std::stoul(param->value());

      else if (strcmp(param->name(), "Page_Metadata_Capacity") == 0)
        Page_Metadata_Capacity = std::stoul(param->value());
    }

  } catch (...) {
    throw mqsim_error("Error in the FlashParameterSet!");

  }

  __update_rw_lat();
  __page_size_in_sector = Page_Capacity / SECTOR_SIZE_IN_BYTE;
  __plane_size_in_sector = Block_No_Per_Plane
                             * Page_No_Per_Block
                             * __page_size_in_sector;
}