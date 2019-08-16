#ifndef DISTRIBUTION_TYPES
#define DISTRIBUTION_TYPES

#include "Exception.h"
#include "InlineTools.h"
#include "StringTools.h"

// ==================
// Enumerator defines
// ==================
namespace Utils
{
  // Not implemented address distribution type
  //  MIXED_STREAMING_RANDOM
  enum class Address_Distribution_Type {
    STREAMING,
    RANDOM_UNIFORM,
    RANDOM_HOTCOLD
  };

  enum class Request_Size_Distribution_Type {
    FIXED,
    NORMAL
  };

  enum class Workload_Type {
    SYNTHETIC,
    TRACE_BASED
  };

  // Not implemented generator type.
  //  Time_INTERVAL: general requests based on the arrival rate definitions,
  //  DEMAND_BASED: just generate a request, every time that there is a demand
  enum class RequestFlowControlType {
    BANDWIDTH,
    QUEUE_DEPTH
  };
}

// ========================
// Enum -> String converter
// ========================
force_inline std::string
to_string(Utils::Address_Distribution_Type v)
{
  switch (v) {
  case Utils::Address_Distribution_Type::STREAMING:      return "STREAMING";
  case Utils::Address_Distribution_Type::RANDOM_HOTCOLD: return "RANDOM_HOTCOLD";
  case Utils::Address_Distribution_Type::RANDOM_UNIFORM: return "RANDOM_UNIFORM";
  }
}

force_inline std::string
to_string(Utils::Request_Size_Distribution_Type v)
{
  switch (v) {
  case Utils::Request_Size_Distribution_Type::FIXED:  return "FIXED";
  case Utils::Request_Size_Distribution_Type::NORMAL: return "NORMAL";
  }
}

force_inline std::string
to_string(Utils::Workload_Type v)
{
  switch (v) {
  case Utils::Workload_Type::SYNTHETIC:   return "SYNTHETIC";
  case Utils::Workload_Type::TRACE_BASED: return "TRACE_BASED";
  }
}

force_inline std::string
to_string(Utils::RequestFlowControlType v)
{
  switch (v) {
  case Utils::RequestFlowControlType::BANDWIDTH:   return "BANDWIDTH";
  case Utils::RequestFlowControlType::QUEUE_DEPTH: return "QUEUE_DEPTH";
  }
}

// ========================
// String -> Enum converter
// ========================
force_inline Utils::Address_Distribution_Type
to_address_distribution_type(std::string v)
{
  Utils::to_upper(v);

  if (v == "STREAMING")      return Utils::Address_Distribution_Type::STREAMING;
  if (v == "RANDOM_HOTCOLD") return Utils::Address_Distribution_Type::RANDOM_HOTCOLD;
  if (v == "RANDOM_UNIFORM") return Utils::Address_Distribution_Type::RANDOM_UNIFORM;

  throw mqsim_error("Wrong address distribution type for input synthetic flow");
}

force_inline Utils::Request_Size_Distribution_Type
to_request_size_distribution_type(std::string v)
{
  Utils::to_upper(v);

  if (v == "FIXED")  return Utils::Request_Size_Distribution_Type::FIXED;
  if (v == "NORMAL") return Utils::Request_Size_Distribution_Type::NORMAL;

  throw mqsim_error("Wrong request size distribution type for input synthetic flow");
}

force_inline Utils::Workload_Type
to_workload_type(std::string v)
{
  Utils::to_upper(v);

  if (v == "SYNTHETIC")   return Utils::Workload_Type::SYNTHETIC;
  if (v == "TRACE_BASED") return Utils::Workload_Type::TRACE_BASED;

  throw mqsim_error("Wrong workload type for input synthetic flow");
}

force_inline Utils::RequestFlowControlType
to_request_generator_type(std::string v)
{
  Utils::to_upper(v);

  if (v == "BANDWIDTH")   return Utils::RequestFlowControlType::BANDWIDTH;
  if (v == "QUEUE_DEPTH") return Utils::RequestFlowControlType::QUEUE_DEPTH;

  throw mqsim_error("Unknown synthetic generator type specified in the input file");
}

#endif