/*
 * (C) Copyright 2023 ECMWF.
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http: //www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to iter by virtue of its status as an intergovernmental organisation
 * nor does iter submit to any jurisdiction.
 */

#include "dasi/api/dasi_c.h"

#include "dasi/api/Dasi.h"
#include "dasi/api/Key.h"
#include "dasi/api/Query.h"
#include "dasi/api/detail/ListDetail.h"
#include "dasi/api/detail/PurgeDetail.h"
#include "dasi/api/detail/RetrieveDetail.h"
#include "dasi/api/detail/WipeDetail.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/io/DataHandle.h"
#include "eckit/log/CodeLocation.h"
#include "eckit/log/Log.h"
#include "eckit/runtime/Main.h"
#include "lib/dasi_version.h"

#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

extern "C" {

// ---------------------------------------------------------------------------------------------------------------------

struct Dasi : public dasi::Dasi {
    using dasi::Dasi::Dasi;
};

struct Key : public dasi::Key {
    using dasi::Key::Key;

    Key(const dasi::Key& k) : dasi::Key(k) { }
};

struct Query : public dasi::Query {
    using dasi::Query::Query;
};

struct dasi_wipe_t {
    dasi_wipe_t(dasi::WipeGenerator&& gen) : first(true), generator(std::move(gen)), iterator(generator.begin()) { }

    bool                                first;
    dasi::WipeGenerator                 generator;
    dasi::WipeGenerator::const_iterator iterator;
    std::string                         value;
};

struct dasi_purge_t {
    dasi_purge_t(dasi::PurgeGenerator&& gen) : first(true), generator(std::move(gen)), iterator(generator.begin()) { }

    bool                                 first;
    dasi::PurgeGenerator                 generator;
    dasi::PurgeGenerator::const_iterator iterator;
    std::string                          value;
};

struct dasi_list_t {
    dasi_list_t(dasi::ListGenerator&& gen) : first(true), generator(std::move(gen)), iterator(generator.begin()) { }

    bool                                first;
    dasi::ListGenerator                 generator;
    dasi::ListGenerator::const_iterator iterator;
    std::string                         uri_cache;
};

struct dasi_retrieve_t {
    dasi_retrieve_t(dasi::RetrieveResult&& ret) : first(true), retrieve(std::move(ret)), iterator(retrieve.begin()) { }

    bool                                 first;
    dasi::RetrieveResult                 retrieve;
    dasi::RetrieveResult::const_iterator iterator;
    std::unique_ptr<eckit::DataHandle>   dh;
    std::optional<eckit::AutoClose>      closer;
};

// ---------------------------------------------------------------------------------------------------------------------
//                           ERROR HANDLING

}  // extern "C"

// template can't have C linkage

static thread_local std::string gCurrentErrorString;

/**
 * @brief Catch the exceptions and capture the detail in dasi_error_t.
 * @param error  Pointer to the error message and code
 * @param fn     Function that throws an exception
 * @return true  No exception caught
 * @return false Exception caught
 */
template<typename Func, typename... Args>
[[nodiscard]] static int try_catch(Func&& fn_ptr, Args&&... args) {
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
            std::forward<Func>(fn_ptr)(std::forward<Args>(args)...);
            return DASI_SUCCESS;
        } else {
            return std::forward<Func>(fn_ptr)(std::forward<Args>(args)...);
        }
    } catch (const eckit::SeriousBug& e) {
        gCurrentErrorString = e.what();
        return DASI_ERROR_BUG;
    } catch (const eckit::UserError& e) {
        gCurrentErrorString = e.what();
        return DASI_ERROR_USER;
    } catch (const eckit::AssertionFailed& e) {
        gCurrentErrorString = e.what();
        return DASI_ERROR_ASSERT;
    } catch (const eckit::Exception& e) {
        gCurrentErrorString = e.what();
        return DASI_ERROR;
    } catch (const std::exception& e) {
        gCurrentErrorString = e.what();
        return DASI_ERROR_UNKNOWN;
    } catch (...) {
        gCurrentErrorString = "<unknown>";
        return DASI_ERROR_UNKNOWN;
    }
}

extern "C" {

// -----------------------------------------------------------------------------
//                           HELPERS
// -----------------------------------------------------------------------------

const char* dasi_get_error_string() {
    return gCurrentErrorString.c_str();
}

int dasi_version(const char** version) {
    ASSERT(version);
    *version = dasi_version();
    return DASI_SUCCESS;
}

int dasi_vcs_version(const char** sha1) {
    ASSERT(sha1);
    *sha1 = dasi_git_sha1();
    return DASI_SUCCESS;
}

int dasi_initialise_api(void) {
    return try_catch([] {
        static bool initialised = false;

        if (initialised) { eckit::Log::warning() << "Initialising DASI library twice" << std::endl; }

        if (!initialised) {
            const char* argv[2] = {"dasi-api", 0};
            eckit::Main::initialise(1, const_cast<char**>(argv));
            initialised = true;
        }
    });
}

// -----------------------------------------------------------------------------
//                           SESSION
// -----------------------------------------------------------------------------

int dasi_open(dasi_t** dasi, const char* config) {
    return try_catch([dasi, config] {
        ASSERT(dasi);
        ASSERT(config);
        *dasi = new Dasi(config);
    });
}

int dasi_close(const dasi_t* dasi) {
    return try_catch([dasi] {
        ASSERT(dasi);
        delete dasi;
    });
}

int dasi_archive(dasi_t* dasi, const dasi_key_t* key, const void* data, long length) {
    return try_catch([dasi, key, data, length] {
        ASSERT(dasi);
        ASSERT(key);
        ASSERT(data);
        ASSERT(length >= 0);
        dasi->archive(*key, data, length);
    });
}

int dasi_wipe(dasi_t* dasi, const dasi_query_t* query, const dasi_bool_t* doit, const dasi_bool_t* all, dasi_wipe_t** wipe) {
    return try_catch([dasi, query, doit, all, wipe] {
        ASSERT(dasi);
        ASSERT(query);
        ASSERT(doit);
        ASSERT(all);
        ASSERT(wipe);
        *wipe = new dasi_wipe_t(dasi->wipe(*query, *doit, true, *all));
    });
}

int dasi_free_wipe(const dasi_wipe_t* wipe) {
    return try_catch([wipe] {
        ASSERT(wipe);
        delete wipe;
    });
}

int dasi_wipe_next(dasi_wipe_t* wipe) {
    return try_catch(std::function<int()> {[wipe] {
        ASSERT(wipe);
        if (wipe->first) {
            wipe->first = false;
        } else {
            ++wipe->iterator;
        }
        if (wipe->iterator == dasi::WipeGenerator::end()) { return DASI_ITERATION_COMPLETE; }
        return DASI_SUCCESS;
    }});
}

int dasi_wipe_get_value(const dasi_wipe_t* wipe, const char** value) {
    return try_catch([wipe, value] {
        ASSERT(wipe);
        ASSERT(wipe->iterator != wipe->generator.end());
        if (value) { *value = wipe->iterator->c_str(); }
    });
}

int dasi_purge(dasi_t* dasi, const dasi_query_t* query, const dasi_bool_t* doit, dasi_purge_t** purge) {
    return try_catch([dasi, query, doit, purge] {
        ASSERT(dasi);
        ASSERT(query);
        ASSERT(doit);
        ASSERT(purge);
        *purge = new dasi_purge_t(dasi->purge(*query, *doit, true));
    });
}

int dasi_free_purge(const dasi_purge_t* purge) {
    return try_catch([purge] {
        ASSERT(purge);
        delete purge;
    });
}

int dasi_purge_next(dasi_purge_t* purge) {
    return try_catch(std::function<int()> {[purge] {
        ASSERT(purge);
        if (purge->first) {
            purge->first = false;
        } else {
            ++purge->iterator;
        }
        if (purge->iterator == purge->generator.end()) { return DASI_ITERATION_COMPLETE; }
        return DASI_SUCCESS;
    }});
}

int dasi_purge_get_value(const dasi_purge_t* purge, const char** value) {
    return try_catch([purge, value] {
        ASSERT(purge);
        ASSERT(purge->iterator != purge->generator.end());
        if (value) { *value = purge->iterator->c_str(); }
    });
}

int dasi_flush(dasi_t* dasi) {
    return try_catch([dasi] {
        ASSERT(dasi);
        dasi->flush();
    });
}

int dasi_list(dasi_t* dasi, const dasi_query_t* query, dasi_list_t** list) {
    return try_catch([dasi, query, list] {
        ASSERT(dasi);
        ASSERT(query);
        ASSERT(list);
        *list = new dasi_list_t(dasi->list(*query));
    });
}

int dasi_free_list(const dasi_list_t* list) {
    return try_catch([list] {
        ASSERT(list);
        delete list;
    });
}

int dasi_list_next(dasi_list_t* list) {
    return try_catch(std::function<int()> {[list] {
        ASSERT(list);
        if (list->first) {
            list->first = false;
        } else {
            ++list->iterator;
        }
        if (list->iterator == dasi::ListGenerator::end()) { return DASI_ITERATION_COMPLETE; }
        list->uri_cache = list->iterator->location.uri.asRawString();
        return DASI_SUCCESS;
    }});
}

int dasi_list_attrs(const dasi_list_t* list,
                    dasi_key_t**       key,
                    dasi_time_t*       timestamp,
                    const char**       uri,
                    long*              offset,
                    long*              length) {
    return try_catch([list, key, timestamp, uri, offset, length] {
        ASSERT(list);
        ASSERT(list->iterator != list->generator.end());
        if (key) { *key = new Key(list->iterator->key); }
        if (timestamp) { *timestamp = list->iterator->timestamp; }
        if (uri) { *uri = list->uri_cache.c_str(); }
        if (offset) { *offset = list->iterator->location.offset; }
        if (length) { *length = list->iterator->location.length; }
    });
}

int dasi_list_count(const dasi_list_t* list, long* count) {
    return try_catch([list, count] {
        ASSERT(list);
        ASSERT(count);
        throw eckit::NotImplemented("dasi_list_count is not implemented yet.", Here());
    });
}

int dasi_retrieve(dasi_t* dasi, const dasi_query_t* query, dasi_retrieve_t** retrieve) {
    return try_catch([dasi, query, retrieve] {
        ASSERT(dasi);
        ASSERT(query);
        ASSERT(retrieve);
        *retrieve = new dasi_retrieve_t(dasi->retrieve(*query));
    });
}

int dasi_free_retrieve(const dasi_retrieve_t* retrieve) {
    return try_catch([retrieve] {
        ASSERT(retrieve);
        delete retrieve;
    });
}

int dasi_retrieve_read(dasi_retrieve_t* retrieve, void* data, long* length) {
    return try_catch(std::function<int()> {[retrieve, data, length] {
        ASSERT(retrieve);
        ASSERT(data);
        ASSERT(length);
        ASSERT(*length > 0);

        if (!retrieve->dh) {
            retrieve->dh = retrieve->retrieve.dataHandle();
            retrieve->dh->openForRead();
            retrieve->closer.emplace(*retrieve->dh);
        }

        *length = retrieve->dh->read(data, *length);
        if (*length == 0) { return DASI_ITERATION_COMPLETE; }
        return DASI_SUCCESS;
    }});
}

int dasi_retrieve_count(const dasi_retrieve_t* retrieve, long* count) {
    return try_catch([retrieve, count] {
        ASSERT(retrieve);
        ASSERT(count);
        *count = retrieve->retrieve.count();
    });
}

int dasi_retrieve_next(dasi_retrieve_t* retrieve) {
    return try_catch(std::function<int()> {[retrieve] {
        ASSERT(retrieve);
        if (retrieve->first) {
            retrieve->first = false;
        } else {
            ++retrieve->iterator;
        }
        if (retrieve->iterator == dasi::RetrieveResult::end()) { return DASI_ITERATION_COMPLETE; }
        return DASI_SUCCESS;
    }});
}

int dasi_retrieve_attrs(const dasi_retrieve_t* retrieve,
                        dasi_key_t**           key,
                        dasi_time_t*           timestamp,
                        long*                  offset,
                        long*                  length) {
    return try_catch([retrieve, key, timestamp, offset, length] {
        ASSERT(retrieve);
        /// @note what happens if retrieve is empty
        ASSERT(retrieve->iterator != retrieve->retrieve.end());
        if (key) { *key = new Key(retrieve->iterator->key); }
        if (timestamp) { *timestamp = retrieve->iterator->timestamp; }
        if (offset) { *offset = retrieve->iterator->location.offset; }
        if (length) { *length = retrieve->iterator->location.length; }
    });
}

// ---------------------------------------------------------------------------------------------------------------------
// KEY

int dasi_new_key(dasi_key_t** key) {
    return try_catch([key] {
        ASSERT(key);
        *key = new Key();
    });
}

int dasi_new_key_from_string(dasi_key_t** key, const char* str) {
    return try_catch([key, str] {
        ASSERT(key);
        ASSERT(str);
        *key = new Key(str);
    });
}

int dasi_free_key(const dasi_key_t* key) {
    return try_catch([key] {
        ASSERT(key);
        delete key;
    });
}

int dasi_key_compare(dasi_key_t* key, dasi_key_t* other, int* result) {
    return try_catch([key, other, result] {
        ASSERT(key);
        ASSERT(other);
        ASSERT(result);
        if (*key < *other) {
            *result = -1;
        } else if (*key > *other) {
            *result = 1;
        } else {
            *result = 0;
        }
    });
}

int dasi_key_set(dasi_key_t* key, const char* keyword, const char* value) {
    return try_catch([key, keyword, value] {
        ASSERT(key);
        ASSERT(keyword);
        ASSERT(value);
        key->set(keyword, value);
    });
}

int dasi_key_get_index(dasi_key_t* key, int n, const char** keyword, const char** value) {
    return try_catch([key, n, keyword, value] {
        ASSERT(key);
        ASSERT(n >= 0);
        ASSERT(static_cast<size_t>(n) < key->size());
        auto iter = key->begin();
        std::advance(iter, n);
        if (keyword) { *keyword = iter->first.c_str(); }
        if (value) { *value = iter->second.c_str(); }
    });
}

int dasi_key_get(dasi_key_t* key, const char* keyword, const char** value) {
    return try_catch([key, keyword, value] {
        ASSERT(key);
        ASSERT(keyword);
        ASSERT(value);
        *value = key->get(keyword).c_str();
    });
}

int dasi_key_has(dasi_key_t* key, const char* keyword, dasi_bool_t* has) {
    return try_catch([key, keyword, has] {
        ASSERT(key);
        ASSERT(keyword);
        ASSERT(has);
        *has = key->has(keyword);
    });
}

int dasi_key_count(dasi_key_t* key, long* count) {
    return try_catch([key, count] {
        ASSERT(key);
        ASSERT(count);
        *count = key->size();
    });
}

int dasi_key_erase(dasi_key_t* key, const char* keyword) {
    return try_catch([key, keyword] {
        ASSERT(key != nullptr);
        ASSERT(keyword != nullptr);
        key->erase(keyword);
    });
}

int dasi_key_clear(dasi_key_t* key) {
    return try_catch([key] {
        ASSERT(key != nullptr);
        key->clear();
    });
}

// ---------------------------------------------------------------------------------------------------------------------
// QUERY

int dasi_new_query(dasi_query_t** query) {
    return try_catch([query] {
        ASSERT(query);
        *query = new Query();
    });
}

int dasi_new_query_from_string(dasi_query_t** query, const char* str) {
    return try_catch([query, str] {
        ASSERT(query);
        ASSERT(str);
        *query = new Query(str);
    });
}

int dasi_free_query(const dasi_query_t* query) {
    return try_catch([query] {
        ASSERT(query);
        delete query;
    });
}

int dasi_query_set(dasi_query_t* query, const char* keyword, const char* values[], int num) {
    return try_catch([query, keyword, values, num] {
        ASSERT(query);
        ASSERT(keyword);
        ASSERT(values);
        ASSERT(num >= 0);
        std::vector<std::string> vals;
        vals.reserve(num);
        for (int i = 0; i < num; i++) {
            ASSERT(values[i]);
            vals.emplace_back(values[i]);
        }
        query->set(keyword, vals);
    });
}

int dasi_query_append(dasi_query_t* query, const char* keyword, const char* value) {
    return try_catch([query, keyword, value] {
        ASSERT(query);
        ASSERT(keyword);
        ASSERT(value);
        query->append(keyword, value);
    });
}

int dasi_query_get(dasi_query_t* query, const char* keyword, int num, const char** value) {
    return try_catch([query, keyword, num, value] {
        ASSERT(query);
        ASSERT(keyword);
        ASSERT(value);
        ASSERT(num >= 0);
        const auto& values(query->get(keyword));
        /// @note different type comparison
        ASSERT(num < values.size());
        *value = values[num].c_str();
    });
}

int dasi_query_has(dasi_query_t* query, const char* keyword, dasi_bool_t* has) {
    return try_catch([query, keyword, has] {
        ASSERT(query);
        ASSERT(keyword);
        ASSERT(has);
        *has = query->has(keyword);
    });
}

int dasi_query_keyword_count(dasi_query_t* query, long* count) {
    return try_catch([query, count] {
        ASSERT(query);
        ASSERT(count);
        *count = query->size();
    });
}

int dasi_query_value_count(dasi_query_t* query, const char* keyword, long* count) {
    return try_catch([query, keyword, count] {
        ASSERT(query);
        ASSERT(count);
        *count = query->get(keyword).size();
    });
}

int dasi_query_erase(dasi_query_t* query, const char* keyword) {
    return try_catch([query, keyword] {
        ASSERT(query != nullptr);
        ASSERT(keyword != nullptr);
        query->erase(keyword);
    });
}

int dasi_query_clear(dasi_query_t* query) {
    return try_catch([query] {
        ASSERT(query != nullptr);
        query->clear();
    });
}

// ---------------------------------------------------------------------------------------------------------------------

}  // extern "C"
