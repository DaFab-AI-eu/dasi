/*
 * (C) Copyright 2023 ECMWF.
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http: //www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "dasi/api/c/Dasi.h"
#include "dasi/api/Dasi.h"

#include <eckit/exception/Exceptions.h>

// -----------------------------------------------------------------------------
//                           ERROR HANDLING
// -----------------------------------------------------------------------------

struct DasiError {
    DasiError() {}
    void Set(const std::string& msg, const dasi_error_enum_t code) {
        this->Code    = code;
        this->Message = msg;
    }

public:
    std::string Message;
    dasi_error_enum_t Code{DASI_SUCCESS};
};

const char* dasi_error_get_message(const dasi_error_t* error) {
    return error->Message.c_str();
}

dasi_error_enum_t dasi_error_get_code(const dasi_error_t* error) {
    return error->Code;
}

DasiError* g_error = new DasiError();

/**
 * @brief Catch the exceptions and capture the detail in dasi_error_t.
 * @param error  Pointer to the error message and code
 * @param fn     Function that throws an exception
 * @return true  No exception caught
 * @return false Exception caught
 */
template <typename F>
bool tryCatch(dasi_error_t** error, F&& fn) {
    *error = g_error;
    try {
        fn();
        // eckit::Log::debug() << "[Success]" << std::endl;
        g_error->Set("Success", DASI_SUCCESS);
    }
    catch (const eckit::UserError& e) {
        eckit::Log::error() << "User Error: " << e.what() << std::endl;
        g_error->Set(e.what(), DASI_ERROR_USER);
        return false;
    }
    catch (const eckit::Exception& e) {
        eckit::Log::error() << "DASI Error: " << e.what() << std::endl;
        g_error->Set(e.what(), DASI_ERROR);
        return false;
    }
    catch (const std::exception& e) {
        eckit::Log::error() << "Unkown Error: " << e.what() << std::endl;
        g_error->Set(e.what(), DASI_ERROR_UNKNOWN);
        return false;
    }
    catch (...) {
        eckit::Log::error() << "Unkown Error!" << std::endl;
        g_error->Set("Unknown Error", DASI_ERROR_UNKNOWN);
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
//                           SESSION
// -----------------------------------------------------------------------------

dasi_t* dasi_new(const char* filename, dasi_error_t** error) {
    dasi::Dasi* result = nullptr;
    tryCatch(error, [filename, &result] {
        ASSERT(filename != nullptr);
        result = new dasi::Dasi(filename);
    });
    return reinterpret_cast<dasi_t*>(result);
}

void dasi_delete(dasi_t* p_session, dasi_error_t** error) {
    auto* session = reinterpret_cast<dasi::Dasi*>(p_session);
    tryCatch(error, [&session] {
        ASSERT(session != nullptr);
        delete session;
        session = nullptr;
    });
}

void dasi_archive(dasi_t* p_session, const dasi_key_t* p_key, const void* data,
                  size_t length, dasi_error_t** error) {
    auto* session   = reinterpret_cast<dasi::Dasi*>(p_session);
    const auto* key = reinterpret_cast<const dasi::Key*>(p_key);
    tryCatch(error, [session, key, data, length] {
        ASSERT(session != nullptr);
        ASSERT(key != nullptr);
        ASSERT(data != nullptr);
        session->archive(*key, data, length);
    });
}

dasi_list_t* dasi_list(dasi_t* p_session, const dasi_query_t* p_query,
                       dasi_error_t** error) {
    dasi_list_t* result = nullptr;
    auto* session       = reinterpret_cast<dasi::Dasi*>(p_session);
    const auto* query   = reinterpret_cast<const dasi::Query*>(p_query);
    tryCatch(error, [session, query, &result] {
        ASSERT(session != nullptr);
        ASSERT(query != nullptr);
        auto tmp   = session->list(*query);
        auto* list = new dasi::ListGenerator(tmp);
        result     = reinterpret_cast<dasi_list_t*>(list);
    });
    return result;
}

void dasi_list_delete(dasi_list_t** p_list, dasi_error_t** error) {
    auto* list = reinterpret_cast<dasi::ListGenerator*>(*p_list);
    tryCatch(error, [&list, &p_list] {
        ASSERT(list != nullptr);
        delete list;
        *p_list = nullptr;
    });
}

void dasi_flush(dasi_t* p_session, dasi_error_t** error) {
    auto* session = reinterpret_cast<dasi::Dasi*>(p_session);
    tryCatch(error, [session] {
        ASSERT(session != nullptr);
        session->flush();
    });
}

// -----------------------------------------------------------------------------
//                           KEY
// -----------------------------------------------------------------------------

dasi_key_t* dasi_key_new(dasi_error_t** error) {
    dasi::Key* result = nullptr;
    tryCatch(error, [&result] { result = new dasi::Key(); });
    return reinterpret_cast<dasi_key_t*>(result);
    ;
}

void dasi_key_delete(dasi_key_t* p_key, dasi_error_t** error) {
    auto* key = reinterpret_cast<dasi::Key*>(p_key);
    tryCatch(error, [&key] {
        ASSERT(key != nullptr);
        delete key;
        key = nullptr;
    });
}

void dasi_key_set(dasi_key_t* p_key, const char* keyword, const char* value,
                  dasi_error_t** error) {
    auto* key = reinterpret_cast<dasi::Key*>(p_key);
    tryCatch(error, [key, keyword, value] {
        ASSERT(key != nullptr);
        ASSERT(keyword != nullptr);
        ASSERT(value != nullptr);
        key->set(keyword, value);
    });
}

void dasi_key_erase(dasi_key_t* p_key, const char* keyword,
                    dasi_error_t** error) {
    auto* key = reinterpret_cast<dasi::Key*>(p_key);
    tryCatch(error, [key, keyword] {
        ASSERT(key != nullptr);
        ASSERT(keyword != nullptr);
        key->erase(keyword);
    });
}

// -----------------------------------------------------------------------------
//                           QUERY
// -----------------------------------------------------------------------------

dasi_query_t* dasi_query_new(dasi_error_t** error) {
    dasi::Query* result = nullptr;
    tryCatch(error, [&result] { result = new dasi::Query(); });
    return reinterpret_cast<dasi_query_t*>(result);
}

void dasi_query_delete(dasi_query_t* p_query, dasi_error_t** error) {
    auto* query = reinterpret_cast<dasi::Query*>(p_query);
    tryCatch(error, [&query] {
        ASSERT(query != nullptr);
        delete query;
        query = nullptr;
    });
}

void dasi_query_set(dasi_query_t* p_query, const char* keyword,
                    const char* values[], const size_t num,
                    dasi_error_t** error) {
    auto* query = reinterpret_cast<dasi::Query*>(p_query);
    tryCatch(error, [query, keyword, values, num] {
        ASSERT(query != nullptr);
        ASSERT(keyword != nullptr);
        ASSERT(values != nullptr);
        query->set(keyword, dasi::Query::value_type{values, values + num});
    });
}

void dasi_query_append(dasi_query_t* p_query, const char* keyword,
                       const char* value, dasi_error_t** error) {
    auto* query = reinterpret_cast<dasi::Query*>(p_query);
    tryCatch(error, [query, keyword, value] {
        ASSERT(query != nullptr);
        ASSERT(keyword != nullptr);
        ASSERT(value != nullptr);
        query->append(keyword, value);
    });
}

// -----------------------------------------------------------------------------
//                           LIST
// -----------------------------------------------------------------------------

dasi_key_t* dasi_list_elem_get_key(dasi_list_elem_t* p_element) {
    auto* elem = reinterpret_cast<dasi::ListElement*>(p_element);
    ASSERT(elem != nullptr);
    auto* key = new dasi::Key(elem->key);
    return reinterpret_cast<dasi_key_t*>(key);
}

// List: Triple-iterator
// -----------------------------------------------------------------------------

dasi_list_elem_t* dasi_list_first(dasi_list_t* p_list) {
    auto* list = reinterpret_cast<dasi::ListGenerator*>(p_list);
    ASSERT(list != nullptr);
    const auto& current = *list->begin();
    auto* result        = new dasi::ListElement(current);
    return reinterpret_cast<dasi_list_elem_t*>(result);
}

dasi_list_elem_t* dasi_list_next(dasi_list_t* p_list,
                                 dasi_list_elem_t* p_element) {
    auto* list = reinterpret_cast<dasi::ListGenerator*>(p_list);
    auto* elem = reinterpret_cast<dasi::ListElement*>(p_element);
    ASSERT(list != nullptr);
    ASSERT(elem != nullptr);
    delete elem;
    p_element = nullptr;
    ++list->begin();
    const auto& next = *list->begin();
    auto* result     = new dasi::ListElement(next);
    return reinterpret_cast<dasi_list_elem_t*>(result);
}

int dasi_list_done(dasi_list_t* p_list) {
    auto* list = reinterpret_cast<dasi::ListGenerator*>(p_list);
    ASSERT(list != nullptr);
    return static_cast<int>(list->begin().done());
}
