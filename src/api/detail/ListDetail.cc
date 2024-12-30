
#include "dasi/api/detail/ListDetail.h"

namespace dasi {

//----------------------------------------------------------------------------------------------------------------------

void DataLocation::print(std::ostream& s) const {
    s << "uri=" << uri;
    s << " offset=" << offset;
    s << " length=" << length;
}

void ListElement::print(std::ostream& s, bool withLocation) const {
    s << key;
    if (withLocation) { s << " " << location; }
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi
