
/// @author Simon Smart
/// @date   Oct 2022

#pragma once

#include "dasi/api/detail/Generators.h"
#include "dasi/api/detail/ListDetail.h"
#include "fdb5/api/helpers/ListIterator.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

class ListGeneratorImpl : public APIGeneratorImpl<ListElement> {

public:  // methods
    explicit ListGeneratorImpl(fdb5::ListIterator&& iter);

    void next() override;

    [[nodiscard]]
    const ListElement& value() const override;

    [[nodiscard]]
    bool done() const override;

private:  // members
    fdb5::ListElement  fdb5Element_;
    dasi::ListElement  dasiElement_;
    fdb5::ListIterator iter_;
    bool               done_;
};

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
