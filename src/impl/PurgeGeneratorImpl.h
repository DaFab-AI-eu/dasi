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

#include "dasi/api/detail/Generators.h"
#include "dasi/api/detail/PurgeDetail.h"
#include "fdb5/api/helpers/PurgeIterator.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

class PurgeGeneratorImpl : public APIGeneratorImpl<PurgeElement> {
public:  // methods
    explicit PurgeGeneratorImpl(fdb5::PurgeIterator&& iter) : APIGeneratorImpl<PurgeElement>(), iter_(std::move(iter)) {
        PurgeGeneratorImpl::next();
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
    const PurgeElement& value() const override {
        return dasiElement_;
    }

    [[nodiscard]]
    bool done() const override {
        return done_;
    }

private:  // members
    bool                done_ {false};
    fdb5::PurgeIterator iter_;

    fdb5::PurgeElement fdb5Element_;
    dasi::PurgeElement dasiElement_;
};

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
