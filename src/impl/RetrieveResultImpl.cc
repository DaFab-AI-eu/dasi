
#include "impl/RetrieveResultImpl.h"

#include "dasi/api/Key.h"
#include "dasi/api/detail/Generators.h"
#include "dasi/api/detail/RetrieveDetail.h"
#include "eckit/io/DataHandle.h"
#include "fdb5/api/helpers/ListElement.h"
#include "fdb5/api/helpers/ListIterator.h"
#include "fdb5/database/FieldLocation.h"
#include "fdb5/io/HandleGatherer.h"

#include <cstddef>
#include <memory>
#include <utility>

namespace dasi {

//----------------------------------------------------------------------------------------------------------------------

RetrieveResultImpl::RetrieveResultImpl(fdb5::ListIterator& iter) {
    fdb5::ListElement elem;
    while (iter.next(elem)) { values_.push_back(elem); }
    // And start the iteration
    iter_ = values_.begin();
    updateResult();
}

void RetrieveResultImpl::next() {
    if (!done_) {
        ++iter_;
        updateResult();
    }
}

void RetrieveResultImpl::updateResult() {
    done_ = (iter_ == values_.end());
    if (!done_) {
        Key key;
        for (const auto& [keyword, value] : iter_->combinedKey()) { key.set(keyword, value); }
        dasiElement_.key             = std::move(key);
        dasiElement_.timestamp       = iter_->timestamp();
        dasiElement_.location.uri    = iter_->location().uri();
        dasiElement_.location.offset = iter_->location().offset();
        dasiElement_.location.length = iter_->location().length();
    }
}

const RetrieveElement& RetrieveResultImpl::value() const {
    return dasiElement_;
}

bool RetrieveResultImpl::done() const {
    return done_;
}

std::unique_ptr<eckit::DataHandle> RetrieveResultImpl::dataHandle() const {

    /// @todo - discuss optimise/sorted
    bool                 sorted = false;
    fdb5::HandleGatherer result(sorted);

    for (const auto& elem : values_) { result.add(elem.location().dataHandle()); }

    return std::unique_ptr<eckit::DataHandle> {result.dataHandle()};
}

size_t RetrieveResultImpl::count() const {
    return values_.size();
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi
