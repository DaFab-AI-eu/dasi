
#include "dasi/api/Dasi.h"

#include "dasi/core/Archiver.h"
#include "dasi/core/Retriever.h"
#include "dasi/util/Exceptions.h"

#include "yaml-cpp/yaml.h"

namespace dasi::api {

//----------------------------------------------------------------------------------------------------------------------

Dasi::Dasi(std::istream& iss) :
    config_(iss) {}

Dasi::Dasi(const char* config) :
    config_(config) {}

Dasi::~Dasi() {}

core::Schema& Dasi::schema() {
    if (!schema_) schema_ = std::make_unique<core::Schema>(config_.value("schema"));
    return *schema_;
}

core::Archiver& Dasi::archiver() {

    if (!archiver_) {

        long archiverLRUsize = config_.getLong("archiveLRUsize", 20);
        archiver_ = std::make_unique<core::Archiver>(config_, schema(), archiverLRUsize);
    }
    return *archiver_;
}

core::Retriever& Dasi::retriever() {

    if (!retriever_) {

        long retrieverLRUsize = config_.getLong("retrieveRUsize", 20);
        retriever_ = std::make_unique<core::Retriever>(config_, schema(), retrieverLRUsize);
    }
    return *retriever_;
}

void Dasi::archive(const Key& key, const void* data, size_t length) {
    archiver().archive(key, data, length);
}

Handle* Dasi::retrieve(const Query& query) {
    return retriever().retrieve(query);
}


//----------------------------------------------------------------------------------------------------------------------

}