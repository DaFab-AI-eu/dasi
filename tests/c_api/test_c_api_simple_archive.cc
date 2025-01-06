/*
 * (C) Copyright 2022- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/filesystem/LocalPathName.h"
#include "eckit/filesystem/TmpDir.h"
#include "eckit/io/FileHandle.h"
#include "eckit/io/MemoryHandle.h"
#include "eckit/testing/Test.h"

#include "dasi/api/dasi_c.h"

#include <tuple>
#include <memory>
#include <cstring>

#define CHECK_RETURN(x) EXPECT((x) == DASI_SUCCESS);

namespace std {
template <> struct default_delete<dasi_query_t> {
    void operator() (const dasi_query_t* q) { CHECK_RETURN(dasi_free_query(q)); }
};
template <> struct default_delete<dasi_key_t> {
    void operator() (const dasi_key_t* k) { CHECK_RETURN(dasi_free_key(k)); }
};
template <> struct default_delete<dasi_t> {
    void operator() (const dasi_t* d) { CHECK_RETURN(dasi_close(d)); }
};
template <> struct default_delete<dasi_list_t> {
    void operator() (const dasi_list_t* l) { CHECK_RETURN(dasi_free_list(l)); }
};
template <> struct default_delete<dasi_retrieve_t> {
    void operator() (const dasi_retrieve_t* r) { CHECK_RETURN(dasi_free_retrieve(r)); }
};
}


using eckit::testing::run_tests;

//----------------------------------------------------------------------------------------------------------------------

constexpr const char SIMPLE_SCHEMA[] = R"(
# Keys are by default Strings, unless otherwise specified

key2:  Integer;
key3a: Integer;

[ key1, key2, key3
    [ key1a, key2a, key3a
       [ key1b, key2b, key3b ]
    ]
]
)";


std::string simple_config(const eckit::PathName& basedir) {
    return std::string(R"(
        schema: )") + (basedir / "simple_schema") + R"(
        catalogue: toc
        store: file
        spaces:
         - roots:
           - path: )" + (basedir / "root") + R"(
        )";
}

//----------------------------------------------------------------------------------------------------------------------

class ListResultChecker {

public:
    ListResultChecker(std::initializer_list<std::map<std::string, std::string>> init) : expectedValues_(init), validated_(false) {}

    ~ListResultChecker() noexcept(false) {
//        validate();
    }

    void found(const std::map<std::string, std::string>& k) {
        ASSERT(!validated_);
        EXPECT(expectedValues_.erase(k) == 1);
    }

    void validate() {
        if (!validated_) {
            validated_ = true;
            EXPECT(expectedValues_.empty());
        }
    }

    void check(dasi_t* dasi, const dasi_query_t* query) {

        dasi_list_t* list;
        CHECK_RETURN(dasi_list(dasi, query, &list));
        EXPECT(list);
        std::unique_ptr<dasi_list_t> ldeleter(list);

        int rc;
        while ((rc = dasi_list_next(list)) == DASI_SUCCESS) {

            /// @todo - test all the return values in list, PROPERLY

            dasi_key_t* key;
            CHECK_RETURN(dasi_list_attrs(list, &key, /* timestamp */ nullptr, /* uri */ nullptr, /* offset */ nullptr, /* length */ nullptr))
            EXPECT(key);
            std::unique_ptr<dasi_key_t> kdeleter(key);

            long count;
            CHECK_RETURN(dasi_key_count(key, &count));

            std::map<std::string, std::string> current_key;
            for (long i = 0; i < count; ++i) {
                const char* k;
                const char* v;
                CHECK_RETURN(dasi_key_get_index(key, i, &k, &v));
                current_key[k] = v;
            }
            found(current_key);
        }

        EXPECT(rc == DASI_ITERATION_COMPLETE);
        validate();
    }

    template <typename GENERATOR>
    void iterate(GENERATOR&& generator) {
        for (const auto& elem : generator) {
            found(elem.key);
        }
        validate();
    }

private:
    std::set<std::map<std::string, std::string>> expectedValues_;
    bool validated_;
};

//----------------------------------------------------------------------------------------------------------------------

CASE("simple archive") {

    eckit::TmpDir test_wd(eckit::LocalPathName::cwd().c_str());
//    std::cout << "Working directory: " << test_wd << std::endl;

    test_wd.mkdir();
    (test_wd / "root").mkdir();

    {
        eckit::FileHandle dh(test_wd / "simple_schema");
        dh.openForWrite(0);
        eckit::AutoClose close(dh);
        dh.write(SIMPLE_SCHEMA, sizeof(SIMPLE_SCHEMA));
    }

    // Create the new dasi session
    dasi_t* dasi;
    CHECK_RETURN(dasi_open(&dasi, simple_config(test_wd).c_str()));
    EXPECT(dasi);
    std::unique_ptr<dasi_t> ddeleter(dasi);

    SECTION("simple archive") {

        dasi_key_t* key;
        CHECK_RETURN(dasi_new_key(&key));
        EXPECT(key);
        std::unique_ptr<dasi_key_t> kdeleter(key);

        CHECK_RETURN(dasi_key_set(key, "key1", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key2", "123"));
        CHECK_RETURN(dasi_key_set(key, "key3", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key1a", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key2a", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key3a", "321"));
        CHECK_RETURN(dasi_key_set(key, "key1b", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key2b", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key3b", "value1"));

        constexpr const char test_data[] = "TESTING SIMPLE ARCHIVE";
        CHECK_RETURN(dasi_archive(dasi, key, test_data, sizeof(test_data) - 1));
    }
}


CASE("Accessing data that has been archived") {

    eckit::TmpDir test_wd(eckit::LocalPathName::cwd().c_str());
    std::cout << "Working directory: " << test_wd << std::endl;

    test_wd.mkdir();
    (test_wd / "root").mkdir();

    {
        eckit::FileHandle dh(test_wd / "simple_schema");
        dh.openForWrite(0);
        eckit::AutoClose close(dh);
        dh.write(SIMPLE_SCHEMA, sizeof(SIMPLE_SCHEMA));
    }

    dasi_t* dasi;
    CHECK_RETURN(dasi_open(&dasi, simple_config(test_wd).c_str()));
    EXPECT(dasi);
    std::unique_ptr<dasi_t> ddeleter(dasi);

    constexpr const char test_data1[] = "TESTING SIMPLE ARCHIVE";
    constexpr const char test_data2[] = "TESTING SIMPLE ARCHIVE 2222222222";
    constexpr const char test_data3[] = "TESTING SIMPLE ARCHIVE 3333333333";

    for (const auto& elems : std::vector<std::tuple<const char*, int, const char*>> {
                {test_data1, sizeof(test_data1)-1, "value1"},
                {test_data2, sizeof(test_data2)-1, "value2"},
                {test_data3, sizeof(test_data3)-1, "value3"}}) {

        dasi_key_t* key;
        CHECK_RETURN(dasi_new_key(&key));
        EXPECT(key);
        std::unique_ptr<dasi_key_t> kdeleter(key);

        CHECK_RETURN(dasi_key_set(key, "key1", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key2", "123"));
        CHECK_RETURN(dasi_key_set(key, "key3", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key1a", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key2a", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key3a", "321"));
        CHECK_RETURN(dasi_key_set(key, "key1b", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key2b", "value1"));
        CHECK_RETURN(dasi_key_set(key, "key3b", std::get<2>(elems)));

        CHECK_RETURN(dasi_archive(dasi, key, std::get<0>(elems), std::get<1>(elems)));
    }

    CHECK_RETURN(dasi_flush(dasi));

    SECTION("We can list all of the data correctly") {

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);
        std::unique_ptr<dasi_query_t> qdeleter(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "123"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value1"));

        ListResultChecker checker{
            {{"key1",  "value1"}, {"key2",  "123"}, {"key3",  "value1"}, {"key1a", "value1"}, {"key2a", "value1"},
             {"key3a", "321"}, {"key1b", "value1"}, {"key2b", "value1"}, {"key3b", "value1"}},
            {{"key1",  "value1"}, {"key2",  "123"}, {"key3",  "value1"}, {"key1a", "value1"}, {"key2a", "value1"},
             {"key3a", "321"}, {"key1b", "value1"}, {"key2b", "value1"}, {"key3b", "value2"}},
            {{"key1",  "value1"}, {"key2",  "123"}, {"key3",  "value1"}, {"key1a", "value1"}, {"key2a", "value1"},
             {"key3a", "321"}, {"key1b", "value1"}, {"key2b", "value1"}, {"key3b", "value3"}},
        };

        checker.check(dasi, query);
    }

    SECTION("We can list a subset of the data") {

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);
        std::unique_ptr<dasi_query_t> qdeleter(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "123"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value3"));

        ListResultChecker checker{
                {{"key1",  "value1"}, {"key2",  "123"}, {"key3",  "value1"}, {"key1a", "value1"}, {"key2a", "value1"},
                 {"key3a", "321"}, {"key1b", "value1"}, {"key2b", "value1"}, {"key3b", "value1"}},
                {{"key1",  "value1"}, {"key2",  "123"}, {"key3",  "value1"}, {"key1a", "value1"}, {"key2a", "value1"},
                 {"key3a", "321"}, {"key1b", "value1"}, {"key2b", "value1"}, {"key3b", "value3"}},
        };

        checker.check(dasi, query);
    }

    SECTION("We can match no data") {

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);
        std::unique_ptr<dasi_query_t> qdeleter(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "123"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value4"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value5"));

        ListResultChecker checker{};
        checker.check(dasi, query);
    }

    SECTION("We can have only some of the query matched") {

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);
        std::unique_ptr<dasi_query_t> qdeleter(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "123"));
        CHECK_RETURN(dasi_query_append(query, "key2", "321"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value3"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value4"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value5"));

        ListResultChecker checker{
                {{"key1",  "value1"}, {"key2",  "123"}, {"key3",  "value1"}, {"key1a", "value1"}, {"key2a", "value1"},
                 {"key3a", "321"}, {"key1b", "value1"}, {"key2b", "value1"}, {"key3b", "value1"}},
                {{"key1",  "value1"}, {"key2",  "123"}, {"key3",  "value1"}, {"key1a", "value1"}, {"key2a", "value1"},
                 {"key3a", "321"}, {"key1b", "value1"}, {"key2b", "value1"}, {"key3b", "value3"}},
        };

        checker.check(dasi, query);
    }

    SECTION("We get no results from a query with no hits") {

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);
        std::unique_ptr<dasi_query_t> qdeleter(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "123"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "unknown"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "values"));

        ListResultChecker checker{};
        checker.check(dasi, query);
    }

    SECTION("simple retrieve reading all data in one pass") {

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);
        std::unique_ptr<dasi_query_t> qdeleter(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "123"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key1a", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2a", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3a", "321"));
        CHECK_RETURN(dasi_query_append(query, "key1b", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2b", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value3"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value1"));

        dasi_retrieve_t* ret;
        CHECK_RETURN(dasi_retrieve(dasi, query, &ret));
        EXPECT(ret);
        std::unique_ptr<dasi_retrieve_t> rdeleter(ret);

        long count;
        CHECK_RETURN(dasi_retrieve_count(ret, &count));
        EXPECT(count == 2);

//        EXPECT(data sizes on iteration);

        // Check with a length that is shorter than the data --> iterate through it...

        char buffer[128];
        char* pos = buffer;
        long length = 12;
        long total_read = 0;
        int rc;
        while ((rc = dasi_retrieve_read(ret, pos, &length)) == DASI_SUCCESS) {
            EXPECT(length == 12 || length == 7);
            pos += length;
            total_read += length;
        }

        EXPECT(rc == DASI_ITERATION_COMPLETE);
        EXPECT(length == 0);
        EXPECT(total_read == 55);
        EXPECT(::memcmp(buffer, "TESTING SIMPLE ARCHIVE 3333333333TESTING SIMPLE ARCHIVE", 55) == 0);
    }

    SECTION("Retrieve data one object at a time") {

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);
        std::unique_ptr<dasi_query_t> qdeleter(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "123"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key1a", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2a", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3a", "321"));
        CHECK_RETURN(dasi_query_append(query, "key1b", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2b", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value3"));
        CHECK_RETURN(dasi_query_append(query, "key3b", "value1"));

        dasi_retrieve_t* ret;
        CHECK_RETURN(dasi_retrieve(dasi, query, &ret));
        EXPECT(ret);
        std::unique_ptr<dasi_retrieve_t> rdeleter(ret);

        long count;
        CHECK_RETURN(dasi_retrieve_count(ret, &count));
        EXPECT(count == 2);

        const char* expected_data[] = {test_data3, test_data1};

        int rc;
        long length;
        long total_read = 0;
        char buffer[128];
        count = 0;
        while ((rc = dasi_retrieve_next(ret)) == DASI_SUCCESS) {

            EXPECT(count < 2);
            long expected_len = ::strlen(expected_data[count]);
            CHECK_RETURN(dasi_retrieve_attrs(ret, nullptr, nullptr, nullptr, &length));
            EXPECT(length == expected_len);
            CHECK_RETURN(dasi_retrieve_read(ret, buffer, &length));
            EXPECT(length == expected_len);
            EXPECT(::strncmp(expected_data[count], buffer, length) == 0);
            total_read += length;
            count++;
        }

        EXPECT(rc == DASI_ITERATION_COMPLETE);
        EXPECT(count = 2);
        EXPECT(total_read == 55);
    }

    /*
    SECTION("Retrieval fails if not fully qualified") {
        EXPECT(false);
    }

    SECTION("Retrieval fails if not all keys in query satisfied") {
        EXPECT(false);
    }*/
}

//CASE("Arhive data that should be masked...") {
//    EXPECT(false);
//}

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}
