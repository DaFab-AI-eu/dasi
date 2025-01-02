/*
 * Copyright 2022- European Centre for Medium-Range Weather Forecasts (ECMWF).
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

#include "eckit/io/MemoryHandle.h"

#include "dasi/api/Dasi.h"

#include "helper.h"

namespace dasi::testing {

//----------------------------------------------------------------------------------------------------------------------

CASE("Archive data") {
    TempDirectory tempDir;

    simpleWrite(tempDir, "simple_schema", SIMPLE_SCHEMA);

    const auto cfg = simpleConfig(tempDir, "simple_schema");

    dasi::Dasi dasi(cfg.c_str());

    SECTION("simple archive") {
        const dasi::Key   key  = *KeySet({"value3b"}).begin();
        const std::string data = "DASI SIMPLE ARCHIVE TEST DATA";
        dasi.archive(key, data.data(), data.size());
    }
}

CASE("Archive data and check list and retrieve") {
    TempDirectory tempDir;

    simpleWrite(tempDir, "simple_schema", SIMPLE_SCHEMA);

    const auto cfg = simpleConfig(tempDir, "simple_schema");

    dasi::Dasi dasi(cfg.c_str());

    const auto keys = KeySet({"value3b1", "value3b2", "value3b3"});

    LOG_D("--- ARCHIVE ---");

    for (auto&& key : keys) {
        const std::string data = "DASI ARCHIVE TEST DATA " + key.get("key3b");
        dasi.archive(key, data.data(), data.size());
    }

    dasi.flush();

    LOG_D("--- LIST ---");

    SECTION("list all") {
        dasi::Query query {{"key1", {"value1"}}, {"key2", {"value2"}}, {"key3", {"value3"}}};

        auto list = dasi.list(query);
        EXPECT(keys.lookup(list) == keys.size());
    }

    SECTION("list subset") {
        dasi::Query query {{"key1", {"value1"}},
                           {"key2", {"value2"}},
                           {"key3", {"value3"}},
                           {"key3b", {"value3b2", "value3b1"}}};

        auto list = dasi.list(query);
        EXPECT(keys.lookup(list) == 2);
    }

    SECTION("list empty") {
        dasi::Query query {{"key1", {"value1"}},
                           {"key2", {"value2"}},
                           {"key3", {"value3"}},
                           {"key3b", {"value4", "value5"}}};

        auto list = dasi.list(query);
        EXPECT(keys.lookup(list) == 0);

        int count = 0;
        for (auto&& item : dasi.list(query)) { ++count; }
        EXPECT(count == 0);
    }

    SECTION("list some") {
        dasi::Query query {{"key1", {"value1"}},
                           {"key2", {"value2", "value2a"}},
                           {"key3", {"value3"}},
                           {"key3b", {"value1", "value3b3"}}};

        auto list = dasi.list(query);
        EXPECT(keys.lookup(list) == 1);
    }

    SECTION("retrieve some") {
        dasi::Query query {{"key1", {"value1"}},   {"key2", {"value2"}},   {"key3", {"value3"}},
                           {"key1a", {"value1a"}}, {"key2a", {"value2a"}}, {"key3a", {"value3a"}},
                           {"key1b", {"value1b"}}, {"key2b", {"value2b"}}, {"key3b", {"value3b1", "value3b3"}}};

        LOG_D("--- RETRIEVE ---");

        auto ret = dasi.retrieve(query);

        EXPECT(ret.count() == 2);

        eckit::MemoryHandle mh;

        const auto len = ret.dataHandle()->saveInto(mh);

        const std::string ref = "DASI ARCHIVE TEST DATA value3b1"   // see query and data
                                "DASI ARCHIVE TEST DATA value3b3";  //

        EXPECT(len == eckit::Length(ref.size()));

        EXPECT(memcmp(mh.data(), ref.data(), ref.size()) == 0);
    }

    /*
    SECTION("Retrieval fails if not fully qualified") {
        EXPECT(false);
    }

    SECTION("Retrieval fails if not all keys in query satisfied") {
        EXPECT(false);
    }*/
}

}  // namespace dasi::testing

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
