
#include "ListGeneratorImpl.h"

#include "dasi/api/Key.h"
#include "dasi/api/detail/ListDetail.h"
#include "fdb5/api/helpers/ListIterator.h"
#include "fdb5/database/FieldLocation.h"

#include <utility>

namespace dasi {

//-------------------------------------------------------------------------------------------------

ListGeneratorImpl::ListGeneratorImpl(fdb5::ListIterator&& iter) : iter_(std::move(iter)), done_(false) {
    ListGeneratorImpl::next();
}

void ListGeneratorImpl::next() {
    if (!done_) {
        if (iter_.next(fdb5Element_)) {
            Key key;
            for (const auto& [keyword, value] : fdb5Element_.combinedKey()) { key.set(keyword, value); }
            dasiElement_.key             = std::move(key);
            dasiElement_.timestamp       = fdb5Element_.timestamp();
            dasiElement_.location.uri    = fdb5Element_.location().uri();
            dasiElement_.location.offset = fdb5Element_.location().offset();
            dasiElement_.location.length = fdb5Element_.location().length();
        } else {
            done_ = true;
        }
    }
}

const ListElement& ListGeneratorImpl::value() const {
    return dasiElement_;
}

bool ListGeneratorImpl::done() const {
    return done_;
}

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
