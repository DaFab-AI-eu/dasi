
#include "dasi/api/detail/RetrieveDetail.h"

#include "impl/RetrieveResultImpl.h"

#include <ostream>

namespace dasi {

//----------------------------------------------------------------------------------------------------------------------

std::unique_ptr<eckit::DataHandle> RetrieveResult::dataHandle() const {
    return impl().dataHandle();
}

size_t RetrieveResult::count() const {
    return impl().count();
}

const RetrieveResultImpl& RetrieveResult::impl() const {
    const auto* ret = static_cast<const RetrieveResultImpl*>(impl_.get());
    ASSERT(ret);
    return *ret;
}

RetrieveResultImpl& RetrieveResult::impl() {
    auto* ret = static_cast<RetrieveResultImpl*>(impl_.get());
    ASSERT(ret);
    return *ret;
}

void RetrieveResult::print(std::ostream& s) const {
    s << "RetrieveResult()";
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi
