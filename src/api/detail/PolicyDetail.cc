
#include "dasi/api/detail/PolicyDetail.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/log/JSON.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

void PolicyElement::print(std::ostream& s, bool pretty) const {
    if (pretty) {
        s << "Key: " << key << eckit::newl;
        s << "Policies: ";
        eckit::JSON json(s, eckit::JSON::Formatting::indent(4));
        json << value;
    } else {
        NOTIMP;
    }
}

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
