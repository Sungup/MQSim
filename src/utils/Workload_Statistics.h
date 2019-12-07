#ifndef WORKLOAD_STATISTICS_H
#define WORKLOAD_STATISTICS_H

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../sim/Sim_Defs.h"
#include "../ssd/SSD_Defs.h"
#include "../utils/DistributionTypes.h"

constexpr sim_time_type MAX_ARRIVAL_TIME_HISTOGRAM  = 1000000;
constexpr uint32_t      MAX_REQSIZE_HISTOGRAM_ITEMS = 1024;
namespace Utils
{
#define MIN_HOT_REGION_TRAFFIC_RATIO 0.5
#define HOT_REGION_METRIC 2.5
#define STATISTICALLY_SUFFICIENT_WRITES_FOR_PRECONDITIONING 10000

  // ================================
  // Workload Stats' support functors
  // ================================
  template <typename RET_TYPE>
  class IOFlowStatsHandlerBase {
  public:
    virtual ~IOFlowStatsHandlerBase() = default;
    virtual RET_TYPE operator()(LHA_type lha) const = 0;
  };

  template <typename T, typename RET_TYPE>
  class IOFlowStatsHandler : public IOFlowStatsHandlerBase<RET_TYPE> {
    typedef RET_TYPE (T::*Handler)(LHA_type lha) const;

  private:
    T* __callee;
    Handler __handler;

  public:
    IOFlowStatsHandler(T* callee, Handler handler)
      : IOFlowStatsHandlerBase<RET_TYPE>(),
        __callee(callee),
        __handler(handler)
    { }

    ~IOFlowStatsHandler() final = default;

    RET_TYPE operator()(LHA_type lha) const final
    { return (__callee->*__handler)(lha); }
  };

  typedef IOFlowStatsHandlerBase<LPA_type>         LhaToLpaConverterBase;
  typedef IOFlowStatsHandlerBase<page_status_type> NvmAccessBitmapFinderBase;

  template <typename T>
  using LhaToLpaConverter = IOFlowStatsHandler<T, LPA_type>;

  template <typename T>
  using NvmAccessBitmapFinder = IOFlowStatsHandler<T, page_status_type>;

  // ===========================
  // Workload Statistics classes
  // ===========================
  struct Address_Histogram_Unit
  {
    int Access_count;
    uint64_t Accessed_sub_units;

    force_inline
    Address_Histogram_Unit() : Access_count(0), Accessed_sub_units(0) { }
  };

  //Parameters defined in: B. Van Houdt, "On the necessity of hot and cold data identification to reduce the write amplification in flash-based SSDs", Perf. Eval., 2014.
  struct Workload_Statistics
  {
    Utils::Workload_Type Type;
    stream_id_type Stream_id;
    double Initial_occupancy_ratio;//Ratio of the logical storage space that is fill with data in steady-state
    uint32_t Replay_no;
    uint32_t Total_generated_reqeusts;

    int random_request_type_generator_seed;
    double Read_ratio;
    std::vector<sim_time_type> Write_arrival_time, Read_arrival_time;//Histogram with 1us resolution

    Utils::Address_Distribution_Type Address_distribution_type;
    double Working_set_ratio;
    uint32_t Total_accessed_lbas;
    /*Rosenblum hot/cold model: 
    - M. Rosenblum and J. K. Ousterhout, "The design and implementation of a log-structured file system", ACM CSUR, 1992.
    A fraction f of the complete address space corresponds to hot data and the remaining fraction to cold data. The fraction
    of write operations to the hot data is denoted as r*/
    double Ratio_of_hot_addresses_to_whole_working_set;//The f parameter in the Rosenblum hot/cold model
    double Ratio_of_traffic_accessing_hot_region;//The r parameter in the Rosenblum hot/cold model
    int random_address_generator_seed;
    int random_hot_cold_generator_seed;
    int random_hot_address_generator_seed;
    std::map<LPA_type, Address_Histogram_Unit> Write_address_access_pattern, Read_address_access_pattern;
    std::set<LPA_type> Write_read_shared_addresses;
    LHA_type First_Accessed_Address, Last_Accessed_Address, Min_LHA, Max_LHA;
    LHA_type hot_region_end_lsa;
    LHA_type streaming_next_address;
    bool generate_aligned_addresses;
    uint32_t alignment_value;;

    Utils::RequestFlowControlType generator_type;
    uint32_t Request_queue_depth;
    int random_time_interval_generator_seed;
    sim_time_type Average_inter_arrival_time_nano_sec;

    Utils::Request_Size_Distribution_Type Request_size_distribution_type;
    int random_request_size_generator_seed;
    uint32_t avg_request_sectors;
    uint32_t STDEV_reuqest_size;
    std::vector<uint32_t> Write_size_histogram, Read_size_histogram;//Histogram with 1 sector resolution

  public:
    double io_rate(sim_time_type avg_rd_lat, sim_time_type avg_wr_lat) const;

    Utils::Address_Distribution_Type address_distribution() const;
    void min_max_lha(LHA_type& min, LHA_type& max) const;
    LPA_type steady_state_pages(LPA_type logical_pages) const;
  };

  force_inline double
  Workload_Statistics::io_rate(sim_time_type avg_rd_lat,
                               sim_time_type avg_wr_lat) const
  {
    using namespace Utils;

    double avg_arrival_time = Average_inter_arrival_time_nano_sec;

    if (Type == Workload_Type::SYNTHETIC &&
        generator_type == RequestFlowControlType::QUEUE_DEPTH) {
      // QD 1 average arrival time
      avg_arrival_time = (Read_ratio * avg_rd_lat)
                           + ((1.0 - Read_ratio) * avg_wr_lat);

      // QD N's average arrival time
      avg_arrival_time /= Request_queue_depth;
    }

    return 1.0 / avg_arrival_time * NSEC_TO_SEC_COEFF * avg_request_sectors;
  }

  force_inline Utils::Address_Distribution_Type
  Workload_Statistics::address_distribution() const
  {
    if ((Type == Utils::Workload_Type::SYNTHETIC) &&
        (Address_distribution_type == Utils::Address_Distribution_Type::RANDOM_HOTCOLD) &&
        (Ratio_of_hot_addresses_to_whole_working_set > 0.3))
      return Utils::Address_Distribution_Type::RANDOM_UNIFORM;
    else
      return Address_distribution_type;
  }

  force_inline void
  Workload_Statistics::min_max_lha(LHA_type& min, LHA_type& max) const
  {
    min = Min_LHA;
    max = Max_LHA - 1;

    if (generate_aligned_addresses) {
      if (min % alignment_value != 0)
        min += alignment_value - (min % alignment_value);

      if (max % alignment_value != 0)
        max -= min % alignment_value;
    }
  }

  force_inline LPA_type
  Workload_Statistics::steady_state_pages(LPA_type logical_pages) const
  {
    return LPA_type(Initial_occupancy_ratio * logical_pages);
  }

  typedef std::vector<Workload_Statistics>     WorkloadStatsList;
}
#endif// !WORKLOAD_STATISTICS_H