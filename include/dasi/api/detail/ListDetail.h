
/// @author Simon Smart
/// @date   Dec 2022

#pragma once

#include "dasi/api/Key.h"
#include "dasi/api/detail/Generators.h"
#include "eckit/filesystem/URI.h"
#include "eckit/io/Length.h"
#include "eckit/io/Offset.h"

#include <ctime>
#include <ostream>

namespace dasi {

//-------------------------------------------------------------------------------------------------

struct DataLocation {
    eckit::URI    uri;
    eckit::Offset offset;
    eckit::Length length;

private:  // members
    friend std::ostream& operator<<(std::ostream& out, const DataLocation& loc) {
        loc.print(out);
        return out;
    };

public:  // methods
    void print(std::ostream& out) const;
};

//-------------------------------------------------------------------------------------------------

struct ListElement {
    Key          key;
    DataLocation location;
    time_t       timestamp {0};

private:  // members
    friend std::ostream& operator<<(std::ostream& out, const ListElement& elem) {
        elem.print(out);
        return out;
    };

public:  // methods
    void print(std::ostream& out, bool location = false) const;
};

//-------------------------------------------------------------------------------------------------

using ListGenerator = GenericGenerator<ListElement>;

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
