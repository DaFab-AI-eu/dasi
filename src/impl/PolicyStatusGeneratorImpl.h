
/// @author Simon Smart
/// @date   Jan 2023

#pragma once

#include "dasi/api/detail/PolicyDetail.h"
#include "fdb5/api/helpers/StatusIterator.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

/// A generator that maps the FDB status() function onto output for the
/// policy framework for Dasi.
///
/// Really we want to rework this such that we can pass policy stuff into
/// the FDB proper (possibly in addition ot the locking/status stuff?),
/// so that we can have backend-specific policies that can be set.

class PolicyStatusGeneratorImpl : public APIGeneratorImpl<PolicyElement> {

public:  // methods
    explicit PolicyStatusGeneratorImpl(fdb5::StatusIterator&& iter, std::vector<std::string>&& policySpecifiers = {});

    void next() override;

    [[nodiscard]]
    const PolicyElement& value() const override;

    [[nodiscard]]
    bool done() const override;

private:  // members
    std::vector<std::string> policySpecifiers_;
    fdb5::StatusElement      fdb5Element_;
    dasi::PolicyElement      dasiElement_;
    fdb5::StatusIterator     iter_;
    bool                     done_;
};

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
