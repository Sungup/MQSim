#ifndef IO_FLOW_TRACE_BASED_H
#define IO_FLOW_TRACE_BASED_H

#include <string>
#include <iostream>
#include <fstream>
#include "IO_Flow_Base.h"
#include "ASCII_Trace_Definition.h"

namespace Host_Components
{
  class IO_Flow_Trace_Based : public IO_Flow_Base {
  private:
    const uint32_t percentage_to_be_simulated;
    const std::string trace_file_path;
    const uint32_t total_replay_no;

    uint32_t total_requests_in_file;
    uint32_t replay_counter;
    sim_time_type time_offset;

    std::vector<std::string> current_trace_line;
    std::ifstream trace_file;

  public:
    IO_Flow_Trace_Based(const sim_object_id_type& name,
                        const HostParameterSet& host_params,
                        const TraceFlowParameterSet& flow_params,
                        const Utils::LogicalAddrPartition& lapu,
                        uint16_t flow_id,
                        uint16_t sq_size,
                        uint16_t cq_size,
                        HostInterface_Types interface_type,
                        PCIe_Root_Complex* root_complex,
                        SATA_HBA* sata_hba);
    ~IO_Flow_Trace_Based() final = default;

    HostIORequest* Generate_next_request();
    void Start_simulation() final;
    void Execute_simulator_event(MQSimEngine::SimEvent* event) final;

    void get_stats(Utils::Workload_Statistics& stats,
                   const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                   const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) final;

  };
}

#endif// !IO_FLOW_TRACE_BASED_H