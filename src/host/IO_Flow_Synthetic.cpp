#include <math.h>
#include <stdexcept>
#include "../sim/Engine.h"
#include "IO_Flow_Synthetic.h"

namespace Host_Components
{
  IO_Flow_Synthetic::IO_Flow_Synthetic(const sim_object_id_type& name, uint16_t flow_id,
    LHA_type start_lsa_on_device, LHA_type end_lsa_on_device, double working_set_ratio, uint16_t io_queue_id,
    uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
    double read_ratio, Utils::Address_Distribution_Type address_distribution, double hot_region_ratio,
    Utils::Request_Size_Distribution_Type request_size_distribution, uint32_t average_request_size, uint32_t variance_request_size,
    Utils::Request_Generator_Type generator_type, sim_time_type Average_inter_arrival_time_nano_sec, uint32_t average_number_of_enqueued_requests,
    bool generate_aligned_addresses, uint32_t alignment_value,
    int seed, sim_time_type stop_time, double initial_occupancy_ratio, uint32_t total_req_count, HostInterface_Types SSD_device_type, PCIe_Root_Complex* pcie_root_complex, SATA_HBA* sata_hba,
    bool enabled_logging, sim_time_type logging_period, const std::string& logging_file_path) :
    IO_Flow_Base(name, flow_id, start_lsa_on_device, LHA_type(start_lsa_on_device + (end_lsa_on_device - start_lsa_on_device) * working_set_ratio), io_queue_id, nvme_submission_queue_size, nvme_completion_queue_size, priority_class, stop_time, initial_occupancy_ratio, total_req_count, SSD_device_type, pcie_root_complex, sata_hba, enabled_logging, logging_period, logging_file_path),
    __run_until(stop_time), read_ratio(read_ratio), address_distribution(address_distribution),
    working_set_ratio(working_set_ratio), hot_region_ratio(hot_region_ratio),
    request_size_distribution(request_size_distribution), average_request_size(average_request_size), variance_request_size(variance_request_size),
    generator_type(generator_type), Average_inter_arrival_time_nano_sec(Average_inter_arrival_time_nano_sec), average_number_of_enqueued_requests(average_number_of_enqueued_requests),
    seed(seed), generate_aligned_addresses(generate_aligned_addresses), alignment_value(alignment_value)
  {
    // Initialize at first time
    random_request_type_generator = nullptr;
    random_address_generator = nullptr;
    random_hot_cold_generator = nullptr;
    random_hot_address_generator = nullptr;
    random_request_size_generator = nullptr;
    random_time_interval_generator = nullptr;

    if (read_ratio == 0.0)//If read ratio is 0, then we change its value to a negative one so that in request generation we never generate a read request
      read_ratio = -1.0;
    random_request_type_generator_seed = seed++;
    random_request_type_generator = new Utils::RandomGenerator(random_request_type_generator_seed);
    random_address_generator_seed = seed++;
    random_address_generator = new Utils::RandomGenerator(random_address_generator_seed);
    if (this->start_lsa > this->end_lsa)
      throw std::logic_error("Problem in IO Flow Synthetic, the start LBA address is greater than the end LBA address");

    if (address_distribution == Utils::Address_Distribution_Type::RANDOM_HOTCOLD)
    {
      random_hot_address_generator_seed = seed++;
      random_hot_address_generator = new Utils::RandomGenerator(random_hot_address_generator_seed);
      random_hot_cold_generator_seed = seed++;
      random_hot_cold_generator = new Utils::RandomGenerator(random_hot_cold_generator_seed);
      hot_region_end_lsa = this->start_lsa + (LHA_type)((double)(this->end_lsa - this->start_lsa) * hot_region_ratio);
    }
    if (request_size_distribution == Utils::Request_Size_Distribution_Type::NORMAL)
    {
      random_request_size_generator_seed = seed++;
      random_request_size_generator = new Utils::RandomGenerator(random_request_size_generator_seed);
    }
    if (generator_type == Utils::Request_Generator_Type::BANDWIDTH)
    {
      random_time_interval_generator_seed = seed++;
      random_time_interval_generator = new Utils::RandomGenerator(random_time_interval_generator_seed);
    }

    if (this->working_set_ratio == 0)
      PRINT_ERROR("The working set ratio is set to zero for workload " << name)
  }

  IO_Flow_Synthetic::~IO_Flow_Synthetic()
  {
    delete random_request_type_generator;
    delete random_address_generator;
    delete random_hot_cold_generator;
    delete random_hot_address_generator;
    delete random_request_size_generator;
    delete random_time_interval_generator;
  }

  HostIORequest* IO_Flow_Synthetic::Generate_next_request()
  {
    if (__run_until > 0)
    {
      if (Simulator->Time() > __run_until)
        return nullptr;
    }
    else if (_all_request_generated())
      return nullptr;
    
    LHA_type start_lba;
    uint32_t lba_count;
    HostIOReqType req_type;

    // Get request type
    if (random_request_type_generator->Uniform(0, 1) <= read_ratio)
    {
      req_type = HostIOReqType::READ;
    }
    else
    {
      req_type = HostIOReqType::WRITE;
    }

    switch (request_size_distribution)
    {
    case Utils::Request_Size_Distribution_Type::FIXED:
      lba_count = average_request_size;
      break;
    case Utils::Request_Size_Distribution_Type::NORMAL:
    {
      double temp_request_size = random_request_size_generator->Normal(average_request_size, variance_request_size);
      lba_count = (uint32_t)(ceil(temp_request_size));
      if (lba_count <= 0)
        lba_count = 1;
      break;
    }
    default:
      throw std::invalid_argument("Uknown distribution type for requset size.");
    }

    switch (address_distribution)
    {
    case Utils::Address_Distribution_Type::STREAMING:
      start_lba = streaming_next_address;
      if (start_lba + lba_count > end_lsa)
        start_lba = start_lsa;
      streaming_next_address += lba_count;
      if (streaming_next_address > end_lsa)
        streaming_next_address = start_lsa;
      if (generate_aligned_addresses)
        if(streaming_next_address % alignment_value != 0)
          streaming_next_address += alignment_value - (streaming_next_address % alignment_value);
      if(streaming_next_address == start_lba)
        PRINT_MESSAGE("Synthetic Message Generator: The same address is always repeated due to configuration parameters!")
      break;
    case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
      if (random_hot_cold_generator->Uniform(0, 1) < hot_region_ratio)// (100-hot)% of requests going to hot% of the address space
      {
        start_lba = random_hot_address_generator->Uniform_ulong(hot_region_end_lsa + 1, end_lsa);
        if (start_lba < hot_region_end_lsa + 1 || start_lba > end_lsa)
          PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
          if (start_lba + lba_count > end_lsa)
            start_lba = hot_region_end_lsa + 1;
      }
      else
      {
        start_lba = random_hot_address_generator->Uniform_ulong(start_lsa, hot_region_end_lsa);
        if (start_lba < start_lsa || start_lba > hot_region_end_lsa)
          PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
      }
      break;
    case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
      start_lba = random_address_generator->Uniform_ulong(start_lsa, end_lsa);
      if (start_lba < start_lsa || start_lba > end_lsa)
        PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
      if (start_lba + lba_count > end_lsa)
        start_lba = start_lsa;
      break;
    default:
      PRINT_ERROR("Unknown address distribution type!\n")
    }

    if (generate_aligned_addresses)
      start_lba -= start_lba % alignment_value;

    PRINT_DEBUG("* Host: Request generated - " << (req_type == HostIOReqType::READ ? "Read, " : "Write, ") << "LBA:" << start_lba << ", Size_in_bytes:" << lba_count << "")

    return _generate_request(Simulator->Time(), start_lba, lba_count, req_type);
  }

  force_inline void
  IO_Flow_Synthetic::__submit_next_request()
  {
    auto* request = Generate_next_request();
    /*
     * In the demand based execution mode, the Generate_next_request() function
     * may return nullptr if 1) the simulation stop is met, or 2) the number of
     * generated I/O requests reaches its threshold.
     */
    if (request != nullptr)
      _submit_io_request(request);
  }

  void IO_Flow_Synthetic::consume_nvme_io(CQEntry* io_request)
  {
    IO_Flow_Base::consume_nvme_io(io_request);

    if (generator_type == Utils::Request_Generator_Type::QUEUE_DEPTH)
      __submit_next_request();
  }

  void IO_Flow_Synthetic::consume_sata_io(HostIORequest* io_request)
  {
    IO_Flow_Base::consume_sata_io(io_request);

    if (generator_type == Utils::Request_Generator_Type::QUEUE_DEPTH)
      __submit_next_request();
  }

  void IO_Flow_Synthetic::Start_simulation() 
  {
    IO_Flow_Base::Start_simulation();

    if (address_distribution == Utils::Address_Distribution_Type::STREAMING)
    {
      streaming_next_address = random_address_generator->Uniform_ulong(start_lsa, end_lsa);
      if (generate_aligned_addresses)
        streaming_next_address -= streaming_next_address % alignment_value;
    }
    if (generator_type == Utils::Request_Generator_Type::BANDWIDTH)
      Simulator->Register_sim_event((sim_time_type)random_time_interval_generator->Exponential((double)Average_inter_arrival_time_nano_sec), this, 0, 0);
    else
      Simulator->Register_sim_event((sim_time_type)1, this, 0, 0);
  }

  void IO_Flow_Synthetic::Execute_simulator_event(MQSimEngine::SimEvent* event)
  {
    if (generator_type == Utils::Request_Generator_Type::BANDWIDTH)
    {
      HostIORequest* req = Generate_next_request();
      if (req != nullptr)
      {
        auto* sim = Simulator;
        _submit_io_request(req);
        sim->Register_sim_event(sim->Time() + (sim_time_type)random_time_interval_generator->Exponential((double)Average_inter_arrival_time_nano_sec), this, 0, 0);
      }
    }
    else {
      for (uint32_t i = 0; i < average_number_of_enqueued_requests; i++)
        _submit_io_request(Generate_next_request());
    }
  }

  void
  IO_Flow_Synthetic::get_stats(Utils::Workload_Statistics& stats,
                               const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                               const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
  {
    IO_Flow_Base::get_stats(stats, convert_lha_to_lpa, find_nvm_subunit_access_bitmap);

    stats.Type = Utils::Workload_Type::SYNTHETIC;
    stats.generator_type = generator_type;
    stats.Working_set_ratio = working_set_ratio;
    stats.Read_ratio = read_ratio;
    stats.random_request_type_generator_seed = random_request_type_generator_seed;
    stats.Address_distribution_type = address_distribution;
    stats.Ratio_of_hot_addresses_to_whole_working_set = hot_region_ratio;
    stats.Ratio_of_traffic_accessing_hot_region = 1 - hot_region_ratio;
    stats.random_address_generator_seed = random_address_generator_seed;
    stats.random_hot_address_generator_seed = random_hot_address_generator_seed;
    stats.random_hot_cold_generator_seed = random_hot_cold_generator_seed;
    stats.generate_aligned_addresses = generate_aligned_addresses;
    stats.alignment_value = alignment_value;
    stats.Request_size_distribution_type = request_size_distribution;
    stats.Average_request_size_sector = average_request_size;
    stats.STDEV_reuqest_size = variance_request_size;
    stats.random_request_size_generator_seed = random_request_size_generator_seed;
    stats.Request_queue_depth = average_number_of_enqueued_requests;
    stats.random_time_interval_generator_seed = random_time_interval_generator_seed;
    stats.Average_inter_arrival_time_nano_sec = Average_inter_arrival_time_nano_sec;
  }

  int
  IO_Flow_Synthetic::_get_progress() const
  {
    return __run_until == 0
             ? IO_Flow_Base::_get_progress()
             : int(double(Simulator->Time()) / __run_until * 100);
  }
}