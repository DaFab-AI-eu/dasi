
#include "PolicyStatusGeneratorImpl.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

PolicyStatusGeneratorImpl::PolicyStatusGeneratorImpl(fdb5::StatusIterator&& iter,
                                                     std::vector<std::string>&& policySpecifiers) :
    APIGeneratorImpl<PolicyElement>(),
    iter_(std::move(iter)),
    policySpecifiers_(policySpecifiers),
    done_(false) {
    PolicyStatusGeneratorImpl::next();
}

void PolicyStatusGeneratorImpl::next() {
    if (!done_) {
        if (iter_.next(fdb5Element_)) {
            /// @todo Write a sensibly placed function to do this for all the adapters
            Key key;
            for (const auto& kv : fdb5Element_.key) {
                key.set(kv.first, kv.second);
            }
            dasiElement_.key = std::move(key);

            /// @todo write some helpers for policy specification, to reduce boilerplate
            if (policySpecifiers_.empty() || policySpecifiers_[0] == "access") {
                if (policySpecifiers_.size() < 2 || policySpecifiers_[1] == "retrieve") {
                    dasiElement_.value.set(
                            "access.retrieve",
                            fdb5Element_.controlIdentifiers.enabled(fdb5::ControlIdentifier::Retrieve));
                }
                if (policySpecifiers_.size() < 2 || policySpecifiers_[1] == "archive") {
                    dasiElement_.value.set(
                            "access.archive",
                            fdb5Element_.controlIdentifiers.enabled(fdb5::ControlIdentifier::Archive));
                }
                if (policySpecifiers_.size() < 2 || policySpecifiers_[1] == "list") {
                    dasiElement_.value.set(
                            "access.list",
                            fdb5Element_.controlIdentifiers.enabled(fdb5::ControlIdentifier::List));
                }
                if (policySpecifiers_.size() < 2 || policySpecifiers_[1] == "wipe") {
                    dasiElement_.value.set(
                            "access.wipe",
                            fdb5Element_.controlIdentifiers.enabled(fdb5::ControlIdentifier::Wipe));
                }
            }

            /// @todo: multi-root???
        } else {
            done_ = true;
        }
    }
}

const dasi::PolicyElement& PolicyStatusGeneratorImpl::value() const {
    return dasiElement_;
}

bool PolicyStatusGeneratorImpl::done() const {
    return done_;
}

//-------------------------------------------------------------------------------------------------

} // namespace dasi
