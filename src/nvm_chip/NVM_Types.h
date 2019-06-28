#ifndef  NVM_TYPES_H
#define NVM_TYPES_H

#include <cstdint>

#include "../utils/Exception.h"
#include "../utils/InlineTools.h"
#include "../utils/StringTools.h"

namespace NVM
{
  enum class NVM_Type {FLASH};
  typedef uint64_t memory_content_type;
}

force_inline std::string
to_string(NVM::NVM_Type /* type */)
{
  // Currently only support Flash memory
  return "FLASH";
}

force_inline NVM::NVM_Type
to_nvm_type(std::string v)
{
  Utils::to_upper(v);

  // Currently only support Flash memory
  if (v != "FLASH")
    throw mqsim_error("Unknown NVM type specified in the SSD configuration "
                      "file");

  return NVM::NVM_Type::FLASH;
}

#endif // ! NVM_TYPES_H
