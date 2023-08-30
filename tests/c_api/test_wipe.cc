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

#include "helper.h"

#include "eckit/filesystem/LocalPathName.h"

namespace dasi::testing {

const auto tempPath = eckit::LocalPathName::cwd() + "/tmp.DASI.c.wipe";

//----------------------------------------------------------------------------------------------------------------------

CASE("testing dasi: 1- archive") {
    TempDirectory tempDir(tempPath, false);  // <== keeps the directory

    tempDir.write("simple_schema", SIMPLE_SCHEMA);

    const auto cfg = simpleConfig(tempDir, "simple_schema");

    LOG_I("--- [ARCHIVE] ---");

    dasi_t* dasi;
    CHECK_RETURN(dasi_open(&dasi, cfg.c_str()));
    EXPECT(dasi);

    const auto keys = KeySet({"value3b1", "value3b2", "value3b3", "value3b4"});

    dasi_key_t* ckey;
    CHECK_RETURN(dasi_new_key(&ckey));
    EXPECT(ckey);

    for (auto&& key : keys) {
        for (auto&& item : key) { CHECK_RETURN(dasi_key_set(ckey, item.first.c_str(), item.second.c_str())); }
        const std::string data = "DASI WIPE TEST 1 DATA " + key.get("key3b");
        CHECK_RETURN(dasi_archive(dasi, ckey, data.data(), data.size()));
    }

    KeySet keys2;
    for (dasi::Key key : keys) {
        key.set("key3a", "value3a2");
        keys2.insert(key);
        for (auto&& item : key) { CHECK_RETURN(dasi_key_set(ckey, item.first.c_str(), item.second.c_str())); }
        const std::string data = "DASI WIPE TEST 2 DATA " + key.get("key3b");
        CHECK_RETURN(dasi_archive(dasi, ckey, data.data(), data.size()));
    }

    CHECK_RETURN(dasi_free_key(ckey));

    CHECK_RETURN(dasi_flush(dasi));

    LOG_I("--- [LIST] ---");

    dasi_query_t* query;
    CHECK_RETURN(dasi_new_query(&query));
    EXPECT(query);

    CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
    CHECK_RETURN(dasi_query_append(query, "key2", "value2"));
    CHECK_RETURN(dasi_query_append(query, "key3", "value3"));

    dasi_list_t* list;
    CHECK_RETURN(dasi_list(dasi, query, &list));
    EXPECT(list);

    auto count = keys.lookup(list);

    CHECK_RETURN(dasi_free_list(list));

    CHECK_RETURN(dasi_list(dasi, query, &list));
    EXPECT(list);

    count += keys2.lookup(list);

    CHECK_RETURN(dasi_free_list(list));

    EXPECT(count == 8);

    LOG_I("--- [COUNT: " << count << "] ---");

    CHECK_RETURN(dasi_free_query(query));

    CHECK_RETURN(dasi_close(dasi));
}

CASE("testing dasi: 2- wipe") {
    SECTION("wipe some") {
        TempDirectory tempDir(tempPath, false);  // <== keeps the directory

        const auto cfg = simpleConfig(tempDir, "simple_schema");

        dasi_t* dasi;
        CHECK_RETURN(dasi_open(&dasi, cfg.c_str()));
        EXPECT(dasi);

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "value2"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value3"));
        CHECK_RETURN(dasi_query_append(query, "key3a", "value3a2"));

        dasi_wipe_t* wipe;
        dasi_bool_t  doit = true;
        dasi_bool_t  all  = false;
        CHECK_RETURN(dasi_wipe(dasi, query, &doit, &all, &wipe));
        EXPECT(wipe);

        int    rc;
        size_t count = 0;
        while ((rc = dasi_wipe_next(wipe)) == DASI_SUCCESS) {
            const char* out;
            CHECK_RETURN(dasi_wipe_attrs(wipe, &out));
            count++;
        }
        EXPECT(count == 3);
        EXPECT(rc == DASI_ITERATION_COMPLETE);

        CHECK_RETURN(dasi_free_query(query));
        CHECK_RETURN(dasi_free_wipe(wipe));

        CHECK_RETURN(dasi_close(dasi));

        LOG_I("--- [WIPED: " << count << "] ---");
    }

    SECTION("wipe all") {
        TempDirectory tempDir(tempPath);

        const auto cfg = simpleConfig(tempDir, "simple_schema");

        dasi_t* dasi;
        CHECK_RETURN(dasi_open(&dasi, cfg.c_str()));
        EXPECT(dasi);

        dasi_query_t* query;
        CHECK_RETURN(dasi_new_query(&query));
        EXPECT(query);

        CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
        CHECK_RETURN(dasi_query_append(query, "key2", "value2"));
        CHECK_RETURN(dasi_query_append(query, "key3", "value3"));

        dasi_wipe_t* wipe;
        dasi_bool_t  doit = true;
        dasi_bool_t  all  = false;
        CHECK_RETURN(dasi_wipe(dasi, query, &doit, &all, &wipe));
        EXPECT(wipe);

        int    rc;
        size_t count = 0;
        while ((rc = dasi_wipe_next(wipe)) == DASI_SUCCESS) {
            const char* out;
            CHECK_RETURN(dasi_wipe_attrs(wipe, &out));
            count++;
        }
        EXPECT(count == 8);  // ??? lock files
        EXPECT(rc == DASI_ITERATION_COMPLETE);

        CHECK_RETURN(dasi_free_query(query));
        CHECK_RETURN(dasi_free_wipe(wipe));

        CHECK_RETURN(dasi_close(dasi));

        LOG_I("--- [WIPED: " << count << "] ---");
    }
}

}  // namespace dasi::testing

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
