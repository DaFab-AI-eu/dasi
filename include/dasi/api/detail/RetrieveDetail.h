
/// @author Simon Smart
/// @date   Jan 2023

#pragma once

#include "dasi/api/detail/ListDetail.h"
#include "eckit/io/DataHandle.h"

#include <memory>

namespace dasi {

//----------------------------------------------------------------------------------------------------------------------

class RetrieveResultImpl;

using RetrieveElement = ListElement;

//----------------------------------------------------------------------------------------------------------------------

class RetrieveResult : public GenericGenerator<RetrieveElement> {

public:  // methods
    using GenericGenerator<RetrieveElement>::GenericGenerator;

    [[nodiscard]]
    std::unique_ptr<eckit::DataHandle> dataHandle() const;

    [[nodiscard]]
    size_t count() const;

private:  // members
    friend std::ostream& operator<<(std::ostream& s, const RetrieveResult& rr) {
        rr.print(s);
        return s;
    };

    [[nodiscard]] const RetrieveResultImpl& impl() const;
    [[nodiscard]] RetrieveResultImpl&       impl();

    void print(std::ostream& s) const;
};

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi
