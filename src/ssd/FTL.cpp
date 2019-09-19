#include <stdexcept>
#include <cmath>
#include <vector>
#include <map>
#include <functional>
#include <iterator>
#include "../sim/Sim_Defs.h"
#include "../utils/DistributionTypes.h"
#include "../utils/Helper_Functions.h"
#include "FTL.h"
#include "mapping/AddressMappingUnitDefs.h"

using namespace SSD_Components;

FTL::FTL(const sim_object_id_type& id,
         const DeviceParameterSet& params,
         const Stats& stats)
  : NVM_Firmware(id),
    block_no_per_plane(params.Flash_Parameters.Block_No_Per_Plane),
    page_no_per_block(params.Flash_Parameters.Page_No_Per_Block),
    page_size_in_sectors(params.Flash_Parameters.page_size_in_sector()),
    preconditioning_seed(0),
    random_generator(params.gen_seed()),
    over_provisioning_ratio(params.Overprovisioning_Ratio),
    avg_flash_read_latency(params.Flash_Parameters.avg_read_latency()),
    avg_flash_program_latency(params.Flash_Parameters.avg_write_latency()),
    __stats(stats),
    __tsu(),
    __block_manager(),
    __address_mapper(),
    __gc_and_wl()
{ }

void
FTL::Validate_simulation_config()
{
  NVM_Firmware::Validate_simulation_config();

  if (Data_cache_manager == nullptr)
    throw std::logic_error("The cache manager is not set for FTL!");

  if (!__address_mapper)
    throw std::logic_error("The mapping module is not set for FTL!");

  if (!__block_manager)
    throw std::logic_error("The block manager is not set for FTL!");

  if (!__gc_and_wl)
    throw std::logic_error("The garbage collector is not set for FTL!");
}

force_inline double
FTL::__overall_io_rate(const Utils::WorkloadStatsList& workload_stats) const
{
  double rate = 0.0;

  for (auto const &stat : workload_stats)
    rate += stat.io_rate(avg_flash_read_latency, avg_flash_program_latency);

  return rate;
}

/// ============================================================================
/// LPA set generator
uint32_t
FTL::__gen_synthetic_lpa_set(Utils::Workload_Statistics& stat,
                             Utils::Address_Distribution_Type& decision_dist_type,
                             std::map<LPA_type, page_status_type>& lpa_set,
                             std::multimap<int, LPA_type, std::greater<int>>& trace_lpas_sorted_histogram,
                             uint32_t& hot_region_last_index_in_histogram)
{
  uint32_t accessed_cmt_entries;

  auto no_of_logical_pages_in_steadystate = LPA_type(stat.Initial_occupancy_ratio * __address_mapper->Get_logical_pages_count(stat.Stream_id));

  LHA_type min_lha = stat.Min_LHA;
  LHA_type max_lha = stat.Max_LHA - 1;
  LPA_type min_lpa = Convert_host_logical_address_to_device_address(min_lha);
  if (stat.generate_aligned_addresses)
  {
    if (min_lha % stat.alignment_value != 0)
      min_lha += stat.alignment_value - (min_lha % stat.alignment_value);

    if (max_lha % stat.alignment_value != 0)
      max_lha -= min_lha % stat.alignment_value;
  }

  LPA_type max_lpa = Convert_host_logical_address_to_device_address(max_lha) - 1;

  accessed_cmt_entries = (uint32_t)(Convert_host_logical_address_to_device_address(max_lha) / page_size_in_sectors - Convert_host_logical_address_to_device_address(min_lha) / page_size_in_sectors) + 1;

  bool hot_range_finished = false;//Used for fast address generation in hot/cold traffic mode

  LHA_type hot_region_end_lsa = 0, hot_lha_used_for_generation = 0;//Used for fast address generation in hot/cold traffic mode
  LPA_type last_hot_lpa = 0;

  uint32_t size = 0;
  LHA_type start_LBA = 0, streaming_next_address = 0;
  Utils::RandomGenerator* random_address_generator = new Utils::RandomGenerator(stat.random_address_generator_seed);
  Utils::RandomGenerator* random_hot_address_generator = nullptr;
  Utils::RandomGenerator* random_hot_cold_generator = nullptr;
  Utils::RandomGenerator* random_request_size_generator = nullptr;
  bool fully_include_hot_addresses = false;

  if (stat.Address_distribution_type == Utils::Address_Distribution_Type::RANDOM_HOTCOLD)//treat a workload with very low hot/cold values as a uniform random workload
    if (stat.Ratio_of_hot_addresses_to_whole_working_set > 0.3)
      decision_dist_type = Utils::Address_Distribution_Type::RANDOM_UNIFORM;

  //Preparing address generation parameters
  switch (decision_dist_type)
  {
    case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
    {
      random_hot_address_generator = new Utils::RandomGenerator(stat.random_hot_address_generator_seed);
      random_hot_cold_generator = new Utils::RandomGenerator(stat.random_hot_cold_generator_seed);
      hot_region_end_lsa = min_lha + (LHA_type)((double)(max_lha - min_lha) * stat.Ratio_of_hot_addresses_to_whole_working_set);
      last_hot_lpa = Convert_host_logical_address_to_device_address(hot_region_end_lsa) - 1;//Be conservative and not include the last_hot_address itself
      hot_lha_used_for_generation = min_lha;
      //Check if enough LPAs could be generated within the working set of the flow
      if ((last_hot_lpa - min_lpa) < no_of_logical_pages_in_steadystate)
        fully_include_hot_addresses = true;
      if ((max_lpa - last_hot_lpa) < LPA_type(1.1 * double(no_of_logical_pages_in_steadystate - (last_hot_lpa - min_lpa))))
      {
        PRINT_MESSAGE("The specified initial occupancy value could not be satisfied as the working set of workload #" << stat.Stream_id << " is small. MQSim made some adjustments!");
        max_lha = min_lha + LHA_type(double(max_lha - min_lha) / stat.Working_set_ratio);
        if (stat.generate_aligned_addresses)
          if (max_lha % stat.alignment_value != 0)
            max_lha -= min_lha % stat.alignment_value;

        max_lpa = Convert_host_logical_address_to_device_address(max_lha);
      }
      break;
    }
    case Utils::Address_Distribution_Type::STREAMING:
    {
      streaming_next_address = random_address_generator->Uniform_ulong(min_lha, max_lha);
      stat.First_Accessed_Address = streaming_next_address;
      //Check if enough LPAs could be generated within the working set of the flow
      if ((max_lpa - min_lpa) < no_of_logical_pages_in_steadystate)
      {
        PRINT_MESSAGE("The specified initial occupancy value could not be satisfied as the working set of workload #" << stat.Stream_id << " is small. MQSim made some adjustments!");
        max_lha = min_lha + LHA_type(double(max_lha - min_lha) / stat.Working_set_ratio);
        if (stat.generate_aligned_addresses)
          if (max_lha % stat.alignment_value != 0)
            max_lha -= min_lha % stat.alignment_value;
        max_lpa = Convert_host_logical_address_to_device_address(max_lha);

        if ((max_lpa - min_lpa) < no_of_logical_pages_in_steadystate)
          no_of_logical_pages_in_steadystate = max_lpa - min_lpa + 1;
      }
      break;
    }
    case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
    {
      //Check if enough LPAs could be generated within the working set of the flow
      if ((max_lpa - min_lpa) < 1.1 * no_of_logical_pages_in_steadystate)
      {
        PRINT_MESSAGE("The specified initial occupancy value could not be satisfied as the working set of workload #" << stat.Stream_id << " is small. MQSim made some adjustments!");
        max_lha = min_lha + LHA_type(double(max_lha - min_lha) / stat.Working_set_ratio);
        if (stat.generate_aligned_addresses)
          if (max_lha % stat.alignment_value != 0)
            max_lha -= min_lha % stat.alignment_value;
        max_lpa = Convert_host_logical_address_to_device_address(max_lha);

        if ((max_lpa - min_lpa) < 1.1 * no_of_logical_pages_in_steadystate)
        {
          no_of_logical_pages_in_steadystate = (uint32_t)(double(max_lpa - min_lpa) * 0.9);
        }
      }
      break;
    }
  }

  if (stat.Request_size_distribution_type == Utils::Request_Size_Distribution_Type::NORMAL)
  {
    random_request_size_generator = new Utils::RandomGenerator(stat.random_request_size_generator_seed);
  }

  while (lpa_set.size() < no_of_logical_pages_in_steadystate)
  {
    switch (stat.Request_size_distribution_type)
    {
      case Utils::Request_Size_Distribution_Type::FIXED:
        size = stat.avg_request_sectors;
        break;
      case Utils::Request_Size_Distribution_Type::NORMAL:
      {
        double temp_request_size = random_request_size_generator->Normal(stat.avg_request_sectors, stat.STDEV_reuqest_size);
        size = (uint32_t)(std::ceil(temp_request_size));
        if (size <= 0)
          size = 1;
        break;
      }
    }

    bool is_hot_address = false;

    switch (decision_dist_type)
    {
      case Utils::Address_Distribution_Type::STREAMING:
        start_LBA = streaming_next_address;
        if (start_LBA + size > max_lha)
          start_LBA = min_lha;
        streaming_next_address += size;
        if (streaming_next_address > max_lha)
          streaming_next_address = min_lha;
        if (stat.generate_aligned_addresses)
          if (streaming_next_address % stat.alignment_value != 0)
            streaming_next_address += stat.alignment_value - (streaming_next_address % stat.alignment_value);
        break;
      case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
      {
        if (fully_include_hot_addresses)//just to speedup address generation
        {
          if (!hot_range_finished)
          {
            start_LBA = hot_lha_used_for_generation;
            hot_lha_used_for_generation += size;
            if (start_LBA > hot_region_end_lsa)
              hot_range_finished = true;
            is_hot_address = true;
          }
          else
          {
            start_LBA = random_hot_address_generator->Uniform_ulong(hot_region_end_lsa + 1, max_lha);
            if (start_LBA < hot_region_end_lsa + 1 || start_LBA > max_lha)
            PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
            if (start_LBA + size > max_lha)
              start_LBA = hot_region_end_lsa + 1;
            is_hot_address = false;
          }
        }
        else
        {
          if (random_hot_cold_generator->Uniform(0, 1) < stat.Ratio_of_hot_addresses_to_whole_working_set)// (100-hot)% of requests going to hot% of the address space
          {
            start_LBA = random_hot_address_generator->Uniform_ulong(hot_region_end_lsa + 1, max_lha);
            if (start_LBA < hot_region_end_lsa + 1 || start_LBA > max_lha)
            PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
            if (start_LBA + size > max_lha)
              start_LBA = hot_region_end_lsa + 1;
            is_hot_address = false;
          }
          else
          {
            start_LBA = random_hot_address_generator->Uniform_ulong(min_lha, hot_region_end_lsa);
            if (start_LBA < min_lha || start_LBA > hot_region_end_lsa)
            PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
            is_hot_address = true;
          }
        }
      }
        break;
      case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
        start_LBA = random_address_generator->Uniform_ulong(min_lha, max_lha);
        if (start_LBA < min_lha || max_lha < start_LBA)
        PRINT_ERROR("Out of range address is generated in IO_Flow_Synthetic!\n")
        if (start_LBA + size > max_lha)
          start_LBA = min_lha;
        break;
    }

    if (stat.generate_aligned_addresses)
      start_LBA -= start_LBA % stat.alignment_value;


    uint32_t hanled_sectors_count = 0;
    LHA_type lsa = start_LBA - min_lha;
    uint32_t transaction_size = 0;
    LPA_type max_lpa_within_device = Convert_host_logical_address_to_device_address(stat.Max_LHA) - Convert_host_logical_address_to_device_address(stat.Min_LHA);
    while (hanled_sectors_count < size)
    {
      transaction_size = page_size_in_sectors - (uint32_t)(lsa % page_size_in_sectors);
      if (hanled_sectors_count + transaction_size >= size)
      {
        transaction_size = size - hanled_sectors_count;
      }
      LPA_type lpa = Convert_host_logical_address_to_device_address(lsa);
      page_status_type access_status_bitmap = Find_NVM_subunit_access_bitmap(lsa);

      lsa = lsa + transaction_size;
      hanled_sectors_count += transaction_size;

      if (lpa_set.find(lpa) == lpa_set.end())
      {
        lpa_set[lpa] = access_status_bitmap;
        if (lpa <= max_lpa_within_device)//The lpas in trace_lpas_sorted_histogram are those that are actually accessed by the application
        {
          if (!is_hot_address && hot_region_last_index_in_histogram == 0)
            hot_region_last_index_in_histogram = (uint32_t)(trace_lpas_sorted_histogram.size());
          std::pair<int, LPA_type> entry((is_hot_address ? 1000 : 1), lpa);
          trace_lpas_sorted_histogram.insert(entry);
        }
      }
      else
        lpa_set[lpa] = access_status_bitmap | lpa_set[lpa];

    }
  }

  // Cleanup random generators
  delete random_address_generator;
  delete random_hot_address_generator;
  delete random_hot_cold_generator;
  delete random_request_size_generator;

  return accessed_cmt_entries;
}

uint32_t
FTL::__gen_trace_lpa_set(Utils::Workload_Statistics& stat,
                         std::map<LPA_type, page_status_type>& lpa_set,
                         std::multimap<int, LPA_type, std::greater<int>>& trace_lpas_sorted_histogram,
                         uint32_t& hot_region_last_index_in_histogram)
{
  uint32_t accessed_cmt_entries;
  auto no_of_logical_pages_in_steadystate = LPA_type(stat.Initial_occupancy_ratio * __address_mapper->Get_logical_pages_count(stat.Stream_id));

  LHA_type min_lha = stat.Min_LHA;
  LHA_type max_lha = stat.Max_LHA - 1;
  if (stat.generate_aligned_addresses)
  {
    if (min_lha % stat.alignment_value != 0)
      min_lha += stat.alignment_value - (min_lha % stat.alignment_value);

    if (max_lha % stat.alignment_value != 0)
      max_lha -= min_lha % stat.alignment_value;
  }

  accessed_cmt_entries = (uint32_t)(Convert_host_logical_address_to_device_address(max_lha) / page_size_in_sectors - Convert_host_logical_address_to_device_address(min_lha) / page_size_in_sectors) + 1;

  //Step 1-1: Read LPAs are preferred for steady-state since each read should be written before the actual access
  for (auto itr = stat.Write_read_shared_addresses.begin(); itr != stat.Write_read_shared_addresses.end(); itr++)
  {
    LPA_type lpa = (*itr);
    if (lpa_set.size() < no_of_logical_pages_in_steadystate)
      lpa_set[lpa] = stat.Write_address_access_pattern[lpa].Accessed_sub_units | stat.Read_address_access_pattern[lpa].Accessed_sub_units;
    else break;
  }

  for (auto itr = stat.Read_address_access_pattern.begin(); itr != stat.Read_address_access_pattern.end(); itr++)
  {
    LPA_type lpa = (*itr).first;
    if (lpa_set.size() < no_of_logical_pages_in_steadystate)
    {
      if (lpa_set.find(lpa) == lpa_set.end())
        lpa_set[lpa] = stat.Read_address_access_pattern[lpa].Accessed_sub_units;
    }
    else break;
  }

  //Step 1-2: if the read LPAs are not enough for steady-state, then fill the lpa_set_for_preconditioning using write LPAs
  for (auto itr = stat.Write_address_access_pattern.begin(); itr != stat.Write_address_access_pattern.end(); itr++)
  {
    LPA_type lpa = (*itr).first;
    if (lpa_set.size() < no_of_logical_pages_in_steadystate)
      if (lpa_set.find(lpa) == lpa_set.end())
        lpa_set[lpa] = stat.Write_address_access_pattern[lpa].Accessed_sub_units;
    std::pair<int, LPA_type> entry((*itr).second.Access_count, lpa);
    trace_lpas_sorted_histogram.insert(entry);
  }

  //Step 1-3: Determine the address distribution type of the input trace
  stat.Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_HOTCOLD;//Initially assume that the trace has hot/cold access pattern
  if (stat.Write_address_access_pattern.size() > STATISTICALLY_SUFFICIENT_WRITES_FOR_PRECONDITIONING)//First check if there are enough number of write requests in the workload to make a statistically correct decision, if not, MQSim assumes the workload has a uniform access pattern
  {
    int hot_region_write_count = 0;
    double f_temp = 0;
    double r_temp = 0;
    double step = 0.01;
    double next_milestone = 0.01;
    double prev_r = 0.0;
    for (auto itr = trace_lpas_sorted_histogram.begin(); itr != trace_lpas_sorted_histogram.end(); itr++)
    {
      hot_region_last_index_in_histogram++;
      hot_region_write_count += (*itr).first;

      f_temp = double(hot_region_last_index_in_histogram) / double(trace_lpas_sorted_histogram.size());
      r_temp = double(hot_region_write_count) / double(stat.Total_accessed_lbas);

      if (f_temp >= next_milestone)
      {
        if ((r_temp - prev_r) < step)
        {
          r_temp = prev_r;
          f_temp = next_milestone - step;
          break;
        }
        prev_r = r_temp;
        next_milestone += step;
      }

    }

    if ((r_temp > MIN_HOT_REGION_TRAFFIC_RATIO) && ((r_temp / f_temp) > HOT_REGION_METRIC))
    {
      stat.Ratio_of_hot_addresses_to_whole_working_set = f_temp;
      stat.Ratio_of_traffic_accessing_hot_region = r_temp;
    }
    else
    {
      stat.Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_UNIFORM;
    }
  }
  else
    stat.Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_UNIFORM;


  Utils::RandomGenerator* random_address_generator = new Utils::RandomGenerator(preconditioning_seed++);
  uint32_t size = stat.avg_request_sectors;
  LHA_type start_LHA = 0;

  //Step 1-4: If both read and write LPAs are not enough for preconditioning flash storage space, then fill the remaining space
  while (lpa_set.size() < no_of_logical_pages_in_steadystate)
  {
    start_LHA = random_address_generator->Uniform_ulong(min_lha, max_lha);
    uint32_t hanled_sectors_count = 0;
    LHA_type lsa = start_LHA;
    uint32_t transaction_size = 0;
    while (hanled_sectors_count < size)
    {
      if (lsa < min_lha || lsa > max_lha)
        lsa = min_lha + (lsa % (max_lha - min_lha + 1));
      LHA_type internal_lsa = lsa - min_lha;

      transaction_size = page_size_in_sectors - (uint32_t)(internal_lsa % page_size_in_sectors);
      if (hanled_sectors_count + transaction_size >= size)
      {
        transaction_size = size - hanled_sectors_count;
      }

      LPA_type lpa = Convert_host_logical_address_to_device_address(internal_lsa);
      page_status_type access_status_bitmap = Find_NVM_subunit_access_bitmap(internal_lsa);

      if (lpa_set.find(lpa) != lpa_set.end())
        lpa_set[lpa] = access_status_bitmap;
      else
        lpa_set[lpa] = access_status_bitmap | lpa_set[lpa];


      lsa = lsa + transaction_size;
      hanled_sectors_count += transaction_size;
    }
  }

  delete random_address_generator;

  return accessed_cmt_entries;
}

force_inline uint32_t
FTL::__gen_lpa_set(Utils::Workload_Statistics& stat,
                   Utils::Address_Distribution_Type& decision_dist_type,
                   std::map<LPA_type, page_status_type>& lpa_set,
                   std::multimap<int, LPA_type, std::greater<int>>& trace_lpas_sorted_histogram,
                   uint32_t& hot_region_last_index_in_histogram)
{
  decision_dist_type = stat.Address_distribution_type;
  hot_region_last_index_in_histogram = 0;

  if (stat.Type == Utils::Workload_Type::SYNTHETIC)
    return __gen_synthetic_lpa_set(stat,
                                   decision_dist_type,
                                   lpa_set,
                                   trace_lpas_sorted_histogram,
                                   hot_region_last_index_in_histogram);

  else
    return __gen_trace_lpa_set(stat,
                               lpa_set,
                               trace_lpas_sorted_histogram,
                               hot_region_last_index_in_histogram);
}

/// ============================================================================
/// Probability distribution builder
force_inline void
FTL::__make_rga_gc_probability(const Utils::Workload_Statistics& stats,
                               double rho,
                               uint32_t d_choice,
                               std::vector<double>& steady_state_probability) const
{
  steady_state_probability.reserve(page_no_per_block + 1);

  // Based on: B. Van Houdt, "A mean field model for a class of garbage
  // collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
  for (uint32_t i = 0; i <= page_no_per_block; i++)
    steady_state_probability.emplace_back(Utils::Combination_count(page_no_per_block, i)
                                            * std::pow(rho, i)
                                            * std::pow(1 - rho, page_no_per_block - i));

  Utils::Euler_estimation(steady_state_probability,
                          page_no_per_block,
                          rho,
                          d_choice,
                          0.001,
                          0.0000000001,
                          10000);
}

force_inline void
FTL::__make_randp_gc_probability(const Utils::Workload_Statistics& stats,
                                 double rho,
                                 std::vector<double>& steady_state_probability) const
{
  steady_state_probability.reserve(page_no_per_block + 1);

  // Based on: B. Van Houdt, "A mean field model for a class of garbage
  // collection algorithms in flash-based solid state drives", SIGMETRICS 2013.
  double i_value, j_value;

  for (uint32_t i = 0; i <= page_no_per_block; i++) {
    i_value = rho / (rho + std::pow(1 - rho, i));

    for (uint32_t j = i + 1; j <= page_no_per_block; j++) {
      j_value = (1 - rho) * j;

      i_value *= (j_value) / (rho + j_value);
    }

    steady_state_probability.emplace_back(i_value);
  }

  for (uint32_t i = page_no_per_block; 0 < i; --i)
    steady_state_probability[i] -= steady_state_probability[i - 1];
}

force_inline void
FTL::__make_randpp_gc_probability(const Utils::Workload_Statistics& stats,
                                  std::vector<double>& steady_state_probability) const
{
  // Based on: B. Van Houdt, "A mean field model for a class of garbage
  // collection algorithms in flash-based solid state drives", SIGMETRICS 2013.

  // Get_GC_policy_specific_parameters will return the random_pp_threshold.
  uint32_t threshold = __gc_and_wl->Get_GC_policy_specific_parameter();

  // initialize the pdf values
  steady_state_probability.resize(page_no_per_block + 1, 0);

  double S_rho_b = 0;
  for (uint32_t j = threshold + 1; j <= page_no_per_block; j++)
    S_rho_b += 1.0 / j;

  const double rho = stats.Initial_occupancy_ratio * (1 - over_provisioning_ratio);
  const double a_r = page_no_per_block - threshold - page_no_per_block * S_rho_b;
  const double b_r = rho * S_rho_b + 1 - rho;
  const double c_r = (0.0 - rho) / page_no_per_block;

  // assume always rho < 1 - 1/b
  const double mu_b = ((0.0 - b_r) + std::sqrt(b_r * b_r - 4 * a_r * c_r)) / (2 * a_r);
  const double probability_seed = rho / (1 - rho - mu_b * (0.0 - a_r));

  steady_state_probability[page_no_per_block] = mu_b;

  for (int i = int(page_no_per_block) - 1; 0 <= i; --i) {
    if (i <= int(threshold))
      steady_state_probability[i] = ((i + 1) * steady_state_probability[i + 1])
                                      / (i + probability_seed);
    else
      steady_state_probability[i] = double(page_no_per_block * mu_b) / i;
  }
}

force_inline void
FTL::__make_random_uniform_probability(const Utils::Workload_Statistics& stats,
                                       double rho,
                                       std::vector<double>& steady_state_probability) const
{
  switch (__gc_and_wl->Get_gc_policy()) {
  case GC_Block_Selection_Policy_Type::RGA:
    // Get_GC_policy_specific_parameter returns the rga_set_size
    __make_rga_gc_probability(stats,
                              rho,
                              __gc_and_wl->Get_GC_policy_specific_parameter(),
                              steady_state_probability);
    break;

  case GC_Block_Selection_Policy_Type::GREEDY:
  case GC_Block_Selection_Policy_Type::FIFO:
    // As specified in the SIGMETRICS 2013 paper, a larger value for d-choices
    // (the name of RGA in Van Houdt's paper) will lead to results close to
    // greedy. We use d=30 to estimate steady-state of the greedy policy with that
    // of d-chioces.
    __make_rga_gc_probability(stats,
                              rho,
                              30,
                              steady_state_probability);
    break;

  case GC_Block_Selection_Policy_Type::RANDOM:
  case GC_Block_Selection_Policy_Type::RANDOM_P:
    __make_randp_gc_probability(stats, rho, steady_state_probability);
    break;

  case GC_Block_Selection_Policy_Type::RANDOM_PP:
    __make_randpp_gc_probability(stats, steady_state_probability);
    break;
  }
}

void
FTL::__make_random_hotcold_probability(const Utils::Workload_Statistics& stats,
                                       double rho,
                                       std::vector<double>& steady_state_probability) const
{
  // Estimate the steady-state of the hot/cold traffic based on the steady-state
  // of the uniform traffic
  __make_random_uniform_probability(stats, rho, steady_state_probability);

  const double r_to_f_ratio = std::sqrt(stats.Ratio_of_traffic_accessing_hot_region
                                          / stats.Ratio_of_hot_addresses_to_whole_working_set);
  double average_page_no_per_block = 0;

  std::vector<double> temporal_probability;
  temporal_probability.reserve(steady_state_probability.size());

  for (uint32_t i = 0; i <= page_no_per_block; i++) {
    double probability = steady_state_probability[i];

    temporal_probability.emplace_back(probability);
    average_page_no_per_block += probability * i;
  }

  double sum = 0;

  for (uint32_t i = 0; i <= page_no_per_block; i++) {
    auto phi_index = int(r_to_f_ratio * (double(i) - average_page_no_per_block));

    double probability = 0;

    if (0 <= phi_index && phi_index < page_no_per_block)
      probability = r_to_f_ratio * temporal_probability[phi_index];

    steady_state_probability[i] = probability;

    sum += probability;
  }

  for (uint32_t i = 0; i <= page_no_per_block; i++)
    steady_state_probability[i] /= sum;
}

void
FTL::__make_streaming_probability(const Utils::Workload_Statistics& stats,
                                  double rho,
                                  std::vector<double>& steady_state_probability) const
{
  // A simple estimation based on: "Stochastic Modeling of Large-Scale
  // Solid-State Storage Systems: Analysis, Design Trade-offs and Optimization",
  // SIGMETRICS 2013
  steady_state_probability.resize(page_no_per_block + 1, 0);

  steady_state_probability[0] = 1 - rho;
  steady_state_probability[page_no_per_block] = rho;

  // None of the GC policies change the the status of blocks in the steady
  // state assuming over-provisioning ratio is always greater than 0
}

force_inline void
FTL::__make_steady_state_probability(const Utils::Workload_Statistics& stats,
                                     Utils::Address_Distribution_Type decision_dist_type,
                                     std::vector<double>& steady_state_probability)
{
  // Note: if hot/cold separation is required, then the following estimations
  // should be changed according to Van Houtd's paper in Performance Evaluation
  // 2014.

  // The probability distribution function of the number of valid pages in
  // a block in the steady state
  double rho = stats.Initial_occupancy_ratio * (1 - over_provisioning_ratio)
                  / (1 - double(__gc_and_wl->Get_minimum_number_of_free_pages_before_GC()) / block_no_per_plane);

  switch (decision_dist_type) {
  case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
    __make_random_hotcold_probability(stats, rho, steady_state_probability);
    break;

  case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
    __make_random_uniform_probability(stats, rho, steady_state_probability);
    break;

  case Utils::Address_Distribution_Type::STREAMING:
    __make_streaming_probability(stats, rho, steady_state_probability);
    break;
  }

  // MQSim assigns PPAs to LPAs based on the estimated steady state status of
  // blocks, assuming that there is no hot/cold data separation.
  double sum = 0;

  // Check if probability distribution is correct
  for (uint32_t i = 0; i <= page_no_per_block; i++)
    sum += steady_state_probability[i];

  // Due to some precision errors the sum may not be exactly equal to 1
  if (sum > 1.001 || sum < 0.99)
    throw mqsim_error("Wrong probability distribution function for the number "
                      "of valid pages in flash blocks in the steady-state! "
                      "It is not safe to continue preconditioning!");
}

/// ============================================================================
/// Warm-up related functions
void
FTL::__warm_up(const Utils::Workload_Statistics& stat,
               uint32_t total_workloads,
               double overall_rate,
               Utils::Address_Distribution_Type decision_dist_type,
               uint32_t hot_region_last_index_in_histogram,
               const std::map<LPA_type, page_status_type>& lpa_set,
               std::multimap<int, LPA_type, std::greater<int>>& trace_lpas_sorted_histogram)
{
  if (!__address_mapper->Is_ideal_mapping_table())
  {
    //Step 4-1: Determine how much share of the entire CMT should be filled based on the flow arrival rate and access pattern
    uint32_t no_of_entries_in_cmt = 0;
    LPA_type min_LPA = Convert_host_logical_address_to_device_address(stat.Min_LHA);
    LPA_type max_LPA = Convert_host_logical_address_to_device_address(stat.Max_LHA);

    switch (__address_mapper->Get_CMT_sharing_mode())
    {
      case CMT_Sharing_Mode::SHARED:
      {
        double flow_rate = 0;
        if (stat.Type == Utils::Workload_Type::SYNTHETIC)
        {
          switch (stat.generator_type)
          {
          case Utils::RequestFlowControlType::BANDWIDTH:
            flow_rate = 1.0 / double(stat.Average_inter_arrival_time_nano_sec) * SIM_TIME_TO_SECONDS_COEFF * stat.avg_request_sectors;
            break;
          case Utils::RequestFlowControlType::QUEUE_DEPTH:
          {
            auto max_arrival_time = sim_time_type(stat.Read_ratio * double(avg_flash_read_latency) + (1 - stat.Read_ratio) * double(avg_flash_program_latency));
            double avg_arrival_time = double(max_arrival_time) / double(stat.Request_queue_depth);
            flow_rate = 1.0 / avg_arrival_time * SIM_TIME_TO_SECONDS_COEFF * stat.avg_request_sectors;
            break;
          }
          default:
            PRINT_ERROR("Unknown request type generator in the FTL preconditioning function.")
          }
        }
        else
        {
          flow_rate = 1.0 / double(stat.Average_inter_arrival_time_nano_sec) * SIM_TIME_TO_SECONDS_COEFF * stat.avg_request_sectors;
        }

        no_of_entries_in_cmt = (uint32_t)(double(flow_rate) / overall_rate * __address_mapper->Get_cmt_capacity());
        break;
      }
      case CMT_Sharing_Mode::EQUAL_SIZE_PARTITIONING:
        no_of_entries_in_cmt = (uint32_t)(1.0 / double(total_workloads) * __address_mapper->Get_cmt_capacity());
        if (max_LPA - min_LPA + 1 < LPA_type(no_of_entries_in_cmt))
          no_of_entries_in_cmt = (uint32_t)(max_LPA - min_LPA + 1);
        break;
      default:
        PRINT_ERROR("Unknown mapping table sharing mode in the FTL preconditioning function.")
    }

   if (max_LPA - min_LPA + 1 < LPA_type(trace_lpas_sorted_histogram.size()))
      no_of_entries_in_cmt = (uint32_t)(trace_lpas_sorted_histogram.size());

    //Step 4-2: Bring the LPAs into CMT based on the flow access pattern
    switch (decision_dist_type)
    {
      case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
      {
        //First bring hot addresses to CMT
        uint32_t required_no_of_hot_cmt_entries = (uint32_t)(stat.Ratio_of_hot_addresses_to_whole_working_set * no_of_entries_in_cmt);
        uint32_t entries_to_bring_into_cmt = required_no_of_hot_cmt_entries;
        if (required_no_of_hot_cmt_entries > hot_region_last_index_in_histogram)
          entries_to_bring_into_cmt = hot_region_last_index_in_histogram;
        auto itr = trace_lpas_sorted_histogram.begin();
        while (__address_mapper->Get_current_cmt_occupancy_for_stream(stat.Stream_id) < entries_to_bring_into_cmt)
        {
          __address_mapper->Bring_to_CMT_for_preconditioning(stat.Stream_id, (*itr).second);
          trace_lpas_sorted_histogram.erase(itr++);
        }

        //If there is free space in the CMT, then bring the remaining addresses to CMT
        no_of_entries_in_cmt -= entries_to_bring_into_cmt;
        auto itr2 = trace_lpas_sorted_histogram.begin();
        std::advance(itr2, hot_region_last_index_in_histogram);
        while (__address_mapper->Get_current_cmt_occupancy_for_stream(stat.Stream_id) < no_of_entries_in_cmt)
        {
          __address_mapper->Bring_to_CMT_for_preconditioning(stat.Stream_id, (*itr2++).second);
          if (itr2 == trace_lpas_sorted_histogram.end())
            break;
        }
        break;
      }
      case Utils::Address_Distribution_Type::STREAMING:
      {
        LPA_type lpa;
        auto itr = lpa_set.find(lpa);
        if (itr != lpa_set.begin())
          itr--;
        while (__address_mapper->Get_current_cmt_occupancy_for_stream(stat.Stream_id) < no_of_entries_in_cmt)
        {
          __address_mapper->Bring_to_CMT_for_preconditioning(stat.Stream_id, (*itr).first);
          if (itr == lpa_set.begin())
          {
            itr = lpa_set.end();
            itr--;
          }
          else itr--;
        }
        break;
      }
      case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
      {
        int random_walker = int(random_generator.Uniform(0, uint32_t(trace_lpas_sorted_histogram.size()) - 2));
        int random_step = random_generator.Uniform_uint(0, (uint32_t)(trace_lpas_sorted_histogram.size()) / no_of_entries_in_cmt);
        auto itr = trace_lpas_sorted_histogram.begin();
        while (__address_mapper->Get_current_cmt_occupancy_for_stream(stat.Stream_id) < no_of_entries_in_cmt)
        {
          std::advance(itr, random_step);
          __address_mapper->Bring_to_CMT_for_preconditioning(stat.Stream_id, (*itr).second);
          if (trace_lpas_sorted_histogram.size() > 1)
          {
            trace_lpas_sorted_histogram.erase(itr++);
            if (random_walker + random_step >= int(trace_lpas_sorted_histogram.size() - 1) || random_walker + random_step < 0)
              random_step *= -1;
            random_walker += random_step;
          }
          else
          {
            trace_lpas_sorted_histogram.erase(itr);
            break;
          }
        }
        break;
      }
    }
  }
}

/// ============================================================================
/// Main unit stats workload initializer
uint32_t
FTL::__unit_precondition(Utils::Workload_Statistics& stat,
                         uint32_t total_workloads,
                         double overall_rate)
{
  Utils::Address_Distribution_Type decision_dist_type;

  // only used for trace workloads to detect hot addresses
  uint32_t hot_region_last_index_in_histogram;

  // Stores the accessed LPAs
  std::map<LPA_type, page_status_type> lpa_set_for_preconditioning;

  // only used for trace workloads
  std::multimap<int, LPA_type, std::greater<int>> trace_lpas_sorted_histogram;

  /// Step 1: generate LPAs that are accessed in the steady-state
  uint32_t accessed_cmt_entries = __gen_lpa_set(stat,
                                                decision_dist_type,
                                                lpa_set_for_preconditioning,
                                                trace_lpas_sorted_histogram,
                                                hot_region_last_index_in_histogram);

  /// Step 2: Determine the probability distribution function of valid pages in
  ///         blocks, in the steady-state.
  std::vector<double> steadystate_block_status_probability;

  __make_steady_state_probability(stat,
                                  decision_dist_type,
                                  steadystate_block_status_probability);

  /// Step 3: Distribute LPAs over the entire flash space
  __address_mapper->Allocate_address_for_preconditioning(stat.Stream_id,
                                                         lpa_set_for_preconditioning,
                                                         steadystate_block_status_probability);

  /// Step 4: Touch the LPAs and bring them to CMT to warm-up address mapping
  ///         unit
  __warm_up(stat,
            total_workloads,
            overall_rate,
            decision_dist_type,
            hot_region_last_index_in_histogram,
            lpa_set_for_preconditioning,
            trace_lpas_sorted_histogram);

  return accessed_cmt_entries;
}


void
FTL::Perform_precondition(Utils::WorkloadStatsList&  workload_stats)
{
  __address_mapper->Store_mapping_table_on_flash_at_start();

  const double overall_rate = __overall_io_rate(workload_stats);

  uint32_t total_accessed_cmt_entries = 0;

  for (auto &stat : workload_stats)
    total_accessed_cmt_entries += __unit_precondition(stat, workload_stats.size(), overall_rate);
}

void
FTL::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
  xmlwriter.Write_start_element_tag(name_prefix + ".FTL");

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Read_CMD",
                                          __stats.IssuedReadCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Interleaved_Read_CMD",
                                          __stats.IssuedInterleaveReadCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Multiplane_Read_CMD",
                                          __stats.IssuedMultiplaneReadCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Copyback_Read_CMD",
                                          __stats.IssuedCopybackReadCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Multiplane_Copyback_Read_CMD",
                                          __stats.IssuedMultiplaneCopybackReadCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Program_CMD",
                                          __stats.IssuedProgramCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Interleaved_Program_CMD",
                                          __stats.IssuedInterleaveProgramCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Multiplane_Program_CMD",
                                          __stats.IssuedMultiplaneProgramCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Interleaved_Multiplane_Program_CMD",
                                          __stats.IssuedInterleaveMultiplaneProgramCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Copyback_Program_CMD",
                                          __stats.IssuedCopybackProgramCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Multiplane_Copyback_Program_CMD",
                                          __stats.IssuedMultiplaneCopybackProgramCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Erase_CMD",
                                          __stats.IssuedEraseCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Interleaved_Erase_CMD",
                                          __stats.IssuedInterleaveEraseCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Multiplane_Erase_CMD",
                                          __stats.IssuedMultiplaneEraseCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Interleaved_Multiplane_Erase_CMD",
                                          __stats.IssuedInterleaveMultiplaneEraseCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Suspend_Program_CMD",
                                          __stats.IssuedSuspendProgramCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Suspend_Erase_CMD",
                                          __stats.IssuedSuspendEraseCMD);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Read_CMD_For_Mapping",
                                          __stats.Total_flash_reads_for_mapping);

  xmlwriter.Write_attribute_string_inline("Issued_Flash_Program_CMD_For_Mapping",
                                          __stats.Total_flash_writes_for_mapping);

  xmlwriter.Write_attribute_string_inline("CMT_Hits",
                                          __stats.CMT_hits);

  xmlwriter.Write_attribute_string_inline("CMT_Hits_For_Read",
                                          __stats.readTR_CMT_hits);

  xmlwriter.Write_attribute_string_inline("CMT_Hits_For_Write",
                                          __stats.writeTR_CMT_hits);

  xmlwriter.Write_attribute_string_inline("CMT_Misses",
                                          __stats.CMT_miss);

  xmlwriter.Write_attribute_string_inline("CMT_Misses_For_Read",
                                          __stats.readTR_CMT_miss);

  xmlwriter.Write_attribute_string_inline("CMT_Misses_For_Write",
                                          __stats.writeTR_CMT_miss);

  xmlwriter.Write_attribute_string_inline("Total_CMT_Queries",
                                          __stats.total_CMT_queries);

  xmlwriter.Write_attribute_string_inline("Total_CMT_Queries_For_Reads",
                                          __stats.total_readTR_CMT_queries);

  xmlwriter.Write_attribute_string_inline("Total_CMT_Queries_For_Writes",
                                          __stats.total_writeTR_CMT_queries);

  xmlwriter.Write_attribute_string_inline("Total_GC_Executions",
                                          __stats.Total_gc_executions);

  xmlwriter.Write_attribute_string_inline("Average_Page_Movement_For_GC",
                                          __stats.avg_page_movement_for_gc());

  xmlwriter.Write_attribute_string_inline("Total_WL_Executions",
                                          __stats.Total_wl_executions);

  xmlwriter.Write_attribute_string_inline("Average_Page_Movement_For_WL",
                                          __stats.avg_page_movement_for_wl());

  xmlwriter.Write_end_element_tag();

  // Report TSU information before return;
  __tsu->Report_results_in_XML(name_prefix, xmlwriter);
}

LPA_type
FTL::Convert_host_logical_address_to_device_address(LHA_type lha) const
{
  return lha / page_size_in_sectors;
}

page_status_type
FTL::Find_NVM_subunit_access_bitmap(LHA_type lha) const
{
  return ((page_status_type)~(0xffffffffffffffff << 1U)) << (lha % page_size_in_sectors);
}

void
FTL::dispatch_transactions(const std::list<NvmTransaction*>& transactionList)
{
  __address_mapper->Translate_lpa_to_ppa_and_dispatch(transactionList);
}