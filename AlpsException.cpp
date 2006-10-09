#include "AlpsLicense.h"
#include <iostream>

#include "AlpsException.h"

std::ostream& operator<<(std::ostream& os, const AlpsException& ex)
{
  os << "Exception at " << ex.File() << " line " << ex.Line() <<std::endl;
  os << "\tErr: " << ex.Msg() << std::endl;
  return os;
}
