/*
 * (C) Copyright 2022- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Simon Smart
/// @date   Oct 2022

#pragma once

#include "eckit/system/Library.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

class LibDasi : public eckit::system::Library {

public:  // methods
    static LibDasi& instance();

    std::string version() const override;

    std::string gitsha1(unsigned int count) const override;

private:  // methods
    LibDasi();
};

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
