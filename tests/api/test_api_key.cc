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

#include "dasi/api/Key.h"

#include <sstream>
#include <string>

using namespace std::string_literals;

//----------------------------------------------------------------------------------------------------------------------

CASE("Construct from empty key") {

    dasi::Key k;
    EXPECT(!k.has("key1"));
    EXPECT(!k.has("key2"));

    {
        std::ostringstream ss;
        ss << k;
        EXPECT(ss.str() == "{}");
    }

    k.set("key1", "value1");
    EXPECT(k.has("key1"));
    EXPECT(!k.has("key2"));

    k.set("key2", "value2");
    EXPECT(k.has("key1"));
    EXPECT(k.has("key2"));

    {
        std::ostringstream ss;
        ss << k;
        EXPECT(ss.str() == "{key1=value1,key2=value2}");
    }

    EXPECT(k.size() == 2);
}

CASE("Construct from initialiser list") {

    dasi::Key k {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"},
    };

    EXPECT(k.has("key1"));
    EXPECT(k.has("key2"));
    EXPECT(k.has("key3"));
    EXPECT(!k.has("other"));

    EXPECT(k.size() == 3);

    std::ostringstream ss;
    ss << k;
    EXPECT(ss.str() == "{key1=value1,key2=value2,key3=value3}");
}

CASE("Construct from string") {

    dasi::Key k("key3=value3,key1=value1,key2=value2");

    EXPECT(k.has("key1"));
    EXPECT(k.has("key2"));
    EXPECT(k.has("key3"));
    EXPECT(!k.has("other"));

    EXPECT(k.size() == 3);

    std::ostringstream ss;
    ss << k;
    EXPECT(ss.str() == "{key1=value1,key2=value2,key3=value3}");
}

CASE("Rejects invalid keys") {
    EXPECT_THROWS_AS(dasi::Key("key3=value3=value3b,key1=value1,key2=value2"), eckit::UserError);
    EXPECT_THROWS_AS(dasi::Key("key3=value3/value3b,key1=value1,key2=value2"), eckit::UserError);
}

CASE("Modify existing key") {

    dasi::Key k {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"},
    };

    {
        std::ostringstream ss;
        ss << k;
        EXPECT(ss.str() == "{key1=value1,key2=value2,key3=value3}");
    }

    k.set("key1", "VALUE1a");
    k.set("key3", "VALUE3b");

    {
        std::ostringstream ss;
        ss << k;
        EXPECT(ss.str() == "{key1=VALUE1a,key2=value2,key3=VALUE3b}");
    }

    EXPECT(k.size() == 3);
}

CASE("Test that clear() works") {

    dasi::Key k {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"},
    };

    EXPECT(k.size() == 3);
    k.clear();
    EXPECT(k.size() == 0);

    for (const auto& kv : k) {
        EXPECT(false);
    }

    k.set("key4", "value4");
    k.set("key2", "value2");
    k.set("key5", "value5");

    EXPECT(k.size() == 3);
}

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
