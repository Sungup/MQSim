#ifndef ASCII_TRACE_DEFINITION_H
#define ASCII_TRACE_DEFINITION_H

#include <string>

#include "../utils/Exception.h"
#include "../utils/InlineTools.h"
#include "../utils/StringTools.h"

//The unit of arrival times in the input file
enum class Trace_Time_Unit {
  PICOSECOND,
  NANOSECOND,
  MICROSECOND
};

force_inline std::string
to_string(Trace_Time_Unit v)
{
  switch (v) {
  case Trace_Time_Unit::PICOSECOND:  return "PICOSECOND";
  case Trace_Time_Unit::NANOSECOND:  return "NANOSECOND";
  case Trace_Time_Unit::MICROSECOND: return "MICROSECOND";
  }
}

force_inline Trace_Time_Unit
to_trace_time_unit(std::string v)
{
  Utils::to_upper(v);

  if (v == "PICOSECOND")  return Trace_Time_Unit::PICOSECOND;
  if (v == "NANOSECOND")  return Trace_Time_Unit::NANOSECOND;
  if (v == "MICROSECOND") return Trace_Time_Unit::MICROSECOND;

  throw mqsim_error("Wrong time unit specified for the trace based flow");
}

#define PicoSecondCoeff  1000000000000  //the coefficient to convert picoseconds to second
#define NanoSecondCoeff  1000000000  //the coefficient to convert nanoseconds to second
#define MicroSecondCoeff  1000000  //the coefficient to convert microseconds to second
#define ASCIITraceTimeColumn 0
#define ASCIITraceDeviceColumn 1
#define ASCIITraceAddressColumn 2
#define ASCIITraceSizeColumn 3
#define ASCIITraceTypeColumn 4
#define ASCIITraceWriteCode "0"
#define ASCIITraceReadCode "1"
#define ASCIITraceWriteCodeInteger 0
#define ASCIITraceReadCodeInteger 1
#define ASCIILineDelimiter ' '
#define ASCIIItemsPerLine 5

#endif // !ASCII_TRACE_DEFINITION_H

