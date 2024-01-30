/*
 * Copyright 2023- European Centre for Medium-Range Weather Forecasts (ECMWF).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/// @author Metin Cakircali
/// @date   Aug 2023

#pragma once

#include "fdb5/api/helpers/WipeIterator.h"
#include "dasi/api/detail/Generators.h"
#include "dasi/api/detail/WipeDetail.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

class WipeGeneratorImpl : public APIGeneratorImpl<WipeElement> {
public:  // methods
    explicit WipeGeneratorImpl(fdb5::WipeIterator&& iter): APIGeneratorImpl<WipeElement>(), iter_(std::move(iter)) {
        WipeGeneratorImpl::next();
    }

    void next() override {
        if (!done_) {
            if (iter_.next(fdb5Element_)) {
                dasiElement_ = fdb5Element_;
            } else {
                done_ = true;
            }
        }
    }

    [[nodiscard]]
    const WipeElement& value() const override {
        return dasiElement_;
    }

    [[nodiscard]]
    bool done() const override {
        return done_;
    }

private:  // members
    bool               done_ {false};
    fdb5::WipeIterator iter_;

    fdb5::WipeElement fdb5Element_;
    dasi::WipeElement dasiElement_;
};

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
