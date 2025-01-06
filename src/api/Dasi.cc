/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "dasi/api/Dasi.h"

#include "dasi/lib/LibDasi.h"
#include "eckit/config/YAMLConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/log/Log.h"
#include "eckit/runtime/Main.h"
#include "eckit/utils/Tokenizer.h"
#include "fdb5/api/FDB.h"
#include "fdb5/api/helpers/FDBToolRequest.h"
#include "fdb5/config/Config.h"
#include "fdb5/rules/Schema.h"
#include "impl/ListGeneratorImpl.h"
#include "impl/PolicyStatusGeneratorImpl.h"
#include "impl/PurgeGeneratorImpl.h"
#include "impl/RetrieveResultImpl.h"
#include "impl/WipeGeneratorImpl.h"
#include "metkit/mars/MarsRequest.h"

#include <memory>

namespace dasi {

//----------------------------------------------------------------------------------------------------------------------

namespace {

// Eckit initialisation helpers

void initialise_eckit_once() {
    static bool initialised = false;
    if (!initialised) {
        if (!eckit::Main::ready()) {
            static const char* argv[2] = {"dasi", nullptr};
            eckit::Main::initialise(1, const_cast<char**>(argv));
        }
        initialised = true;
    }
}

// This is implemented as a class, such that it can be placed as the first
// initialiser of DamiImpl, before any of the data members.
struct EckitMainHelper {
    EckitMainHelper() { initialise_eckit_once(); }
};

// Dasi Implementation construction helpers

eckit::LocalConfiguration parse_application_config(const char* application_config) {

    if (application_config) {
        std::istringstream app_ss(application_config);
        return eckit::LocalConfiguration {eckit::YAMLConfiguration(app_ss)};
    }

    return eckit::LocalConfiguration {};
}

fdb5::Config construct_config(const char* dasi_config, const char* application_config) {

    // Is the dasi_configation a raw yaml, or is it a path?
    // FIXME: TODO: This behaves badly if there is a _path_ that doesn't exist (fails on parsing)

    /// @todo the logic below fails to default schema (etc/fdb/schema): add a warning
    eckit::PathName config_path(dasi_config);
    std::string     yaml_config;
    /// @todo when the yml file does not exist. Also, it causes to break cfg object
    if (config_path.exists()) {
        std::unique_ptr<eckit::DataHandle> dh(config_path.fileHandle());
        size_t                             sz = dh->size();
        dh->openForRead();
        eckit::AutoClose closer(*dh);
        yaml_config.resize(sz);
        ASSERT(dh->read(&yaml_config[0], sz) == sz);
    } else {
        yaml_config = dasi_config;
    }

    ASSERT(dasi_config);
    LOG_DEBUG_LIB(LibDasi) << "Dasi Configuration: " << eckit::newl;
    LOG_DEBUG_LIB(LibDasi) << yaml_config << eckit::newl << std::endl;

    LOG_DEBUG_LIB(LibDasi) << "Application Configuration:";
    if (application_config) {
        LOG_DEBUG_LIB(LibDasi) << eckit::newl;
        LOG_DEBUG_LIB(LibDasi) << application_config << eckit::newl << std::endl;
    } else {
        LOG_DEBUG_LIB(LibDasi) << " <default>" << std::endl;
    }

    std::istringstream       dasi_ss(yaml_config);
    eckit::YAMLConfiguration cfg(dasi_ss);

    if (cfg.has("schema")) {
        LOG_DEBUG_LIB(LibDasi) << "Dasi Schema: " << eckit::newl;
        LOG_DEBUG_LIB(LibDasi) << fdb5::Schema {cfg.getString("schema")} << eckit::newl << std::endl;
    }

    fdb5::Config retval {cfg, parse_application_config(application_config)};

    // By default, we specify local type, so we don't have to expose all of the FDB frontend-routing at this time
    if (!retval.has("type")) { retval.set("type", "local"); }

    return retval;
}

}  // namespace

//----------------------------------------------------------------------------------------------------------------------

class DasiImpl {

public:  // methods
    DasiImpl(const char* dasi_config, const char* application_config)
        : mainHelper_(), fdb_(construct_config(dasi_config, application_config)) { }

    void archive(const Key& key, const void* data, size_t length) {
        fdb5::Key fdb_key;
        for (const auto& kv : key) { fdb_key.set(kv.first, kv.second); }
        fdb_.archive(fdb_key, data, length);
    }

    WipeGenerator wipe(const Query& query, const bool doit, const bool porcelain, const bool all) {
        auto&& iter = fdb_.wipe(fdb5::FDBToolRequest(queryToMarsRequest(query)), doit, porcelain, all);
        return WipeGenerator(std::make_unique<WipeGeneratorImpl>(std::move(iter)));
    }

    PurgeGenerator purge(const Query& query, const bool doit, const bool porcelain) {
        auto&& iter = fdb_.purge(fdb5::FDBToolRequest(queryToMarsRequest(query)), doit, porcelain);
        return PurgeGenerator(std::make_unique<PurgeGeneratorImpl>(std::move(iter)));
    }

    ListGenerator list(const Query& query) {
        bool   deduplicate = true;
        auto&& iter        = fdb_.list(fdb5::FDBToolRequest(queryToMarsRequest(query)), deduplicate);

        return ListGenerator(std::make_unique<ListGeneratorImpl>(std::move(iter)));
    }

    /// @todo - deduplicate FDB results inside the inspect() function instead

    RetrieveResult retrieve(const Query& query) {
        auto&& iter = fdb_.inspect(queryToMarsRequest(query));
        return RetrieveResult {std::make_unique<RetrieveResultImpl>(std::move(iter))};
    }

    void flush() { fdb_.flush(); }

    PolicyGenerator setPolicy(const Query& query, const PolicyDict& policyDict) {
        /// @todo Put this properly through the entire FDB infrastructure. This implementation is a bit of a hack...
        /* Currently not wired into the FDB. */

        if (policyDict.has("access")) {

            const auto& access = policyDict.getSubConfiguration("access");

            bool                     foundAny = false;
            fdb5::ControlIdentifiers identifiers;
            fdb5::ControlAction      action;
            for (const auto& key : access.keys()) {

                if (key == "list") { identifiers |= fdb5::ControlIdentifier::List; }
                if (key == "retrieve") { identifiers |= fdb5::ControlIdentifier::Retrieve; }
                if (key == "archive") { identifiers |= fdb5::ControlIdentifier::Archive; }
                if (key == "wipe") { identifiers |= fdb5::ControlIdentifier::Wipe; }

                fdb5::ControlAction newAction = access.getBool(key) ? fdb5::ControlAction::Enable
                                                                    : fdb5::ControlAction::Disable;
                if (foundAny) {
                    if (newAction != action) {
                        throw eckit::UserError("Can only enable or disable locks in one action", Here());
                    }
                }

                action   = newAction;
                foundAny = true;
            }

            auto&& iter = fdb_.control(fdb5::FDBToolRequest(queryToMarsRequest(query)), action, identifiers);
            return PolicyGenerator(std::make_unique<PolicyStatusGeneratorImpl>(std::move(iter)));

        } else {
            /// @todo implementation for anything other than locking
            NOTIMP;
        }
    }

    PolicyGenerator queryPolicy(const Query& query, const std::string& name = "") {
        /// @todo Put this properly through the entire FDB infrastructure for _policies_,
        //        not just reusing/abusing locking...
        std::vector<std::string> policySpecifiers;
        eckit::Tokenizer(".")(name, policySpecifiers);

        auto&& iter = fdb_.status(fdb5::FDBToolRequest(queryToMarsRequest(query)));
        return PolicyGenerator(std::make_unique<PolicyStatusGeneratorImpl>(std::move(iter), std::move(policySpecifiers)));
    }

    void dumpSchema(std::ostream& out) const { fdb_.config().schema().dump(out); }

private:  // methods
    metkit::mars::MarsRequest queryToMarsRequest(const Query& query) {
        metkit::mars::MarsRequest rq("retrieve");
        for (const auto& kv : query) { rq.values(kv.first, kv.second); }
        return rq;
    }

private:  // members
    // DASI may well be the first eckit-like entry point to the application.
    // If it is, then make sure that everything goes nicely.
    EckitMainHelper mainHelper_;

    // The real deal, this is where most of the underlying work is done!
    fdb5::FDB fdb_;
};

//----------------------------------------------------------------------------------------------------------------------

Dasi::Dasi(const char* dasi_config, const char* application_config)
    : impl_(new DasiImpl(dasi_config, application_config)) { }

// Destructor must be implemented in .cc, not as default in header, as DasiImpl
// is only forward declared, whereas it is available in this translation unit
Dasi::~Dasi() = default;

void Dasi::archive(const Key& key, const void* data, size_t length) {
    ASSERT(impl_);
    impl_->archive(key, data, length);
}

WipeGenerator Dasi::wipe(const Query& query, const bool doit, const bool porcelain, const bool all) {
    ASSERT(impl_);
    return impl_->wipe(query, doit, porcelain, all);
}

PurgeGenerator Dasi::purge(const Query& query, const bool doit, const bool porcelain) {
    ASSERT(impl_);
    return impl_->purge(query, doit, porcelain);
}

ListGenerator Dasi::list(const Query& query) {
    ASSERT(impl_);
    return impl_->list(query);
}

RetrieveResult Dasi::retrieve(const Query& query) {
    ASSERT(impl_);
    return impl_->retrieve(query);
}

void Dasi::flush() {
    ASSERT(impl_);
    impl_->flush();
}

PolicyGenerator Dasi::setPolicy(const Query& query, const PolicyDict& policyDict) {
    ASSERT(impl_);
    return impl_->setPolicy(query, policyDict);
}

PolicyGenerator Dasi::queryPolicy(const Query& query, const std::string& name) {
    ASSERT(impl_);
    return impl_->queryPolicy(query, name);
}

void Dasi::dumpSchema(std::ostream& out) const {
    ASSERT(impl_);
    impl_->dumpSchema(out);
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi
