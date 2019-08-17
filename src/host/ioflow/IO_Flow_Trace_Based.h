#ifndef IO_FLOW_TRACE_BASED_H
#define IO_FLOW_TRACE_BASED_H

#include <string>
#include <iostream>
#include <fstream>

#include "IO_Flow_Base.h"

namespace Host_Components
{
  /// ---------------
  /// TraceItem class
  /// ---------------
  class TraceItem {
  private:
    static constexpr char DELIMITER_COUNT = 4;
    static constexpr char DELIMITER_CHAR = ' ';

    static constexpr char READ_CODE = '1';
    static constexpr int DELIMITER_LEN = 1;

  private:
    sim_time_type __time;
    uint32_t __device;
    LHA_type __lba;
    uint32_t __count;
    HostIOReqType __type;

  public:
    TraceItem();
    ~TraceItem() = default;

    static bool check_valid(const char* string, int len);

    bool parse(const char* str, char** end,
               sim_time_type offset_base,
               LHA_type start, LHA_type region_size);

    bool parse(const std::string& str,
               sim_time_type offset_base,
               LHA_type start, LHA_type region_size);

    sim_time_type time() const;
    LHA_type lba() const;
    uint32_t lba_count() const;
    HostIOReqType io_type() const;

    bool is_read() const;
  };

  /// -----------------------
  /// IO flow for trace class
  /// -----------------------
  class IO_Flow_Trace_Based : public IO_Flow_Base {
  private:
    const LHA_type    __flow_region_size;
    const std::string __trc_path;
    const double      __load_percent;
    const uint32_t    __max_replay;
    const uint32_t    __max_reqs_in_file;

    uint32_t      __replay_count;
    uint32_t      __req_count_in_file;

    sim_time_type __time_offset;
    TraceItem     __item;
    std::ifstream __trc_stream;

  private:
    HostIORequest* __generate_next_req();

    uint32_t __check_valid_requests();
    void __move_to_begin();
    void __load_item();

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

    void Start_simulation() final;
    void Execute_simulator_event(MQSimEngine::SimEvent* event) final;

    void get_stats(Utils::Workload_Statistics& stats,
                   const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                   const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) final;

  };
}

#endif// !IO_FLOW_TRACE_BASED_H