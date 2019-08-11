#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>
#include <string>
#include <iostream>

typedef uint64_t sim_time_type;
typedef uint16_t stream_id_type;
typedef sim_time_type data_timestamp_type;

#define INVALID_TIME 18446744073709551615ULL
#define T0 0
#define INVALID_TIME_STAMP 18446744073709551615ULL
#define MAXIMUM_TIME 18446744073709551615ULL
#define ONE_SECOND 1000000000
typedef std::string sim_object_id_type;

#define CurrentTimeStamp Simulator->Time()
#define PRINT_ERROR(MSG) {\
              std::cerr << "ERROR:" ;\
              std::cerr << MSG << std::endl; \
              std::cin.get();\
              exit(1);\
             }
#define PRINT_MESSAGE(M) std::cout << M << std::endl;

#ifdef DEBUG
#define PRINT_DEBUG(M) //std::cout<<M<<std::endl;
#else
#define PRINT_DEBUG(M)
#endif

#define SIM_TIME_TO_MICROSECONDS_COEFF 1000
#define SIM_TIME_TO_SECONDS_COEFF 1000000000
constexpr sim_time_type NSEC_TO_USEC_COEFF = 1000;

#endif // !DEFINITIONS_H
