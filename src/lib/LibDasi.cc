
#include "dasi/lib/LibDasi.h"

#include "lib/dasi_version.h"

namespace dasi {

//-------------------------------------------------------------------------------------------------

REGISTER_LIBRARY(LibDasi);

LibDasi::LibDasi() : Library("dasi") { }

LibDasi& LibDasi::instance() {
    static LibDasi thelib;
    return thelib;
}

std::string LibDasi::version() const {
    return dasi_version_str();
}

std::string LibDasi::gitsha1(unsigned int count) const {
    std::string sha1(dasi_git_sha1());
    if (sha1.empty()) { return "not available"; }
    return sha1.substr(0, std::min(count, 40u));
}

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
