/*
 * (C) Copyright 2022- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "eckit/testing/Test.h"

#include "dasi/api/detail/PolicyDetail.h"

using namespace std::string_literals;

//----------------------------------------------------------------------------------------------------------------------

CASE("Construct from name and value") {

    {
        dasi::PolicyDict pd("config.toc.threads", 15);
        EXPECT(pd.has("config"));
        EXPECT(pd.has("config.toc"));
        EXPECT(pd.has("config.toc.threads"));
        EXPECT(pd.getLong("config.toc.threads") == 15);

        EXPECT(!pd.has("configy"));
        EXPECT(!pd.has("config.tocy"));
        EXPECT(!pd.has("config.toc.threadsy"));

        EXPECT(pd.getSubConfiguration("config")
                 .getSubConfiguration("toc")
                 .getLong("threads") == 15);
    }

    {
        dasi::PolicyDict pd("policy.file.values", "elephant");
        EXPECT(pd.has("policy"));
        EXPECT(pd.has("policy.file"));
        EXPECT(pd.has("policy.file.values"));
        EXPECT(pd.getString("policy.file.values") == "elephant");

        EXPECT(pd.getSubConfiguration("policy")
                 .getSubConfiguration("file")
                 .getString("values") == "elephant");
    }
}

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
