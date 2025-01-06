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

#include "dasi/api/Dasi.h"

#include "helper.h"

#include "eckit/filesystem/LocalPathName.h"

namespace dasi::testing {

const auto tempPath = eckit::LocalPathName::cwd() + "/tmp.DASI.wipe";

//----------------------------------------------------------------------------------------------------------------------

CASE("testing dasi: 1- archive") {
    TempDirectory tempDir(tempPath, false);  // <== keeps the directory

    tempDir.write("simple_schema", SIMPLE_SCHEMA);

    const auto cfg = simpleConfig(tempDir, "simple_schema");

    LOG_I("--- [ARCHIVE] ---");

    dasi::Dasi dasi(cfg.c_str());

    auto keys = KeySet({"value3b1", "value3b2", "value3b3", "value3b4"});

    for (auto&& key : keys) {
        const std::string data = "DASI WIPE TEST 1 DATA " + key.get("key3b");
        dasi.archive(key, data.data(), data.size());
    }

    for (auto key : keys) {
        key.set("key3a", "value3a2");
        const std::string data = "DASI WIPE TEST 2 DATA " + key.get("key3b");
        dasi.archive(key, data.data(), data.size());
    }

    dasi.flush();

    LOG_I("--- [LIST] ---");

    dasi::Query query {{"key1", {"value1"}}, {"key2", {"value2"}}, {"key3", {"value3"}}};

    size_t count = 0;
    for (auto&& item : dasi.list(query)) { count++; }
    EXPECT(count == 8);

    LOG_I("--- [COUNT: " << count << "] ---");
}

CASE("testing dasi: 2- wipe") {
    SECTION("wipe some") {
        TempDirectory tempDir(tempPath, false);  // <== keeps the directory

        const auto fileCount = tempDir.countFilesRecursive();

        dasi::Dasi dasi(simpleConfig(tempDir, "simple_schema").c_str());

        dasi::Query query {{"key1", {"value1"}}, {"key2", {"value2"}}, {"key3", {"value3"}}, {"key3a", {"value3a2"}}};

        size_t count = 0;
        for (auto&& item : dasi.wipe(query, true, true, false)) {
            LOG_D("WIPE: " << item);
            count++;
        }

        LOG_I("--- [WIPED: " << count << "] ---");

        count = 0;
        for (auto&& item : dasi.list(query)) { count++; }
        EXPECT(count == 0);

        LOG_I("--- [LIST: " << count << "] ---");

        EXPECT(tempDir.countFilesRecursive() == fileCount - 2);
    }

    SECTION("wipe all") {
        TempDirectory tempDir(tempPath);

        dasi::Dasi dasi(simpleConfig(tempDir, "simple_schema").c_str());

        dasi::Query query = {{"key1", {"value1"}}, {"key2", {"value2"}}, {"key3", {"value3"}}};

        size_t count = 0;
        for (auto&& item : dasi.wipe(query, true, true, false)) {
            LOG_D("WIPE: " << item);
            count++;
        }

        LOG_I("--- [WIPED: " << count << "] ---");

        count = 0;
        for (auto&& item : dasi.list(query)) { count++; }
        EXPECT(count == 0);

        LOG_I("--- [LIST: " << count << "] ---");

        EXPECT(tempDir.countFilesRecursive() == 1);
    }
}

}  // namespace dasi::testing

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
