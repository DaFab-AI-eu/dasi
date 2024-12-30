
/// @author Simon Smart
/// @date   Dec 2022

#pragma once

#include "dasi/api/Key.h"
#include "dasi/api/detail/Generators.h"
#include "eckit/config/LocalConfiguration.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

class PolicyDict : public eckit::LocalConfiguration {

public:  // methods
    PolicyDict() = default;

    template<typename T>
    PolicyDict(const std::string& name, const T& value) {
        LocalConfiguration::set(name, value);
    }

    ~PolicyDict() override = default;
};

//-------------------------------------------------------------------------------------------------

struct PolicyElement {
    Key        key;
    PolicyDict value;

private:  // members
    friend std::ostream& operator<<(std::ostream& s, const PolicyElement& elem) {
        elem.print(s);
        return s;
    };

public:  // methods
    void print(std::ostream& s, bool pretty = false) const;
};

//-------------------------------------------------------------------------------------------------

using PolicyGenerator = GenericGenerator<PolicyElement>;

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
