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

const auto tempPath = eckit::LocalPathName::cwd() + "/tmp.DASI.c";

//----------------------------------------------------------------------------------------------------------------------

CASE("testing dasi archive") {
    TempDirectory tempDir(tempPath, false);

    tempDir.write("simple_schema", SIMPLE_SCHEMA);

    LOG_I("--- [ARCHIVE] ---");

    dasi_t* dasi;
    CHECK_RETURN(dasi_open(&dasi, simpleConfig(tempDir, "simple_schema").c_str()));
    EXPECT(dasi);

    const auto keys = KeySet({"value3b1", "value3b2", "value3b3", "value3b4"});

    dasi_key_t* ckey;
    CHECK_RETURN(dasi_new_key(&ckey));
    EXPECT(ckey);

    for (auto&& key : keys) {
        for (auto&& item : key) { CHECK_RETURN(dasi_key_set(ckey, item.first.c_str(), item.second.c_str())); }
        const std::string data = "DASI PURGE TEST DATA " + key.get("key3b");
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

    const auto count = keys.lookup(list);
    EXPECT(count == 4);

    LOG_I("--- [COUNT: " << count << "] ---");

    CHECK_RETURN(dasi_free_query(query));
    CHECK_RETURN(dasi_free_list(list));
    CHECK_RETURN(dasi_close(dasi));
}

CASE("testing dasi purge") {
    TempDirectory tempDir(tempPath);

    const auto cfg = simpleConfig(tempDir, "simple_schema");

    LOG_I("--- [PURGE] ---");

    dasi_t* dasi;
    CHECK_RETURN(dasi_open(&dasi, cfg.c_str()));
    EXPECT(dasi);

    dasi_query_t* query;
    CHECK_RETURN(dasi_new_query(&query));
    EXPECT(query);

    CHECK_RETURN(dasi_query_append(query, "key1", "value1"));
    CHECK_RETURN(dasi_query_append(query, "key2", "value2"));
    CHECK_RETURN(dasi_query_append(query, "key3", "value3"));

    dasi_purge_t* purge;
    dasi_bool_t   doit = true;
    CHECK_RETURN(dasi_purge(dasi, query, &doit, &purge));
    EXPECT(purge);

    size_t count = 0;
    int    rc;
    while ((rc = dasi_purge_next(purge)) == DASI_SUCCESS) {
        const char* out;
        CHECK_RETURN(dasi_purge_get_value(purge, &out));
        LOG_D("PURGED: " << out);
        count++;
    }
    EXPECT(rc == DASI_ITERATION_COMPLETE);
    EXPECT(count == 2);

    LOG_I("--- [COUNT: " << count << "] ---");

    CHECK_RETURN(dasi_free_query(query));
    CHECK_RETURN(dasi_free_purge(purge));
    CHECK_RETURN(dasi_close(dasi));
}

}  // namespace dasi::testing

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    eckit::Main::initialise(argc, argv);

    int err = 0;

    {
        dasi::testing::DasiForker archiver;
        archiver.start();
        err += archiver.runTest("testing dasi archive");
        err += archiver.runTest("testing dasi archive");
    }

    {
        dasi::testing::DasiForker purger;
        purger.start();
        err += purger.runTest("testing dasi purge");
    }

    return err;
}
