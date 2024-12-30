/*
 * (C) Copyright 2022- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Simon Smart
/// @author Metin Cakircali
/// @date   Sep 2022

#pragma once

#include "dasi/api/Key.h"
#include "dasi/api/Query.h"
#include "dasi/api/detail/ListDetail.h"
#include "dasi/api/detail/PolicyDetail.h"
#include "dasi/api/detail/PurgeDetail.h"
#include "dasi/api/detail/RetrieveDetail.h"
#include "dasi/api/detail/WipeDetail.h"

#include <memory>

namespace dasi {

//-------------------------------------------------------------------------------------------------

class DasiImpl;

class Dasi {

public:  // methods
    /// Dasi Constructor.
    /// @param dasi_config A YAML configuration, or configuration path for Dasi. Specifies where data is stored
    /// @param application_config A configuration for runtime-specific overrides within Dasi. Controls how data is written/read
    explicit Dasi(const char* dasi_config, const char* application_config = nullptr);

    ~Dasi();

    /// Write data to be stored according to Dasi configuration
    /// @note After this call returns, Dasi has written this data to a backend or taken an internal copy. It is not
    ///       guaranteed accessible, or persisted wrt. failure, until flush() is called.
    /// @param key The metadata description of the data to store and index
    /// @param data A pointer to a (read-only) copy of the data
    /// @param length The length of the data to store in bytes
    void archive(const Key& key, const void* data, size_t length);

    /// Removes the data from Dasi up to 2nd-level rules.
    /// @note The data removal of 3rd-level rule is not possible.
    ///
    /// @param query A description of the span of data to wipe (up-to 2nd-level rule)
    /// @param doit Delete the files (data and indexes)
    /// @param porcelain List only the deleted files (short output)
    /// @param all Wipe all (unowned) contents of an unclean database (unsafe)
    WipeGenerator wipe(const Query& query, bool doit, bool porcelain, bool all);

    /// Purge does several things:
    /// 1. Enumerates all data internally, noting every field that is not accessible because it is masked by newer
    /// fields
    /// 2. Determines which indexes no longer have any accessible fields in them. With --doit, these indexes are then
    /// masked out by putting a masking entry into Dasi so that they will not be visited.
    /// 3. If all of the data in a data file, and/or all of the indexes in an index file are no longer accessible (and
    /// are masked out appropriately) then these files will be removed from Dasi.
    ///
    /// @param query A description of the span of data to purge (only 1st level rule applies)
    /// @param doit Delete the files (data and indexes)
    /// @param porcelain List only the deleted files (short output)
    PurgeGenerator purge(const Query& query, bool doit, bool porcelain);

    /// Flushes all buffers and ensures all internal state is safe wrt. failure
    /// @note always safe to call
    void flush();

    /// Retrieve data objects from the archive
    /// @todo - a retrieve function that gets data sizes as well.
    ///         Support explicit data locations for lazy retrievals (e.g. nice xarray or dask support)
    ///         Build on the Query functionality?
    /// @todo - Explicitly and cleanly distinguish _all_ from _unspecified_ (currently all cases treated as _unspecified_)
    /// @param query A description of the span of data to retrieve
    /// @returns A generic data handle, that will retrieve the data.
    RetrieveResult retrieve(const Query& query);

    /// List data present and retrievable from the archive
    /// @param query A description of the span of metadata to list within
    /// @returns An iterable generator object of ListElements, containing details of the objects found, the
    ///          keys describing them and a timestamp of object archival.
    ListGenerator list(const Query& query);

    /// Set a named policy, or set of policies, for the data collections identified by the query
    /// @param query The data collections to modify
    /// @param policyDict A (nested) dictionary of policy keys/values to set
    /// @returns Identified policy objects for each identified data collection, relative to the specified name.
    ///          Corresponds to the output from queryPolicy
    PolicyGenerator setPolicy(const Query& query, const PolicyDict& policyDict);

    /// Retrieve a named policy, or set of policies, for the data collections identified by the query
    /// @param query The data collections to modify
    /// @param name The name of the policy to set, or an identifier of the subset of policies to set
    /// @returns Identified policy objects for each identified data collection, relative to the specified name
    PolicyGenerator queryPolicy(const Query& query, const std::string& name = "");

    void dumpSchema(std::ostream& out) const;

    /// @note - move should follow same api and/or an ----- ioctl-type ----- api

private:  // members
    std::unique_ptr<DasiImpl> impl_;
};

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
