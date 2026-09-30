
#include "dasi/api/detail/ListDetail.h"

#include <ostream>

namespace dasi {

//----------------------------------------------------------------------------------------------------------------------

void DataLocation::print(std::ostream& out) const {
    out << "uri=" << uri;
    out << " offset=" << offset;
    out << " length=" << length;
}

void ListElement::print(std::ostream& out, bool withLocation) const {
    out << key;
    if (withLocation) { out << " " << location; }
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi
