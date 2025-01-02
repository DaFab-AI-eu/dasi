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

#include "dasi/api/dasi_c.h"

#include <memory>
#include <cstring>

#define CHECK_RETURN(x) EXPECT((x) == DASI_SUCCESS);

namespace std {
template <> struct default_delete<dasi_key_t> {
    void operator() (const dasi_key_t* k) { CHECK_RETURN(dasi_free_key(k)); }

};
}

using namespace std::string_literals;

//----------------------------------------------------------------------------------------------------------------------

CASE("Construct from empty key") {

    dasi_key_t* k;
    CHECK_RETURN(dasi_new_key(&k));
    EXPECT(k);
    std::unique_ptr<dasi_key_t> kdeleter(k);

    dasi_bool_t has;
    CHECK_RETURN(dasi_key_has(k, "key1", &has));
    EXPECT(!has);
    CHECK_RETURN(dasi_key_has(k, "key2", &has));
    EXPECT(!has);

    CHECK_RETURN(dasi_key_set(k, "key1", "value1"));
    CHECK_RETURN(dasi_key_has(k, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_key_has(k, "key2", &has));
    EXPECT(!has);

    CHECK_RETURN(dasi_key_set(k, "key2", "value2"));
    CHECK_RETURN(dasi_key_has(k, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_key_has(k, "key2", &has));
    EXPECT(has);

    long count;
    CHECK_RETURN(dasi_key_count(k, &count));
    EXPECT(count == 2);

    const char* val;
    CHECK_RETURN(dasi_key_get(k, "key1", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value1") == 0);
    CHECK_RETURN(dasi_key_get(k, "key2", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value2") == 0);
}

CASE("Construct from string") {

    dasi_key_t* k;
    CHECK_RETURN(dasi_new_key_from_string(&k, "key3=value3,key1=value1,key2=value2"));
    EXPECT(k);
    std::unique_ptr<dasi_key_t> kdeleter(k);

    dasi_bool_t has;
    CHECK_RETURN(dasi_key_has(k, "key1", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_key_has(k, "key2", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_key_has(k, "key3", &has));
    EXPECT(has);
    CHECK_RETURN(dasi_key_has(k, "other", &has));
    EXPECT(!has);

    long count;
    CHECK_RETURN(dasi_key_count(k, &count));
    EXPECT(count == 3);

    const char* val;
    CHECK_RETURN(dasi_key_get(k, "key1", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value1") == 0);
    CHECK_RETURN(dasi_key_get(k, "key2", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value2") == 0);
    CHECK_RETURN(dasi_key_get(k, "key3", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value3") == 0);
}

CASE("Rejects invalid keys") {
    dasi_key_t* k = nullptr;
    int rc = dasi_new_key_from_string(&k, "key3=value3=value3b,key1=value1,key2=value2");
    EXPECT(rc == DASI_ERROR_USER);
    EXPECT(k == 0);

    rc = dasi_new_key_from_string(&k, "key3=value3/value3b,key1=value1,key2=value2");
    EXPECT(rc == DASI_ERROR_USER);
    EXPECT(k == 0);
}

CASE("Modify existing key") {

    dasi_key_t* k = nullptr;
    CHECK_RETURN(dasi_new_key_from_string(&k, "key1=value1,key2=value2,key3=value3"));
    EXPECT(k);
    std::unique_ptr<dasi_key_t> kdeleter(k);

    CHECK_RETURN(dasi_key_set(k, "key1", "value1a"));
    CHECK_RETURN(dasi_key_set(k, "key3", "value3b"));

    long count;
    CHECK_RETURN(dasi_key_count(k, &count));
    EXPECT(count == 3);

    const char* val;
    CHECK_RETURN(dasi_key_get(k, "key1", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value1a") == 0);
    CHECK_RETURN(dasi_key_get(k, "key2", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value2") == 0);
    CHECK_RETURN(dasi_key_get(k, "key3", &val));
    EXPECT(val);
    EXPECT(::strcmp(val, "value3b") == 0);
}

CASE("Test that clear() works") {

    dasi_key_t* k = nullptr;
    CHECK_RETURN(dasi_new_key_from_string(&k, "key1=value1,key2=value2,key3=value3"));
    EXPECT(k);
    std::unique_ptr<dasi_key_t> kdeleter(k);

    long count;
    CHECK_RETURN(dasi_key_count(k, &count));
    EXPECT(count == 3);

    CHECK_RETURN(dasi_key_clear(k));

    CHECK_RETURN(dasi_key_count(k, &count));
    EXPECT(count == 0);
}

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
