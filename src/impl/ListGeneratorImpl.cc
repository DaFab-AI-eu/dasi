
#include "ListGeneratorImpl.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

ListGeneratorImpl::ListGeneratorImpl(fdb5::ListIterator&& iter) :
    APIGeneratorImpl<ListElement>(),
    iter_(std::move(iter)),
    done_(false) {
    ListGeneratorImpl::next();
}

void ListGeneratorImpl::next() {
    if (!done_) {
        if (iter_.next(fdb5Element_)) {
            Key key;
            for (const auto& subkey : fdb5Element_.key()) {
                for (const auto& kv : subkey) {
                    key.set(kv.first, kv.second);
                }
            }
            dasiElement_.key = std::move(key);
            dasiElement_.timestamp = fdb5Element_.timestamp();
            dasiElement_.location.uri = fdb5Element_.location().uri();
            dasiElement_.location.offset = fdb5Element_.location().offset();
            dasiElement_.location.length = fdb5Element_.location().length();
        } else {
            done_ = true;
        }
    }
}

const ListElement& ListGeneratorImpl::value() const { return dasiElement_; }

bool ListGeneratorImpl::done() const { return done_; }

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
