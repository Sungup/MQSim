#ifndef PARAMETER_SET_BASE_H
#define PARAMETER_SET_BASE_H

#include "../../utils/rapidxml/rapidxml.hpp"
#include "../../utils/XMLWriter.h"

class ParameterSetBase
{
public:
  ParameterSetBase() = default;
  virtual ~ParameterSetBase() = default;

  virtual void XML_serialize(Utils::XmlWriter& xmlwriter) const = 0;
  virtual void XML_deserialize(rapidxml::xml_node<> *node) = 0;
};

#endif // !PARAMETER_SET_BASE_H