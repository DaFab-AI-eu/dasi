
/// @author Simon Smart
/// @date   Dec 2022

#pragma once

#include "dasi/api/Key.h"
#include "dasi/api/detail/Generators.h"
#include "eckit/filesystem/URI.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

struct DataLocation {
    eckit::URI    uri;
    eckit::Offset offset;
    eckit::Length length;

private:  // members
    friend std::ostream& operator<<(std::ostream& s, const DataLocation& loc) {
        loc.print(s);
        return s;
    };

public:  // methods
    void print(std::ostream& s) const;
};

//-------------------------------------------------------------------------------------------------

struct ListElement {
    Key          key;
    DataLocation location;
    time_t       timestamp;

private:  // members
    friend std::ostream& operator<<(std::ostream& s, const ListElement& elem) {
        elem.print(s);
        return s;
    };

public:  // methods
    void print(std::ostream& s, bool location = false) const;
};

//-------------------------------------------------------------------------------------------------

using ListGenerator = GenericGenerator<ListElement>;

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
