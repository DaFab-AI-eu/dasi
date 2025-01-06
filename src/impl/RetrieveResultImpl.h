
/// @author Simon Smart
/// @date   Jan 2023

#pragma once

#include "dasi/api/detail/Generators.h"
#include "dasi/api/detail/RetrieveDetail.h"
#include "fdb5/api/helpers/ListIterator.h"

#include <memory>

namespace eckit {
class DataHandle;
}

namespace dasi {

//----------------------------------------------------------------------------------------------------------------------

class RetrieveResultImpl : public APIGeneratorImpl<RetrieveElement> {

    using vector_type = std::vector<fdb5::ListElement>;

public:
    using const_iterator = vector_type::const_iterator;

public:  // methods
    explicit RetrieveResultImpl(fdb5::ListIterator&& iter);

    // Functions to implement iteration in RetrieveResult

    void next() override;

    [[nodiscard]]
    const RetrieveElement& value() const override;

    [[nodiscard]]
    bool done() const override;

    // Additional functions to implement non-iteration RetrieveResult functionality

    [[nodiscard]]
    std::unique_ptr<eckit::DataHandle> dataHandle() const;

    /// The number of objects to be returned in this request
    [[nodiscard]]
    size_t count() const;

private:  // methods
    void updateResult();

private:  // members
    /// @todo - use something other than ListElement, such that it can be properly iterated by the user-facing code
    vector_type values_;

    vector_type::const_iterator iter_;
    dasi::ListElement           dasiElement_;
    bool                        done_;
};

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi
