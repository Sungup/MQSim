#ifndef IO_FLOW_TRACE_BASED_H
#define IO_FLOW_TRACE_BASED_H

#include <string>
#include <iostream>
#include <fstream>
#include "IO_Flow_Base.h"
#include "ASCII_Trace_Definition.h"

namespace Host_Components
{
  class IO_Flow_Trace_Based : public IO_Flow_Base
  {
  public:
    IO_Flow_Trace_Based(const sim_object_id_type& name, uint16_t flow_id, LHA_type start_lsa_on_device, LHA_type end_lsa_on_device, uint16_t io_queue_id,
      uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class, double initial_occupancy_ratio,
      std::string trace_file_path, Trace_Time_Unit time_unit, uint32_t total_replay_count, uint32_t percentage_to_be_simulated,
      HostInterface_Types SSD_device_type, PCIe_Root_Complex* pcie_root_complex, SATA_HBA* sata_hba,
      bool enabled_logging, sim_time_type logging_period, std::string logging_file_path);
    ~IO_Flow_Trace_Based() final;
    Host_IO_Request* Generate_next_request();
    void NVMe_consume_io_request(Completion_Queue_Entry*);
    void SATA_consume_io_request(Host_IO_Request*);
    void Start_simulation();
    void Validate_simulation_config();
    void Execute_simulator_event(MQSimEngine::Sim_Event*);

    void get_stats(Utils::Workload_Statistics& stats,
                   const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                   const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) final;

  private:
    Trace_Time_Unit time_unit;
    uint32_t percentage_to_be_simulated;
    std::string trace_file_path;
    std::ifstream trace_file;
    uint32_t total_replay_no, replay_counter;
    uint32_t total_requests_in_file;
    std::vector<std::string> current_trace_line;
    sim_time_type time_offset;
  };
}

#endif// !IO_FLOW_TRACE_BASED_H