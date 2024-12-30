/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http: //www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

/**
 * @file DASI C API Conventions
 *
 * - #define and Enum constants should be all CAPS.
 * - Function, variable, typedef, struct, enum tag names should be lower case.
 *   Separate the words by underscore "_" for clarity. Typedefs end with "_t".
 * - Functions that are PUBLIC should start with "dasi_".
 * - Each object is represented as a pointer to an opaque structure.
 * - If the function creates a new object, it has to be released using the
 *   appropriate "<obj>_delete" call.
 *
 */

#ifndef DASI_API_DASI_C_H
#define DASI_API_DASI_C_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------------------------------------------------
 * TYPES
 * -----*/

/** DASI bool type */
typedef int dasi_bool_t;

/** DASI time type */
typedef long dasi_time_t;

struct Dasi;
/** DASI instance type */
typedef struct Dasi dasi_t;

struct Key;
/** DASI key type */
typedef struct Key dasi_key_t;

struct Query;
/** DASI query type */
typedef struct Query dasi_query_t;

struct dasi_wipe_t;
/** DASI wipe type */
typedef struct dasi_wipe_t dasi_wipe_t;

struct dasi_purge_t;
/** DASI purge type */
typedef struct dasi_purge_t dasi_purge_t;

struct dasi_list_t;
/** DASI list type */
typedef struct dasi_list_t dasi_list_t;

struct dasi_retrieve_t;
/** DASI retrieve type */
typedef struct dasi_retrieve_t dasi_retrieve_t;

/* ---------------------------------------------------------------------------------------------------------------------
 * ERROR HANDLING
 * -------------- */

/** DASI Error Codes */
typedef enum dasi_error_values_t {
    DASI_SUCCESS            = 0, /* Operation succeded. */
    DASI_ITERATION_COMPLETE = 1, /* All elements have been returned */
    DASI_ERROR              = 2, /* Operation failed. */
    DASI_ERROR_UNKNOWN      = 3, /* Failed with an unknown error. */
    DASI_ERROR_USER         = 4, /* Failed with an user error. */
    DASI_ERROR_ITERATOR     = 5, /* Failed with an iterator error. */
    DASI_ERROR_ASSERT       = 6  /* Failed with an assert() */
} dasi_error_enum_t;

/** Returns pointer to a globally allocated string.
 * DO NOT modify/free the returned pointer.
 */
const char* dasi_get_error_string(void);

/* -----------------------------------------------------------------------------
 * HELPERS
 * ------- */

/**
 * Gets version.
 * @param version Version string. DO NOT modify/free the returned pointer.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_version(const char** version);

/**
 * Gets git sha1 version.
 * @param sha1 SHA1 version string. DO NOT modify/free the returned pointer.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_vcs_version(const char** sha1);

/**
 * Initialise Main() context.
 * @note This is ONLY required when Main() is NOT initialised, such as loading
 * the DASI as shared library in Python.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_initialise_api(void);

/* ---------------------------------------------------------------------------------------------------------------------
 * DASI SESSION
 * ------------ */

/**
 * Creates a new dasi object using the given configuration file.
 * @param dasi output dasi object
 * @param config input file or string to dasi configuration (yaml format)
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_open(dasi_t** dasi, const char* config);

/**
 * Deletes the dasi object.
 * @param dasi object to delete
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_close(const dasi_t* dasi);

/**
 * Writes data to the object store.
 *
 * @note Data is not guaranteed accessible nor persisted (wrt. failure),
 * until dasi_flush() is called.
 *
 * @param dasi dasi object
 * @param key Metadata description of the data to store and index
 * @param data Pointer to the read-only data
 * @param length Length of "data" in bytes
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_archive(dasi_t* dasi, const dasi_key_t* key, const void* data, long length);

int dasi_flush(dasi_t* dasi);

/* List functionality */

int dasi_list(dasi_t* dasi, const dasi_query_t* query, dasi_list_t** list);

int dasi_free_list(const dasi_list_t* list);

int dasi_list_count(const dasi_list_t* list, long* count);

int dasi_list_next(dasi_list_t* list);

int dasi_list_attrs(const dasi_list_t* list, dasi_key_t** key, dasi_time_t* timestamp, const char** uri, long* offset,
                    long* length);

/* Retrieve functionality */

int dasi_retrieve(dasi_t* dasi, const dasi_query_t* query, dasi_retrieve_t** retrieve);

int dasi_free_retrieve(const dasi_retrieve_t* retrieve);

int dasi_retrieve_read(dasi_retrieve_t* retrieve, void* data, long* length);

int dasi_retrieve_count(const dasi_retrieve_t* retrieve, long* count);

int dasi_retrieve_next(dasi_retrieve_t* retrieve);

int dasi_retrieve_attrs(const dasi_retrieve_t* retrieve, dasi_key_t** key, dasi_time_t* timestamp, long* offset,
                        long* length);

/* Wipe functionality */

/**
 * Deletes the data that matches the query.
 * @param dasi dasi object
 * @param query metadata description of the data
 * @param doit true: wipe, false: simulate
 * @param all wipe all (unowned) contents of an unclean database
 * @param wipe new wipe object
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_wipe(dasi_t* dasi, const dasi_query_t* query, const dasi_bool_t* doit, const dasi_bool_t* all,
              dasi_wipe_t** wipe);

int dasi_free_wipe(const dasi_wipe_t* wipe);

int dasi_wipe_next(dasi_wipe_t* wipe);

/**
 * Gets the output value of wipe.
 * @param value pointer to value of wipe.
 * DO NOT modify/free the returned pointer.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_wipe_get_value(const dasi_wipe_t* wipe, const char** value);

/* Purge functionality */

int dasi_purge(dasi_t* dasi, const dasi_query_t* query, const dasi_bool_t* doit, dasi_purge_t** purge);

int dasi_free_purge(const dasi_purge_t* purge);

int dasi_purge_next(dasi_purge_t* purge);

/**
 * Gets the output value of wipe.
 * @param value Pointer to value of wipe.
 * DO NOT modify/free the returned pointer.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_purge_get_value(const dasi_purge_t* purge, const char** value);

/* ---------------------------------------------------------------------------------------------------------------------
 * KEY
 * --- */

/**
 * Constructs a new dasi key object.
 * @param key pointer to new object. Returned value must be freed via dasi_free_...
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_new_key(dasi_key_t** key);

/**
 * Constructs a new dasi key from a string.
 * @param str input string.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_new_key_from_string(dasi_key_t** key, const char* str);

/**
 * Deletes the key object.
 * @param key input key.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_free_key(const dasi_key_t* key);

/**
 * Sets the value of the specified keyword.
 * @note The keyword is added if it's missing.
 * @param key input key.
 * @param keyword input keyword.
 * @param value input value.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_key_set(dasi_key_t* key, const char* keyword, const char* value);

int dasi_key_compare(dasi_key_t* key, dasi_key_t* other, int* result);

/** Get the name of a numbered key */
int dasi_key_get_index(dasi_key_t* key, int n, const char** keyword, const char** value);

/**
 * Gets the value of a specified keyword in a key.
 * @param key input key.
 * @param keyword input keyword.
 * @param value pointer to key value.
 * DO NOT modify/free the returned pointer.
 * @return dasi error code, see dasi_error_enum_t.
 */
int dasi_key_get(dasi_key_t* key, const char* keyword, const char** value);

/** Does the key have a specified keyword */
int dasi_key_has(dasi_key_t* key, const char* keyword, dasi_bool_t* has);

/** How many keys have been set */
int dasi_key_count(dasi_key_t* key, long* count);

/** Erase the keyword:value pair specified by its keyword. */
int dasi_key_erase(dasi_key_t* key, const char* keyword);

/** Erase all elements in the key */
int dasi_key_clear(dasi_key_t* key);

/* ---------------------------------------------------------------------------------------------------------------------
 * QUERY
 * ----- */

int dasi_new_query(dasi_query_t** query);

int dasi_new_query_from_string(dasi_query_t** query, const char* str);

int dasi_free_query(const dasi_query_t* query);

int dasi_query_set(dasi_query_t* query, const char* keyword, const char* values[], int num);

int dasi_query_append(dasi_query_t* query, const char* keyword, const char* value);

int dasi_query_keyword_count(dasi_query_t* query, long* count);

int dasi_query_value_count(dasi_query_t* query, const char* keyword, long* count);

int dasi_query_get(dasi_query_t* query, const char* keyword, int num, const char** value);

int dasi_query_has(dasi_query_t* query, const char* keyword, dasi_bool_t* has);

int dasi_query_erase(dasi_query_t* query, const char* keyword);

int dasi_query_clear(dasi_query_t* query);

/* -------------------------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* DASI_API_DASI_C_H */
